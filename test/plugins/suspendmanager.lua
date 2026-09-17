config.mode = 'fortress'
config.target = 'suspendmanager'

local function is_enabled()
    local output = dfhack.run_command_silent('suspendmanager')
    return output:find('is enabled') ~= nil
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
    return dfhack.with_finalize(function()
        dfhack.run_command_silent('suspendmanager', 'set',
            'preventblocking', 'true')
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
