config.mode = 'fortress'
config.target = '3dveins'

function test.rewrites_veins()
    local output, status = dfhack.run_command_silent('3dveins')
    expect.eq(CR_OK, status)
    expect.str_find('Writing tiles', output)
end

function test.verbose_option()
    local _, status = dfhack.run_command_silent('3dveins', 'verbose')
    expect.eq(CR_OK, status)
end

function test.bad_option_is_wrong_usage()
    local _, status = dfhack.run_command_silent('3dveins', 'bogus')
    expect.eq(CR_WRONG_USAGE, status)
end
