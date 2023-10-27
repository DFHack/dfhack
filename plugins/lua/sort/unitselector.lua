local _ENV = mkmodule('plugins.sort.unitselector')

local sortoverlay = require('plugins.sort.sortoverlay')
local widgets = require('gui.widgets')

local unit_selector = df.global.game.main_interface.unit_selector

-- ----------------------
-- UnitSelectorOverlay
--

UnitSelectorOverlay = defclass(UnitSelectorOverlay, sortoverlay.SortOverlay)
UnitSelectorOverlay.ATTRS{
    default_pos={x=62, y=6},
    viewscreens='dwarfmode/UnitSelector',
    frame={w=31, h=1},
    handled_screens=DEFAULT_NIL,
}

local function get_unit_id_search_key(unit_id)
    local unit = df.unit.find(unit_id)
    if not unit then return end
    return sortoverlay.get_unit_search_key(unit)
end

function UnitSelectorOverlay:init()
    self:addviews{
        widgets.BannerPanel{
            frame={l=0, t=0, r=0, h=1},
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

    -- pen, pit, chain, and cage assignment are handled by dedicated screens
    -- squad fill position screen has a specialized overlay
    -- we *could* add search functionality to vanilla screens for pit and cage,
    -- but then we'd have to handle the itemid vector
    self.handled_screens = self.handled_screens or {
        ZONE_BEDROOM_ASSIGNMENT='already',
        ZONE_OFFICE_ASSIGNMENT='already',
        ZONE_DINING_HALL_ASSIGNMENT='already',
        ZONE_TOMB_ASSIGNMENT='already',
        OCCUPATION_ASSIGNMENT='selected',
        BURROW_ASSIGNMENT='selected',
        SQUAD_KILL_ORDER='selected',
    }

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
    if keys._MOUSE_L then
        self.refresh_search = true
    end
    return UnitSelectorOverlay.super.onInput(self, keys)
end

-- ----------------------
-- WorkerAssignmentOverlay
--

WorkerAssignmentOverlay = defclass(WorkerAssignmentOverlay, UnitSelectorOverlay)
WorkerAssignmentOverlay.ATTRS{
    default_pos={x=6, y=6},
    viewscreens='dwarfmode/UnitSelector',
    frame={w=31, h=1},
    handled_screens={WORKER_ASSIGNMENT='selected'},
}

return _ENV
