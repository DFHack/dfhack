config.mode = 'fortress'
config.target = 'fastdwarf'

local function status_output()
    local output, status = dfhack.run_command_silent('fastdwarf', 'status')
    expect.eq(CR_OK, status)
    return output
end

function test.status_reports_state()
    expect.str_find('Current state: fast = %d, teleport = %d%.', status_output())
end

function test.set_fast_and_tele()
    local original = status_output()

    local output, status = dfhack.run_command_silent('fastdwarf', '1', '0')
    expect.eq(CR_OK, status)
    expect.str_find('fast = 1, teleport = 0', status_output())

    output, status = dfhack.run_command_silent('fastdwarf', '0', '1')
    expect.eq(CR_OK, status)
    expect.str_find('fast = 0, teleport = 1', status_output())

    output, status = dfhack.run_command_silent('fastdwarf', '0', '0')
    expect.eq(CR_OK, status)
    expect.str_find('fast = 0, teleport = 0', status_output())

    -- restore the original speed level
    local fast, tele = original:match('fast = (%d), teleport = (%d)')
    dfhack.run_command_silent('fastdwarf', fast, tele)
end

function test.tele_without_fast_arg()
    -- a single argument sets fast mode only; tele defaults to 0
    dfhack.run_command_silent('fastdwarf', '1')
    expect.str_find('fast = 1, teleport = 0', status_output())
    dfhack.run_command_silent('fastdwarf', '0', '0')
end

function test.out_of_range_fast_value()
    -- fast mode only goes up to 2; a rejected value must not change state
    dfhack.run_command_silent('fastdwarf', '0', '0')
    local _, status = dfhack.run_command_silent('fastdwarf', '3')
    expect.eq(CR_WRONG_USAGE, status)
    expect.str_find('fast = 0', status_output())
end

function test.out_of_range_tele_value()
    -- tele mode is a boolean; a rejected value must not change state
    dfhack.run_command_silent('fastdwarf', '0', '0')
    local _, status = dfhack.run_command_silent('fastdwarf', '0', '2')
    expect.eq(CR_WRONG_USAGE, status)
    expect.str_find('teleport = 0', status_output())
end

function test.too_many_params()
    local _, status = dfhack.run_command_silent('fastdwarf', '1', '0', 'extra')
    expect.eq(CR_WRONG_USAGE, status)
end
