-- dig-related logic for the quickfort script
--@ module = true
--[[
This file designates tiles with the same rules as the in-game UI. For example,
if the tile is hidden, we designate blindly to avoid spoilers. If it's visible,
the shape and material of the target tile affects whether the designation has
any effect.
]]

if not dfhack_flags.module then
    qerror('this script cannot be called directly')
end

local utils = require('utils')
local quickfort_common = reqscript('internal/quickfort/common')
local quickfort_map = reqscript('internal/quickfort/map')
local quickfort_parse = reqscript('internal/quickfort/parse')
local quickfort_preview = reqscript('internal/quickfort/preview')
local quickfort_set = reqscript('internal/quickfort/set')
local quickfort_transform = reqscript('internal/quickfort/transform')

local log = quickfort_common.log

local function is_construction(tileattrs)
    return tileattrs.material == df.tiletype_material.CONSTRUCTION
end

local function is_floor(tileattrs)
    return tileattrs.shape == df.tiletype_shape.FLOOR
end

local function is_ramp(tileattrs)
    return tileattrs.shape == df.tiletype_shape.RAMP
end

local function is_diggable_floor(tileattrs)
    return is_floor(tileattrs) or
            tileattrs.shape == df.tiletype_shape.BOULDER or
            tileattrs.shape == df.tiletype_shape.PEBBLES
end

local function is_wall(tileattrs)
    return tileattrs.shape == df.tiletype_shape.WALL
end

local function is_tree(tileattrs)
    return tileattrs.material == df.tiletype_material.TREE
end

local function is_fortification(tileattrs)
    return tileattrs.shape == df.tiletype_shape.FORTIFICATION
end

local function is_up_stair(tileattrs)
    return tileattrs.shape == df.tiletype_shape.STAIR_UP
end

local function is_down_stair(tileattrs)
    return tileattrs.shape == df.tiletype_shape.STAIR_DOWN
end

local function is_removable_shape(tileattrs)
    return is_ramp(tileattrs) or
            is_up_stair(tileattrs) or
            is_down_stair(tileattrs)
end

local function is_gatherable(tileattrs)
    return tileattrs.shape == df.tiletype_shape.SHRUB
end

local function is_sapling(tileattrs)
    return tileattrs.shape == df.tiletype_shape.SAPLING
end

local hard_natural_materials = utils.invert({
    df.tiletype_material.STONE,
    df.tiletype_material.FEATURE,
    df.tiletype_material.LAVA_STONE,
    df.tiletype_material.MINERAL,
    df.tiletype_material.FROZEN_LIQUID,
})

local function is_hard(tileattrs)
    return hard_natural_materials[tileattrs.material]
end

local function is_smooth(tileattrs)
    return tileattrs.special == df.tiletype_special.SMOOTH
end

-- TODO: it would be useful to migrate has_designation and clear_designation to
-- the Maps module
local function has_designation(flags, occupancy)
    return flags.dig ~= df.tile_dig_designation.No or
            flags.smooth > 0 or
            occupancy.carve_track_north or
            occupancy.carve_track_east or
            occupancy.carve_track_south or
            occupancy.carve_track_west
end

local function clear_designation(flags, occupancy)
    flags.dig = df.tile_dig_designation.No
    flags.smooth = 0
    occupancy.carve_track_north = 0
    occupancy.carve_track_east = 0
    occupancy.carve_track_south = 0
    occupancy.carve_track_west = 0
end

local values = nil

local values_run = {
    dig_default=df.tile_dig_designation.Default,
    dig_channel=df.tile_dig_designation.Channel,
    dig_upstair=df.tile_dig_designation.UpStair,
    dig_downstair=df.tile_dig_designation.DownStair,
    dig_updownstair=df.tile_dig_designation.UpDownStair,
    dig_ramp=df.tile_dig_designation.Ramp,
    dig_no=df.tile_dig_designation.No,
    tile_smooth=1,
    tile_engrave=2,
    track=1,
    item_claimed=false,
    item_forbidden=true,
    item_melted=true,
    item_unmelted=false,
    item_dumped=true,
    item_undumped=false,
    item_hidden=true,
    item_unhidden=false,
    traffic_normal=0,
    traffic_low=1,
    traffic_high=2,
    traffic_restricted=3,
}

