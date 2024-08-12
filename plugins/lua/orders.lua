local _ENV = mkmodule('plugins.orders')

local dialogs = require('gui.dialogs')
local gui = require('gui')
local overlay = require('plugins.overlay')
local textures = require('gui.textures')
local utils = require('utils')
local widgets = require('gui.widgets')

--
-- OrdersOverlay
--

local function do_sort()
    dfhack.run_command('orders', 'sort')
end

local function do_clear()
    dialogs.showYesNoPrompt('Clear manager orders?',
        'Are you sure you want to clear the manager orders?', nil,
        function() dfhack.run_command('orders', 'clear') end)
end

local function get_import_choices()
    return dfhack.run_command_silent('orders', 'list'):split('\n')
end

local function do_import()
    local dlg
    local function get_dlg() return dlg end
    dlg = dialogs.ListBox{
        frame_title='Import/Delete Manager Orders',
        with_filter=true,
        choices=get_import_choices(),
        on_select=function(_, choice)
            dfhack.run_command('orders', 'import', choice.text)
        end,
        dismiss_on_select2=false,
        on_select2=function(_, choice)
            if choice.text:startswith('library/') then return end
            local fname = 'dfhack-config/orders/'..choice.text..'.json'
            if not dfhack.filesystem.isfile(fname) then return end
            dialogs.showYesNoPrompt('Delete orders file?',
                'Are you sure you want to delete "' .. fname .. '"?', nil,
                function()
                    print('deleting ' .. fname)
                    os.remove(fname)
                    local list = get_dlg().subviews.list
                    local filter = list:getFilter()
                    list:setChoices(get_import_choices(), list:getSelected())
                    list:setFilter(filter)
                end)
        end,
        select2_hint='Delete file',
    }:show()
end

local function do_export()
    dialogs.InputBox{
        frame_title='Export Manager Orders',
        on_input=function(text)
            dfhack.run_command('orders', 'export', text)
        end
    }:show()
end

local function do_recheck()
    dfhack.run_command('orders', 'recheck')
end

OrdersOverlay = defclass(OrdersOverlay, overlay.OverlayWidget)
OrdersOverlay.ATTRS{
    desc='Adds import, export, and other functions to the manager orders screen.',
    default_pos={x=53,y=-6},
    default_enabled=true,
    viewscreens='dwarfmode/Info/WORK_ORDERS/Default',
    frame={w=43, h=4},
}

function OrdersOverlay:init()
    self.minimized = false

    local main_panel = widgets.Panel{
        frame={t=0, l=0, r=0, h=4},
        frame_style=gui.MEDIUM_FRAME,
        frame_background=gui.CLEAR_PEN,
        visible=function() return not self.minimized end,
        subviews={
            widgets.HotkeyLabel{
                frame={t=0, l=0},
                label='import',
                key='CUSTOM_CTRL_I',
                auto_width=true,
                on_activate=do_import,
            },
            widgets.HotkeyLabel{
                frame={t=1, l=0},
                label='export',
                key='CUSTOM_CTRL_E',
                auto_width=true,
                on_activate=do_export,
            },
            widgets.HotkeyLabel{
                frame={t=0, l=15},
                label='recheck conditions',
                key='CUSTOM_CTRL_K',
                auto_width=true,
                on_activate=do_recheck,
            },
            widgets.HotkeyLabel{
                frame={t=1, l=15},
                label='sort',
                key='CUSTOM_CTRL_O',
                auto_width=true,
                on_activate=do_sort,
            },
            widgets.HotkeyLabel{
                frame={t=1, l=28},
                label='clear',
                key='CUSTOM_CTRL_C',
                auto_width=true,
                on_activate=do_clear,
            },
        },
    }

    local minimized_panel = widgets.Panel{
        frame={t=0, r=4, w=3, h=1},
        subviews={
            widgets.Label{
                frame={t=0, l=0, w=1, h=1},
                text='[',
                text_pen=COLOR_RED,
                visible=function() return self.minimized end,
            },
            widgets.Label{
                frame={t=0, l=1, w=1, h=1},
                text={{text=function() return self.minimized and string.char(31) or string.char(30) end}},
                text_pen=dfhack.pen.parse{fg=COLOR_BLACK, bg=COLOR_GREY},
                text_hpen=dfhack.pen.parse{fg=COLOR_BLACK, bg=COLOR_WHITE},
                on_click=function() self.minimized = not self.minimized end,
            },
            widgets.Label{
                frame={t=0, r=0, w=1, h=1},
                text=']',
                text_pen=COLOR_RED,
                visible=function() return self.minimized end,
            },
        },
    }

    self:addviews{
        main_panel,
        minimized_panel,
        widgets.HelpButton{
            command='orders',
            visible=function() return not self.minimized end,
        },
    }
