-- zone-related data and logic for the quickfort script
--@ module = true

if not dfhack_flags.module then
    qerror('this script cannot be called directly')
end

require('dfhack.buildings') -- loads additional functions into dfhack.buildings
local utils = require('utils')
local quickfort_common = reqscript('internal/quickfort/common')
local quickfort_building = reqscript('internal/quickfort/building')
local quickfort_parse = reqscript('internal/quickfort/parse')

local log = quickfort_common.log
local logfn = quickfort_common.logfn

local function parse_pit_pond_props(zone_data, props)
    if props.pond == 'true' then
        ensure_key(zone_data, 'zone_settings').pit_pond = df.building_civzonest.T_zone_settings.T_pit_pond.top_of_pond
        props.pond = nil
    end
end

local function parse_gather_props(zone_data, props)
    if props.pick_trees == 'false' then
        ensure_keys(zone_data, 'zone_settings', 'gather').pick_trees = false
        props.pick_trees = nil
    end
    if props.pick_shrubs == 'false' then
        ensure_keys(zone_data, 'zone_settings', 'gather').pick_shrubs = false
        props.pick_shrubs = nil
    end
    if props.gather_fallen == 'false' then
        ensure_keys(zone_data, 'zone_settings', 'gather').gather_fallen = false
        props.gather_fallen = nil
    end
end

local function parse_archery_props(zone_data, props)
    if not props.shoot_from then return end
    local archery = ensure_keys(zone_data, 'zone_settings', 'archery')
    if props.shoot_from == 'west' or props.shoot_from == 'left' then
        archery.dir_x = 1
        archery.dir_y = 0
        props.shoot_from = nil
    elseif props.shoot_from == 'east' or props.shoot_from == 'right' then
        archery.dir_x = -1
        archery.dir_y = 0
        props.shoot_from = nil
    elseif props.shoot_from == 'north' or props.shoot_from == 'top' then
        archery.dir_x = 0
        archery.dir_y = 1
        props.shoot_from = nil
    elseif props.shoot_from == 'south' or props.shoot_from == 'bottom' then
        archery.dir_x = 0
        archery.dir_y = -1
        props.shoot_from = nil
    end
end

local function parse_tomb_props(zone_data, props)
    if props.pets == 'true' then
        ensure_keys(zone_data, 'zone_settings', 'tomb').no_pets = false
        props.pets = nil
    end
    if props.citizens == 'false' then
        ensure_keys(zone_data, 'zone_settings', 'tomb').no_citizens = true
        props.citizens = nil
    end
end

local function is_valid_zone_tile(pos)
    return not dfhack.maps.getTileFlags(pos).hidden
end

local function is_valid_zone_extent(s)
    for _, col in ipairs(s.extent_grid) do
        for _, in_extent in ipairs(col) do
            if in_extent then return true end
        end
    end
    return false
end

local function merge_db_entries(self, other)
    if self.label ~= other.label then
        error(('cannot merge db entries of different types: %s != %s'):format(self.label, other.label))
    end
    if other.data then
        for i=1,#self.data do
            utils.assign(self.data[i], other.data[i] or {})
        end
    end
end

local zone_template = {
    has_extents=true, min_width=1, max_width=math.huge, min_height=1, max_height=math.huge,
    is_valid_tile_fn=is_valid_zone_tile,
    is_valid_extent_fn=is_valid_zone_extent,
    merge_fn=merge_db_entries,
}

local zone_db_raw = {
    m={label='Meeting Area', default_data={type=df.civzone_type.MeetingHall}},
    b={label='Bedroom', default_data={type=df.civzone_type.Bedroom}},
    h={label='Dining Hall', default_data={type=df.civzone_type.DiningHall}},
    n={label='Pen/Pasture', default_data={type=df.civzone_type.Pen,
       assign={zone_settings={pen={unk=1}}}}},
    p={label='Pit/Pond', props_fn=parse_pit_pond_props, default_data={type=df.civzone_type.Pond,
       assign={zone_settings={pit_pond=df.building_civzonest.T_zone_settings.T_pit_pond.top_of_pit}}}},
    w={label='Water Source', default_data={type=df.civzone_type.WaterSource}},
    j={label='Dungeon', default_data={type=df.civzone_type.Dungeon}},
    f={label='Fishing', default_data={type=df.civzone_type.FishingArea}},
    s={label='Sand', default_data={type=df.civzone_type.SandCollection}},
    o={label='Office', default_data={type=df.civzone_type.Office}},
    D={label='Dormitory', default_data={type=df.civzone_type.Dormitory}},
    B={label='Barracks', default_data={type=df.civzone_type.Barracks}},
    a={label='Archery Range', props_fn=parse_archery_props, default_data={type=df.civzone_type.ArcheryRange,
       assign={zone_settings={archery={dir_x=1, dir_y=0}}}}},
    d={label='Garbage Dump', default_data={type=df.civzone_type.Dump}},
    t={label='Animal Training', default_data={type=df.civzone_type.AnimalTraining}},
    T={label='Tomb', props_fn=parse_tomb_props, default_data={type=df.civzone_type.Tomb,
       assign={zone_settings={tomb={whole=1}}}}},
    g={label='Gather/Pick Fruit', props_fn=parse_gather_props, default_data={type=df.civzone_type.PlantGathering,
       assign={zone_settings={gather={pick_trees=true, pick_shrubs=true, gather_fallen=true}}}}},
    c={label='Clay', default_data={type=df.civzone_type.ClayCollection}},
}
for _, v in pairs(zone_db_raw) do
    utils.assign(v, zone_template)
    ensure_key(v.default_data, 'assign').is_active = 8 -- set to active by default
