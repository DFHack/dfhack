local _ENV = mkmodule('plugins.orders')

local dialogs = require('gui.dialogs')
local gui = require('gui')
local overlay = require('plugins.overlay')
local textures = require('gui.textures')
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
    -- get the tile above the order status icon
    local pen = dfhack.screen.readTile(7, 7, false)
    -- in graphics mode, `0` when one row, something else when two (`67` aka 'C' from "Creatures")
    -- in ASCII mode, `32` aka ' ' when one row, something else when two (`196` aka '-' from tab frame's top)
    return (pen.ch ~= 0 and pen.ch ~= 32)
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
-- LaborRestrictionsOverlay
--

LaborRestrictionsOverlay = defclass(LaborRestrictionsOverlay, overlay.OverlayWidget)
LaborRestrictionsOverlay.ATTRS{
    desc='Adds a UI to the Workers tab for vanilla workshop labor restrictions.',
    default_pos={x=-40,y=24},
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
    frame={w=37, h=17},
}

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
        frame={t=2, l=0, r=0, b=2},
        on_double_click=toggle_labor,
    }

    local panel = widgets.Panel{
        frame_style=gui.FRAME_MEDIUM,
        frame_background=gui.CLEAR_PEN,
        -- will get clamped to parent frame and a scrollbar will appear if the list
        -- is too long
        frame={l=0, r=0, t=0, h=#labors+7},
        visible=function()
            local bld = dfhack.gui.getSelectedBuilding(true)
            return bld and bld:getType() == bld_type and bld.type == bld_subtype
        end,
        subviews={
            widgets.Label{
                frame={t=0, l=0},
                text='Permitted General Work Order Labors',
            },
            list,
            widgets.HotkeyLabel{
                frame={l=0, b=1},
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
                frame={l=0, b=0},
                key='SELECT',
                label='Or double click to toggle',
                auto_width=true,
                on_activate=function() toggle_labor(list:getSelected()) end,
            },
        },
    }

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

-- -------------------

OVERLAY_WIDGETS = {
    recheck=RecheckOverlay,
    importexport=OrdersOverlay,
    laborrestrictions=LaborRestrictionsOverlay,
}

return _ENV
