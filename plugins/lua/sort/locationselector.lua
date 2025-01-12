local _ENV = mkmodule('plugins.sort.locationselector')

local sortoverlay = require('plugins.sort.sortoverlay')
local widgets = require('gui.widgets')

local location_selector = df.global.game.main_interface.location_selector

-- ----------------------
-- LocationSelectorOverlay
--

LocationSelectorOverlay = defclass(LocationSelectorOverlay, sortoverlay.SortOverlay)
LocationSelectorOverlay.ATTRS{
    desc='Adds search and filter capabilities to the temple and guildhall establishment screens.',
    default_pos={x=48, y=6},
    viewscreens='dwarfmode/LocationSelector',
    frame={w=26, h=3},
}

local function add_spheres(hf, spheres)
    if not hf then return end
    for _, sphere in ipairs(hf.info.spheres.spheres) do
        spheres[sphere] = true
    end
end

local function stringify_spheres(spheres)
    local strs = {}
    for sphere in pairs(spheres) do
        table.insert(strs, df.sphere_type[sphere])
    end
    return table.concat(strs, ' ')
end

function get_religion_string(religion_id, religion_type)
    if religion_id == -1 then return end
    local entity
    local spheres = {}
    if religion_type == df.religious_practice_type.WORSHIP_HFID then
        entity = df.historical_figure.find(religion_id)
        add_spheres(entity, spheres)
    elseif religion_type == df.religious_practice_type.RELIGION_ENID then
        entity = df.historical_entity.find(religion_id)
        if entity then
            for _, deity in ipairs(entity.relations.deities) do
                add_spheres(df.historical_figure.find(deity), spheres)
            end
        end
    end
    if not entity then return end
    return ('%s %s'):format(dfhack.translation.translateName(entity.name, true), stringify_spheres(spheres))
end

function get_profession_string(profession)
    local profession_string = df.profession[profession]:gsub('_', ' ')
    local dwarfified_string = profession_string:gsub('[Mm][Aa][Nn]', 'dwarf')
    return profession_string .. ' ' .. dwarfified_string
end

function LocationSelectorOverlay:init()
    local panel = widgets.Panel{
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
                    view_id='hide_established',
                    frame={l=1, t=0, r=1},
                    label="Hide established:",
                    key='CUSTOM_SHIFT_E',
                    initial_option=false,
                    on_change=function() self:do_search(self.subviews.search.text, true) end,
                },
            },
        },
    }
    self:addviews{panel}

    self:register_handler('TEMPLE', location_selector.valid_religious_practice_id,
        curry(sortoverlay.flags_vector_search,
            {
                get_search_key_fn=get_religion_string,
                matches_filters_fn=self:callback('matches_temple_filter'),
            },
            location_selector.valid_religious_practice))
    self:register_handler('GUILDHALL', location_selector.valid_craft_guild_type,
        curry(sortoverlay.single_vector_search,
            {
                get_search_key_fn=get_profession_string,
                matches_filters_fn=self:callback('matches_guildhall_filter'),
            }))
end

function LocationSelectorOverlay:get_key()
    if location_selector.choosing_temple_religious_practice then
        return 'TEMPLE'
    elseif location_selector.choosing_craft_guild then
        return 'GUILDHALL'
    end
end

function LocationSelectorOverlay:reset()
    LocationSelectorOverlay.super.reset(self)
    self.cache = nil
    self.subviews.hide_established:setOption(false, false)
end

function LocationSelectorOverlay:get_cache()
    if self.cache then return self.cache end
    local cache = {}
    local site = dfhack.world.getCurrentSite() or {}
    for _,location in ipairs(site.buildings or {}) do
        if df.abstract_building_templest:is_instance(location) then
            ensure_keys(cache, 'temple', location.deity_type)[location.deity_data.Religion] = true
        elseif df.abstract_building_guildhallst:is_instance(location) then
            ensure_keys(cache, 'guildhall')[location.contents.profession] = true
        end
    end
    self.cache = cache
    return self.cache
end

function LocationSelectorOverlay:matches_temple_filter(id, flag)
    local hide_established = self.subviews.hide_established:getOptionValue()
    return not hide_established or not safe_index(self:get_cache(), 'temple', flag, id)
end

function LocationSelectorOverlay:matches_guildhall_filter(id)
    local hide_established = self.subviews.hide_established:getOptionValue()
    return not hide_established or not safe_index(self:get_cache(), 'guildhall', id)
end

return _ENV
