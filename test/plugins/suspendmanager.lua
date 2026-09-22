config.mode = 'fortress'
config.target = 'suspendmanager'

local function is_enabled()
    local output = dfhack.run_command_silent('suspendmanager')
    return output:find('is enabled') ~= nil
end

local function prevents_blocking()
    local output = dfhack.run_command_silent('suspendmanager')
    return output:find('but not suspending') == nil
end

function test.status_reflects_enable_state()
    local was_enabled = is_enabled()

    return dfhack.with_finalize(function()
        dfhack.run_command_silent('suspendmanager',
            was_enabled and 'enable' or 'disable')
    end, function()
        local _, status = dfhack.run_command_silent('suspendmanager', 'enable')
        expect.eq(CR_OK, status)
        expect.true_(is_enabled())

        local _, status2 = dfhack.run_command_silent('suspendmanager', 'disable')
        expect.eq(CR_OK, status2)
        expect.false_(is_enabled())
    end)
end

function test.now_runs_cycle()
    local _, status = dfhack.run_command_silent('suspendmanager', 'now')
    expect.eq(CR_OK, status)
end

function test.set_preventblocking()
    local was_preventing = prevents_blocking()

    return dfhack.with_finalize(function()
        dfhack.run_command_silent('suspendmanager', 'set',
            'preventblocking', was_preventing and 'true' or 'false')
    end, function()
        local _, status = dfhack.run_command_silent('suspendmanager', 'set',
            'preventblocking', 'false')
        expect.eq(CR_OK, status)
        local _, status2 = dfhack.run_command_silent('suspendmanager', 'set',
            'preventblocking', 'true')
        expect.eq(CR_OK, status2)
    end)
end

function test.set_missing_args_is_wrong_usage()
    local _, status = dfhack.run_command_silent('suspendmanager', 'set')
    expect.eq(CR_WRONG_USAGE, status)
end

function test.bad_option_is_wrong_usage()
    local _, status = dfhack.run_command_silent('suspendmanager', 'bogus')
    expect.eq(CR_WRONG_USAGE, status)
end

function test.unsuspend_runs()
    local _, status = dfhack.run_command_silent('unsuspend')
    expect.eq(CR_OK, status)
end

