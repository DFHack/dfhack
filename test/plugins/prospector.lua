config.mode = 'fortress'
config.target = 'prospector'

local prospector = require('plugins.prospector')

local function parse(args)
    local opts = {}
    prospector.parse_commandline(opts, args)
    return opts
end

function test.help_flag()
    expect.true_(parse({'--help'}).help)
    expect.true_(parse({'-h'}).help)
end

function test.all_and_hell_set_hidden()
    expect.true_(parse({'all'}).hidden)
    expect.nil_(parse({'all'}).tube)
    local opts = parse({'hell'})
    expect.true_(opts.hidden)
    expect.true_(opts.tube)
end

function test.unknown_keyword_errors()
    expect.error_match('unknown keyword',
        function() prospector.parse_commandline({}, {'bogus'}) end)
end

function test.show_sections_set_flags()
    local opts = parse({'-s', 'gems,ores'})
    expect.true_(opts.gems)
    expect.true_(opts.ores)
    -- selecting any section disables the rest
    expect.false_(opts.summary)
    expect.false_(opts.layers)
end

function test.show_all_sections()
    local opts = parse({'-s', 'summary,liquids,layers,features,ores,gems,veins,shrubs,trees'})
    for _,s in ipairs{'summary', 'liquids', 'layers', 'features', 'ores',
            'gems', 'veins', 'shrubs', 'trees'} do
        expect.true_(opts[s], 'expected section flag set: '..s)
    end
end

function test.unknown_show_section_errors()
    expect.error_match('unknown report section',
        function() prospector.parse_commandline({}, {'-s', 'bogus'}) end)
end

function test.values_flag()
    expect.true_(parse({'-v'}).value)
    expect.true_(parse({'--values'}).value)
end
