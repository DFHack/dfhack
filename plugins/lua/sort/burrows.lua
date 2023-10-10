local _ENV = mkmodule('plugins.sort.burrows')

local sortoverlay = require('plugins.sort.sortoverlay')
local widgets = require('gui.widgets')

local unit_selector = df.global.game.main_interface.unit_selector

-- ----------------------
-- BurrowOverlay
--

BurrowOverlay = defclass(BurrowOverlay, sortoverlay.SortOverlay)
BurrowOverlay.ATTRS{
    default_pos={x=62, y=6},
    viewscreens='dwarfmode/UnitSelector/BURROW_ASSIGNMENT',
    frame={w=26, h=1},
}

local function get_unit_id_search_key(unit_id)
    local unit = df.unit.find(unit_id)
    if not unit then return end
    return ('%s %s %s'):format(
        dfhack.units.getReadableName(unit),  -- last name is in english
        dfhack.units.getProfessionName(unit),
        dfhack.TranslateName(unit.name, false, true))  -- get untranslated last name
end

function BurrowOverlay:init()
    self:addviews{
        widgets.BannerPanel{
            frame={l=0, t=0, r=0, h=1},
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

    self:register_handler('BURROW', unit_selector.unid,
        curry(sortoverlay.flags_vector_search, {get_search_key_fn=get_unit_id_search_key},
            unit_selector.selected))
end

function BurrowOverlay:get_key()
    if unit_selector.context == df.unit_selector_context_type.BURROW_ASSIGNMENT then
        return 'BURROW'
    end
end

function BurrowOverlay:onRenderBody(dc)
    BurrowOverlay.super.onRenderBody(self, dc)
    if self.refresh_search then
        self.refresh_search = nil
        self:do_search(self.subviews.search.text)
    end
end

function BurrowOverlay:onInput(keys)
    if keys._MOUSE_L then
        self.refresh_search = true
    end
    return BurrowOverlay.super.onInput(self, keys)
end

return _ENV
