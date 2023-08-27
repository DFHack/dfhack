local world = df.global.world

if not dfhack.isWorldLoaded() then
    qerror('World not loaded.')
end

local args = {...}
local list_only = false
local creatures = {}

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

local total = 0
local total_changed = 0
local total_created = 0

local males = {} --as:df.unit[][]
local females = {} --as:df.unit[][]

for _, unit in pairs(world.units.active) do
    if dfhack.units.isDead(unit) then
        goto continue
    end
    local id = world.raws.creatures.all[unit.race].creature_id
    males[id] = males[id] or {}
    females[id] = females[id] or {}
    table.insert((dfhack.units.isFemale(unit) and females or males)[id], unit)
    ::continue::
end

if list_only then
    print("Type                   Male # Female #")
    -- sort IDs alphabetically
    local ids = {} --as:string[]
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
    local female_list = females[id] or {}
    total = total + #female_list
    for _, female in pairs(female_list) do
        if female.pregnancy_timer ~= 0 then
            female.pregnancy_timer = math.random(1, 100)
            total_changed = total_changed + 1
        elseif not female.pregnancy_genes then
            local preg = df.unit_genes:new()
            preg.appearance:assign(female.appearance.genes.appearance)
            preg.colors:assign(female.appearance.genes.colors)
            female.pregnancy_genes = preg
            female.pregnancy_timer = math.random(1, 100)
            female.pregnancy_caste = female.caste  -- To handle female only species
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