end

-- we may want to offer full name aliases for the single letter ones above
local aliases = {}

local valid_locations = {
    tavern={new=df.abstract_building_inn_tavernst,
        assign={name={type=df.language_name_type.SymbolFood},
                contents={desired_goblets=10, desired_instruments=5,
                need_more={goblets=true, instruments=true}}}},
    hospital={new=df.abstract_building_hospitalst,
        assign={name={type=df.language_name_type.Hospital},
                contents={desired_splints=5, desired_thread=75000,
                desired_cloth=50000, desired_crutches=5, desired_powder=750,
                desired_buckets=2, desired_soap=750, need_more={splints=true,
                thread=true, cloth=true, crutches=true, powder=true,
                buckets=true, soap=true}}}},
    guildhall={new=df.abstract_building_guildhallst,
        assign={name={type=df.language_name_type.Guildhall}}},
    library={new=df.abstract_building_libraryst,
        assign={name={type=df.language_name_type.Library},
                contents={desired_paper=10, need_more={paper=true}}}},
    temple={new=df.abstract_building_templest,
        assign={name={type=df.language_name_type.Temple},
                deity_data={Religion=-1},
                contents={desired_instruments=5, need_more={instruments=true}}}},
}
local valid_restrictions = {
    visitors={AllowVisitors=true, AllowResidents=true, OnlyMembers=false},
    residents={AllowVisitors=false, AllowResidents=true, OnlyMembers=false},
    citizens={AllowVisitors=false, AllowResidents=false, OnlyMembers=false},
    members={AllowVisitors=false, AllowResidents=false, OnlyMembers=true},
}
for _, v in pairs(valid_locations) do
    ensure_key(v, 'assign').flags = valid_restrictions.visitors
    ensure_key(v.assign, 'name').has_name = true
    ensure_key(v.assign.name, 'parts_of_speech').resize = false
    v.assign.name.parts_of_speech.FirstAdjective = df.part_of_speech.Adjective
end

local prop_prefix = 'desired_'

