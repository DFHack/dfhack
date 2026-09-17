config.mode = 'fortress'
config.target = 'strangemood'

-- note: the --force/--type/--skill happy path is deliberately not tested
-- here: inducing a real strange mood mutates the fort (dwarf claims a
-- workshop and can go berserk if unmet), which is too disruptive mid-suite.

function test.help_is_wrong_usage()
    local _, status = dfhack.run_command_silent('strangemood', 'help')
    expect.eq(CR_WRONG_USAGE, status)
end

function test.unknown_option_is_wrong_usage()
    local output, status = dfhack.run_command_silent('strangemood', '--bogus')
    expect.eq(CR_WRONG_USAGE, status)
    expect.str_find('Unrecognized parameter', output)
end

function test.id_missing_value()
    local output, status = dfhack.run_command_silent('strangemood', '--id')
    expect.eq(CR_WRONG_USAGE, status)
    expect.str_find('No unit id specified', output)
end

function test.id_not_a_number()
    -- regression: the raw std::stoi call threw an uncaught exception on
    -- non-numeric input, crashing the game
    local output, status = dfhack.run_command_silent('strangemood', '--id', 'abc')
    expect.eq(CR_WRONG_USAGE, status)
    expect.str_find('Invalid unit id', output)
end

function test.id_nonexistent_unit()
    local _, status = dfhack.run_command_silent('strangemood', '--id', '99999999')
    expect.eq(CR_FAILURE, status)
end

function test.type_missing_value()
    local _, status = dfhack.run_command_silent('strangemood', '--type')
    expect.eq(CR_WRONG_USAGE, status)
end

function test.type_bad_value()
    local output, status = dfhack.run_command_silent('strangemood', '--type', 'bogus')
    expect.eq(CR_WRONG_USAGE, status)
    expect.str_find('not recognized', output)
end