-- undo isn't guaranteed to restore what was set on the tile before the last
-- 'run' command; it just sets a sensible default. we could implement true undo
-- if there is demand, though.
local values_undo = {
    dig_default=df.tile_dig_designation.No,
    dig_channel=df.tile_dig_designation.No,
    dig_upstair=df.tile_dig_designation.No,
    dig_downstair=df.tile_dig_designation.No,
    dig_updownstair=df.tile_dig_designation.No,
    dig_ramp=df.tile_dig_designation.No,
    dig_no=df.tile_dig_designation.No,
    tile_smooth=0,
    tile_engrave=0,
    track=0,
    item_claimed=false,
    item_forbidden=false,
    item_melted=false,
    item_unmelted=false,
    item_dumped=false,
    item_undumped=false,
    item_hidden=false,
    item_unhidden=false,
    traffic_normal=0,
    traffic_low=0,
    traffic_high=0,
    traffic_restricted=0,
}

-- these functions return a function if a designation needs to be made; else nil
local function do_mine(digctx)
    if digctx.on_map_edge then return nil end
    if not digctx.flags.hidden then -- always designate if the tile is hidden
        if is_construction(digctx.tileattrs) or
                (not is_wall(digctx.tileattrs) and
                 not is_fortification(digctx.tileattrs)) then
            return nil
        end
    end
    return function() digctx.flags.dig = values.dig_default end
end

local function do_chop(digctx)
    if digctx.flags.hidden then return nil end
    if is_tree(digctx.tileattrs) then
        return function() digctx.flags.dig = values.dig_default end
    end
    return function() end -- noop, but not an error
end

local function do_channel(digctx)
    if digctx.on_map_edge then return nil end
    if not digctx.flags.hidden then -- always designate if the tile is hidden
        if is_construction(digctx.tileattrs) or
                is_tree(digctx.tileattrs) or
                (not is_wall(digctx.tileattrs) and
                 not is_fortification(digctx.tileattrs) and
                 not is_diggable_floor(digctx.tileattrs) and
                 not is_down_stair(digctx.tileattrs) and
                 not is_removable_shape(digctx.tileattrs) and
                 not is_gatherable(digctx.tileattrs) and
                 not is_sapling(digctx.tileattrs)) then
            return nil
        end
    end
    return function() digctx.flags.dig = values.dig_channel end
end

local function do_up_stair(digctx)
    if digctx.on_map_edge then return nil end
    if not digctx.flags.hidden then -- always designate if the tile is hidden
        if is_construction(digctx.tileattrs) or
                (not is_wall(digctx.tileattrs) and
                 not is_fortification(digctx.tileattrs)) then
            return nil
        end
    end
    return function() digctx.flags.dig = values.dig_upstair end
end

local function do_down_stair(digctx)
    if digctx.on_map_edge then return nil end
    if not digctx.flags.hidden then -- always designate if the tile is hidden
        if is_construction(digctx.tileattrs) or
                is_tree(digctx.tileattrs) or
                (not is_wall(digctx.tileattrs) and
                 not is_fortification(digctx.tileattrs) and
                 not is_diggable_floor(digctx.tileattrs) and
                 not is_removable_shape(digctx.tileattrs) and
                 not is_gatherable(digctx.tileattrs) and
                 not is_sapling(digctx.tileattrs)) then
            return nil
        end
    end
    return function() digctx.flags.dig = values.dig_downstair end
end

local function do_up_down_stair(digctx)
    if digctx.on_map_edge then return nil end
    if not digctx.flags.hidden then -- always designate if the tile is hidden
        if is_construction(digctx.tileattrs) or
                (not is_wall(digctx.tileattrs) and
                 not is_fortification(digctx.tileattrs) and
                 not is_up_stair(digctx.tileattrs)) then
            return nil
        end
    end
    if is_up_stair(digctx.tileattrs) then
        return function() digctx.flags.dig = values.dig_downstair end
    end
    return function() digctx.flags.dig = values.dig_updownstair end
end

local function do_ramp(digctx)
    if digctx.on_map_edge then return nil end
    if not digctx.flags.hidden then -- always designate if the tile is hidden
        if is_construction(digctx.tileattrs) or
                (not is_wall(digctx.tileattrs) and
                 not is_fortification(digctx.tileattrs)) then
            return nil
        end
    end
    return function() digctx.flags.dig = values.dig_ramp end
end

local function do_remove_ramps(digctx)
    if digctx.on_map_edge or digctx.flags.hidden then return nil end
    if is_construction(digctx.tileattrs) or
            not is_removable_shape(digctx.tileattrs) then
        return nil;
    end
    return function() digctx.flags.dig = values.dig_default end
