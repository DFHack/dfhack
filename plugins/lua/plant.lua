local _ENV = mkmodule('plugins.plant')

local argparse = require('argparse')
local utils = require('utils')

local function search_str(s)
    return dfhack.upperCp437(dfhack.toSearchNormalized(s))
end

local function find_plant_idx(s) --find plant raw index by id string
    local id_str = search_str(s)
    for k, v in ipairs(df.global.world.raws.plants.all) do
        if search_str(v.id) == id_str then
            return k
        end
    end

    qerror('Plant raw not found: "'..s..'"')
end

local function find_plant(s) --accept index string or match id string
    if tonumber(s) then
        return argparse.nonnegativeInt(s, 'plant_id')
    else
        return find_plant_idx(s)
    end
end

local function build_filter(vec, s)
    if #vec > 0 then
        qerror('Filter already defined!')
    end

    local set = {}
    for _,id in ipairs(argparse.stringList(s, 'list')) do
        if id ~= '' then
            set[find_plant(id)] = true
        end
    end

    for idx,_ in pairs(set) do
        vec:insert('#', idx) --add plant raw indices to vector
    end
end

local year_table =
{
    tree = 3, --sapling_to_tree_threshold
    ["1x1"] = 3,
    ["2x2"] = 201, --kapok, ginkgo, highwood
    ["3x3"] = 401, --highwood (tower-cap is bugged)
}

local function plant_age(s) --tree stage or numerical value
    local n
    if tonumber(s) then
        n = argparse.nonnegativeInt(s, 'age')
    else
        n = year_table[s:lower()]
    end

    if n then
        n = math.min(n, 1250)
        if n > 0 then
            return 40320*n-1 --years to tens of ticks - 1
        else
            return 0 --don't subtract 1
        end
    end

    qerror('Invalid age: "'..s..'"')
end

function parse_commandline(opts, pos_1, pos_2, filter_vec, args)
    local positionals = argparse.processArgsGetopt(args,
    {
        {'c', 'force', handler=function() opts.force = true end},
        {'s', 'shrubs', handler=function() opts.shrubs = true end},
        {'p', 'saplings', handler=function() opts.saplings = true end},
        {'t', 'trees', handler=function() opts.trees = true end},
        {'d', 'dead', handler=function() opts.dead = true end},
        {'a', 'age', hasArg=true, handler=function(optarg)
            opts.age = plant_age(optarg) end},
        {'f', 'filter', hasArg=true, handler=function(optarg)
            build_filter(filter_vec, optarg) end},
        {'e', 'exclude', hasArg=true, handler=function(optarg)
            opts.filter_ex = true
            build_filter(filter_vec, optarg) end},
        {'z', 'zlevel', handler=function() opts.zlevel = true end},
        {'n', 'dryrun', handler=function() opts.dry_run = true end},
    })

    if #positionals > 3 then
        qerror('Too many positionals!')
    end

    local p1 = positionals[1]
    if not p1 then
        qerror('Specify mode: list, create, grow, or remove!')
    elseif p1 == 'list' then
        opts.plant_idx = -2 --will print all non-grass IDs
    elseif p1 == 'create' then
        opts.create = true

        if positionals[2] then
            opts.plant_idx = find_plant(positionals[2])
        else
            qerror('Must specify plant_id for create!')
        end
    elseif p1 == 'grow' then
        opts.grow = true
    elseif p1 == 'remove' then
        opts.del = true
    else
        qerror('Invalid mode: "'..p1..'"! Must be list, create, grow, or remove!')
    end

    local n = opts.create and 3 or 2
    if positionals[n] then
        utils.assign(pos_1, argparse.coords(positionals[n], 'pos_1', true))
    end

    if not opts.create and positionals[3] then
        utils.assign(pos_2, argparse.coords(positionals[3], 'pos_2', true))
    end
end

return _ENV
