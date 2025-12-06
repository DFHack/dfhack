local _ENV = mkmodule('plugins.sort.places')

local sortoverlay = require('plugins.sort.sortoverlay')
local locationselector = require('plugins.sort.locationselector')
local info_overlay = require('plugins.sort.info')
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
    [df.civzone_type.Barracks] = 'Barracks',
    [df.civzone_type.ArcheryRange] = 'Archery Range',
    [df.civzone_type.Dump] = 'Garbage Dump',
    [df.civzone_type.AnimalTraining] = 'Animal Training',
    [df.civzone_type.Tomb] = 'Tomb',
    [df.civzone_type.PlantGathering] = 'Gather Fruit',
    [df.civzone_type.ClayCollection] = 'Clay'
}

-- I used strings rather than df.civzone_type because nobody is going to search "MeadHall" they're going to search "Tavern"
local language_name_types = {
    [df.language_name_type.FoodStore] = 'Inn Tavern',
    [df.language_name_type.Temple] = 'Temple',
    [df.language_name_type.Hospital] = 'Hospital',
    [df.language_name_type.Guildhall] = 'Guildhall',
    [df.language_name_type.Library] = 'Library',
}

local stockpile_settings_flag_names = {
    animals = 'Animal',
    food = 'Food',
    furniture = 'Furniture',
    corpses = 'Corpse',
    refuse = 'Refuse',
    stone = 'Stone',
    ammo = 'Ammo',
    coins = 'Coins',
    bars_blocks = 'Bars/Blocks',
    gems = 'Gems',
    finished_goods = 'Finished Goods',
    leather = 'Leather',
    cloth = 'Cloth',
    wood = 'Wood',
    weapons = 'Weapon',
    armor = 'Armor',
    sheet = 'Sheet'
}

local function get_location_religion(religion_id, religion_type)
    if religion_type == df.religious_practice_type.NONE then return 'Temple'
    else return locationselector.get_religion_string(religion_id, religion_type) or '' end
end

local function get_default_zone_name(zone_type)
    return zone_names[zone_type] or ''
end

local function get_zone_search_key(zone)
    local result = {}

    -- allow zones to be searchable by their name
    if #zone.name == 0 then
        table.insert(result, get_default_zone_name(zone.type))
    else
        table.insert(result, zone.name)
    end

    -- allow zones w/ assignments to be searchable by their assigned unit
    local owner = dfhack.buildings.getOwner(zone)
    if owner then
        table.insert(result, sortoverlay.get_unit_search_key(owner))
    end

    -- allow zones to be searchable by type
    if zone.location_id == -1 then -- zone is NOT a special location and we don't need to do anything special for type searching
        table.insert(result, df.civzone_type[zone.type]);
    else -- zone is a special location and we need to get its type from world data
        local site = dfhack.world.getCurrentSite() or {}
        local building, success = utils.binsearch(site.buildings or {}, zone.location_id, 'id')

        if success and building.name then
            table.insert(result, language_name_types[building.name.type] or '')
            if building.name.has_name then
                table.insert(result, dfhack.translation.translateName(building.name, true))
            end
        end
    end

    -- allow barracks to be searchable by assigned squad
    for _, squad in ipairs(zone.squad_room_info) do
        table.insert(result, dfhack.military.getSquadName(squad.squad_id))
    end

    return table.concat(result, ' ')
end

local function get_location_search_key(zone)
    local site = dfhack.world.getCurrentSite() or {}
    local result = {}

    -- get language_name and type (we dont need user-given zone name because it does not appear on this page)
    local building, success = utils.binsearch(site.buildings or {}, zone.location_id, 'id')
    if success and building.name then
        table.insert(result, language_name_types[building.name.type] or '')
        if building.name.has_name then
            table.insert(result, dfhack.translation.translateName(building.name, true))
        end

        -- for temples and guildhalls, get assigned organization
        if df.abstract_building_templest:is_instance(building) then
            table.insert(result, get_location_religion(building.deity_data.Deity or building.deity_data.Religion, building.deity_type))
        elseif df.abstract_building_guildhallst:is_instance(building) then
            local dwarfified_profession = locationselector.get_profession_string(building.contents.profession)
            table.insert(result, dwarfified_profession)
        end

    end

    return table.concat(result, ' ')
