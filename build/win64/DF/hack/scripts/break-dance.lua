--Breaks up a stuck dance activity.
--[====[

break-dance
===========
Breaks up dances, such as broken or otherwise hung dances that occur when a unit
can't find a partner.

]====]
local unit
if dfhack.world.isAdventureMode() then
    unit = df.global.world.units.active[0]
else
    unit = dfhack.gui.getSelectedUnit(true) or qerror('No unit selected')
end
local count = 0
for _, act_id in ipairs(unit.social_activities) do
    local act = df.activity_entry.find(act_id)
    if act and act.type == 8 then
        act.events[0].flags.dismissed = true
        count = count + 1
    end
end
print('Broke up ' .. count .. ' dance(s)')
