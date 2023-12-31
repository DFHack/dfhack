local _ENV = mkmodule('plugins.sort.unitselector')

local info = require('plugins.sort.info')
local gui = require('gui')
local sortoverlay = require('plugins.sort.sortoverlay')
local utils = require('utils')
local widgets = require('gui.widgets')

local unit_selector = df.global.game.main_interface.unit_selector

-- ----------------------
-- UnitSelectorOverlay
--

local WIDGET_WIDTH = 31

UnitSelectorOverlay = defclass(UnitSelectorOverlay, sortoverlay.SortOverlay)
UnitSelectorOverlay.ATTRS{
    desc='Adds search functionality to the unit assignment screens.',
    default_pos={x=62, y=6},
    viewscreens='dwarfmode/UnitSelector',
    frame={w=31, h=1},
    -- pen, pit, chain, and cage assignment are handled by dedicated screens
    -- squad fill position screen has a specialized overlay
    -- we *could* add search functionality to vanilla screens for pit and cage,
    -- but then we'd have to handle the itemid vector
    handled_screens={
        ZONE_BEDROOM_ASSIGNMENT='already',
        ZONE_OFFICE_ASSIGNMENT='already',
        ZONE_DINING_HALL_ASSIGNMENT='already',
        ZONE_TOMB_ASSIGNMENT='already',
        OCCUPATION_ASSIGNMENT='selected',
        SQUAD_KILL_ORDER='selected',
    },
}

local function get_unit_id_search_key(unit_id)
    local unit = df.unit.find(unit_id)
    if not unit then return end
    return sortoverlay.get_unit_search_key(unit)
end

function UnitSelectorOverlay:init()
    self:addviews{
        widgets.BannerPanel{
            frame={l=0, t=0, w=WIDGET_WIDTH, h=1},
            visible=self:callback('get_key'),
            subviews={
                widgets.EditField{
                    view_id='search',
                    frame={l=1, t=0, r=1},
                    label_text="Search: ",
                    key='CUSTOM_ALT_S',
                    on_change=function(text) self:do_search(text) end,
                },
            },
        },
    }

    self:register_handlers()
end

function UnitSelectorOverlay:register_handlers()
    for name,flags_vec in pairs(self.handled_screens) do
        self:register_handler(name, unit_selector.unid,
            curry(sortoverlay.flags_vector_search, {get_search_key_fn=get_unit_id_search_key},
            unit_selector[flags_vec]))
    end
end

function UnitSelectorOverlay:get_key()
    local key = df.unit_selector_context_type[unit_selector.context]
    if self.handled_screens[key] then
        return key
    end
end

function UnitSelectorOverlay:onRenderBody(dc)
    UnitSelectorOverlay.super.onRenderBody(self, dc)
    if self.refresh_search then
        self.refresh_search = nil
        self:do_search(self.subviews.search.text)
    end
end

function UnitSelectorOverlay:onInput(keys)
    if UnitSelectorOverlay.super.onInput(self, keys) then
        return true
    end
    if keys._MOUSE_L then
        self.refresh_search = true
    end
end

-- -----------------------
-- WorkerAssignmentOverlay
--

WorkerAssignmentOverlay = defclass(WorkerAssignmentOverlay, UnitSelectorOverlay)
WorkerAssignmentOverlay.ATTRS{
    default_pos={x=6, y=6},
    viewscreens='dwarfmode/UnitSelector',
    frame={w=31, h=1},
    handled_screens={WORKER_ASSIGNMENT='selected'},
}

-- -----------------------
-- BurrowAssignmentOverlay
--

local DEFAULT_OVERLAY_WIDTH = 58

BurrowAssignmentOverlay = defclass(BurrowAssignmentOverlay, UnitSelectorOverlay)
BurrowAssignmentOverlay.ATTRS{
    viewscreens='dwarfmode/UnitSelector',
    frame={w=DEFAULT_OVERLAY_WIDTH, h=5},
    handled_screens={BURROW_ASSIGNMENT='selected'},
}

local function get_screen_width()
    local sw = dfhack.screen.getWindowSize()
    return sw
end

local function toggle_all()
    if #unit_selector.unid == 0 then return end
    local burrow = df.burrow.find(unit_selector.burrow_id)
    if not burrow then return end
    local target_state = unit_selector.selected[0] == 0
    local target_val = target_state and 1 or 0
    for i,unit_id in ipairs(unit_selector.unid) do
        local unit = df.unit.find(unit_id)
        if unit then
            dfhack.burrows.setAssignedUnit(burrow, unit, target_state)
            unit_selector.selected[i] = target_val
        end
    end
end

