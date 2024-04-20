local _ENV = mkmodule('plugins.prospector')

local argparse = require('argparse')
local utils = require('utils')

local VALID_SHOW_VALUES = utils.invert{
    'summary', 'liquids', 'layers', 'features', 'ores',
    'gems',    'veins',   'shrubs', 'trees'
}

function parse_commandline(opts, args)
    local show = {}
    local positionals = argparse.processArgsGetopt(args, {
            {'h', 'help', handler=function() opts.help = true end},
            {'s', 'show', hasArg=true, handler=function(optarg)
                    show = argparse.stringList(optarg) end},
            {'v', 'values', handler=function() opts.value = true end},
        })

    for _,p in ipairs(positionals) do
        if p == 'all' then opts.hidden = true
        elseif p == 'hell' then
            opts.hidden = true
            opts.tube = true
        else
            qerror(('unknown keyword: "%s"'):format(p))
        end
    end

    if #show > 0 then
        for s in pairs(VALID_SHOW_VALUES) do
            opts[s] = false
        end
    end

    for _,s in ipairs(show) do
        if VALID_SHOW_VALUES[s] then
            opts[s] = true
        else
            qerror(('unknown report section: "%s"'):format(s))
        end
    end
end

return _ENV
