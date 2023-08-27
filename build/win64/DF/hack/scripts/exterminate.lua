local argparse = require('argparse')

local function spawnLiquid(position, liquid_level, liquid_type, update_liquids)
  local map_block = dfhack.maps.getTileBlock(position)
  local tile = dfhack.maps.getTileFlags(position)

  tile.flow_size = liquid_level
  tile.liquid_type = liquid_type
  tile.flow_forbid = false

  map_block.flags.update_liquid = update_liquids
  map_block.flags.update_liquid_twice = update_liquids
end

local function checkUnit(unit)
    return (unit.body.blood_count ~= 0 or unit.body.blood_max == 0) and
        not unit.flags1.inactive and
        not unit.flags1.caged and
        not unit.flags1.chained
end

local function isUnitFriendly(unit)
    return dfhack.units.isCitizen(unit) or
        dfhack.units.isOwnCiv(unit) or
        dfhack.units.isOwnGroup(unit) or
        dfhack.units.isVisiting(unit) or
        dfhack.units.isTame(unit) or
        dfhack.units.isDomesticated(unit) or
        dfhack.units.isVisitor(unit) or
        dfhack.units.isDiplomat(unit) or
        dfhack.units.isMerchant(unit)
end

local killMethod = {
    INSTANT = 0,
    BUTCHER = 1,
    MAGMA = 2,
    DROWN = 3,
    VAPORIZE = 4,
}

-- removes the unit from existence, leaving no corpse if the unit hasn't died
-- by the time the vanish countdown expires
local function vaporizeUnit(unit, target_value)
    target_value = target_value or 1
    unit.animal.vanish_countdown = target_value
end

-- Kills a unit by removing blood and also setting a vanish countdown as a failsafe.
local function destroyUnit(unit)
    unit.body.blood_count = 0
    vaporizeUnit(unit, 2)
end

--  Marks a unit for slaughter at the butcher's shop.
local function butcherUnit(unit)
    unit.flags2.slaughter = true
end

local function drownUnit(unit, liquid_type)
    previousPositions = previousPositions or {}
    previousPositions[unit.id] = copyall(unit.pos)

    local function createLiquid()
        spawnLiquid(unit.pos, 7, liquid_type)
        if not same_xyz(previousPositions[unit.id], unit.pos) then
            spawnLiquid(previousPositions[unit.id], 0, nil, false)
            previousPositions[unit.id] = copyall(unit.pos)
        end
        if unit.flags2.killed then
            spawnLiquid(previousPositions[unit.id], 0, nil, false)
        else
            dfhack.timeout(1, 'ticks', createLiquid)
        end
    end
    createLiquid()
end

local function killUnit(unit, method)
    if method == killMethod.BUTCHER then
        butcherUnit(unit)
    elseif method == killMethod.MAGMA then
        drownUnit(unit, df.tile_liquid.Magma)
    elseif method == killMethod.DROWN then
        drownUnit(unit, df.tile_liquid.Water)
    elseif method == killMethod.VAPORIZE then
        vaporizeUnit(unit)
    else
        destroyUnit(unit)
    end
end

local function getRaceCastes(race_id)
    local unit_castes = {}
    for _, caste in pairs(df.creature_raw.find(race_id).caste) do
        unit_castes[caste.caste_id] = {}
    end
    return unit_castes
end

local function getMapRaces(only_visible, include_friendly)
    local map_races = {}
    for _, unit in pairs(df.global.world.units.active) do
        if only_visible and not dfhack.units.isVisible(unit) then
            goto skipunit
        end
        if not include_friendly and isUnitFriendly(unit) then
            goto skipunit
        end
        if dfhack.units.isActive(unit) and checkUnit(unit) then
            local unit_race_name = dfhack.units.isUndead(unit) and "UNDEAD" or df.creature_raw.find(unit.race).creature_id

            local race = ensure_key(map_races, unit_race_name)
            race.id = unit.race
            race.name = unit_race_name
            race.count = (race.count or 0) + 1
        end
        :: skipunit ::
    end

    return map_races
end

local options, args = {
    help = false,
    method = killMethod.INSTANT,
    only_visible = false,
    include_friendly = false,
}, {...}

local positionals = argparse.processArgsGetopt(args, {
    {'h', 'help', handler = function() options.help = true end},
    {'m', 'method', handler = function(arg) options.method = killMethod[arg:upper()] end, hasArg = true},
    {'o', 'only-visible', handler = function() options.only_visible = true end},
    {'f', 'include-friendly', handler = function() options.include_friendly = true end},
})

if not dfhack.isMapLoaded() then
    qerror('This script requires a fortress map to be loaded')
end

if positionals[1] == "help" or options.help then
    print(dfhack.script_help())
    return
end

if positionals[1] == "this" then
    local selected_unit = dfhack.gui.getSelectedUnit()
    if not selected_unit then
        qerror("Select a unit and run the script again.")
    end
    killUnit(selected_unit, options.method)
    print('Unit exterminated.')
    return
end

local map_races = getMapRaces(options.only_visible, options.include_friendly)

if not positionals[1] then
    local sorted_races = {}
    for race, value in pairs(map_races) do
        table.insert(sorted_races, { name = race, count = value.count })
    end
    table.sort(sorted_races, function(a, b)
        return a.count > b.count
    end)
    for _, race in ipairs(sorted_races) do
        print(([[%4s %s]]):format(race.count, race.name))
    end
    return
end

local count = 0
if positionals[1]:lower() == 'undead' then
    if not map_races.UNDEAD then
        qerror("No undead found on the map.")
    end
    for _, unit in pairs(df.global.world.units.active) do
        if dfhack.units.isUndead(unit) and checkUnit(unit) then
            killUnit(unit, options.method)
            count = count + 1
        end
    end
else
    local selected_race, selected_caste = positionals[1], nil

    if string.find(selected_race, ':') then
        local tokens = positionals[1]:split(':')
        selected_race, selected_caste = tokens[1], tokens[2]
    end

    if not map_races[selected_race] then
        qerror("No creatures of this race on the map.")
    end

    local race_castes = getRaceCastes(map_races[selected_race].id)

    if selected_caste and not race_castes[selected_caste] then
        qerror("Invalid caste.")
    end

    for _, unit in pairs(df.global.world.units.active) do
        if not checkUnit(unit) then
            goto skipunit
        end
        if options.only_visible and not dfhack.units.isVisible(unit) then
            goto skipunit
        end
        if not options.include_friendly and isUnitFriendly(unit) then
            goto skipunit
        end
        if selected_caste and selected_caste ~= df.creature_raw.find(unit.race).caste[unit.caste].caste_id then
            goto skipunit
        end

        if selected_race == df.creature_raw.find(unit.race).creature_id then
            killUnit(unit, options.method)
            count = count + 1
        end

        :: skipunit ::
    end
end

print(([[Exterminated %d creatures.]]):format(count))