end

function OrdersOverlay:onInput(keys)
    if df.global.game.main_interface.job_details.open then return end
    if keys.CUSTOM_ALT_M then
        self.minimized = not self.minimized
        return true
    end
    if OrdersOverlay.super.onInput(self, keys) then
        return true
    end
end

function OrdersOverlay:render(dc)
    if df.global.game.main_interface.job_details.open then return end
    OrdersOverlay.super.render(self, dc)
end

-- Resets the selected work order to the `Checking` state

local function set_current_inactive()
    local scrConditions = df.global.game.main_interface.info.work_orders.conditions
    if scrConditions.open then
        dfhack.run_command('orders', 'recheck', 'this')
    else
        qerror("Order conditions is not open")
    end
end

local function can_recheck()
    local scrConditions = df.global.game.main_interface.info.work_orders.conditions
    local order = scrConditions.wq
    return order.status.active and #order.item_conditions > 0
end

-- -------------------
-- RecheckOverlay
--

local focusString = 'dwarfmode/Info/WORK_ORDERS/Conditions'

RecheckOverlay = defclass(RecheckOverlay, overlay.OverlayWidget)
RecheckOverlay.ATTRS{
    desc='Adds a button to the work order details page to tell the manager to recheck conditions.',
    default_pos={x=6,y=8},
    default_enabled=true,
    viewscreens=focusString,
    -- width is the sum of lengths of `[` + `Ctrl+A` + `: ` + button.label + `]`
    frame={w=1 + 6 + 2 + 19 + 1, h=3},
}

local function areTabsInTwoRows()
    return gui.get_interface_rect().width < 155
end

function RecheckOverlay:updateTextButtonFrame()
    local twoRows = areTabsInTwoRows()
    if (self._twoRows == twoRows) then return false end

    self._twoRows = twoRows
    local frame = twoRows
            and {b=0, l=0, r=0, h=1}
            or  {t=0, l=0, r=0, h=1}
    self.subviews.button.frame = frame

    return true
end

function RecheckOverlay:init()
    self:addviews{
        widgets.TextButton{
            view_id = 'button',
            -- frame={t=0, l=0, r=0, h=1}, -- is set in `updateTextButtonFrame()`
            label='re-check conditions',
            key='CUSTOM_CTRL_A',
            on_activate=set_current_inactive,
            enabled=can_recheck,
        },
    }

    self:updateTextButtonFrame()
end

function RecheckOverlay:onRenderBody(dc)
    if (self.frame_rect.y1 == 7) then
        -- only apply this logic if the overlay is on the same row as
        -- originally thought: just above the order status icon

        if self:updateTextButtonFrame() then
            self:updateLayout()
        end
    end

    RecheckOverlay.super.onRenderBody(self, dc)
end

--
-- SkillRestrictionOverlay
--

SkillRestrictionOverlay = defclass(SkillRestrictionOverlay, overlay.OverlayWidget)
SkillRestrictionOverlay.ATTRS{
    desc='Adds a UI to the Workers tab for vanilla workshop labor restrictions.',
    default_pos={x=-40, y=16},
    default_enabled=true,
    viewscreens={
        'dwarfmode/ViewSheets/BUILDING/Furnace',
        'dwarfmode/ViewSheets/BUILDING/Workshop',
    },
    frame={w=57, h=7},
}

