config.mode = 'fortress'
config.target = 'work-now'

local function status_output()
    local output, status = dfhack.run_command_silent('work-now')
    expect.eq(CR_OK, status)
    return output
end

function test.status_reports_enabled_state()
    dfhack.run_command_silent('disable', 'work-now')
    expect.str_find('work_now is not actively poking', status_output())

    dfhack.run_command_silent('enable', 'work-now')
    expect.str_find('work_now is actively poking', status_output())

    dfhack.run_command_silent('disable', 'work-now')
end

function test.status_subcommand()
    local output, status = dfhack.run_command_silent('work-now', 'status')
    expect.eq(CR_OK, status)
    expect.str_find('work_now is ', output)
end

function test.unknown_command_is_wrong_usage()
    local _, status = dfhack.run_command_silent('work-now', 'bogus')
    expect.eq(CR_WRONG_USAGE, status)
end
