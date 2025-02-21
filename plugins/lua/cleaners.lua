local _ENV = mkmodule('plugins.cleaners')

local argparse = require('argparse')
local utils = require('utils')

function parse_commandline(opts, pos_1, pos_2, args)
    local positionals = argparse.processArgsGetopt(args,
    {
        {'a', 'all', handler=function()
            opts.map = true
            opts.units = true
            opts.items = true
            opts.plants = true end},
        {'m', 'map', handler=function() opts.map = true end},
        {'d', 'mud', handler=function() opts.mud = true end},
        {'s', 'snow', handler=function() opts.snow = true end},
        {'t', 'item', handler=function() opts.item_spat = true end},
        {'g', 'grass', handler=function() opts.grass = true end},
        {'x', 'desolate', handler=function() opts.desolate = true end},
        {'o', 'only', handler=function() opts.only = true end},
        {'u', 'units', handler=function() opts.units = true end},
        {'i', 'items', handler=function() opts.items = true end},
        {'z', 'zlevel', handler=function() opts.zlevel = true end},
    })

    if #positionals > 2 then
        qerror('Too many positionals!')
    end

    if positionals[1] then
        utils.assign(pos_1, argparse.coords(positionals[1], 'pos_1', true))
    end

    if positionals[2] then
        utils.assign(pos_2, argparse.coords(positionals[2], 'pos_2', true))
    end
end

return _ENV
