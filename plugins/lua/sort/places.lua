local _ENV = mkmodule('plugins.sort.places')

local sortoverlay = require('plugins.sort.sortoverlay')
local widgets = require('gui.widgets')
local utils = require('utils')

local info = df.global.game.main_interface.info
local buildings = info.buildings

local zone_names = {
    [df.civzone_type.MeetingHall] = 'Meeting Area',
    [df.civzone_type.Bedroom] = 'Bedroom',
    [df.civzone_type.DiningHall] = 'Dining Hall',
    [df.civzone_type.Pen] = 'Pen Pasture',
    [df.civzone_type.Pond] = 'Pit Pond',
    [df.civzone_type.WaterSource] = 'Water Source',
    [df.civzone_type.Dungeon] = 'Dungeon',
    [df.civzone_type.FishingArea] = 'Fishing',
    [df.civzone_type.SandCollection] = 'Sand',
    [df.civzone_type.Office] = 'Office',
    [df.civzone_type.Dormitory] = 'Dormitory',
    [df.civzone_type.Barracks] = 'Barrachs',
    [df.civzone_type.ArcheryRange] = 'Archery Range',
    [df.civzone_type.Dump] = 'Garbage Dump',
    [df.civzone_type.AnimalTraining] = 'Animal Training',
    [df.civzone_type.Tomb] = 'Tomb',
    [df.civzone_type.PlantGathering] = 'Gather Fruit',
    [df.civzone_type.ClayCollection] = 'Clay'
}

-- I used strings rather than df.civzone_type because nobody is going to search "MeadHall" they're going to search "Tavern"
local language_name_types = {
    [df.language_name_type.SymbolFood] = 'Inn Tavern',
    [df.language_name_type.Temple] = 'Temple',
    [df.language_name_type.Hospital] = 'Hospital',
    [df.language_name_type.Guildhall] = 'Guildhall'
}

local function get_default_zone_name(zone_type)
    return zone_names[zone_type] or ''
end

local function get_zone_search_key(zone)
    local site = df.global.world.world_data.active_site[0]
    local result = {}

    -- allow zones to be searchable by their name
    if #zone.name == 0 then
        table.insert(result, get_default_zone_name(zone.type))
    else
        table.insert(result, zone.name)
    end

    -- allow zones w/ assignments to be searchable by their assigned unit
    if zone.assigned_unit ~= nil then
        table.insert(result, sortoverlay.get_unit_search_key(zone.assigned_unit))
    end

    -- allow zones to be searchable by type
    if zone.location_id == -1 then -- zone is NOT a special location and we don't need to do anything special for type searching
        table.insert(result, df.civzone_type[zone.type]);
    else -- zone is a special location and we need to get its type from world data
        local building, success, _ = utils.binsearch(site.buildings, zone.location_id, 'id')
        if success and building.name then
            table.insert(result, language_name_types[building.name.type] or '')
            if building.name.has_name then
                table.insert(result, dfhack.TranslateName(building.name, true))
            end
        end
    end

    -- allow barracks to be searchable by assigned squad
    for _, squad in ipairs(zone.squad_room_info) do
        table.insert(result, dfhack.military.getSquadName(squad.squad_id))
    end

    return table.concat(result, ' ')
end

-- ----------------------
-- PlacesOverlay
--

PlacesOverlay = defclass(PlacesOverlay, sortoverlay.SortOverlay)
PlacesOverlay.ATTRS{
    default_pos={x=71, y=9},
    viewscreens='dwarfmode/Info',
    frame={w=40, h=6}
}

function PlacesOverlay:init()
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

    self:register_handler('ZONES', buildings.list[df.buildings_mode_type.ZONES], curry(sortoverlay.single_vector_search, {get_search_key_fn=get_zone_search_key}))
end

function PlacesOverlay:get_key()
    if info.current_mode == df.info_interface_mode_type.BUILDINGS then
        -- TODO: Replace nested if with 'return df.buildings_mode_type[buildings.mode]' once other handlers are written
        -- Not there right now so it doesn't render a search bar on unsupported Places subpages
        if buildings.mode == df.buildings_mode_type.ZONES then
            return 'ZONES'
        end
    end
end

return _ENV
