config.mode = 'fortress'
config.target = 'autogems'

-- autogems hooks should end removed
config.wrapper = function(test_fn)
    return dfhack.with_finalize(function()
        dfhack.run_command_silent('disable', 'autogems')
    end, test_fn)
end

function test.reload_config()
    local output, status = dfhack.run_command_silent('autogems-reload')
    expect.ne('', output)
end

function test.enable_disable()
    dfhack.run_command_silent('enable', 'autogems')
    local output = dfhack.run_command_silent('autogems-reload')
    dfhack.run_command_silent('disable', 'autogems')
    expect.ne('', output)
end
