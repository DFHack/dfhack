-- makes a job involving current selection high priority

local function print_help()
    print [====[

do-job-now
==========

The script will try its best to find a job related to the selected entity
(which can be a job, dwarf, animal, item, building, plant or work order) and then
mark this job as high priority. There is no visual indicator, please look
at the dfhack console for output. If a work order is selected, every job
currently present job from this work order is affected, but not the future ones.

For best experience add the following to your ``dfhack*.init``::

    keybinding add Alt-N do-job-now

Also see ``do-job-now`` `tweak` and `prioritize`.
]====]
end

local utils = require 'utils'

local function getUnitName(unit)
    local language_name = dfhack.units.getVisibleName(unit)
    if language_name.has_name then
        return dfhack.df2console(dfhack.TranslateName( language_name ))
    end

    -- animals
    return dfhack.units.getProfessionName(unit)
end

local function doJobNow(job)
    local job_str = dfhack.job.getName(job)
    if not job.flags.do_now then
        job.flags.do_now = true
        print("Made the job " .. job_str .. " top priority")
    else
        print("The job " .. job_str .. " is already top priority")
    end
    local building = dfhack.job.getHolder(job)
    if building then
        print("... at " .. utils.getBuildingName(building))
    end
    local unit = dfhack.job.getWorker(job)
    if unit then
        print("... by " .. getUnitName(unit) )
    end
end

local function doItemJobNow(item)
    if not item.flags.in_job then
        qerror(dfhack.items.getDescription(item, 0) .. " must be in a job! (look for 'TSK')")
    end

    local sref = dfhack.items.getSpecificRef(item, df.specific_ref_type.JOB)
    if sref then
        doJobNow(sref.data.job)
        return
    end
    print("Couldn't find any job for " .. dfhack.items.getDescription(item, 0))
end

local function doBuildingJobNow(building)
    --print('This will attempt to make a job of a building a top priority')

    if #building.jobs > 0
        and    (
            building.jobs[0].job_type == df.job_type.ConstructBuilding
         or building.jobs[0].job_type == df.job_type.DestroyBuilding
        )
    then
        doJobNow(building.jobs[0])
        return
    end
    print("Couldn't find either construct or destroy building job for " .. utils.getBuildingName(building))
end

local function doUnitJobNow(unit)
    if dfhack.units.isCitizen(unit) then
        --print('This will attempt to make a job of ' .. getUnitName(unit) .. ' a top priority')
        local t_job = unit.job
        if t_job then
            local job = t_job.current_job
            if job then
                doJobNow(job)
                return
            end
        end
        print("Couldn't find any job for " .. getUnitName(unit) )
    else
        --print('This will attempt to make a job with ' .. getUnitName(unit) .. ' a top priority')

        local needle = unit.id
        for _link, job in utils.listpairs(df.global.world.jobs.list) do
            if #job.general_refs > 0 then
                for _, gref in ipairs(job.general_refs) do
                    --if gref:getType() == df.general_ref_type.UNIT then -- can't do this: there are many different types that can be units
                    local u = gref:getUnit()
                    if u and u.id == needle then
                        doJobNow(job)
                        return
                    end
                    --end
                end
            end
        end

        print("Couldn't find any job involving " .. getUnitName(unit) )
    end
end

local function doPlantJobNow(plant)
    --print('This will attempt to make a job with a plant a top priority')

    for _link, job in utils.listpairs(df.global.world.jobs.list) do
        if plant.pos.x == job.pos.x
        and plant.pos.y == job.pos.y
        and plant.pos.z == job.pos.z
        then
            doJobNow(job)
            return
        end
    end

    print("Couldn't find any job involving this plant.")
end

local function doWorkOrderJobsNow(order)
    local needle = order.id
    local cnt = 0
    for _link, job in utils.listpairs(df.global.world.jobs.list) do
        if job.order_id == needle then
            doJobNow(job)
            cnt = cnt + 1
        end
    end

    if cnt > 0 then
        print("Found " .. cnt .. " jobs for this work order.")
    else
        print("Couldn't find any job for this work order.")
    end
end

local function getSelectedWorkOrder()
    local scr = dfhack.gui.getCurViewscreen()
    local orders
    local idx
    if df.viewscreen_jobmanagementst:is_instance(scr) then
        orders = df.global.world.manager_orders
        idx = scr.sel_idx
    elseif df.viewscreen_workshop_profilest:is_instance(scr)
        and scr.tab == df.viewscreen_workshop_profilest.T_tab.Orders
    then
        orders = scr.orders
        idx = scr.order_idx
    end
    if orders then
        if idx < #orders then
            return orders[idx]
        else
            qerror("Invalid work order selected")
        end
    end

    return nil
end

local function doSelectedEntityJobNow()
    -- do we have a job selected?
    local job = dfhack.gui.getSelectedJob(true)
    if job then
        doJobNow(job)
        return
    end

    -- do we have an item selected?
    local item = dfhack.gui.getSelectedItem(true)
    if item then
        doItemJobNow(item)
        return
    end

    -- do we have a building selected?
    local building = dfhack.gui.getSelectedBuilding(true)
    if building then
        doBuildingJobNow(building)
        return
    end

    -- do we have a unit selected?
    local unit = dfhack.gui.getSelectedUnit(true)
    if unit then
        doUnitJobNow(unit)
        return
    end

    -- do we have a plant selected?
    local plant = dfhack.gui.getSelectedPlant(true)
    if plant then
        doPlantJobNow(plant)
        return
    end

    -- do we have a work order selected?
    local order = getSelectedWorkOrder()
    if order then
        doWorkOrderJobsNow(order)
        return
    end

    qerror("Select something job-related in game.")
end

local default_action = print_help

local actions = {
    -- help
    ["-?"] = print_help,
    ["?"] = print_help,
    ["--help"] = print_help,
    ["help"] = print_help,
    -- action
    ["default"] = doSelectedEntityJobNow,
}

-- Lua is beautiful.
(actions[ (...) or "default" ] or default_action)(...)
