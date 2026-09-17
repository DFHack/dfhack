config.mode = 'fortress'
config.target = 'autofarm'

-- autofarm enabled state persists; leave it disabled
config.wrapper = function(test_fn)
    return dfhack.with_finalize(function()
        dfhack.run_command_silent('autofarm', 'disable')
    end, test_fn)
end

function test.status()
    local output = dfhack.run_command_silent('autofarm', 'status')
    expect.ne('', output)
end

function test.enable_disable()
    dfhack.run_command_silent('autofarm', 'enable')
    local output = dfhack.run_command_silent('autofarm', 'status')
    expect.str_find('Active', output)
    dfhack.run_command_silent('autofarm', 'disable')
    output = dfhack.run_command_silent('autofarm', 'status')
    expect.str_find('Stopped', output)
end

function test.runonce()
    local output, status = dfhack.run_command_silent('autofarm', 'runonce')
    expect.eq(0, status)
end

function test.unknown_arg_shows_usage()
    local output, status = dfhack.run_command_silent('autofarm', 'bogus')
    expect.true_(output:find('autofarm', 1, true) ~= nil or status ~= 0)
end

function test.threshold_rejects_bad_plant()
    local output, status = dfhack.run_command_silent('autofarm',
                                                   'threshold', '-1', '5')
    expect.true_(output:find('Cannot find plant', 1, true) ~= nil
                 or status ~= 0)
end
