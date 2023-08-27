-- List, create, or change wild colonies (eg honey bees)
-- By PeridexisErrant and Warmist

local help = [====[

colonies
========
List vermin colonies, place honey bees, or convert all vermin
to honey bees.  Usage:

:colonies:          List all vermin colonies on the map.
:colonies place:    Place a honey bee colony under the cursor.
:colonies convert:  Convert all existing colonies to honey bees.

The ``place`` and ``convert`` subcommands by default create or
convert to honey bees, as this is the most commonly useful.
However both accept an optional flag to use a different vermin
type, for example ``colonies place ANT`` creates an ant colony
and ``colonies convert TERMITE`` ends your beekeeping industry.

]====]

function findVermin(target_verm)
    for k,v in ipairs(df.global.world.raws.creatures.all) do
        if v.creature_id == target_verm then
            return k
        end
    end
    qerror("No vermin found with name: "..target_verm)
end

function list_colonies()
    for idx, col in pairs(df.global.world.vermin.colonies) do
        local race = df.global.world.raws.creatures.all[col.race].creature_id
        print(race..'    at  '..col.pos.x..', '..col.pos.y..', '..col.pos.z)
    end
end

function convert_vermin_to(target_verm)
    local vermin_id = findVermin(target_verm)
    local changed = 0
    for _, verm in pairs(df.global.world.vermin.colonies) do
        verm.race = vermin_id
        verm.caste = -1 -- check for queen bee?
        verm.amount = 18826
        verm.visible = true
        changed = changed + 1
    end
    print('Converted '..changed..' colonies to '..target_verm)
end

function place_vermin(target_verm)
    local pos = copyall(df.global.cursor)
    if pos.x == -30000 then
        qerror("Cursor must be pointing somewhere")
    end
    local verm = df.vermin:new()
    verm.race = findVermin(target_verm)
    verm.flags.is_colony = true
    verm.caste = -1 -- check for queen bee?
    verm.amount = 18826
    verm.visible = true
    verm.pos:assign(pos)
    df.global.world.vermin.colonies:insert("#", verm)
    df.global.world.vermin.all:insert("#", verm)
end

local args = {...}
local target_verm = args[2] or "HONEY_BEE"

if args[1] == 'help' or args[1] == '?' then
    print(help)
elseif args[1] == 'convert' then
    convert_vermin_to(target_verm)
elseif args[1] == 'place' then
    place_vermin(target_verm)
else
    if #df.global.world.vermin.colonies < 1 then
        dfhack.printerr('There are no colonies on the map.')
    end
    list_colonies()
end