function BurrowAssignmentOverlay:init()
    self:addviews{
        widgets.Panel{
            view_id='top_mask',
            frame={l=WIDGET_WIDTH, r=0, t=0, h=1},
            frame_background=gui.CLEAR_PEN,
            visible=function() return self:get_key() and get_screen_width() >= 144 end,
        },
        widgets.Panel{
            view_id='wide_mask',
            frame={r=0, t=1, h=2, w=DEFAULT_OVERLAY_WIDTH},
            frame_background=gui.CLEAR_PEN,
            visible=function() return self:get_key() and get_screen_width() >= 144 end,
        },
        widgets.Panel{
            view_id='narrow_mask',
            frame={l=0, t=1, h=2, w=24},
            frame_background=gui.CLEAR_PEN,
            visible=function() return self:get_key() and get_screen_width() < 144 end,
        },
        widgets.BannerPanel{
            view_id='subset_panel',
            frame={l=0, t=1, w=WIDGET_WIDTH, h=1},
            visible=self:callback('get_key'),
            subviews={
                widgets.CycleHotkeyLabel{
                    view_id='subset',
                    frame={l=1, t=0, r=1},
                    key='CUSTOM_SHIFT_F',
                    label='Show:',
                    options={
                        {label='All', value='all', pen=COLOR_GREEN},
                        {label='Military', value='military', pen=COLOR_YELLOW},
                        {label='Civilians', value='civilian', pen=COLOR_CYAN},
                        {label='Burrowed', value='burrow', pen=COLOR_MAGENTA},
                    },
                    on_change=function(value)
                        local squad = self.subviews.squad
                        local burrow = self.subviews.burrow
                        squad.visible = false
                        burrow.visible = false
                        if value == 'military' then
                            squad.options = info.get_squad_options()
                            squad:setOption('all')
                            squad.visible = true
                        elseif value == 'burrow' then
                            burrow.options = info.get_burrow_options()
                            burrow:setOption('all')
                            burrow.visible = true
                        end
                        self:do_search(self.subviews.search.text, true)
                    end,
                },
            },
        },
        widgets.BannerPanel{
            view_id='subfilter_panel',
            frame={l=0, t=2, w=WIDGET_WIDTH, h=1},
            visible=function()
                if not self:get_key() then return false end
                local subset = self.subviews.subset:getOptionValue()
                return subset == 'military' or subset == 'burrow'
            end,
            subviews={
                widgets.CycleHotkeyLabel{
                    view_id='squad',
                    frame={l=1, t=0, r=1},
                    key='CUSTOM_SHIFT_S',
                    label='Squad:',
                    options={
                        {label='Any', value='all', pen=COLOR_GREEN},
                    },
                    visible=false,
                    on_change=function() self:do_search(self.subviews.search.text, true) end,
                },
                widgets.CycleHotkeyLabel{
                    view_id='burrow',
                    frame={l=1, t=0, r=1},
                    key='CUSTOM_SHIFT_B',
                    label='Burrow:',
                    options={
                        {label='Any', value='all', pen=COLOR_GREEN},
                    },
                    visible=false,
                    on_change=function() self:do_search(self.subviews.search.text, true) end,
                },
            },
        },
        widgets.BannerPanel{
            frame={r=0, t=4, w=25, h=1},
            visible=self:callback('get_key'),
            subviews={
                widgets.HotkeyLabel{
                    frame={l=1, t=0, r=1},
                    label='Select all/none',
                    key='CUSTOM_CTRL_A',
                    on_activate=toggle_all,
                },
            },
        },
    }
end

function BurrowAssignmentOverlay:register_handlers()
    for name,flags_vec in pairs(self.handled_screens) do
        self:register_handler(name, unit_selector.unid,
            curry(sortoverlay.flags_vector_search, {
                get_search_key_fn=get_unit_id_search_key,
                matches_filters_fn=self:callback('matches_filters'),
            },
            unit_selector[flags_vec]))
    end
end

function BurrowAssignmentOverlay:reset()
    BurrowAssignmentOverlay.super.reset(self)
    self.subviews.subset:setOption('all')
end

function BurrowAssignmentOverlay:matches_filters(unit_id)
    local unit = df.unit.find(unit_id)
    if not unit then return false end
    return info.matches_squad_burrow_filters(unit, self.subviews.subset:getOptionValue(),
        self.subviews.squad:getOptionValue(), self.subviews.burrow:getOptionValue())
end

local function clicked_on_mask(self, keys)
    if not keys._MOUSE_L then return false end
    for _,mask in ipairs{'top_mask', 'wide_mask', 'narrow_mask'} do
        if utils.getval(self.subviews[mask].visible) then
            if self.subviews[mask]:getMousePos() then
                return true
            end
        end
    end
    return false
end

function BurrowAssignmentOverlay:onInput(keys)
    return BurrowAssignmentOverlay.super.onInput(self, keys) or
        clicked_on_mask(self, keys)
end

function BurrowAssignmentOverlay:onRenderFrame(dc, rect)
    local sw = get_screen_width()
    self.frame.w = math.min(DEFAULT_OVERLAY_WIDTH, sw - 94)
    BurrowAssignmentOverlay.super.onRenderFrame(self, dc, rect)
end

return _ENV