end

local function get_stockpile_search_key(stockpile)
    local result = {}
    if #stockpile.name ~= 0 then table.insert(result, stockpile.name) end

    local flags = stockpile.settings.flags
    for flag, name in pairs(stockpile_settings_flag_names) do
        if flags[flag] then table.insert(result, name) end
    end

    table.insert(result, ('Stockpile #%s'):format(stockpile.stockpile_number))
    return table.concat(result, ' ')
end

local function get_workshop_search_key(workshop)
    local result = {}
    for _, unit_id in ipairs(workshop.profile.permitted_workers) do
        local unit = df.unit.find(unit_id)
        if unit then table.insert(result, sortoverlay.get_unit_search_key(unit)) end
    end

    table.insert(result, workshop.name)
    table.insert(result, df.workshop_type.attrs[workshop.type].name or '')
    table.insert(result, df.workshop_type[workshop.type])

    return table.concat(result, ' ')
end

local function get_farmplot_search_key(farmplot)
    local result = {}

    if #farmplot.name ~= 0 then table.insert(result, farmplot.name) else table.insert(result, 'Farm Plot') end

    local plant = df.plant_raw.find(farmplot.plant_id[farmplot.last_season])
    if plant then table.insert(result, plant.name_plural) end

    return table.concat(result, ' ')
end

---@param siege_engine df.building_siegeenginest
---@return string
local function siege_engine_type(siege_engine)
    if siege_engine.type == df.siegeengine_type.BoltThrower then
        return 'Bolt Thrower'
    end
    return df.siegeengine_type[siege_engine.type]
end

---@param siege_engine df.building_siegeenginest
---@return string
local function siege_engine_status(siege_engine)
    -- portions of return value with with underscores are to allow easier
    -- word-anchored matching even when the DFHack full-text search mode is
    -- enabled; e.g.
    -- - "loaded" would match "Loaded" and "Unloaded",
    --   - but "_loaded" would only match "_Loaded"
    local count = 0
    local count_all = siege_engine.type == df.siegeengine_type.BoltThrower
    for _, building_item in ipairs(siege_engine.contained_items) do
        if building_item.use_mode == df.building_item_role_type.TEMP then
            if not count_all then
                return 'Loaded _Loaded'
            end
            count = count + building_item.item:getStackSize()
        end
    end
    if count_all and count > 0 then
        return ('%d bolts _%d_bolts'):format(count, count)
    end
    return 'Unloaded'
end

---@param siege_engine df.building_siegeenginest
---@return string
local function siege_engine_job_status(siege_engine)
    for _, job in ipairs(siege_engine.jobs) do
        if job.job_type == df.job_type.LoadCatapult
            or job.job_type == df.job_type.LoadBallista
            or job.job_type == df.job_type.LoadBoltThrower
        then
            if dfhack.job.getWorker(job) ~= nil then
                return 'Loading'
            else
                return 'Inactive load task'
            end
        end
        local firing_bolt_thrower = job.job_type == df.job_type.FireBoltThrower
        local firing = job.job_type == df.job_type.FireCatapult
            or job.job_type == df.job_type.FireBallista
            or firing_bolt_thrower
        if firing then
            local unit = dfhack.job.getWorker(job)
            if unit == nil then
                return 'No operator'
            else
                ---@type integer?, integer?, integer?
                local x, y, z = dfhack.units.getPosition(unit)
                -- DF shows "present" when the unit is inside the building's
                -- footprint (or, for bolt throwers, next to it); the unit does
                -- not need to be at the exact firing position tile (which
                -- varies based on siege engine type and direction)
                if x ~= nil and z == siege_engine.z then
                    ---@cast y integer
                    if firing_bolt_thrower then
                        if siege_engine.x1 - 1 <= x and x <= siege_engine.x2 + 1
                            and siege_engine.y1 - 1 <= y and y <= siege_engine.y2 + 1
                        then
                            return 'Operator present'
                        end
                    elseif dfhack.buildings.containsTile(siege_engine, x, y) then
                        return 'Operator present'
                    end
                end
                return 'Operator assigned'
            end
        end
    end
    return ''
