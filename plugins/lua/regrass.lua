local _ENV = mkmodule('plugins.regrass')

local argparse = require('argparse')
local utils = require('utils')

local function search_str(s)
    return dfhack.upperCp437(dfhack.toSearchNormalized(s))
end

local function find_grass_idx(s)
    local grasses = df.global.world.raws.plants.grasses

    if #grasses == 0 then
        return -1 --no grass raws, world not loaded
    elseif s == '' then
        local rng = dfhack.random.new()
        return grasses[rng:random(#grasses)].index
    end

    local id_str = search_str(s)
    for _,grass in ipairs(grasses) do
        if search_str(grass.id) == id_str then
            return grass.index
        end
    end

    return -1
end

function parse_commandline(opts, pos_1, pos_2, args)
    local positionals = argparse.processArgsGetopt(args,
    {
        {'m', 'max', handler=function() opts.max_grass = true end},
        {'n', 'new', handler=function() opts.new_grass = true end},
        {'a', 'ashes', handler=function() opts.ashes = true end},
        {'d', 'buildings', handler=function() opts.buildings = true end},
        {'u', 'mud', handler=function() opts.mud = true end},
        {'b', 'block', handler=function() opts.block = true end},
        {'z', 'zlevel', handler=function() opts.zlevel = true end},
        {'f', 'force', hasArg=true, handler=function(optarg)
            opts.force = true
            opts.forced_plant = find_grass_idx(optarg)
        end},
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
