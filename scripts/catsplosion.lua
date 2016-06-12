-- Make cats just /multiply/.
--[[=begin

catsplosion
===========
Makes cats (and other animals) just *multiply*. It is not a good idea to run this
more than once or twice.

Usage:

:catsplosion:           Make all cats pregnant
:catsplosion list:      List IDs of all animals on the map
:catsplosion ID ...:    Make animals with given ID(s) pregnant

Animals will give birth within two in-game hours (100 ticks or fewer).

=end]]

world = df.global.world

if not dfhack.isWorldLoaded() then
    qerror('World not loaded.')
end

args = {...}
list_only = false
creatures = {}

if #args > 0 then
    for _, arg in pairs(args) do
        if arg == 'list' then
            list_only = true
        else
            creatures[arg:upper()] = true
        end
    end
else
    creatures.CAT = true
end

total = 0
total_changed = 0
total_created = 0

males = {}
females = {}

for _, unit in pairs(world.units.all) do
    local id = world.raws.creatures.all[unit.race].creature_id
    males[id] = males[id] or {}
    females[id] = females[id] or {}
    table.insert((dfhack.units.isFemale(unit) and females or males)[id], unit)
end

if list_only then
    print("Type                   Male # Female #")
    -- sort IDs alphabetically
    local ids = {}
    for id in pairs(males) do
        table.insert(ids, id)
    end
    table.sort(ids)
    for _, id in pairs(ids) do
        print(("%22s %6d %8d"):format(id, #males[id], #females[id]))
    end
    return
end

for id in pairs(creatures) do
    local females = females[id] or {}
    total = total + #females
    for _, female in pairs(females) do
        if female.relations.pregnancy_timer ~= 0 then
            female.relations.pregnancy_timer = math.random(1, 100)
            total_changed = total_changed + 1
        elseif not female.relations.pregnancy_genes then
            local preg = df.unit_genes:new()
            preg.appearance:assign(female.appearance.genes.appearance)
            preg.colors:assign(female.appearance.genes.colors)
            female.relations.pregnancy_genes = preg
            female.relations.pregnancy_timer = math.random(1, 100)
            female.relations.pregnancy_caste = 1
            total_created = total_created + 1
        end
    end
end

if total_changed ~= 0 then
    print(("%d pregnancies accelerated."):format(total_changed))
end
if total_created ~= 0 then
    print(("%d pregnancies created."):format(total_created))
end
if total == 0 then
    qerror("No creatures matched.")
end
print(("Total creatures checked: %d"):format(total))
