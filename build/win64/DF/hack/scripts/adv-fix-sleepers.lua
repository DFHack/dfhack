--Fixes all local bugged sleepers in adventure mode.
--[====[

adv-fix-sleepers
================
Fixes :bug:`6798`. This bug is characterized by sleeping units who refuse to
awaken in adventure mode regardless of talking to them, hitting them, or waiting
so long you die of thirst. If you come accross one or more bugged sleepers in
adventure mode, simply run the script and all nearby sleepers will be cured.

Usage::

    adv-fix-sleepers


]====]

--========================
-- Author: ArrowThunder on bay12 & reddit
-- Version: 1.1
--=======================

-- get the list of all the active units currently loaded
local active_units = df.global.world.units.active -- get all active units

-- check every active unit for the bug
local num_fixed = 0 -- this is the number of army controllers fixed, not units
    -- I've found that often, multiple sleepers share a bugged army controller
for k, unit in pairs(active_units) do
    if unit then
        local army_controller = df.army_controller.find(unit.enemy.army_controller_id)
        if army_controller and army_controller.type == 4 then -- sleeping code is possible
            if army_controller.unk_64.t4.unk_2.not_sleeping == false then
                army_controller.unk_64.t4.unk_2.not_sleeping = true -- fix bug
                num_fixed = num_fixed + 1
            end
        end
    end
end

if num_fixed == 0 then
    print ("No sleepers with the fixable bug were found, sorry.")
else
    print ("Fixed " .. num_fixed .. " bugged army_controller(s).")
end
