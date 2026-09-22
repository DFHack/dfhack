config.mode = 'fortress'
config.target = 'autonestbox'

local autonestbox = require('plugins.autonestbox')

local function parse(args)
    local opts = {}
    autonestbox.parse_commandline(opts, args)
    return opts
end

function test.now_flag()
    expect.true_(parse({'now'}).now)
end

function test.help_flag()
    expect.true_(parse({'help'}).help)
    expect.true_(parse({'--help'}).help)
    expect.true_(parse({'-h'}).help)
end

function test.help_does_not_set_now()
    local opts = parse({'help', 'now'})
    expect.true_(opts.help)
    expect.nil_(opts.now)
end

function test.unknown_positionals_ignored()
    local opts = parse({'bogus'})
    expect.nil_(opts.now)
    expect.nil_(opts.help)
end

function test.mixed_positionals()
    local opts = parse({'bogus', 'now', 'other'})
    expect.true_(opts.now)
end