local function parse_location_props(props)
    if not props.location then return nil end
    local token_and_label = quickfort_parse.parse_token_and_label(props.location, 1)
    props.location = nil
    if not valid_locations[token_and_label.token] then
        dfhack.printerr(('ignoring invalid location type: "%s"'):format(token_and_label.token))
        return nil
    end
    local location_data = {
        type=token_and_label.token,
        label=token_and_label.label,
        data={},
    }
    if props.allow then
        if valid_restrictions[props.allow] then
            location_data.data.flags = copyall(valid_restrictions[props.allow])
        else
            dfhack.printerr(('ignoring invalid allow value: "%s"'):format(props.allow))
        end
        props.allow = nil
    end
    if location_data.type == 'guildhall' and props.profession then
        local profession = df.profession[props.profession:upper()]
        if not profession then
            dfhack.printerr(('ignoring invalid guildhall profession: "%s"'):format(props.profession))
        else
            ensure_key(location_data.data, 'contents').profession = profession
        end
        props.profession = nil
    end
    if safe_index(valid_locations[location_data.type], 'assign', 'contents') then
        for k in pairs(valid_locations[location_data.type].assign.contents) do
            if not k:startswith(prop_prefix) then goto continue end
            local short_prop = k:sub(#prop_prefix+1)
            if props[short_prop] then
                local prop = props[short_prop]
                props[short_prop] = nil
                local val = tonumber(prop)
                if not val or val ~= math.floor(val) or val < 0 or val > 999 then
                    dfhack.printerr(('ignoring invalid %s value: "%s"'):format(short_prop, prop))
                    goto continue
                end
                if short_prop == 'thread' then val = val * 15000
                elseif short_prop == 'cloth' then val = val * 10000
                elseif short_prop == 'powder' or short_prop == 'soap' then
                    val = val * 150
                end
                ensure_key(location_data.data, 'contents')[k] = val
                ensure_keys(location_data.data, 'contents', 'need_more')[short_prop] = (val > 0)
            end
            ::continue::
        end
    end
    return location_data
end

local function get_noble_unit(noble)
    local unit = dfhack.units.getUnitByNobleRole(noble)
    if not unit then log('could not find a noble position for: "%s"', noble) end
    return unit
end

local function parse_zone_config(c, props)
    if not rawget(zone_db_raw, c) then
        return 'Invalid', nil
    end
    local zone_data = {}
    local db_entry = zone_db_raw[c]
    utils.assign(zone_data, db_entry.default_data)
    zone_data.location = parse_location_props(props)
    if props.active == 'false' then
        zone_data.is_active = 0
        props.active = nil
    end
    if props.name then
        zone_data.name = props.name
        props.name = nil
    end
    if props.assigned_unit then
        zone_data.assigned_unit = get_noble_unit(props.assigned_unit)
        if not zone_data.assigned_unit and props.assigned_unit:lower() == 'sheriff' then
            zone_data.assigned_unit = get_noble_unit('captain_of_the_guard')
        end
        if not zone_data.assigned_unit then
            dfhack.printerr(('could not find a unit assigned to noble position: "%s"'):format(props.assigned_unit))
        end
        props.assigned_unit = nil
    end
    if db_entry.props_fn then db_entry.props_fn(zone_data, props) end

    for k,v in pairs(props) do
        dfhack.printerr(('unhandled property: "%s"="%s"'):format(k, v))
    end

    return db_entry.label, zone_data
end

local zone_key_pattern = '%a' -- just one letter

local function custom_zone(_, keys)
    local labels, set_global_label = {}, false
    local db_entry = {data={}}
    local token_and_label, props_start_pos = quickfort_parse.parse_token_and_label(keys, 1, zone_key_pattern)
    while token_and_label do
        local props, next_token_pos = quickfort_parse.parse_properties(keys, props_start_pos)
        local label, zone_data = parse_zone_config(token_and_label.token, props)
        if token_and_label.label then
            label = ('%s/%s'):format(label, token_and_label.label)
            set_global_label = true
        end
        table.insert(labels, label)
        table.insert(db_entry.data, zone_data)
        token_and_label, props_start_pos = quickfort_parse.parse_token_and_label(keys, next_token_pos, zone_key_pattern)
    end
    db_entry.label = table.concat(labels, '+')
    if set_global_label then
        db_entry.global_label = db_entry.label
    end
    utils.assign(db_entry, zone_template)
    return db_entry
end

local zone_db = {}
setmetatable(zone_db, {__index=custom_zone})

local word_table = df.global.world.raws.language.word_table[0][35]

local function generate_name()
    local adj_index = math.random(0, #word_table.words.Adjectives - 1)
    local thex_index = math.random(0, #word_table.words.TheX - 1)
    return {words={
        resize=false,
        FirstAdjective=word_table.words.Adjectives[adj_index],
        TheX=word_table.words.TheX[thex_index],
    }}
end

local function set_location(zone, location, ctx)
    if location.type == 'guildhall' and not safe_index(location.data, 'contents', 'profession') then
        dfhack.printerr('cannot create a guildhall without a specified profession')
        return
    end
    local site = df.global.world.world_data.active_site[0]
    local loc_id = nil
    if location.label and safe_index(ctx, 'zone', 'locations', location.label) then
        local cached_loc = ctx.zone.locations[location.label]
        loc_id = cached_loc.id
        local bld = cached_loc.bld
        for flag, val in pairs(location.data.flags or {}) do
            bld.flags[flag] = val
        end
    end
    if not loc_id then
        loc_id = site.next_building_id
        local data = copyall(valid_locations[location.type])
        utils.assign(data, location.data)
        data.name = generate_name()
        data.id = loc_id
        data.site_id = site.id
        data.pos = copyall(site.pos)
        for _,entity_site_link in ipairs(site.entity_links) do
            local he = df.historical_entity.find(entity_site_link.entity_id)
            if he and he.type == df.historical_entity_type.SiteGovernment then
                data.site_owner_id = he.id
                break
            end
        end
        site.buildings:insert('#', data)
        site.next_building_id = site.next_building_id + 1
        -- fix up BitArray flags (which don't seem to get set by the insert above)
        local bld = site.buildings[#site.buildings-1]
        for flag, val in pairs(data.assign.flags) do
            bld.flags[flag] = val
        end
        for flag, val in pairs(data.flags or {}) do
            bld.flags[flag] = val
        end
        bld.contents.building_ids:insert('#', zone.id)
        if location.label then
            -- remember this location for future associations in this blueprint
            local cached_loc = ensure_keys(ctx, 'zone', 'locations', location.label)
            cached_loc.id = loc_id
            cached_loc.flags = data.flags
            cached_loc.bld = bld
        end
    end
    zone.site_id = site.id
    zone.location_id = loc_id
    -- recategorize the civzone as attached to a location
    zone:uncategorize()
    zone:categorize(true)
end

local function create_zone(zone, data, ctx)
    local extents, ntiles =
            quickfort_building.make_extents(zone, ctx.dry_run)
    if ctx.dry_run then return ntiles end
    local fields = {
        assigned_unit_id=-1,
        room={x=zone.pos.x, y=zone.pos.y, width=zone.width, height=zone.height,
            extents=extents},
    }
    local bld, err = dfhack.buildings.constructBuilding{
        type=df.building_type.Civzone, subtype=data.type,
        abstract=true, pos=zone.pos, width=zone.width, height=zone.height,
        fields=fields}
    if not bld then
        -- this is an error instead of a qerror since our validity checking
        -- is supposed to prevent this from ever happening
        error(string.format('unable to designate zone: %s', err))
    end
    if data.location then
        data = copyall(data)
        set_location(bld, data.location, ctx)
        data.location = nil
    end
    if data.assigned_unit then
        dfhack.buildings.setOwner(bld, data.assigned_unit)
        data.assigned_unit = nil
    end
    utils.assign(bld, data)
    return ntiles
end

function do_run(zlevel, grid, ctx)
    local stats = ctx.stats
    stats.zone_designated = stats.zone_designated or
            {label='Zones designated', value=0, always=true}
    stats.zone_tiles = stats.zone_tiles or
            {label='Zone tiles designated', value=0}
    stats.zone_occupied = stats.zone_occupied or
            {label='Zone tiles skipped (tile occupied)', value=0}

    local zones = {}
    stats.invalid_keys.value =
            stats.invalid_keys.value + quickfort_building.init_buildings(
                ctx, zlevel, grid, zones, zone_db, aliases, true)
    stats.out_of_bounds.value =
            stats.out_of_bounds.value + quickfort_building.crop_to_bounds(ctx, zones)
    stats.zone_occupied.value =
            stats.zone_occupied.value +
            quickfort_building.check_tiles_and_extents(ctx, zones)

    for _,zone in ipairs(zones) do
        if not zone.pos then goto continue end
        local db_entry = zone.db_entry
        log('creating %s zone(s) at map coordinates (%d, %d, %d), defined' ..
            ' from spreadsheet cells: %s',
            db_entry.label, zone.pos.x, zone.pos.y, zone.pos.z,
            table.concat(zone.cells, ', '))
        for _,data in ipairs(db_entry.data or {}) do
            log('creating zone with properties:')
            logfn(printall_recurse, data)
            local ntiles = create_zone(zone, data, ctx)
            stats.zone_tiles.value = stats.zone_tiles.value + ntiles
            stats.zone_designated.value = stats.zone_designated.value + 1
        end
        ::continue::
    end
end

function do_orders()
    log('nothing to do for blueprints in mode: zone')
end

local function get_activity_zones(pos)
    local activity_zones = {}
    local civzones = dfhack.buildings.findCivzonesAt(pos)
    if not civzones then return activity_zones end
    for _,civzone in ipairs(civzones) do
        table.insert(activity_zones, civzone)
    end
    return activity_zones
end

function do_undo(zlevel, grid, ctx)
    local stats = ctx.stats
    stats.zone_removed = stats.zone_removed or
            {label='Zones removed', value=0, always=true}

    local zones = {}
    stats.invalid_keys.value =
            stats.invalid_keys.value + quickfort_building.init_buildings(
                ctx, zlevel, grid, zones, zone_db, aliases, true)

    -- ensure we don't delete the currently selected zone, which causes crashes.
    local selected_zone = dfhack.gui.getSelectedCivZone(true)

    for _, zone in ipairs(zones) do
        for extent_x, col in ipairs(zone.extent_grid) do
            for extent_y, in_extent in ipairs(col) do
                if not in_extent then goto continue end
                local pos = xyz2pos(zone.pos.x+extent_x-1,
                                    zone.pos.y+extent_y-1, zone.pos.z)
                local activity_zones = get_activity_zones(pos)
                for _,activity_zone in ipairs(activity_zones) do
                    log('removing zone at map coordinates (%d, %d, %d)',
                        pos.x, pos.y, pos.z)
                    if not ctx.dry_run then
                        if activity_zone == selected_zone then
                            dfhack.printerr('cannot remove actively selected zone.')
                            dfhack.printerr('please deselect the zone and try again.')
                        else
                            dfhack.buildings.deconstruct(activity_zone)
                        end
                    end
                    stats.zone_removed.value = stats.zone_removed.value + 1
                end
                ::continue::
            end
        end
    end
end