end

local function do_gather(digctx)
    if digctx.flags.hidden then return nil end
    if not is_gatherable(digctx.tileattrs) then
        return function() end
    end
    return function() digctx.flags.dig = values.dig_default end
end

local function do_smooth(digctx)
    if digctx.flags.hidden then return nil end
    if is_construction(digctx.tileattrs) or
            not is_hard(digctx.tileattrs) or
            is_smooth(digctx.tileattrs) or
            not (is_floor(digctx.tileattrs) or is_wall(digctx.tileattrs)) then
        return nil
    end
    return function() digctx.flags.smooth = values.tile_smooth end
end

local function do_engrave(digctx)
    if digctx.flags.hidden or
            not is_smooth(digctx.tileattrs) or
            not (is_floor(digctx.tileattrs) or is_wall(digctx.tileattrs)) or
            digctx.engraving ~= nil then
        return nil
    end
    return function() digctx.flags.smooth = values.tile_engrave end
end

local function do_fortification(digctx)
    if digctx.flags.hidden then return nil end
    if not is_wall(digctx.tileattrs) or
            not is_smooth(digctx.tileattrs) then return nil end
    return function() digctx.flags.smooth = values.tile_smooth end
end

local function do_track(digctx)
    if digctx.on_map_edge or
            digctx.flags.hidden or
            is_construction(digctx.tileattrs) or
            not (is_floor(digctx.tileattrs) or is_ramp(digctx.tileattrs)) or
            not is_hard(digctx.tileattrs) then
        return nil
    end
    local direction, occupancy = digctx.direction, digctx.occupancy
    return function()
        -- don't overwrite all directions, only 'or' in the new bits. we could
        -- be adding to a previously-designated track.
        if direction.north then occupancy.carve_track_north = values.track end
        if direction.east then occupancy.carve_track_east = values.track end
        if direction.south then occupancy.carve_track_south = values.track end
        if direction.west then occupancy.carve_track_west = values.track end
    end
end

local function do_toggle_engravings(digctx)
    if digctx.flags.hidden then return nil end
    local engraving = digctx.engraving
    if engraving == nil then return nil end
    return function() engraving.flags.hidden = not engraving.flags.hidden end
end

local function do_toggle_marker(digctx)
    if not has_designation(digctx.flags, digctx.occupancy) then return nil end
    return function()
        digctx.occupancy.dig_marked = not digctx.occupancy.dig_marked end
end

local function do_remove_construction(digctx)
    if digctx.flags.hidden or not is_construction(digctx.tileattrs) then
        return nil
    end
    return function() digctx.flags.dig = values.dig_default end
end

local function do_remove_designation(digctx)
    if not has_designation(digctx.flags, digctx.occupancy) then return nil end
    return function() clear_designation(digctx.flags, digctx.occupancy) end
end

local function is_valid_item(item)
    return not item.flags.garbage_collect
end

local function get_items_at(pos, include_buildings)
    local items = {}
    if include_buildings then
        local bld = dfhack.buildings.findAtTile(pos)
        if bld and same_xyz(pos, xyz2pos(bld.centerx, bld.centery, bld.z)) then
            for _, contained_item in ipairs(bld.contained_items) do
                if is_valid_item(contained_item.item) then
                    table.insert(items, contained_item.item)
                end
            end
        end
    end
    for _, item_id in ipairs(dfhack.maps.getTileBlock(pos).items) do
        local item = df.item.find(item_id)
        if same_xyz(pos, item.pos) and
                is_valid_item(item) and item.flags.on_ground then
            table.insert(items, item)
        end
    end
    return items
end

local function do_item_flag(digctx, flag_name, flag_value, include_buildings)
    if digctx.flags.hidden then return nil end
    local items = get_items_at(digctx.pos, include_buildings)
    if #items == 0 then return function() end end -- noop, but not an error
    return function()
        for _,item in ipairs(items) do item.flags[flag_name] = flag_value end
    end
end

local function do_claim(digctx)
    return do_item_flag(digctx, "forbid", values.item_claimed, true)
end

local function do_forbid(digctx)
    return do_item_flag(digctx, "forbid", values.item_forbidden, true)
end

local function do_melt(digctx)
    -- the game appears to autoremove the flag from unmeltable items, so we
    -- don't actually need to do any filtering here
    return do_item_flag(digctx, "melt", values.item_melted, false)
