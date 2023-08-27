-- Show or edit regional plant and animal populations
local help = [====[

region-pops
===========
Show or modify the populations of animals in the region.

Usage:

:region-pops list [pattern]:
        Lists encountered populations of the region, possibly restricted by pattern.
:region-pops list-all [pattern]:
        Lists all populations of the region.
:region-pops boost <TOKEN> <factor>:
        Multiply all populations of TOKEN by factor.
        If the factor is greater than one, increases the
        population, otherwise decreases it.
:region-pops boost-all <pattern> <factor>:
        Same as above, but match using a pattern acceptable to list.
:region-pops incr <TOKEN> <factor>:
        Augment (or diminish) all populations of TOKEN by factor (additive).
:region-pops incr-all <pattern> <factor>:
        Same as above, but match using a pattern acceptable to list.

]====]

local utils = require 'utils'

local function sort_keys(tab)
    local kt = {} --as:string[]
    for k,v in pairs(tab) do table.insert(kt,k) end
    table.sort(kt)
    return kt
end

local is_plant_map = {
    Animal = false, Vermin = false, VerminInnumerable = false,
    ColonyInsect = false, Tree = true, Grass = true, Bush = true
}

function enum_populations()
    local stat_table = { --as:{plants:{_array:{_type:table,obj:df.plant_raw,token:string,id:number,records:'df.local_population[]',count:number,known_count:number,known:bool,infinite:bool}},creatures:{_array:{_type:table,obj:df.creature_raw,token:string,id:number,records:'df.local_population[]',count:number,known_count:number,known:bool,infinite:bool}},any:{_array:{_type:table,token:string,id:number,records:'df.local_population[]',count:number,known_count:number,known:bool,infinite:bool}}}
        plants = {},
        creatures = {},
        any = {}
    }

    local function add_entry(v, id, obj, otable, idtoken)
        local entry = otable[idtoken]
        if not entry then
            entry = {
                obj = obj, token = idtoken, id = id, records = {},
                count = 0, known_count = 0,
                known = false, infinite = false
            }
            otable[idtoken] = entry
            stat_table.any[idtoken] = entry
        end

        table.insert(entry.records, v)
        entry.known = entry.known or v.flags.discovered

        if v.quantity < 10000001 then
            entry.count = entry.count + v.quantity
            if v.flags.discovered then
                entry.known_count = entry.known_count + v.quantity
            end
        else
            entry.infinite = true
        end
    end

    for i,v in ipairs(df.global.world.populations) do
        local typeid = df.world_population_type[v.type]
        local is_plant = is_plant_map[typeid]

        if is_plant then
            local id = v.plant
            local obj = df.plant_raw.find(id)
            add_entry(v, id, obj, stat_table.plants, obj.id)
        else
            local id = v.race
            local obj = df.creature_raw.find(id)
            add_entry(v, id, obj, stat_table.creatures, obj.creature_id)
        end
    end

    return stat_table
end

function list_poptable(entries, all, pattern)
    for _,k in ipairs(sort_keys(entries)) do
        local entry = entries[k]
        if (all or entry.known) and (not pattern or string.match(k,pattern)) then
            local count = tostring(entry.known_count)
            if all then
                count = tostring(entry.count)
            end
            if entry.infinite then
                count = 'innumerable'
            end
            print(string.format('%-40s %s', entry.token, count))
        end
    end
end

function list_populations(stat_table, all, pattern)
    print('Plants:')
    list_poptable(stat_table.plants, true, pattern)
    print('\nCreatures and vermin:')
    list_poptable(stat_table.creatures, all, pattern)
end


function boost_population(entry, factor, boost_count)
    for _,v in ipairs(entry.records) do
        if v.quantity < 10000001 then
            boost_count = boost_count + 1
            v.quantity = math.floor(v.quantity * factor)
        end
    end
    return boost_count
end

function incr_population(entry, factor, boost_count)
    for _,v in ipairs(entry.records) do
        if v.quantity < 10000001 then
            boost_count = boost_count + 1
            v.quantity = math.max(0, v.quantity + factor)
        end
    end
    return boost_count
end

local args = {...}
local pops = enum_populations()

if args[1] == 'list' or args[1] == 'list-all' then
    list_populations(pops, args[1] == 'list-all', args[2])
elseif args[1] == 'boost' or args[1] == 'boost-all' then
    local factor = tonumber(args[3])
    if not factor or factor < 0 then
        qerror('Invalid boost factor.')
    end

    local count = 0

    if args[1] == 'boost' then
        local entry = pops.any[args[2]] or qerror('Unknown population token.')
        count = boost_population(entry, factor, count)
    else
        for k,entry in pairs(pops.any) do
            if string.match(k, args[2]) then
                count = boost_population(entry, factor, count)
            end
        end
    end

    print('Updated '..count..' populations.')
elseif args[1] == 'incr' or args[1] == 'incr-all' then
    local factor = tonumber(args[3])
    if not factor then
        qerror('Invalid increment factor.')
    end

    local count = 0

    if args[1] == 'incr' then
        local entry = pops.any[args[2]] or qerror('Unknown population token.')
        count = incr_population(entry, factor, count)
    else
        for k,entry in pairs(pops.any) do
            if string.match(k, args[2]) then
                count = incr_population(entry, factor, count)
            end
        end
    end

    print('Updated '..count..' populations.')
else
    print(help)
end
