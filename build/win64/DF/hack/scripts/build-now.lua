-- instantly completes unsuspended building construction jobs

local argparse = require('argparse')
local dig_now = require('plugins.dig-now')
local gui = require('gui')
local tiletypes = require('plugins.tiletypes')
local utils = require('utils')

local ok, buildingplan = pcall(require, 'plugins.buildingplan')
if not ok then
    buildingplan = nil
end

local function min_to_max(...)
    local args = {...}
    table.sort(args, function(a, b) return a < b end)
    return table.unpack(args)
end

local function parse_commandline(args)
    local opts = {}
    local positionals = argparse.processArgsGetopt(args, {
            {'h', 'help', handler=function() opts.help = true end},
            {'q', 'quiet', handler=function() opts.quiet = true end},
            {nil, 'really', handler=function() opts.really = true end},
        })

    if positionals[1] == 'help' then opts.help = true end
    if opts.help then return opts end

    if not opts.really then
        qerror('This script is known to cause corruption and crashes with some building types, and the DFHack team is still looking into solutions. To bypass this message, pass the "--really" option to the script.')
    end

    if #positionals >= 1 then
        opts.start = argparse.coords(positionals[1])
        if #positionals >= 2 then
            opts['end'] = argparse.coords(positionals[2])
            opts.start.x, opts['end'].x = min_to_max(opts.start.x,opts['end'].x)
            opts.start.y, opts['end'].y = min_to_max(opts.start.y,opts['end'].y)
            opts.start.z, opts['end'].z = min_to_max(opts.start.z,opts['end'].z)
        else
            opts['end'] = opts.start
        end
    else
        -- default to covering entire map
        opts.start = xyz2pos(0, 0, 0)
        local x, y, z = dfhack.maps.getTileSize()
        opts['end'] = xyz2pos(x-1, y-1, z-1)
    end
    return opts
end

-- gets list of jobs that meet all of the following criteria:
--   is a building construction job
--   has all job_items attached
--   is not suspended
--   target building is within the processing area
local function get_jobs(opts)
    local num_suspended, num_incomplete, num_clipped, jobs = 0, 0, 0, {}
    for _,job in utils.listpairs(df.global.world.jobs.list) do
        if job.job_type ~= df.job_type.ConstructBuilding then goto continue end
        if job.flags.suspend then
            num_suspended = num_suspended + 1
            goto continue
        end

        -- job_items are not items, they're filters that describe the kinds of
        -- items that need to be attached.
        for _,job_item in ipairs(job.job_items) do
            -- we have to check for quantity != 0 instead of just the existence
            -- of the job_item since buildingplan leaves 0-quantity job_items in
            -- place to protect against persistence errors.
            if job_item.quantity > 0 then
                num_incomplete = num_incomplete + 1
                goto continue
            end
        end

        local bld = dfhack.job.getHolder(job)
        if not bld then
            dfhack.printerr(
                'skipping construction job without attached building')
            goto continue
        end

        -- accept building if if any part is within the processing area
        if bld.z < opts.start.z or bld.z > opts['end'].z
                or bld.x2 < opts.start.x or bld.x1 > opts['end'].x
                or bld.y2 < opts.start.y or bld.y1 > opts['end'].y then
            num_clipped = num_clipped + 1
            goto continue
        end

        table.insert(jobs, job)
        ::continue::
    end
    if not opts.quiet then
        if num_suspended > 0 then
            print(('Skipped %d suspended building%s')
                  :format(num_suspended, num_suspended ~= 1 and 's' or ''))
        end
        if num_incomplete > 0 then
            print(('Skipped %d building%s with missing items')
                  :format(num_incomplete, num_incomplete ~= 1 and 's' or ''))
        end
        if num_clipped > 0 then
            print(('Skipped %d building%s out of processing range')
                  :format(num_clipped, num_clipped ~= 1 and 's' or ''))
        end
    end
    return jobs
end

-- returns a list of map blocks that contain items that are in the footprint
local function get_map_blocks_with_items_in_footprint(bld)
    local blockset = {}
    for x = bld.x1,bld.x2 do
        for y = bld.y1,bld.y2 do
            local block = dfhack.maps.ensureTileBlock(x, y, bld.z)
            if block.occupancy[x%16][y%16].item ~= 0 then
                blockset[block] = true
            end
        end
    end
    local blocks = {}
    for block in pairs(blockset) do table.insert(blocks, block) end
    return blocks
