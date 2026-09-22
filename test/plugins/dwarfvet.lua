config.mode = 'fortress'
config.target = 'dwarfvet'

local function status_output()
    local output, status = dfhack.run_command_silent('dwarfvet', 'status')
    expect.eq(CR_OK, status)
    return output
end

function test.status_reports_state_and_patients()
    dfhack.run_command_silent('disable', 'dwarfvet')
    local output = status_output()
    expect.str_find('dwarfvet is not running', output)
    expect.str_find('The following animals are receiving treatment:', output)

    dfhack.run_command_silent('enable', 'dwarfvet')
    output = status_output()
    expect.str_find('dwarfvet is running', output)
    dfhack.run_command_silent('disable', 'dwarfvet')
end

function test.bare_command_is_status()
    local output, status = dfhack.run_command_silent('dwarfvet')
    expect.eq(CR_OK, status)
    expect.str_find('dwarfvet is ', output)
end

function test.unknown_command_is_wrong_usage()
    local _, status = dfhack.run_command_silent('dwarfvet', 'bogus')
    expect.eq(CR_WRONG_USAGE, status)
end

function test.now_runs_checkup()
    -- checkup scans hospital zones and animal patients; safe on any fort
    dfhack.run_command_silent('enable', 'dwarfvet')
    return dfhack.with_finalize(function()
        dfhack.run_command_silent('disable', 'dwarfvet')
    end, function()
        local _, status = dfhack.run_command_silent('dwarfvet', 'now')
        expect.eq(CR_OK, status)
    end)
end
