config.mode = 'fortress'
config.target = 'autolabor'

-- autolabor enabled state and labor configs persist; leave it off and
-- restore the test labor's defaults
config.wrapper = function(test_fn)
    return dfhack.with_finalize(function()
        dfhack.run_command_silent('autolabor', 'MINE', 'reset')
        dfhack.run_command_silent('autolabor', 'disable')
    end, test_fn)
end

function test.no_args_shows_help()
    local output = dfhack.run_command_silent('autolabor')
    expect.ne('', output)
end

function test.labor_command_requires_enable()
    dfhack.run_command_silent('autolabor', 'disable')
    local output = dfhack.run_command_silent('autolabor', 'MINE', 'haulers')
    expect.str_find('not enabled', output)
end

function test.set_labor_mode()
    dfhack.run_command_silent('autolabor', 'enable')
    local output = dfhack.run_command_silent('autolabor', 'MINE', 'haulers')
    expect.str_find('MINE.-haulers', output)
    output = dfhack.run_command_silent('autolabor', 'MINE', 'reset')
    expect.str_find('MINE.-minimum', output)
end

function test.unknown_labor_errors()
    dfhack.run_command_silent('autolabor', 'enable')
    local output = dfhack.run_command_silent('autolabor', 'BOGUS_LABOR',
                                             'haulers')
    expect.str_find('Could not find labor', output)
end
