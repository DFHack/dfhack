config.mode = 'fortress'
config.target = 'autoclothing'

function test.status()
    local output = dfhack.run_command_silent('autoclothing')
    expect.str_find('Automatic clothing management is currently', output)
    expect.str_find('automatic clothing orders', output)
end

function test.wrong_arg_count_shows_usage()
    local output, status = dfhack.run_command_silent('autoclothing', 'bogus')
    expect.true_(output:find('Wrong number', 1, true) ~= nil
                 or status ~= 0)
end

function test.invalid_requirement_rejected()
    local output, status = dfhack.run_command_silent('autoclothing',
                                                   'bogus', 'bogus')
    expect.ne(0, status)
end
