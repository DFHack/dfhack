local _ENV = mkmodule('plugins.sort.locationselector')

local sortoverlay = require('plugins.sort.sortoverlay')
local widgets = require('gui.widgets')

local location_selector = df.global.game.main_interface.location_selector

-- ----------------------
-- LocationSelectorOverlay
--

LocationSelectorOverlay = defclass(LocationSelectorOverlay, sortoverlay.SortOverlay)
LocationSelectorOverlay.ATTRS{
    default_pos={x=48, y=7},
    viewscreens='dwarfmode/LocationSelector',
    frame={w=26, h=1},
}

local function get_religion_string(religion_id, religion_type)
    if religion_id == -1 then return end
    local entity
    if religion_type == 0 then
        entity = df.historical_figure.find(religion_id)
    elseif religion_type == 1 then
        entity = df.historical_entity.find(religion_id)
    end
    if not entity then return end
    return dfhack.TranslateName(entity.name, true)
end

local function get_profession_string(profession)
    return df.profession[profession]:gsub('_', ' ')
end

function LocationSelectorOverlay:init()
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

    self:register_handler('TEMPLE', location_selector.valid_religious_practice_id,
        curry(sortoverlay.flags_vector_search, {get_search_key_fn=get_religion_string},
        location_selector.valid_religious_practice))
    self:register_handler('GUILDHALL', location_selector.valid_craft_guild_type,
        curry(sortoverlay.single_vector_search, {get_search_key_fn=get_profession_string}))
end

function LocationSelectorOverlay:get_key()
    if location_selector.choosing_temple_religious_practice then
        return 'TEMPLE'
    elseif location_selector.choosing_craft_guild then
        return 'GUILDHALL'
    end
end

return _ENV