local function can_set_skill_level()
    local ret = false
    for _,fs in ipairs(dfhack.gui.getFocusStrings(dfhack.gui.getDFViewscreen(true))) do
        if fs:endswith('/Workers') then
            local bld = dfhack.gui.getSelectedBuilding(true)
            if not bld then return false end
            ret = #bld.profile.permitted_workers == 0
        elseif fs:endswith('WORKER_ASSIGNMENT') then
            return false
        end
    end
    return ret and not df.global.game.main_interface.stockpile_link.adding_new_link
end

local function set_skill_level(which, val, bld)
    bld = bld or dfhack.gui.getSelectedBuilding(true)
    if not bld then return end
    bld.profile[which] = val
end

-- the UI isn't wide enough to accomodate all the skills. select a few to combine
-- into the adjacent tier.
local SKIP_RATINGS = utils.invert{
    df.skill_rating.Adequate,
    df.skill_rating.Skilled,
    df.skill_rating.Adept,
    df.skill_rating.Professional,
    df.skill_rating.Great,
    df.skill_rating.HighMaster,
    df.skill_rating.Legendary1,
    df.skill_rating.Legendary2,
    df.skill_rating.Legendary3,
    df.skill_rating.Legendary4,
    df.skill_rating.Legendary5,
}

local SKILL_OPTIONS = {}
local MIN_SKILL_RATINGS, MAX_SKILL_RATINGS = {}, {}
for ridx in ipairs(df.skill_rating) do
    if SKIP_RATINGS[ridx] then goto continue end
    local idx = #SKILL_OPTIONS + 1
    table.insert(SKILL_OPTIONS, {
        label=df.skill_rating.attrs[ridx].caption,
        value=idx,
    })
    local prev_max = MAX_SKILL_RATINGS[idx-1]
    MIN_SKILL_RATINGS[idx] = prev_max and (prev_max+1) or 0
    MAX_SKILL_RATINGS[idx] = ridx
    ::continue::