end

local function do_remove_melt(digctx)
    return do_item_flag(digctx, "melt", values.item_unmelted, false)
end

local function do_dump(digctx)
    return do_item_flag(digctx, "dump", values.item_dumped, false)
end

local function do_remove_dump(digctx)
    return do_item_flag(digctx, "dump", values.item_undumped, false)
end

local function do_hide(digctx)
    return do_item_flag(digctx, "hidden", values.item_hidden, true)
end

local function do_unhide(digctx)
    return do_item_flag(digctx, "hidden", values.item_unhidden, true)
end

local function do_traffic_high(digctx)
    if digctx.flags.hidden then return nil end
    return function() digctx.flags.traffic = values.traffic_high end
end

local function do_traffic_normal(digctx)
    if digctx.flags.hidden then return nil end
    return function() digctx.flags.traffic = values.traffic_normal end
end

local function do_traffic_low(digctx)
    if digctx.flags.hidden then return nil end
    return function() digctx.flags.traffic = values.traffic_low end
end

local function do_traffic_restricted(digctx)
    if digctx.flags.hidden then return nil end
    return function() digctx.flags.traffic = values.traffic_restricted end
end

local unit_vectors = quickfort_transform.unit_vectors
local unit_vectors_revmap = quickfort_transform.unit_vectors_revmap

local track_end_data = {
    N=unit_vectors.north,
    E=unit_vectors.east,
    S=unit_vectors.south,
    W=unit_vectors.west
}
local track_end_revmap = {
    [unit_vectors_revmap.north]='N',
    [unit_vectors_revmap.east]='E',
    [unit_vectors_revmap.south]='S',
    [unit_vectors_revmap.west]='W'
}

local track_through_data = {
    NS=unit_vectors.north,
    EW=unit_vectors.east
}
local track_through_revmap = {
    [unit_vectors_revmap.north]='NS',
    [unit_vectors_revmap.east]='EW',
    [unit_vectors_revmap.south]='NS',
    [unit_vectors_revmap.west]='EW'
}

local track_corner_data = {
    NE={x=1, y=-2},
    NW={x=-2, y=-1},
    SE={x=2, y=1},
    SW={x=-1, y=2}
}
local track_corner_revmap = {
    ['x=1, y=-2'] = 'NE',
    ['x=2, y=-1'] = 'NE',
    ['x=2, y=1'] = 'SE',
    ['x=1, y=2'] = 'SE',
    ['x=-1, y=2'] = 'SW',
    ['x=-2, y=1'] = 'SW',
    ['x=-2, y=-1'] = 'NW',
    ['x=-1, y=-2'] = 'NW'
}

local track_tee_data = {
    NSE={x=1, y=-2},
    NEW={x=-2, y=-1},
    SEW={x=2, y=1},
    NSW={x=-1, y=2}
}
local track_tee_revmap = {
    ['x=1, y=-2'] = 'NSE',
    ['x=2, y=-1'] = 'NEW',
    ['x=2, y=1'] = 'SEW',
    ['x=1, y=2'] = 'NSE',
    ['x=-1, y=2'] = 'NSW',
    ['x=-2, y=1'] = 'SEW',
    ['x=-2, y=-1'] = 'NEW',
    ['x=-1, y=-2'] = 'NSW'
}

local function make_transform_track_fn(vector, revmap)
    return function(ctx)
        return 'track' .. quickfort_transform.resolve_transformed_vector(ctx, vector, revmap)
    end
end
local function make_track_entry(name, data, revmap)
    local transform = nil
    if data and revmap then
        transform = make_transform_track_fn(data[name], revmap)
    end
    return {action=do_track, use_priority=true, can_clobber_engravings=true,
            direction={single_tile=true, north=name:find('N'),
                       south=name:find('S'), east=name:find('E'),
                       west=name:find('W')},
            transform=transform}
end

