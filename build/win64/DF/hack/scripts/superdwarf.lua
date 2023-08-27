-- makes units very speedy
--@ module = true
local repeatUtil = require('repeat-util')

local timerId = 'superdwarf'
superUnits = superUnits or {}
local commands = {}

local function getUnit(obj)
    -- No argument or nil
    if not obj then
        return dfhack.gui.getAnyUnit()
    end
    -- Passed an ID as a string or number
    if type(obj) == 'string' or type(obj) == 'number' then
        obj = tonumber(obj) -- Can be nil if someone passed something like a name
        return obj and df.unit.find(obj)
    end
    -- Passed a unit object as a table or userdata
    return obj
end

local function getName(unit)
    local name = dfhack.TranslateName(unit.name)
    -- Animals will have a profession name if they aren't named
    if name == '' then
        name = dfhack.units.getProfessionName(unit)
    end
    return name
end

local function onTimer()
    if not next(superUnits) then
        repeatUtil.cancel(timerId)
    else
        for id in pairs(superUnits) do
            local unit = getUnit(id)
            if unit and not unit.flags1.inactive then
                dfhack.units.setGroupActionTimers(unit, 1, df.unit_action_type_group.All)
            else
                superUnits[id] = nil
            end
        end
    end
end

commands = {
    add = function(arg)
        local unit = getUnit(arg)
        if not unit then
            qerror('No unit found to add; select a unit or provide a unit id')
        end
        print(
            string.format('Applying superdwarf to [%d] %s', unit.id, getName(unit))
        )
        superUnits[unit.id] = true
        repeatUtil.scheduleEvery(timerId, 1, 'ticks', onTimer)
    end,
    all = function(arg)
        for _, unit in pairs(df.global.world.units.active) do
            if dfhack.units.isCitizen(unit) then
                commands.add(unit)
            end
        end
    end,
    del = function(arg)
        arg = arg and tonumber(arg)
        local unit = getUnit(arg)
        if unit then
            print(
                string.format('Removing superdwarf from [%d] %s', unit.id, getName(unit))
            )
            superUnits[unit.id] = nil
        elseif arg and superUnits[arg] then -- We could, theoretically, have an invalid unit listed
            print(
                string.format('Removing superdwarf from an invalid unit with ID %d', arg)
            )
            superUnits[arg] = nil
        else
            qerror('No unit found to remove; select a unit or provide a unit id')
        end
    end,
    clear = function(arg)
        print('Removing superdwarf from all units')
        superUnits = {}
    end,
    list = function(arg)
        print('Current Superdwarfs: ')
        for id in pairs(superUnits) do
            local unit = getUnit(id)
            if not unit then
                superUnits[id] = nil
            else
                print(
                    string.format("\t[%d] %s", id, getName(unit))
                )
            end
        end
    end
}

function main(args)
    local command = args[1] and string.lower(args[1])

    if commands[command] then
        commands[command](args[2])
    else
        print(dfhack.script_help())
    end
end

if not dfhack_flags.module then
    main({...})
end
