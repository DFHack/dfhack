config.mode = 'fortress'
config.target = 'autobutcher'

local autobutcher = require('plugins.autobutcher')

local function parse(args)
    -- emulate the autobutcher_options struct passed by the C++ command handler
    local opts = {races={insert=function(self, pos, v) table.insert(self, v) end}}
    autobutcher.parse_commandline(opts, args)
    return opts
end

function test.help()
    expect.true_(parse{'help'}.help)
    expect.true_(parse{'-h'}.help)
    expect.true_(parse{'--help'}.help)
end

function test.simple_commands()
    for _,cmd in ipairs{'now', 'autowatch', 'noautowatch', 'list',
                        'list_export'} do
        expect.eq(cmd, parse{cmd}.command)
    end
end

function test.watch_races()
    local opts = parse{'watch', 'CAT', 'BIRD_CROW'}
    expect.eq('watch', opts.command)
    expect.eq(2, #opts.races)
    expect.eq('CAT', opts.races[1].value)
    expect.eq('BIRD_CROW', opts.races[2].value)
end

function test.watch_all_and_new()
    expect.true_(parse{'watch', 'all'}.races_all)
    expect.true_(parse{'unwatch', 'new'}.races_new)
    expect.true_(parse{'forget', 'all'}.races_all)
end

function test.watch_requires_races()
    expect.error_match('missing list of races', function()
        parse{'watch'} end)
end

function test.target()
    local opts = parse{'target', '1', '2', '3', '4', 'CAT'}
    expect.eq('target', opts.command)
    expect.eq(1, opts.fk)
    expect.eq(2, opts.mk)
    expect.eq(3, opts.fa)
    expect.eq(4, opts.ma)
    expect.eq(1, #opts.races)
    expect.eq('CAT', opts.races[1].value)
end

function test.target_zero_allowed()
    local opts = parse{'target', '0', '0', '0', '0', 'all'}
    expect.eq(0, opts.fk)
    expect.true_(opts.races_all)
end

function test.target_rejects_bad_numbers()
    expect.error_match('non%-negative integer', function()
        parse{'target', '-1', '0', '0', '0', 'all'} end)
    expect.error_match('non%-negative integer', function()
        parse{'target', '1.5', '0', '0', '0', 'all'} end)
    expect.error_match('non%-negative integer', function()
        parse{'target', 'x', '0', '0', '0', 'all'} end)
end

function test.target_requires_all_counts()
    expect.error(function() parse{'target', '1', '2', '3'} end)
end

function test.target_requires_races()
    expect.error_match('missing list of races', function()
        parse{'target', '1', '2', '3', '4'} end)
end

function test.unrecognized_command()
    expect.error_match('unrecognized command', function()
        parse{'bogus'} end)
end