local dig_db = {
    d={action=do_mine, use_priority=true, can_clobber_engravings=true},
    h={action=do_channel, use_priority=true, can_clobber_engravings=true},
    u={action=do_up_stair, use_priority=true, can_clobber_engravings=true},
    j={action=do_down_stair, use_priority=true, can_clobber_engravings=true},
    i={action=do_up_down_stair, use_priority=true, can_clobber_engravings=true},
    r={action=do_ramp, use_priority=true, can_clobber_engravings=true},
    z={action=do_remove_ramps, use_priority=true},
    t={action=do_chop, use_priority=true},
    p={action=do_gather, use_priority=true},
    s={action=do_smooth, use_priority=true},
    e={action=do_engrave, use_priority=true},
    F={action=do_fortification, use_priority=true, can_clobber_engravings=true},
    T={action=do_track, use_priority=true, can_clobber_engravings=true},
    v={action=do_toggle_engravings},
    -- the semantics are unclear if the code is M but m or force_marker_mode is
    -- also specified. skipping all other marker mode settings when toggling
    -- marker mode seems to make the most sense.
    M={action=do_toggle_marker, skip_marker_mode=true},
    n={action=do_remove_construction, use_priority=true},
    x={action=do_remove_designation},
    bc={action=do_claim},
    bf={action=do_forbid},
    bm={action=do_melt},
    bM={action=do_remove_melt},
    bd={action=do_dump},
    bD={action=do_remove_dump},
    bh={action=do_hide},
    bH={action=do_unhide},
    oh={action=do_traffic_high},
    on={action=do_traffic_normal},
    ol={action=do_traffic_low},
    ['or']={action=do_traffic_restricted},
    -- single-tile track aliases
    trackN=make_track_entry('N', track_end_data, track_end_revmap),
    trackS=make_track_entry('S', track_end_data, track_end_revmap),
    trackE=make_track_entry('E', track_end_data, track_end_revmap),
    trackW=make_track_entry('W', track_end_data, track_end_revmap),
    trackNS=make_track_entry('NS', track_through_data, track_through_revmap),
    trackEW=make_track_entry('EW', track_through_data, track_through_revmap),
    trackNE=make_track_entry('NE', track_corner_data, track_corner_revmap),
    trackNW=make_track_entry('NW', track_corner_data, track_corner_revmap),
    trackSE=make_track_entry('SE', track_corner_data, track_corner_revmap),
    trackSW=make_track_entry('SW', track_corner_data, track_corner_revmap),
    trackNSE=make_track_entry('NSE', track_tee_data, track_tee_revmap),
    trackNSW=make_track_entry('NSW', track_tee_data, track_tee_revmap),
    trackNEW=make_track_entry('NEW', track_tee_data, track_tee_revmap),
    trackSEW=make_track_entry('SEW', track_tee_data, track_tee_revmap),
    trackNSEW=make_track_entry('NSEW'),
}

-- add trackramp aliases for the track aliases
-- (trackramps are just tracks carved over ramps)
dig_db.trackrampN = dig_db.trackN
dig_db.trackrampS = dig_db.trackS
dig_db.trackrampE = dig_db.trackE
dig_db.trackrampW = dig_db.trackW
dig_db.trackrampNS = dig_db.trackNS
dig_db.trackrampNE = dig_db.trackNE
dig_db.trackrampNW = dig_db.trackNW
dig_db.trackrampSE = dig_db.trackSE
dig_db.trackrampSW = dig_db.trackSW
dig_db.trackrampEW = dig_db.trackEW
dig_db.trackrampNSE = dig_db.trackNSE
dig_db.trackrampNSW = dig_db.trackNSW
dig_db.trackrampNEW = dig_db.trackNEW
dig_db.trackrampSEW = dig_db.trackSEW
dig_db.trackrampNSEW = dig_db.trackNSEW

-- set default dig priorities
for _,v in pairs(dig_db) do
    if v.use_priority then v.priority = 4 end
end

-- handles marker mode 'm' prefix and priority suffix
local function extended_parser(_, keys)
    local marker_mode = false
    if keys:startswith('m') then
        keys = string.sub(keys, 2)
        marker_mode = true
    end
    local found, _, code, priority = keys:find('^(%D*)(%d*)$')
    if not found then return nil end
    if #priority == 0 then
        priority = 4
    else
        priority = tonumber(priority)
        if priority < 1 or priority > 7 then
            log('priority must be between 1 and 7 (inclusive)')
            return nil
        end
    end
    if #code == 0 then code = 'd' end
    if not rawget(dig_db, code) then return nil end
    local custom_designate = copyall(dig_db[code])
    custom_designate.marker_mode = marker_mode
    custom_designate.priority = priority
    return custom_designate
end

setmetatable(dig_db, {__index=extended_parser})

local function get_priority_block_square_event(block_events)
    for i,v in ipairs(block_events) do
        if v:getType() == df.block_square_event_type.designation_priority then
            return v
        end
    end
    return nil
