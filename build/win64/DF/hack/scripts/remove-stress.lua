-- Sets stress to negative one million
--By Putnam; http://www.bay12forums.com/smf/index.php?topic=139553.msg5820486#msg5820486
--edited by Bumber
--@module = true
local help = [====[

remove-stress
=============
Sets stress to -1,000,000; the normal range is 0 to 500,000 with very stable or
very stressed dwarves taking on negative or greater values respectively.
Applies to the selected unit, or use ``remove-stress -all`` to apply to all units.

Using the argument ``-value 0`` will reduce stress to the value 0 instead of -1,000,000.
Negative values must be preceded by a backslash (\): ``-value \-10000``.
Note that this can only be used to *decrease* stress - it cannot be increased
with this argument.

]====]

local utils = require 'utils'

function removeStress(unit,value)
    if unit.counters.soldier_mood > df.unit.T_counters.T_soldier_mood.Enraged then
        -- Tantrum, Depressed, or Oblivious
        unit.counters.soldier_mood = df.unit.T_counters.T_soldier_mood.None
    end
    if unit.status.current_soul then
        if unit.status.current_soul.personality.stress > value then
            unit.status.current_soul.personality.stress = value
        end
    end
end

local validArgs = utils.invert({
    'help',
    'all',
    'value'
})

function main(...)
    local args = utils.processArgs({...}, validArgs)
    local stress_value = -1000000

    if args.help then
        print(help)
        return
    end

    if args.value then
        if tonumber(args.value) ~= nil then
            stress_value = tonumber(args.value)
        else
            error 'Invalid usage: Missing stress value for -value.'
        end
    end

    if args.all then
        for k,v in ipairs(df.global.world.units.active) do
            removeStress(v,stress_value)
        end
    else
        local unit = dfhack.gui.getSelectedUnit()
        if unit then
            removeStress(unit,stress_value)
        else
            error 'Invalid usage: No unit selected and -all argument not given.'
        end
    end
end

if not dfhack_flags.module then
    main(...)
end