end
MAX_SKILL_RATINGS[#MAX_SKILL_RATINGS] = 3000 -- DF value for upper cap


function SkillRestrictionOverlay:init()
    local panel = widgets.Panel{
        frame_style=gui.FRAME_MEDIUM,
        frame_background=gui.CLEAR_PEN,
    }
    panel:addviews{
        widgets.CycleHotkeyLabel{
            view_id='min_skill',
            frame={l=1, t=0, w=16},
            label='Min skill:',
            label_below=true,
            key_back='CUSTOM_SHIFT_C',
            key='CUSTOM_SHIFT_V',
            options=SKILL_OPTIONS,
            initial_option=SKILL_OPTIONS[1].value,
            on_change=function(val)
                local bld = dfhack.gui.getSelectedBuilding(true)
                if self.subviews.max_skill:getOptionValue() < val then
                    self.subviews.max_skill:setOption(val)
                    set_skill_level('max_level', MAX_SKILL_RATINGS[val], bld)
                end
                set_skill_level('min_level', MIN_SKILL_RATINGS[val], bld)
            end,
        },
        widgets.CycleHotkeyLabel{
            view_id='max_skill',
            frame={r=1, t=0, w=16},
            label='Max skill:',
            label_below=true,
            key_back='CUSTOM_SHIFT_E',
            key='CUSTOM_SHIFT_R',
            options=SKILL_OPTIONS,
            initial_option=SKILL_OPTIONS[#SKILL_OPTIONS].value,
            on_change=function(val)
                local bld = dfhack.gui.getSelectedBuilding(true)
                if self.subviews.min_skill:getOptionValue() > val then
                    self.subviews.min_skill:setOption(val)
                    set_skill_level('min_level', MIN_SKILL_RATINGS[val], bld)
                end
                set_skill_level('max_level', MAX_SKILL_RATINGS[val], bld)
            end,
        },
        widgets.RangeSlider{
            frame={l=1, t=3},
            num_stops=#SKILL_OPTIONS,
            get_left_idx_fn=function()
                return self.subviews.min_skill:getOptionValue()
            end,
            get_right_idx_fn=function()
                return self.subviews.max_skill:getOptionValue()
            end,
            on_left_change=function(idx) self.subviews.min_skill:setOption(idx, true) end,
            on_right_change=function(idx) self.subviews.max_skill:setOption(idx, true) end,
        },
    }

    self:addviews{
        panel,
        widgets.HelpButton{command='orders'},
    }
end

function SkillRestrictionOverlay:refresh_slider()
    local bld = dfhack.gui.getSelectedBuilding(true)
    if not bld then return end
    self.subviews.min_skill:setOption(#SKILL_OPTIONS)
    for min_idx=1,#SKILL_OPTIONS do
        if bld.profile.min_level > MAX_SKILL_RATINGS[min_idx] then goto continue end
        self.subviews.min_skill:setOption(min_idx)
        self.subviews.max_skill:setOption(min_idx)
        for max_idx=min_idx,#SKILL_OPTIONS do
            if bld.profile.max_level <= MAX_SKILL_RATINGS[max_idx] then
                self.subviews.max_skill:setOption(max_idx)
                break
            end
        end
        break
        ::continue::
    end
end

function SkillRestrictionOverlay:render(dc)
    if can_set_skill_level() then
        self:refresh_slider()
        SkillRestrictionOverlay.super.render(self, dc)
    end
end

function SkillRestrictionOverlay:onInput(keys)
    if can_set_skill_level() and
        not df.global.game.main_interface.view_sheets.building_entering_nickname
    then
        return SkillRestrictionOverlay.super.onInput(self, keys)
    end
end

--
-- LaborRestrictionsOverlay
--

LaborRestrictionsOverlay = defclass(LaborRestrictionsOverlay, overlay.OverlayWidget)
LaborRestrictionsOverlay.ATTRS{
    desc='Adds a UI to the Workers tab for vanilla workshop labor restrictions.',
    default_pos={x=-40, y=24},
    default_enabled=true,
    viewscreens={
        'dwarfmode/ViewSheets/BUILDING/Furnace/Kiln/Workers',
        'dwarfmode/ViewSheets/BUILDING/Furnace/MagmaKiln/Workers',
        'dwarfmode/ViewSheets/BUILDING/Workshop/Ashery/Workers',
        'dwarfmode/ViewSheets/BUILDING/Workshop/Butchers/Workers',
        'dwarfmode/ViewSheets/BUILDING/Workshop/Carpenters/Workers',
        'dwarfmode/ViewSheets/BUILDING/Workshop/Craftsdwarfs/Workers',
        'dwarfmode/ViewSheets/BUILDING/Workshop/Custom/SCREW_PRESS/Workers',
        'dwarfmode/ViewSheets/BUILDING/Workshop/Farmers/Workers',
        'dwarfmode/ViewSheets/BUILDING/Workshop/Fishery/Workers',
        'dwarfmode/ViewSheets/BUILDING/Workshop/Jewelers/Workers',
        'dwarfmode/ViewSheets/BUILDING/Workshop/MagmaForge/Workers',
        'dwarfmode/ViewSheets/BUILDING/Workshop/Masons/Workers',
        'dwarfmode/ViewSheets/BUILDING/Workshop/MetalsmithsForge/Workers',
        'dwarfmode/ViewSheets/BUILDING/Workshop/Millstone/Workers',
        'dwarfmode/ViewSheets/BUILDING/Workshop/Quern/Workers',
        'dwarfmode/ViewSheets/BUILDING/Workshop/Still/Workers',
    },
    frame={w=57, h=15},
}

local function can_set_labors()
    for _,fs in ipairs(dfhack.gui.getFocusStrings(dfhack.gui.getDFViewscreen(true))) do
        if fs:endswith('WORKER_ASSIGNMENT') then
            return false
        end
    end
    return true
end

local WORKSHOP_LABORS = {
    [df.workshop_type.Carpenters]={'CARPENTER', 'TRAPPER'},
    [df.workshop_type.Farmers]={'PROCESS_PLANT', 'MAKE_CHEESE', 'MILK',
        'SHEARER', 'SPINNER', 'PAPERMAKING'},
    [df.workshop_type.Masons]={'STONECUTTER', 'STONE_CARVER',},
    [df.workshop_type.Craftsdwarfs]={'STONE_CRAFT', 'WOOD_CRAFT', 'BONE_CARVE',
        'WAX_WORKING', 'BOOKBINDING', 'EXTRACT_STRAND',
        'LEATHER', 'CLOTHESMAKER', 'METAL_CRAFT', 'GLASSMAKER'},
    [df.workshop_type.Jewelers]={'CUT_GEM', 'ENCRUST_GEM'},
    [df.workshop_type.MetalsmithsForge]={'FORGE_WEAPON', 'FORGE_ARMOR',
        'FORGE_FURNITURE', 'METAL_CRAFT', 'TRAPPER'},
    [df.workshop_type.MagmaForge]={'FORGE_WEAPON', 'FORGE_ARMOR',
        'FORGE_FURNITURE', 'METAL_CRAFT', 'TRAPPER'},
    [df.workshop_type.Butchers]={'BUTCHER', 'DISSECT_VERMIN'},
    [df.workshop_type.Fishery]={'FISH', 'CLEAN_FISH', 'DISSECT_FISH'},
    [df.workshop_type.Still]={'BREWER', 'HERBALIST'},
    [df.workshop_type.Quern]={'MILLER', 'PAPERMAKING'},
    [df.workshop_type.Ashery]={'POTASH_MAKING', 'LYE_MAKING'},
    [df.workshop_type.Millstone]={'MILLER', 'PAPERMAKING'},
    -- this is specifically for the Screw Press, but we don't need to differentiate (yet)
    -- since we only support one kind of custom workshop
    [df.workshop_type.Custom]={'PRESSING', 'PAPERMAKING'},
}

local FURNACE_LABORS = {
    [df.furnace_type.Kiln]={'SMELT', 'POTTERY', 'GLAZING'},
    [df.furnace_type.MagmaKiln]={'SMELT', 'POTTERY', 'GLAZING'},
}

-- used by quickfort
function get_profile_labors(bld_type, bld_subtype)
    if bld_type == df.building_type.Workshop then
        return WORKSHOP_LABORS[bld_subtype] or {}
    elseif bld_type == df.building_type.Furnace then
        return FURNACE_LABORS[bld_subtype] or {}
    end
    return {}
end

local ENABLED_PEN_LEFT = dfhack.pen.parse{fg=COLOR_CYAN,
        tile=curry(textures.tp_control_panel, 1), ch=string.byte('[')}
local ENABLED_PEN_CENTER = dfhack.pen.parse{fg=COLOR_LIGHTGREEN,
        tile=curry(textures.tp_control_panel, 2) or nil, ch=251} -- check
local ENABLED_PEN_RIGHT = dfhack.pen.parse{fg=COLOR_CYAN,
        tile=curry(textures.tp_control_panel, 3) or nil, ch=string.byte(']')}
local DISABLED_PEN_LEFT = dfhack.pen.parse{fg=COLOR_CYAN,
        tile=curry(textures.tp_control_panel, 4) or nil, ch=string.byte('[')}
local DISABLED_PEN_CENTER = dfhack.pen.parse{fg=COLOR_RED,
        tile=curry(textures.tp_control_panel, 5) or nil, ch=string.byte('x')}
local DISABLED_PEN_RIGHT = dfhack.pen.parse{fg=COLOR_CYAN,
        tile=curry(textures.tp_control_panel, 6) or nil, ch=string.byte(']')}

local function set_labor(bld, labor, val)
    bld.profile.blocked_labors[labor] = val
end

local function toggle_labor(_, choice)
    if not choice then return end
    local bld = dfhack.gui.getSelectedBuilding(true)
    if not bld then return end
    set_labor(bld, choice.labor, not bld.profile.blocked_labors[choice.labor])
end

local function is_labor_blocked(labor, bld)
    bld = bld or dfhack.gui.getSelectedBuilding(true)
    return bld and bld.profile.blocked_labors[labor]
end

function make_labor_panel(bld_type, bld_subtype, labors)
    local list = widgets.List{
        frame={t=2, l=1, w=28, b=0},
        on_double_click=toggle_labor,
    }

    local panel = widgets.Panel{
        frame_style=gui.FRAME_MEDIUM,
        frame_background=gui.CLEAR_PEN,
        -- will get clamped to parent frame and a scrollbar will appear if the list
        -- is too long
        frame={l=0, r=0, t=0, h=math.max(#labors+5,9)},
        visible=function()
            local bld = dfhack.gui.getSelectedBuilding(true)
            return bld and bld:getType() == bld_type and bld.type == bld_subtype and
                not df.global.game.main_interface.stockpile_link.adding_new_link
        end,
        subviews={
            widgets.Label{
                frame={t=0, l=0},
                text='Permitted General Work Order Labors',
            },
            list,
            widgets.HotkeyLabel{
                frame={l=30, t=2},
                key='CUSTOM_CTRL_A',
                label='Toggle all',
                auto_width=true,
                on_activate=function()
                    local bld = dfhack.gui.getSelectedBuilding(true)
                    if not bld then return end
                    local choices = list:getChoices()
                    local target = not is_labor_blocked(choices[1].labor, bld)
                    for _, choice in ipairs(choices) do
                        set_labor(bld, choice.labor, target)
                    end
                end,
            },
            widgets.HotkeyLabel{
                frame={l=30, t=4},
                key='SELECT',
                label='Toggle selected',
                auto_width=true,
                on_activate=function() toggle_labor(list:getSelected()) end,
            },
            widgets.Label{
                frame={l=37, t=5},
                text='(or double click)'
            }
        },
    }

    panel.onInput = function(self, keys)
        local handled = widgets.Panel.onInput(self, keys)
        if keys._MOUSE_L then
            local idx = list:getIdxUnderMouse()
            if idx then
                local x = list:getMousePos()
                if x <= 2 then
                    toggle_labor(list:getSelected())
                    list.last_select_click_ms = 0
                end
            end
        end
        return handled
    end

    local choices = {}
    for _,labor_name in ipairs(labors) do
        local labor = df.unit_labor[labor_name]
        local function get_enabled_button_token(e_tile, d_tile)
            return {
                tile=function()
                    return is_labor_blocked(labor) and d_tile or e_tile
                end,
            }
        end
        table.insert(choices, {
            labor=labor,
            text={
                    get_enabled_button_token(ENABLED_PEN_LEFT, DISABLED_PEN_LEFT),
                    get_enabled_button_token(ENABLED_PEN_CENTER, DISABLED_PEN_CENTER),
                    get_enabled_button_token(ENABLED_PEN_RIGHT, DISABLED_PEN_RIGHT),
                    {gap=1, text=df.unit_labor.attrs[labor].caption},
            },
        })
    end
    list:setChoices(choices)

    return panel
end

function LaborRestrictionsOverlay:init()
    for ws_type, labors in pairs(WORKSHOP_LABORS) do
        self:addviews{make_labor_panel(df.building_type.Workshop, ws_type, labors)}
    end
    for f_type, labors in pairs(FURNACE_LABORS) do
        self:addviews{make_labor_panel(df.building_type.Furnace, f_type, labors)}
    end
    self:addviews{widgets.HelpButton{command='orders'}}
end

function LaborRestrictionsOverlay:render(dc)
    if can_set_labors() then
        LaborRestrictionsOverlay.super.render(self, dc)
    end
end

function LaborRestrictionsOverlay:onInput(keys)
    if can_set_labors() and
        not df.global.game.main_interface.view_sheets.building_entering_nickname
    then
        return LaborRestrictionsOverlay.super.onInput(self, keys)
    end
end

-- -------------------

OVERLAY_WIDGETS = {
    recheck=RecheckOverlay,
    importexport=OrdersOverlay,
    skillrestrictions=SkillRestrictionOverlay,
    laborrestrictions=LaborRestrictionsOverlay,
}

return _ENV
