config.mode = 'fortress'
config.target = 'flows'

function test.counts_liquids()
    local output, status = dfhack.run_command_silent('flows')
    expect.eq(CR_OK, status)
    expect.str_find('Blocks with liquid_1', output)
    expect.str_find('Water tiles:', output)
    expect.str_find('Magma tiles:', output)
end
