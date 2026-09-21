config.mode = 'fortress'
config.target = 'tubefill'

function test.fills_tubes()
    local output, status = dfhack.run_command_silent('tubefill')
    expect.eq(CR_OK, status)
    expect.str_find('Found and changed', output)
end

function test.hollow_option()
    local output, status = dfhack.run_command_silent('tubefill', 'hollow')
    expect.eq(CR_OK, status)
    expect.str_find('Found and changed', output)
end

function test.help_is_wrong_usage()
    local _, status = dfhack.run_command_silent('tubefill', '?')
    expect.eq(CR_WRONG_USAGE, status)
end