-- machine collapse protection (#5777)

local function tile_shape(x, y, z)
    local block = dfhack.maps.getTileBlock(x, y, z)
    if not block then return nil end
    local tt = block.tiletype[x % 16][y % 16]
    return df.tiletype.attrs[tt].shape
end

local function free_tile(x, y, z)
    local block = dfhack.maps.getTileBlock(x, y, z)
    if not block then return false end
    local lx, ly = x % 16, y % 16
    local des = block.designation[lx][ly]
    if des.dig ~= df.tile_dig_designation.No or des.flow_size ~= 0 then
        return false
    end
    if block.occupancy[lx][ly].building ~= df.tile_building_occ.None then
        return false
    end
    return not dfhack.buildings.findAtTile(xyz2pos(x, y, z))
end

-- find a floor tile with two open tiles to its east; if none exists,
-- channel open tiles out of a wall face so the site is floor + two open tiles
local function make_pit_site()
    local function clear_run(x, y, z, shape)
        for i = 1, 2 do
            if tile_shape(x + i, y, z) ~= shape or not free_tile(x + i, y, z) then
                return false
            end
        end
        return true
    end
    -- stay clear of the unbuildable map border
    local margin = 20
    local map = df.global.world.map
    local wall_site = nil
    for _, block in ipairs(df.global.world.map.map_blocks) do
        local bx, by, bz = block.map_pos.x, block.map_pos.y, block.map_pos.z
        if bx < margin or by < margin or bx > map.x_count - margin - 16
                or by > map.y_count - margin - 16 then
            goto next_block
        end
        for x = 0, 13 do for y = 0, 15 do
            local wx, wy = bx + x, by + y
            if tile_shape(wx, wy, bz) == df.tiletype_shape.FLOOR
                    and free_tile(wx, wy, bz) then
                if clear_run(wx, wy, bz, df.tiletype_shape.EMPTY) then
                    return wx, wy, bz
                end
                if not wall_site
                        and clear_run(wx, wy, bz, df.tiletype_shape.WALL) then
                    wall_site = {wx, wy, bz}
                end
            end
        end end
        ::next_block::
    end
    if not wall_site then return nil end
    local x, y, z = wall_site[1], wall_site[2], wall_site[3]
    for i = 1, 2 do
        local db = dfhack.maps.getTileBlock(x + i, y, z)
        db.designation[(x + i) % 16][y % 16].dig = df.tile_dig_designation.Channel
    end
    dfhack.run_command_silent('dig-now')
    if not clear_run(x, y, z, df.tiletype_shape.EMPTY) then return nil end
    return x, y, z
end

-- complete a machine building without dwarf labor: remove its construction
-- job, mark it built, and join it to the machine graph
local function build_machine(btype, x, y, z, width)
    local bld = dfhack.buildings.constructBuilding{
        type=btype, pos=xyz2pos(x, y, z), width=width, height=1,
    }
    if not bld then return nil end
    for i = #bld.jobs - 1, 0, -1 do
        dfhack.job.removeJob(bld.jobs[i])
    end
    bld.flags.exists = true
    bld.construction_stage = 1
    df.global.world.machines:add_to_machine(bld)
    return bld
end

local function destroy_job_of(bld)
    bld:queueDestroy()
    for _, job in ipairs(bld.jobs) do
        if job.job_type == df.job_type.DestroyBuilding then
            return job
        end
    end
end

-- fully remove a completed building: deconstruct() only queues a destroy job
-- for those, so reset the build stage to reach its immediate-deletion path
local function delete_building(bld)
    if not bld then return end
    local id = bld.id
    for i = #bld.jobs - 1, 0, -1 do
        dfhack.job.removeJob(bld.jobs[i])
    end
    bld.construction_stage = 0
    dfhack.buildings.deconstruct(bld)
    -- the object is deleted but its pointer lingers in the global vector
    local all = df.global.world.buildings.all
    for i = #all - 1, 0, -1 do
        if all[i].id == id then
            all:erase(i)
        end
    end
end

-- drop machine graph nodes left pointing at deleted buildings
local function prune_dead_machines()
    local machines = df.global.world.machines.all
    for i = #machines - 1, 0, -1 do
        local machine = machines[i]
        for j = #machine.components - 1, 0, -1 do
            if not df.building.find(machine.components[j].building_id) then
                machine.components:erase(j)
            end
        end
        if #machine.components == 0 then
            machines:erase(i)
        end
    end
end

function test.destroying_machine_anchor_suspends_job()
    local x, y, z = make_pit_site()
    expect.ne(nil, x, 'no diggable site for the machine rig')
    if not x then return end

    local was_enabled = is_enabled()
    if not was_enabled then
        dfhack.run_command_silent('suspendmanager', 'enable')
    end

    local anchor = build_machine(df.building_type.GearAssembly, x, y, z, 1)
    local axle = build_machine(df.building_type.AxleHorizontal, x + 1, y, z, 1)
    local hanging = build_machine(df.building_type.GearAssembly, x + 2, y, z, 1)

    return dfhack.with_finalize(function()
        if not was_enabled then
            dfhack.run_command_silent('suspendmanager', 'disable')
        end
        for _, bld in pairs{anchor, axle, hanging} do
            delete_building(bld)
        end
        prune_dead_machines()
    end, function()
        expect.ne(nil, anchor, 'anchor gear not built')
        expect.ne(nil, axle, 'axle not built')
        expect.ne(nil, hanging, 'hanging gear not built')
        if not anchor or not axle or not hanging then return end

        -- all three components must be in the same machine
        local machine_id = anchor.machine.machine_id
        expect.ne(-1, machine_id)
        expect.eq(machine_id, axle.machine.machine_id)
        expect.eq(machine_id, hanging.machine.machine_id)

        -- removing the endpoint leaves an anchored component: safe
        local end_job = destroy_job_of(hanging)
        expect.ne(nil, end_job, 'no destroy job on endpoint')
        -- removing the anchor orphans the axle and the endpoint: unsafe
        local anchor_job = destroy_job_of(anchor)
        expect.ne(nil, anchor_job, 'no destroy job on anchor')
        if not end_job or not anchor_job then return end

        dfhack.run_command_silent('suspendmanager', 'now')
        expect.true_(anchor_job.flags.suspend,
            'anchor destroy job was not suspended')
        expect.false_(end_job.flags.suspend,
            'endpoint destroy job was suspended')
    end)
end


local suspendmanager = require('plugins.suspendmanager')

local function save_suspend_flags()
    local flags = {}
    suspendmanager.foreach_construction_job(function(job)
        flags[job.id] = job.flags.suspend
    end)
    return flags
end

local created_buildings = {}

config.wrapper = function(test_fn)
    local flags = save_suspend_flags()
    local saved_onstatechange = dfhack.onStateChange.unsuspend
    return dfhack.with_finalize(function()
        dfhack.onStateChange.unsuspend = saved_onstatechange
        for _,bld in ipairs(created_buildings) do
            dfhack.buildings.deconstruct(bld)
        end
        created_buildings = {}
        suspendmanager.foreach_construction_job(function(job)
            if flags[job.id] ~= nil then
                job.flags.suspend = flags[job.id]
            end
        end)
    end, test_fn)
end

-- scans the map for a tile where a 1x1 building can be placed
local function construct_test_building()
    for _,block in ipairs(df.global.world.map.map_blocks) do
        for x = 0, 15 do
            for y = 0, 15 do
                local attrs = df.tiletype.attrs[block.tiletype[x][y]]
                if attrs.shape == df.tiletype_shape.FLOOR and
                        block.occupancy[x][y].building == 0 then
                    local bld = dfhack.buildings.constructBuilding{
                        type=df.building_type.Chair,
                        pos={x=block.map_pos.x+x, y=block.map_pos.y+y,
                             z=block.map_pos.z}}
                    if bld then
                        table.insert(created_buildings, bld)
                        return bld
                    end
                end
            end
        end
    end
end

local function find_job_for(bld)
    local found = nil
    suspendmanager.foreach_construction_job(function(job)
        if dfhack.job.getHolder(job) == bld then found = job end
    end)
    return found
end

function test.foreach_construction_job()
    local seen = {}
    suspendmanager.foreach_construction_job(function(job)
        expect.eq(df.job_type.ConstructBuilding, job.job_type)
        seen[job.id] = true
    end)
    local bld = construct_test_building()
    if not bld then return end
    local job = find_job_for(bld)
    expect.ne(nil, job)
    if not job then return end
    expect.nil_(seen[job.id])
    expect.eq(df.job_type.ConstructBuilding, job.job_type)
end

function test.run_once_unsuspends_everything()
    local bld = construct_test_building()
    if not bld then return end
    local job = find_job_for(bld)
    expect.ne(nil, job)
    if not job then return end
    job.flags.suspend = true
    suspendmanager.runOnce(false, true, true)
    expect.false_(job.flags.suspend)
end

function test.unsuspend_command_args()
    local bld = construct_test_building()
    if not bld then return end
    local job = find_job_for(bld)
    expect.ne(nil, job)
    if not job then return end
    job.flags.suspend = true
    -- -q is quiet, -f forces unsuspension of everything
    suspendmanager.unsuspend_command('-q', '-f')
    expect.false_(job.flags.suspend)
end

function test.is_buildingplan_job()
    local bld = construct_test_building()
    if not bld then return end
    local job = find_job_for(bld)
    if not job then return end
    -- buildings created without a material selection have mat_type == -1,
    -- which is how suspendmanager identifies buildingplan-placed jobs
    expect.true_(suspendmanager.isBuildingPlanJob(job))
end

function test.is_kept_suspended()
    local bld = construct_test_building()
    if not bld then return end
    local job = find_job_for(bld)
    if not job then return end
    -- clearing all suspension reasons must synchronously unmark the job;
    -- this is only deterministic within a single frame since an enabled
    -- suspendmanager instance re-populates reasons on its own cycle
    suspendmanager.runOnce(false, true, true)
    expect.false_(suspendmanager.isKeptSuspended(job))
end

function test.overlay_update_building()
    local overlay = suspendmanager.SuspendOverlay{}
    local job = {flags={suspend=true}}
    overlay:update_building(42, job)
    local data = overlay.in_progress_buildings[42]
    expect.true_(data.suspended)
    expect.eq(1, data.suspend_count)
    -- re-suspending while already suspended doesn't bump the count
    overlay:update_building(42, job)
    expect.eq(1, overlay.in_progress_buildings[42].suspend_count)
    -- unsuspend then suspend again -> repeat offender
    job.flags.suspend = false
    overlay:update_building(42, job)
    job.flags.suspend = true
    overlay:update_building(42, job)
    expect.eq(2, overlay.in_progress_buildings[42].suspend_count)
end

function test.overlay_update_viewport()
    local overlay = suspendmanager.SuspendOverlay{}
    local viewport = {z=1, x1=0, y1=0, x2=10, y2=10}
    expect.true_(overlay:update_viewport(viewport))
    expect.false_(overlay:update_viewport(viewport))
    expect.true_(overlay:update_viewport(
        {z=2, x1=0, y1=0, x2=10, y2=10}))
end

function test.overlay_onupdate()
    local bld = construct_test_building()
    if not bld then return end
    local job = find_job_for(bld)
    if not job then return end
    job.flags.suspend = true
    local overlay = suspendmanager.SuspendOverlay{}
    overlay:overlay_onupdate()
    local data = overlay.in_progress_buildings[bld.id]
    expect.ne(nil, data)
    expect.true_(data.suspended)
    -- a second update clears entries whose version wasn't refreshed
    overlay.in_progress_buildings[999999] = {version=0}
    overlay:overlay_onupdate()
    expect.nil_(overlay.in_progress_buildings[999999])
end

function test.status_overlay_no_selection()
    local overlay = suspendmanager.StatusOverlay{}
    expect.eq('Not suspended.', overlay:get_status_string())
end

function test.toggle_overlay_no_selection()
    local overlay = suspendmanager.ToggleOverlay{}
    expect.false_(overlay:shouldRender())
end
