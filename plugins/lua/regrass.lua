local _ENV = mkmodule('plugins.regrass')

local argparse = require('argparse')
local utils = require('utils')

local function search_str(s)
    return dfhack.upperCp437(dfhack.toSearchNormalized(s))
end

local function find_grass_idx(s)
    local id_str = search_str(s)
    for _,grass in ipairs(df.global.world.raws.plants.grasses) do
        if search_str(grass.id) == id_str then
            return grass.index
        end
    end

    return -1
end

function parse_commandline(opts, pos_1, pos_2, args)
    local plant_str
    local positionals = argparse.processArgsGetopt(args,
    {
        {'m', 'max', handler=function() opts.max_grass = true end},
        {'n', 'new', handler=function() opts.new_grass = true end},
        {'a', 'ashes', handler=function() opts.ashes = true end},
        {'d', 'buildings', handler=function() opts.buildings = true end},
        {'u', 'mud', handler=function() opts.mud = true end},
        {'b', 'block', handler=function() opts.block = true end},
        {'z', 'zlevel', handler=function() opts.zlevel = true end},
        {'f', 'force', handler=function() opts.force = true end},
        {'p', 'plant', hasArg=true, handler=function(optarg) plant_str = optarg end},
    })

    if plant_str then
        if plant_str == '' then --will print all ids
            opts.forced_plant = -2
        elseif not opts.force then
            qerror('Use of --plant without --force!')
        else
            opts.forced_plant = find_grass_idx(plant_str)
        end
    elseif opts.force then
        local grasses = df.global.world.raws.plants.grasses
        if #grasses == 0 then
            return -1 --no grass raws, world not loaded
        else
            local rng = dfhack.random.new()
            opts.forced_plant = grasses[rng:random(#grasses)].index
        end
    end

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
