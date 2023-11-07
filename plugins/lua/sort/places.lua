local _ENV = mkmodule('plugins.sort.places')

local sortoverlay = require('plugins.sort.sortoverlay')
local widgets = require('gui.widgets')

local info = df.global.game.main_interface.info
local buildings = info.buildings

local function get_default_zone_name(zone_type)
    if zone_type == df.civzone_type.Dump then return 'Garbage dump' end
    if zone_type == df.civzone_type.WaterSource then return 'Water source' end
    if zone_type == df.civzone_type.SandCollection then return 'Sand' end
    if zone_type == df.civzone_type.FishingArea then return 'Fishing' end
    if zone_type == df.civzone_type.Pond then return 'Pit/pond' end
    if zone_type == df.civzone_type.Pen then return 'Pen/pasture' end
    if zone_type == df.civzone_type.ClayCollection then return 'Clay' end
    if zone_type == df.civzone_type.AnimalTraining then return 'Animal training' end
    if zone_type == df.civzone_type.PlantGathering then return 'Gather fruit' end
    if zone_type == df.civzone_type.Dungeon then return 'Dungeon' end
    if zone_type == df.civzone_type.MeetingHall then return 'Meeting area' end
    if zone_type == df.civzone_type.Barracks then return 'Barracks' end
    if zone_type == df.civzone_type.ArcheryRange then return 'Archery range' end
    if zone_type == df.civzone_type.Office then return 'Office' end
    if zone_type == df.civzone_type.DiningHall then return 'Dining hall' end
    if zone_type == df.civzone_type.Dormitory then return 'Dormitory' end
    if zone_type == df.civzone_type.Bedroom then return 'Bedroom' end
    if zone_type == df.civzone_type.Tomb then return 'Tomb' end

    return '' -- If zone_type is anything else it will not have a default name populated in the Zones page when left nameless
end

local function get_zone_search_key(zone)
    local site = df.global.world.world_data.active_site[0]
    local result = {}

    -- allow zones to be searchable by their name
    if zone.name == nil or zone.name == '' then
        table.insert(result, get_default_zone_name(zone.type))
    else
        table.insert(result, zone.name)
    end

    -- allow zones w/ assignments to be searchable by their assigned unit
    if zone.assigned_unit ~= nil then
        table.insert(result, dfhack.TranslateName(zone.assigned_unit.name))
        table.insert(result, dfhack.units.getReadableName(zone.assigned_unit))
    end

    -- allow zones to be searchable by type
    if zone.location_id == -1 then -- zone is NOT a special location and we don't need to do anything special for type searching
        table.insert(result, df.civzone_type[zone.type]);
    else -- zone is a special location and we need to get its type from world data
        table.insert(result, dfhack.TranslateName(site.buildings[zone.location_id].name, true))
        table.insert(result, df.civzone_type[site.buildings[zone.location_id].name.type])
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
    default_pos={x=71, y=8},
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

function PlacesOverlay:onRenderBody(dc)
    PlacesOverlay.super.onRenderBody(self, dc)
    if self.refresh_search then
        self.refresh_search = nil
        self:do_search(self.subviews.search.text)
    end
end

function PlacesOverlay:onInput(keys)
    if keys._MOUSE_L then
        self.refresh_search = true
    end
    return PlacesOverlay.super.onInput(self, keys)
end

return _ENV
