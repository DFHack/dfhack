config.mode = 'fortress'
config.target = 'suspendmanager'

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
