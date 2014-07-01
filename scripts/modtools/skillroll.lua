-- skillroll.lua
-- Allows skills to activate lua scripts.
-- author Putnam
-- edited by expwnent

--[[Example usage:
...syndrome stuff...
[SYN_CLASS:\COMMAND][SYN_CLASS:modtools/skillroll][SYN_CLASS:\WORKER_ID] For autoSyndrome/syndromeTrigger.
[SYN_CLASS:MELEE_COMBAT] Can use any skill, including NONE (no bonus)
[SYN_CLASS:20] Rolls uniformly from 1 to 20 inclusive. Skill will be weighted to this value.
[SYN_CLASS:DICEROLL_1] If diceroll ends up as one...
[SYN_CLASS:kill][SYN_CLASS:\SKILL_UNIT_ID] Theoretical kill-given-unit-id command; slayrace doesn't do so.
[SYN_CLASS:DICEROLL_10] If diceroll is between 1 and 10 (2-10, inclusive)...
[SYN_CLASS:modtools/force][SYN_CLASS:migrants][SYN_CLASS:player] Force migrants.
[SYN_CLASS:DICEROLL_19] If diceroll is between 10 and 19 (11-19, inclusive)...
[SYN_CLASS:fullheal][SYN_CLASS:\SKILL_UNIT_ID] Fully heals unit.
[SYN_CLASS:DICEROLL_20] If diceroll is at least 20...
[SYN_CLASS:modtools/shapechange][SYN_CLASS:\SKILL_UNIT_ID] Turns unit into any creature permanently.

or from the console
modtools/skillroll workerId MELEE_COMBAT 20 DICEROLL_1 kill workerId DICEROLL_10 modtools/force migrants player DICEROLL_19 fullheal workerId DICEROLL_20 modtools/shapechange workerId
]]

local args={...}
if args[1]=='dryrun' then 
 unit=df.global.world.units.all[0]
end
local unit = unit or df.unit.find(args[1])
rando=rando or dfhack.random.new()
local roll=rando:random(tonumber(args[3]))
if args[2] ~= 'NONE' then
 local result=roll+(dfhack.units.getEffectiveSkill(unit,df.job_skill[args[2]])*(tonumber(args[3])/20))
 result = result%1<.5 and math.floor(result) or math.ceil(result)
 roll = result
end

local i=4
local command={}
local scriptIsFinished
repeat
 local arg=args[i]
 if arg:find('DICEROLL') then
  local dicerollnumber=tonumber(arg:match('%d+')) --yes this is truly naive as hell; I imagine if you put DICEROLL3%moa5oam3 it'll return 353.
  if dicerollnumber>=roll then
   repeat
    i=i+1
    if i<=#args and (not args[i]:find('DICEROLL')) then
     if args[i]~='\\SKILL_UNIT_ID' then table.insert(command,args[i]) else table.insert(command,args[1]) end
    end
   until i>#args or args[i]:find('DICEROLL')
   dfhack.run_command(table.unpack(command))
   scriptIsFinished=true
  else
   i=i+1
  end
 else
  i=i+1
 end
until i>#args or scriptIsFinished
