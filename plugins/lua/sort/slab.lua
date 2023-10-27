local _ENV = mkmodule('plugins.sort.slab')

local gui = require('gui')
local sortoverlay = require('plugins.sort.sortoverlay')
local widgets = require('gui.widgets')

local building = df.global.game.main_interface.building

-- ----------------------
-- SlabOverlay
--

SlabOverlay = defclass(SlabOverlay, sortoverlay.SortOverlay)
SlabOverlay.ATTRS{
    default_pos={x=-40, y=12},
    viewscreens='dwarfmode/ViewSheets/BUILDING/Workshop',
    frame={w=57, h=3},
}

function SlabOverlay:init()
    local panel = widgets.Panel{
        frame_background=gui.CLEAR_PEN,
        visible=self:callback('get_key'),
    }
    panel:addviews{
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
        widgets.BannerPanel{
            frame={l=0, t=2, r=0, h=1},
            subviews={
                widgets.ToggleHotkeyLabel{
                    view_id='only_needs_slab',
                    frame={l=1, t=0, r=1},
                    label="Show only citizens who need a slab:",
                    key='CUSTOM_SHIFT_E',
                    initial_option=false,
                    on_change=function() self:do_search(self.subviews.search.text, true) end,
                },
            },
        },
    }
    self:addviews{panel}

    self:register_handler('SLAB', building.filtered_button,
        curry(sortoverlay.single_vector_search,
            {
                get_search_key_fn=self:callback('get_search_key'),
                matches_filters_fn=self:callback('matches_filters'),
            }))
end

function SlabOverlay:onInput(keys)
    if SlabOverlay.super.onInput(self, keys) then return true end
    if keys._MOUSE_L and self:get_key() and self:getMousePos() then
        return true
    end
end

function SlabOverlay:get_key()
    -- DF fails to set building.category back to NONE if there are no units that
    -- can be memorialized, so we have to manually check for a populated button vector
    if #building.button > 0 and
        building.category == df.interface_category_building.SELECT_MEMORIAL_UNIT
    then
        return 'SLAB'
    else
        if safe_index(self.state, 'SLAB', 'saved_original') then
            -- elements get freed as soon as the screen changes
            self.state.SLAB.saved_original = nil
        end
    end
end

function SlabOverlay:reset()
    SlabOverlay.super.reset(self)
    self.subviews.only_needs_slab:setOption(false, false)
end

local function get_unit(if_button)
    local hf = df.historical_figure.find(if_button.spec_id)
    return hf and df.unit.find(hf.unit_id) or nil
end

function SlabOverlay:get_search_key(if_button)
    local unit = get_unit(if_button)
    if not unit then return if_button.filter_str end
    return sortoverlay.get_unit_search_key(unit)
end

local function needs_slab(if_button)
    local unit = get_unit(if_button)
    if not unit then return false end
    if not dfhack.units.isOwnGroup(unit) then return false end
    local info = dfhack.toSearchNormalized(if_button.info)
    if not info:find('no slabs engraved', 1, true) then return false end
    return info:find('not memorialized', 1, true) or info:find('ghost', 1, true)
end

function SlabOverlay:matches_filters(if_button)
    local only_needs_slab = self.subviews.only_needs_slab:getOptionValue()
    return not only_needs_slab or needs_slab(if_button)
end

return _ENV