end

---@param siege_engine df.building_siegeenginest
---@return string
local function get_siege_engine_search_key(siege_engine)
    -- DF 53.05 Info window, Places tab, Siege Engines subtab shows this info:
    --       name: assigned name or siege engine type name
    --     status: "Unloaded", "Loaded", "<N> bolts"
    -- job status:
    --             - "Inactive load task" (load job unassigned),
    --             - "Loading" (load job assigned),
    --             - "No operator" (fire job unassigned),
    --             - "Operator present" (fire job assigned),
    --             - "Operator assigned" (fire job assigned, but not in position),
    --             - blank
    --     action: (icons) fire-at-will, practice, prepare-to-fire, keep-loaded, not-in-use
    --             These have associated text blurbs that are shown in the
    --             building info window, but those texts are not discoverable
    --             from the Info > Places > Siege engine list view.
    local result = {}

    if #siege_engine.name ~= 0 then table.insert(result, siege_engine.name) end

    table.insert(result, siege_engine_type(siege_engine))

    table.insert(result, siege_engine_status(siege_engine))

    table.insert(result, siege_engine_job_status(siege_engine))

    return table.concat(result, ' ')
end

-- ----------------------
-- PlacesOverlay
--

PlacesOverlay = defclass(PlacesOverlay, sortoverlay.SortOverlay)
PlacesOverlay.ATTRS{
    desc='Adds search functionality to the places overview screens.',
    default_pos={x=52, y=9},
    version=2,
    viewscreens='dwarfmode/Info',
    frame={w=40, h=6}
}

function PlacesOverlay:init()
    self:addviews{
        widgets.BannerPanel{
            view_id='panel',
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
    self:register_handler('LOCATIONS', buildings.list[df.buildings_mode_type.LOCATIONS], curry(sortoverlay.single_vector_search, {get_search_key_fn=get_location_search_key}))
    self:register_handler('STOCKPILES', buildings.list[df.buildings_mode_type.STOCKPILES], curry(sortoverlay.single_vector_search, {get_search_key_fn=get_stockpile_search_key}))
    self:register_handler('WORKSHOPS', buildings.list[df.buildings_mode_type.WORKSHOPS], curry(sortoverlay.single_vector_search, {get_search_key_fn=get_workshop_search_key}))
    self:register_handler('FARMPLOTS', buildings.list[df.buildings_mode_type.FARMPLOTS], curry(sortoverlay.single_vector_search, {get_search_key_fn=get_farmplot_search_key}))
    self:register_handler('SIEGE_ENGINES', buildings.list[df.buildings_mode_type.SIEGE_ENGINES], curry(sortoverlay.single_vector_search, {get_search_key_fn=get_siege_engine_search_key}))
end

function PlacesOverlay:get_key()
    if info.current_mode == df.info_interface_mode_type.BUILDINGS then
        return df.buildings_mode_type[buildings.mode]
    end
end

function PlacesOverlay:updateFrames()
    local ret = info_overlay.resize_overlay(self)
    local l, t = info_overlay.get_panel_offsets()
    local frame = self.subviews.panel.frame
    if frame.l == l and frame.t == t then return ret end
    frame.l, frame.t = l, t
    return true
end

function PlacesOverlay:onRenderBody(dc)
    PlacesOverlay.super.onRenderBody(self, dc)
    if self:updateFrames() then
        self:updateLayout()
    end
end

return _ENV