end

local function transform(tab, transform_fn)
    local ret = {}
    for k,v in pairs(tab) do
        ret[k] = transform_fn(v)
    end
    return ret
end

local function get_item_id(item)
    return item.id
end

local function get_items_within_footprint(blocks, bld, ignore_items)
    -- can't compare userdata items directly, so we'll compare ids
    local ignore_set = utils.invert(transform(ignore_items, get_item_id))
    local items = {}
    for _,block in ipairs(blocks) do
        for _,itemid in ipairs(block.items) do
            local item = df.item.find(itemid)
            local pos = item.pos
            if item.flags.on_ground and gui.is_in_rect(bld, pos.x, pos.y) then
                if item.flags.in_job then
                    -- if the job that the item is associated with is the one
                    -- for building this building, then that's ok. it will be
                    -- moved directly into the building later.
                    if not ignore_set[item.id] then
                        return false
                    end
                else
                    table.insert(items, item)
                end
            end
        end
    end
    return true, items
end

-- returns whether this is a match and whether we should continue searching
-- beyond this tile
local function is_good_dump_pos(pos)
    local tt = dfhack.maps.getTileType(pos)
    -- reject bad coordinates (or map blocks that haven't been loaded)
    if not tt then return false, false end
    local flags, occupancy = dfhack.maps.getTileFlags(pos)
    local attrs = df.tiletype.attrs[tt]
    local shape_attrs = df.tiletype_shape.attrs[attrs.shape]
    -- reject hidden tiles
    if flags.hidden then return false, false end
    -- reject unwalkable tiles
    if not shape_attrs.walkable then return false, false end
    -- reject footprints within other buildings. this could potentially be
    -- relaxed a bit since we can technically dump items on passable tiles
    -- within other buildings, but that would look messy.
    if occupancy.building ~= df.tile_building_occ.None then
        return false, true
    end
    -- success!
    return true
end

-- noop if pos is in the seen map. otherwise marks pos in the seen map and
-- enqueues pos in queue
local function enqueue_if_unseen(seen, queue, pos)
    if seen[pos.x] and seen[pos.x][pos.y] then return end
    seen[pos.x] = seen[pos.x] or {}
    seen[pos.x][pos.y] = true
    table.insert(queue, pos)
end

local function check_and_flood(seen, queue, pos)
    local is_match, should_flood = is_good_dump_pos(pos)
    if is_match then return pos end
    if not should_flood then return end
    local x, y, z = pos.x, pos.y, pos.z
    enqueue_if_unseen(seen, queue, xyz2pos(x-1, y-1, z))
    enqueue_if_unseen(seen, queue, xyz2pos(x,   y-1, z))
    enqueue_if_unseen(seen, queue, xyz2pos(x+1, y-1, z))
    enqueue_if_unseen(seen, queue, xyz2pos(x-1, y,   z))
    enqueue_if_unseen(seen, queue, xyz2pos(x+1, y,   z))
    enqueue_if_unseen(seen, queue, xyz2pos(x-1, y+1, z))
    enqueue_if_unseen(seen, queue, xyz2pos(x,   y+1, z))
    enqueue_if_unseen(seen, queue, xyz2pos(x+1, y+1, z))
end

-- does a flood search to find the nearest tile where we can freely dump items
local function search_dump_pos(bld)
    local seen = {[bld.centerx]={[bld.centery]=true}}
    local queue, i = {xyz2pos(bld.centerx, bld.centery, bld.z)}, 1
    while queue[i] do
        local good_pos = check_and_flood(seen, queue, queue[i])
        if good_pos then return good_pos end
        queue[i] = nil
        i = i + 1
    end
end

-- uses the flood search algorithm to find a free tile. if that fails, returns
-- the position of the first fort citizen. if that fails, returns the position
-- of the first active unit.
local function get_dump_pos(bld)
    local dump_pos = search_dump_pos(bld)
    if dump_pos then
        return dump_pos
    end
    for _,unit in ipairs(df.global.world.units.active) do
        if dfhack.units.isCitizen(unit) then
            return unit.pos
        end
    end
    -- fall back to position of first active unit
    return df.global.world.units.active[0].pos
end

-- move items away from the construction site
local function clear_footprint(bld, ignore_items)
    -- check building tiles for items. if none exist, exit early with success
    local blocks = get_map_blocks_with_items_in_footprint(bld)
    if #blocks == 0 then return true end
    local ok, items = get_items_within_footprint(blocks, bld, ignore_items)
    if not ok then return false end
    local dump_pos = get_dump_pos(bld)
    for _,item in ipairs(items) do
        if not dfhack.items.moveToGround(item, dump_pos) then return false end
    end
    return true
end

local function get_items(job)
    local items = {}
    for _,item_ref in ipairs(job.items) do
        table.insert(items, item_ref.item)
    end
    return items
end

-- disconnect item from the workshop that it is cluttering, if any
local function disconnect_clutter(item)
    local bld = dfhack.items.getHolderBuilding(item)
    if not bld then return true end
    -- remove from contained items list, fail if not found
    local found = false
    for i,contained_item in ipairs(bld.contained_items) do
        if contained_item.item == item then
            bld.contained_items:erase(i)
            found = true
            break
        end
    end
    if not found then
        dfhack.printerr('failed to find clutter item in expected building')
        return false
    end
    -- remove building ref from item and move item into containing map block
    -- we do this manually instead of calling dfhack.items.moveToGround()
    -- because that function will cowardly refuse to work with items with
    -- BUILDING_HOLDER references (because it could crash the game). However,
    -- we know that this particular setup is safe to work with.
    for i,ref in ipairs(item.general_refs) do
        if ref:getType() == df.general_ref_type.BUILDING_HOLDER then
            item.general_refs:erase(i)
            -- this call can return failure, but it always succeeds in setting
            -- the required item flags and adding the item to the map block,
            -- which is all we care about here. dfhack.items.moveToBuilding()
            -- will fix things up later.
            item:moveToGround(item.pos.x, item.pos.y, item.pos.z)
            return true
        end
    end
    return false
end

-- teleport any items that are not already part of the building to the building
-- center and mark them as part of the building. this handles both partially-
-- built buildings and items that are being carried to the building correctly.
local function attach_items(bld, items)
    for _,item in ipairs(items) do
        -- skip items that have already been brought to the building
        if item.flags.in_building then goto continue end
        -- ensure we have no more holder building references so moveToBuilding
        -- can succeed
        if not disconnect_clutter(item) then return false end
        -- 2 means "make part of bld" (which causes constructions to crash on
        -- deconstruct)
        local use = bld:getType() == df.building_type.Construction and 0 or 2
        if not dfhack.items.moveToBuilding(item, bld, use) then return false end
        ::continue::
    end
    return true
end

-- from observation of vectors sorted by the DF, pos sorting seems to be by x,
-- then by y, then by z
local function pos_cmp(a, b)
    local xcmp = utils.compare(a.x, b.x)
    if xcmp ~= 0 then return xcmp end
    local ycmp = utils.compare(a.y, b.y)
    if ycmp ~= 0 then return ycmp end
    return utils.compare(a.z, b.z)
end

local function get_original_tiletype(pos)
    -- TODO: this is not always exactly the existing tile type. for example,
    -- tracks are ignored
    return dfhack.maps.getTileType(pos)
end

local function reuse_construction(construction, item)
    construction.item_type = item:getType()
    construction.item_subtype = item:getSubtype()
    construction.mat_type = item:getMaterial()
    construction.mat_index = item:getMaterialIndex()
    construction.flags.top_of_wall = false
    construction.flags.no_build_item = true
end

local function create_and_link_construction(pos, item, top_of_wall)
    local construction = df.construction:new()
    utils.assign(construction.pos, pos)
    construction.item_type = item:getType()
    construction.item_subtype = item:getSubtype()
    construction.mat_type = item:getMaterial()
    construction.mat_index = item:getMaterialIndex()
    construction.flags.top_of_wall = top_of_wall
    construction.flags.no_build_item = not top_of_wall
    construction.original_tile = get_original_tiletype(pos)
    utils.insert_sorted(df.global.world.constructions, construction,
                        'pos', pos_cmp)
end

-- maps construction_type to the resulting tiletype
local const_to_tile = {
    [df.construction_type.Fortification] = df.tiletype.ConstructedFortification,
    [df.construction_type.Wall] = df.tiletype.ConstructedPillar,
    [df.construction_type.Floor] = df.tiletype.ConstructedFloor,
    [df.construction_type.UpStair] = df.tiletype.ConstructedStairU,
    [df.construction_type.DownStair] = df.tiletype.ConstructedStairD,
    [df.construction_type.UpDownStair] = df.tiletype.ConstructedStairUD,
    [df.construction_type.Ramp] = df.tiletype.ConstructedRamp,
}
-- fill in all the track mappings, which have nice consistent naming conventions
for i,v in ipairs(df.construction_type) do
    if type(v) ~= 'string' then goto continue end
    local _, _, base, dir = v:find('^(TrackR?a?m?p?)([NSEW]+)')
    if base == 'Track' then
        const_to_tile[i] = df.tiletype['ConstructedFloorTrack'..dir]
    elseif base == 'TrackRamp' then
        const_to_tile[i] = df.tiletype['ConstructedRampTrack'..dir]
    end
    ::continue::
end

local function set_tiletype(pos, tt)
    local block = dfhack.maps.ensureTileBlock(pos)
    block.tiletype[pos.x%16][pos.y%16] = tt
    if tt == df.tiletype.ConstructedPillar then
        block.designation[pos.x%16][pos.y%16].outside = 0
    end
    -- all tiles below this one are now "inside"
    for z = pos.z-1,0,-1 do
        block = dfhack.maps.ensureTileBlock(pos.x, pos.y, z)
        if not block or block.designation[pos.x%16][pos.y%16].outside == 0 then
            return
        end
        block.designation[pos.x%16][pos.y%16].outside = 0
    end
end

local function adjust_tile_above(pos_above, item, construction_type)
    if not dfhack.maps.ensureTileBlock(pos_above) then return end
    local tt_above = dfhack.maps.getTileType(pos_above)
    local shape_above = df.tiletype.attrs[tt_above].shape
    if shape_above ~= df.tiletype_shape.EMPTY
            and shape_above ~= df.tiletype_shape.RAMP_TOP then
        return
    end
    if construction_type == df.construction_type.Wall then
        create_and_link_construction(pos_above, item, true)
        set_tiletype(pos_above, df.tiletype.ConstructedFloor)
    elseif df.construction_type[construction_type]:find('Ramp') then
        set_tiletype(pos_above, df.tiletype.RampTop)
    end
end

-- add new construction to the world list and manage tiletype conversion
local function build_construction(bld)
    -- remember required metadata and get rid of building used for designation
    local item = bld.contained_items[0].item
    local pos = copyall(item.pos)
    local construction_type = bld.type
    dfhack.buildings.deconstruct(bld)

    -- check if we're building on a construction (i.e. building a construction on top of a wall)
    local tiletype = dfhack.maps.getTileType(pos)
    local tileattrs = df.tiletype.attrs[tiletype]
    if tileattrs.material == df.tiletype_material.CONSTRUCTION then
        -- modify the construction to the new type
        local construction, found = utils.binsearch(df.global.world.constructions, pos, 'pos', pos_cmp)
        if not found then
            error('Could not find construction entry for construction tile at ' .. pos.x .. ', ' .. pos.y .. ', ' .. pos.z)
        end
        reuse_construction(construction, item)
    else
        -- add entry to df.global.world.constructions
        create_and_link_construction(pos, item, false)
    end
    -- adjust tiletypes for the construction itself
    set_tiletype(pos, const_to_tile[construction_type])
    if construction_type == df.construction_type.Wall then
        dig_now.link_adjacent_smooth_walls(pos)
    end

    -- for walls and ramps with empty space above, adjust the tile above
    if construction_type == df.construction_type.Wall
            or df.construction_type[construction_type]:find('Ramp') then
        adjust_tile_above(xyz2pos(pos.x, pos.y, pos.z+1), item,
                          construction_type)
    end

    -- a duplicate item will get created on deconstruction due to the
    -- no_build_item flag set in create_and_link_construction; destroy this item
    dfhack.items.remove(item)
end

-- complete architecture, if required, and perform the adjustments the game
-- normally does when a building is built. this logic is reverse engineered from
-- observing game behavior and may be incomplete.
local function build_building(bld)
    if bld:needsDesign() then
        -- unlike "natural" builds, we don't set the architect or builder unit
        -- id. however, this doesn't seem to have any in-game effect.
        local design = bld.design
        design.flags.designed = true
        design.flags.built = true
        design.hitpoints = 80640
        design.max_hitpoints = 80640
    end
    bld:setBuildStage(bld:getMaxBuildStage())
    bld.flags.exists = true
    -- update occupancy flags and build dirt roads (they don't build themselves)
    local bld_type = bld:getType()
    local is_dirt_road = bld_type == df.building_type.RoadDirt
    for x = bld.x1,bld.x2 do
        for y = bld.y1,bld.y2 do
            bld:updateOccupancy(x, y)
            if is_dirt_road and dfhack.buildings.containsTile(bld, x, y) then
                -- note that this does not clear shrubs. we need to figure out
                -- how to do that
                if not tiletypes.tiletypes_setTile(xyz2pos(x, y, bld.z),
                        -1, df.tiletype_material.SOIL, df.tiletype_special.NORMAL, -1) then
                    dfhack.printerr('failed to build tile of dirt road')
                end
            end
        end
    end
    -- all buildings call this, though it only appears to have an effect for
    -- farm plots
    bld:initFarmSeasons()
    -- doors and floodgates link to adjacent smooth walls
    if bld_type == df.building_type.Door or
            bld_type == df.building_type.Floodgate then
        dig_now.link_adjacent_smooth_walls(bld.centerx, bld.centery, bld.z)
    end
end

local function throw(bld, msg)
    msg = msg .. ('; please remove and recreate the %s at (%d, %d, %d)')
                 :format(df.building_type[bld:getType()],
                         bld.centerx, bld.centery, bld.z)
    qerror(msg)
end

-- main script
local opts = parse_commandline({...})
if opts.help then print(dfhack.script_help()) return end

-- ensure buildingplan is up to date so we don't skip buildings just because
-- buildingplan hasn't scanned them yet
if buildingplan then
    buildingplan.doCycle()
end

local num_jobs = 0
for _,job in ipairs(get_jobs(opts)) do
    local bld = dfhack.job.getHolder(job)

    -- retrieve the items attached to the job before we destroy the references
    local items = get_items(job)

    local bld_type = bld:getType()
    if #items == 0 and bld_type ~= df.building_type.RoadDirt
            and bld_type ~= df.building_type.FarmPlot then
        print(('skipping building with no items attached at'..
               ' (%d, %d, %d)'):format(bld.centerx, bld.centery, bld.z))
        goto continue
    end

    -- skip jobs whose attached items are already owned by the target building
    -- but are not already part of the building. They are actively being used to
    -- construct the building and we can't safely change the building's state.
    for _,item in ipairs(items) do
        if not item.flags.in_building and
                bld == dfhack.items.getHolderBuilding(item) then
            if not opts.quiet then
                print(
                    ('skipping building that is actively being constructed at'..
                     ' (%d, %d, %d)'):format(bld.centerx, bld.centery, bld.z))
            end
            goto continue
        end
    end

    -- clear non-job items from the planned building footprint
    if not clear_footprint(bld, items) then
        dfhack.printerr(
            ('cannot move items blocking building site at (%d, %d, %d)')
            :format(bld.centerx, bld.centery, bld.z))
        goto continue
    end

    -- remove job data and clean up ref links. we do this first because
    -- dfhack.items.moveToBuilding() refuses to work with items that already
    -- hold references to buildings.
    if not dfhack.job.removeJob(job) then
        throw(bld, 'failed to remove job; job state may be inconsistent')
    end

    if not attach_items(bld, items) then
        throw(bld,
              'failed to attach items to building; state may be inconsistent')
    end

    if bld_type == df.building_type.Construction then
        build_construction(bld)
    else
        build_building(bld)
    end

    num_jobs = num_jobs + 1
    ::continue::
end

if num_jobs > 0 then
    df.global.world.reindex_pathfinding = true
end

if not opts.quiet then
    print(('Completed %d construction job%s')
        :format(num_jobs, num_jobs ~= 1 and 's' or ''))
end
