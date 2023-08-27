-- Sets a unit's combat-hardened value to a given percent
--@ module = true

local help = [====[

combat-harden
=============
Sets the combat-hardened value on a unit, making them care more/less about seeing corpses.
Requires a value and a target.

Valid values:

:``-value <0-100>``:
    A percent value to set combat hardened to.
:``-tier <1-4>``:
    Choose a tier of hardenedness to set it to.
      1 = No hardenedness.
      2 = "is getting used to tragedy"
      3 = "is a hardened individual"
      4 = "doesn't really care about anything anymore" (max)

If neither are provided, the script defaults to using a value of 100.

Valid targets:

:``-all``:
    All active units will be affected.
:``-citizens``:
    All (sane) citizens of your fort will be affected. Will do nothing in adventure mode.
:``-unit <UNIT ID>``:
    The given unit will be affected.

If no target is given, the provided unit can't be found, or no unit id is given with the unit
argument, the script will try and default to targeting the currently selected unit.

]====]

local utils = require 'utils'

local validArgs = utils.invert({
    'help',
    'all',
    'citizens',
    'tier',
    'unit',
    'value',
})

local tiers = {0, 33, 67, 100}

function setUnitCombatHardened(unit, value)
    if unit.status.current_soul ~= nil then
        -- Ensure value is in the bounds of 0-100
        local value = math.max(0, math.min(100, value))

        unit.status.current_soul.personality.combat_hardened = value
    end
end

function main(...)
    local args = utils.processArgs({...}, validArgs)

    if args.help then
        print(help)
        return
    end

    local value
    if not args.tier and not args.value then
        -- Default to 100
        value = 100
    elseif args.tier then
        -- Bound between 1-4
        local tierNum = math.max(1, math.min(4, tonumber(args.tier)))
        value = tiers[tierNum]
    elseif args.value then
        -- Function ensures value is bound, so no need to bother here
        -- Will check it's a number, though
        value = tonumber(args.value) or 100
    end

    local unitsList = {} --as:df.unit[]

    if not args.all and not args.citizens then
        -- Assume trying to target a unit
        local unit
        if args.unit then
            if tonumber(args.unit) then
                unit = df.unit.find(args.unit)
            end
        end

        -- If unit ID wasn't provided / unit couldn't be found,
        -- Try getting selected unit
        if unit == nil then
            unit = dfhack.gui.getSelectedUnit(true)
        end

        if unit == nil then
            qerror("Couldn't find unit. If you don't want to target a specific unit, use -all or -citizens.")
        else
            table.insert(unitsList, unit)
        end
    elseif args.all then
        for _, unit in pairs(df.global.world.units.active) do
            table.insert(unitsList, unit)
        end
    elseif args.citizens then
        -- Technically this will exclude insane citizens, but this is the
        -- easiest thing that dfhack provides

        -- Abort if not in Fort mode
        if not dfhack.world.isFortressMode() then
            qerror('-citizens requires fortress mode')
        end

        for _, unit in pairs(df.global.world.units.active) do
            if dfhack.units.isCitizen(unit) then
                table.insert(unitsList, unit)
            end
        end
    end

    for index, unit in ipairs(unitsList) do
        setUnitCombatHardened(unit, value)
    end
end

if not dfhack_flags.module then
    main(...)
end
