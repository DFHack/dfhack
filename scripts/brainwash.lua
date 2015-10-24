-- This script will brainwash a dwarf, modifying their personality
-- usage is:  target a unit in DF, and execute this script in dfhack
-- by vjek
--[[=begin

brainwash
=========
Modify the personality traits of the selected dwarf to match an 'ideal'
personality - as stable and reliable as possible. This makes dwarves very
stable, preventing tantrums even after months of misery.

=end]]

function brainwash_unit(profile)
    local i,unit_name

    unit=dfhack.gui.getSelectedUnit()
        if unit==nil then
            print ("No unit under cursor!  Aborting with extreme prejudice.")
        return
    end

    unit_name=dfhack.TranslateName(dfhack.units.getVisibleName(unit))

    print("Previous personality values for "..unit_name)
    printall(unit.status.current_soul.personality.traits)

    --now set new personality
    for i=1, #profile do
        unit.status.current_soul.personality.traits[i-1]=profile[i]
    end

    print("New personality values for "..unit_name)
    printall(unit.status.current_soul.personality.traits)
    print(unit_name.." has been brainwashed, praise Armok!")
end

-- main script starts here
-- profiles are listed here and passed to the brainwash function
--
local baseline={50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50}
local ideal={75,25,25,75,25,25,25,99,25,25,25,50,75,50,25,75,75,50,75,75,25,75,75,50,75,25,50,25,75,75,75,25,75,75,25,75,25,25,75,75,25,75,75,75,25,75,75,25,25,50}
local stepford={99,1,1,99,1,1,1,99,1,1,1,1,50,50,1,99,99,50,50,50,1,1,99,50,50,50,50,50,50,99,50,1,1,99,1,99,1,1,99,99,1,99,99,99,1,50,50,1,1,1}
local wrecked={1,99,99,1,99,99,99,1,99,99,99,1,1,99,99,1,1,1,1,1,99,1,1,99,1,99,99,99,1,1,1,99,1,1,99,1,99,99,1,1,99,1,1,1,99,1,1,99,99,99}
local opt = ...

if opt then
    if opt=="ideal" then
        brainwash_unit(ideal)
        return
    end
    if opt=="baseline" then
        brainwash_unit(baseline)
        return
    end
    if opt=="stepford" then
        brainwash_unit(stepford)
        return
    end
    if opt=="wrecked" then
        brainwash_unit(wrecked)
        return
    end
else
    print ("Invalid or missing personality argument.\nValid choices are ideal , baseline , stepford, and wrecked.")
    print ("ideal will create a reliable dwarf with generally positive personality traits.")
    print ("baseline will reset all personality traits to a default / the average.")
    print ("stepford amplifies all good qualities to an excessive degree.")
    print ("wrecked amplifies all bad qualities to an excessive degree.")
end
