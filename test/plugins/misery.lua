config.mode = 'fortress'
config.target = 'misery'

local misery = require('plugins.misery')

local saved_factor = misery.misery_getFactor()

config.wrapper = function(test_fn)
    return dfhack.with_finalize(function()
        if saved_factor >= 2 then
            misery.misery_setFactor(saved_factor)
        end
    end, test_fn)
end

function test.status()
    expect.true_(misery.parse_commandline('status'))
    expect.true_(misery.parse_commandline())
end

function test.help()
    expect.false_(misery.parse_commandline('help'))
    expect.false_(misery.parse_commandline('-h'))
    expect.false_(misery.parse_commandline('--help'))
end

function test.set_factor()
    expect.true_(misery.parse_commandline('5'))
    expect.eq(5, misery.misery_getFactor())
    expect.true_(misery.parse_commandline('2'))
    expect.eq(2, misery.misery_getFactor())
end

function test.set_factor_below_minimum()
    misery.misery_setFactor(5)
    -- the C++ layer rejects the value with a console error message rather
    -- than failing the command
    misery.misery_setFactor(1)
    expect.eq(5, misery.misery_getFactor())
    expect.true_(misery.parse_commandline('1'))
    expect.eq(5, misery.misery_getFactor())
end

function test.clear()
    expect.true_(misery.parse_commandline('clear'))
end

function test.unrecognized_command()
    expect.false_(misery.parse_commandline('bogus'))
end

function test.status_output()
    misery.misery_setFactor(5)
    local lines = {}
    mock.patch({{misery, 'print', function(line)
        table.insert(lines, line) end}}, function()
        misery.status()
    end)
    expect.str_find('misery factor is: 5', table.concat(lines, '\n'))
end
