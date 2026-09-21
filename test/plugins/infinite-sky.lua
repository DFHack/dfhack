config.mode = 'fortress'
config.target = 'infinite-sky'

function test.status()
    local output = dfhack.run_command_silent('infinite-sky')
    expect.str_find('Construction monitoring is', output)
end

function test.help_shows_usage()
    local output, status = dfhack.run_command_silent('infinite-sky', 'help')
    expect.true_(output:find('infinite%-sky') ~= nil
                 or output:find('z%-level', 1, true) ~= nil
                 or output:find('Usage', 1, true) ~= nil
                 or output:find('usage', 1, true) ~= nil)
end

function test.rejects_non_numeric()
    local output, status = dfhack.run_command_silent('infinite-sky', 'bogus')
    expect.true_(output:find('positive integer', 1, true) ~= nil
                 or output:find('invalid', 1, true) ~= nil
                 or status ~= 0)
end

function test.rejects_non_positive()
    local output, status = dfhack.run_command_silent('infinite-sky', '-3')
    expect.true_(output:find('positive integer', 1, true) ~= nil
                 or output:find('invalid', 1, true) ~= nil
                 or status ~= 0)
end