end

-- modifies any existing priority block_square_event to the specified priority.
-- if the block_square_event doesn't already exist, create it.
local function set_priority(digctx, priority)
    log('setting priority to %d', priority)
    local block_events = dfhack.maps.getTileBlock(digctx.pos).block_events
    local pbse = get_priority_block_square_event(block_events)
    if not pbse then
        block_events:insert('#',
                            {new=df.block_square_event_designation_priorityst})
        pbse = block_events[#block_events-1]
    end
    pbse.priority[digctx.pos.x % 16][digctx.pos.y % 16] = priority * 1000
end

local function dig_tile(digctx, db_entry)
    local action_fn = db_entry.action(digctx)
    if not action_fn then return nil end
    return function()
        action_fn()
        -- set the block's designated flag so the game does a check to see what
        -- jobs need to be created
        dfhack.maps.getTileBlock(digctx.pos).flags.designated = true
        if not has_designation(digctx.flags, digctx.occupancy) then
            -- reset marker mode and priority to defaults
            digctx.occupancy.dig_marked = false
            set_priority(digctx, 4)
        else
            if not db_entry.skip_marker_mode then
                local marker_mode = db_entry.marker_mode or
                        quickfort_set.get_setting('force_marker_mode')
                digctx.occupancy.dig_marked = marker_mode
            end
            if db_entry.use_priority then
                set_priority(digctx, db_entry.priority)
            end
        end
    end
end

local function ensure_engravings_cache(ctx)
    if ctx.engravings_cache then return end
    local engravings_cache = {}
    for _,engraving in ipairs(df.global.world.engravings) do
        local pos = engraving.pos
        local grid = ensure_key(engravings_cache, pos.z)
        local row = ensure_key(grid, pos.y)
        row[pos.x] = engraving
    end
    ctx.engravings_cache = engravings_cache
end

local function init_dig_ctx(ctx, pos, direction)
    local flags, occupancy = dfhack.maps.getTileFlags(pos)
    if not flags then return end
    local tileattrs = df.tiletype.attrs[dfhack.maps.getTileType(pos)]
    local engraving = nil
    if is_smooth(tileattrs) then
        -- potentially has an engraving
        ensure_engravings_cache(ctx)
        engraving = safe_index(ctx.engravings_cache, pos.z, pos.y, pos.x)
    end
    return {
        pos=pos,
        direction=direction,
        on_map_edge=ctx.bounds:is_on_map_edge(pos),
        flags=flags,
        occupancy=occupancy,
        tileattrs=tileattrs,
        engraving=engraving,
    }
end

local function should_preserve_engraving(ctx, db_entry, engraving)
    if not db_entry.can_clobber_engravings or not engraving then
        return false
    end
    return ctx.preserve_engravings and
            engraving.quality >= ctx.preserve_engravings
end

-- returns a map of which track directions should be enabled
-- width and height can be negative
local function get_track_direction(x, y, width, height)
    local neg_width, w = width < 0, math.abs(width)
    local neg_height, h = height < 0, math.abs(height)

    -- initialize assuming positive width and height
    local north = x == 1 and y > 1
    local east = x < w and y == 1
    local south = x == 1 and y < h
    local west = x > 1 and y == 1

    if neg_width then
        north = x == w and y > 1
        south = x == w and y < h
    end
    if neg_height then
        east = x < w and y == h
        west = x > 1 and y == h
    end

    return {north=north, east=east, south=south, west=west}
end

local function do_run_impl(zlevel, grid, ctx)
    local stats = ctx.stats
    ctx.bounds = ctx.bounds or quickfort_map.MapBoundsChecker{}
    for y, row in pairs(grid) do
        for x, cell_and_text in pairs(row) do
            local cell, text = cell_and_text.cell, cell_and_text.text
            local pos = xyz2pos(x, y, zlevel)
            log('applying spreadsheet cell %s with text "%s" to map' ..
                ' coordinates (%d, %d, %d)', cell, text, pos.x, pos.y, pos.z)
            local db_entry = nil
            local keys, extent = quickfort_parse.parse_cell(ctx, text)
            if keys then db_entry = dig_db[keys] end
            if not db_entry then
                dfhack.printerr(('invalid key sequence: "%s" in cell %s')
                                :format(text, cell))
                stats.invalid_keys.value = stats.invalid_keys.value + 1
                goto continue
            end
            if db_entry.transform then
                db_entry = copyall(db_entry)
                db_entry.direction = dig_db[db_entry.transform(ctx)].direction
            end
            if db_entry.action == do_track and not db_entry.direction and
                    math.abs(extent.width) == 1 and
                    math.abs(extent.height) == 1 then
                dfhack.printerr(('Warning: ambiguous direction for track:' ..
                    ' "%s" in cell %s; please use T(width x height) syntax' ..
                    ' (e.g. specify both width > 1 and height > 1 for a' ..
                    ' track that extends both South and East from this corner')
                               :format(text, cell))
                stats.invalid_keys.value = stats.invalid_keys.value + 1
                goto continue
            end
            if extent.specified then
                -- shift pos to the upper left corner of the extent and convert
                -- the extent dimenions to positive, simplifying the logic below
                pos.x = math.min(pos.x, pos.x + extent.width + 1)
                pos.y = math.min(pos.y, pos.y + extent.height + 1)
            end
            for extent_x=1,math.abs(extent.width) do
                for extent_y=1,math.abs(extent.height) do
                    local extent_pos = xyz2pos(
                        pos.x+extent_x-1,
                        pos.y+extent_y-1,
                        pos.z)
                    if not ctx.bounds:is_on_map(extent_pos) then
                        log('coordinates out of bounds; skipping (%d, %d, %d)',
                            extent_pos.x, extent_pos.y, extent_pos.z)
                        stats.out_of_bounds.value =
                                stats.out_of_bounds.value + 1
                        goto inner_continue
                    end
                    local direction = db_entry.direction or
                            (db_entry.action == do_track and
                             get_track_direction(extent_x, extent_y,
                                                 extent.width, extent.height))
                    local digctx = init_dig_ctx(ctx, extent_pos, direction)
                    if not digctx then goto inner_continue end
                    if db_entry.action == do_smooth or db_entry.action == do_engrave or
                            db_entry.action == do_track then
                        -- can only smooth passable tiles
                        if digctx.occupancy.building > df.tile_building_occ.Passable and
                                digctx.occupancy.building ~= df.tile_building_occ.Dynamic then
                            goto inner_continue
                        end
                    elseif db_entry.action == do_traffic_high or db_entry.action == do_traffic_normal
                        or db_entry.action == do_traffic_low or db_entry.action == do_traffic_restricted
                    then
                        -- pass
                    else
                        -- can't dig through buildings
                        if digctx.occupancy.building ~= 0 then
                            goto inner_continue
                        end
                    end
                    local action_fn = dig_tile(digctx, db_entry)
                    quickfort_preview.set_preview_tile(ctx, extent_pos,
                                                       action_fn ~= nil)
                    if not action_fn then
                        log('cannot apply "%s" to coordinate (%d, %d, %d)',
                            keys, extent_pos.x, extent_pos.y, extent_pos.z)
                        stats.dig_invalid_tiles.value =
                                stats.dig_invalid_tiles.value + 1
                    else
                        if should_preserve_engraving(ctx, db_entry,
                                                     digctx.engraving) then
                            stats.dig_protected_engraving.value =
                                    stats.dig_protected_engraving.value + 1
                        else
                            if not ctx.dry_run then action_fn() end
                            stats.dig_designated.value =
                                    stats.dig_designated.value + 1
                        end
                    end
                    ::inner_continue::
                end
            end
            ::continue::
        end
    end
end

local function ensure_ctx_stats(ctx, prefix)
    local designated_label = ('Tiles %sdesignated for digging'):format(prefix)
    ctx.stats.dig_designated = ctx.stats.dig_designated or
            {label=designated_label, value=0, always=true}
    ctx.stats.dig_invalid_tiles = ctx.stats.dig_invalid_tiles or
            {label='Tiles that could not be designated for digging', value=0}
    ctx.stats.dig_protected_engraving = ctx.stats.dig_protected_engraving or
            {label='Engravings protected from destruction', value=0}
end

function do_run(zlevel, grid, ctx)
    values = values_run
    ensure_ctx_stats(ctx, '')
    do_run_impl(zlevel, grid, ctx)
    if not ctx.dry_run then
        dfhack.job.checkDesignationsNow()
    end
end

function do_orders()
    log('nothing to do for blueprints in mode: dig')
end

function do_undo(zlevel, grid, ctx)
    values = values_undo
    ensure_ctx_stats(ctx, 'un')
    do_run_impl(zlevel, grid, ctx)
end
