-- Sets stress to negative one million
-- With unit selected, affects that unit.  Use "remove-stress all" to affect all units.

--By Putnam; http://www.bay12forums.com/smf/index.php?topic=139553.msg5820486#msg5820486

local args = {...}

if args[1]=='all' then
    for k,v in ipairs(df.global.world.units.active) do
        v.status.current_soul.personality.stress_level=-1000000
    end
else
    dfhack.gui.getSelectedUnit().status.current_soul.personality.stress_level=-1000000
end