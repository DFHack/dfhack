-- Inspect and pull levers
local argparse = require('argparse')

function leverPullJob(lever, priority)
    local ref = df.general_ref_building_holderst:new()
    ref.building_id = lever.id

    local job = df.job:new()
    job.job_type = df.job_type.PullLever
    job.pos = {
        x = lever.centerx,
        y = lever.centery,
        z = lever.z
    }
    job.flags.do_now = priority
    job.general_refs:insert("#", ref)
    lever.jobs:insert("#", job)

    dfhack.job.linkIntoWorld(job, true)
    dfhack.job.checkBuildingsNow()

    print(leverDescribe(lever))
end

function leverPullInstant(lever)
    for _, m in ipairs(lever.linked_mechanisms) do
        local tref = dfhack.items.getGeneralRef(m, df.general_ref_type.BUILDING_HOLDER)
        if tref then
            tref:getBuilding():setTriggerState(lever.state)
        end
    end

    if lever.state == 1 then
        lever.state = 0
    else
        lever.state = 1
    end

    print(leverDescribe(lever))
end

function leverDescribe(lever)
    local lever_name = ''
    if #lever.name > 0 then
        lever_name = ' ' .. lever.name
    end

    if lever.state == 0 then
        state_text = '\\'
    else
        state_text = '/'
    end

    local t = ('lever #%d%s @[%d, %d, %d] %s'):format(lever.id, lever_name, lever.centerx, lever.centery, lever.z,
        state_text)

    for _, j in ipairs(lever.jobs) do
        if j.job_type == df.job_type.PullLever then
            local r = ''
            if j.flags.do_now then
                r = r .. ', now'
            end
            if j.flags['repeat'] then
                r = r .. ', repeat'
            end
            if j.flags.suspend then
                r = r .. ', suspended'
            end

            t = t .. (' (pull order%s)'):format(r)
        end
    end

    for _, m in ipairs(lever.linked_mechanisms) do
        local tref = dfhack.items.getGeneralRef(m, df.general_ref_type.BUILDING_HOLDER)
        if tref then
            tg = tref:getBuilding()
            if pcall(function()
                return tg.gate_flags
            end) then
                if tg.gate_flags.closed then
                    state = "closed"
                else
                    state = "opened"
                end

                if tg.gate_flags.closing then
                    state = state .. (', closing (%d)'):format(tg.timer)
                end
                if tg.gate_flags.opening then
                    state = state .. (', opening (%d)'):format(tg.timer)
                end

            end

            t = t ..
                    ("\n      linked to %s %s #%d @[%d, %d, %d]"):format(state, df.building_type[tg:getType()], tg.id,
                    tg.centerx, tg.centery, tg.z)
        end
    end

    return t
end

function ListLevers()
    for k, v in ipairs(df.global.world.buildings.other.TRAP) do -- hint:df.building_trapst
        if v.trap_type == df.trap_type.Lever then
            print(('#%d: %s'):format(k, leverDescribe(v)))
        end
    end
end

function getLever(opts)
    local lever
    if opts.pullId == nil then
        lever = dfhack.gui.getSelectedBuilding()
    else
        lever = df.building.find(opts.pullId)
    end

    if df.building_trapst:is_instance(lever) and lever.trap_type == df.trap_type.Lever then
        return lever
    end
end

function ShowLever(opts)
    local lever = getLever(opts)
    if lever == nil then
        print("Can't find lever with ID, or no lever selected")
        return
    end

    dfhack.gui.revealInDwarfmodeMap(lever.centerx, lever.centery, lever.z, true)
    print(leverDescribe(lever))
end

function PullLever(opts)
    local lever = getLever(opts)
    if lever == nil then
        print("Can't find lever with ID, or no lever selected")
        return
    end

    if opts.instant then
        leverPullInstant(lever)
    else
        leverPullJob(lever, opts.priority)
    end
end

local function parse_commandline(args)
    local opts = {}
    local commands = argparse.processArgsGetopt(args, {{
        'h',
        'help',
        handler = function()
            opts.help = true
        end
    }, {
        'i',
        'id',
        hasArg = true,
        handler = function(a)
            opts.pullId = tonumber(a)
        end
    }, {
        nil,
        'priority',
        handler = function()
            opts.priority = true
        end
    }, {
        nil,
        'instant',
        handler = function()
            opts.instant = true
        end
    }})

    if commands[1] == "list" then
        opts.list = true
    elseif commands[1] == "show" then
        opts.show = true
    elseif commands[1] == "pull" then
        opts.pull = true
    elseif commands[1] == "help" then
        opts.help = true
    end
    return opts
end

local opts = parse_commandline({...})
if opts.help then
    print(dfhack.script_help())
elseif opts.list then
    ListLevers()
elseif opts.show then
    ShowLever(opts)
elseif opts.pull then
    PullLever(opts)
else
    print(dfhack.script_help())
end
