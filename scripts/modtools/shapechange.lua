-- shapechange.lua
-- transforms unit (by number) into another creature, choice given to user. Syntax is: unitID tickamount maxsize namefilter. A size of 0 is ignored. A length of 0 is also ignored. If no filter, all units will be sorted. A filter of ALL will also work with all units.
-- author Putnam
-- edited by expwnent

--shapechange gui [unitId] [duration] [maxsize] [namefilter]
--shapechange manual [unitId] [creature name] [caste name] [duration]

local dialog = require('gui.dialogs')
local script = require('gui.script')
function transform(target,race,caste,length)
 if target==nil then
  qerror("Not a valid target")
 end
 local defaultRace = target.enemy.normal_race
 local defaultCaste = target.enemy.normal_caste
 target.enemy.normal_race = race --that's it???
 target.enemy.normal_caste = caste; --that's it!
 if length and length>0 then dfhack.timeout(length,'ticks',function() target.enemy.normal_race = defaultRace target.enemy.normal_caste = defaultCaste end) end
end

function getBodySize(caste)
 return caste.body_size_1[#caste.body_size_1-1]
end

function selectCreature(unitID,length,size,filter) --taken straight from here, but edited so I can understand it better: https://gist.github.com/warmist/4061959/... again. Also edited for syndromeTrigger, but in a completely different way.
 size = size or 0
 filter = filter or "all"
 length = length or 2400
 local creatures=df.global.world.raws.creatures.all
 local tbl={}
 local tunit=df.unit.find(unitID)
 for cr_k,creature in ipairs(creatures) do
  for ca_k,caste in ipairs(creature.caste) do
   local name=caste.caste_name[0]
   if name=="" then name="?" end
   if (not filter or string.find(name,filter) or string.lower(filter)=="all") and (not size or size>getBodySize(caste) or size<1 and not creature.flags.DOES_NOT_EXIST) then table.insert(tbl,{name,nil,cr_k,ca_k}) end
  end
 end
 table.sort(tbl,function(a,b) return a[1]<b[1] end)
 local f=function(name,C)
  transform(tunit,C[3],C[4],length)
 end
 script.start(function()
  local ok = 
   script.showYesNoPrompt(
    "Just checking","Do you want " 
    .. dfhack.TranslateName(dfhack.units.getVisibleName(tunit))
	.. " to transform into a creature of size below "..NEWLINE
    .. (not not size and size>1 and size or "infinity")
	.. " (" 
    .. size/(getBodySize(df.creature_raw.find(tunit.race).caste[tunit.caste]))*100
	.. "% of current size) for "
    ..length
    .." ticks ("
    ..length/1200
    .." days, ~"
    ..length/df.global.enabler.fps
    .." seconds)?",
    COLOR_LIGHTRED
   )
  if ok then dialog.showListPrompt("Creature Selection","Choose creature:",COLOR_WHITE,tbl,f) end
 end)
end

local args = {...}
--unit id, length, size, filter
if args[1] == 'gui' then
 selectCreature(tonumber(args[2]),tonumber(args[3]),tonumber(args[4]),args[5])
else
 local race-- = df.creature_raw.find(args[3])
 local raceIndex
 for index,raceCandidate in ipairs(df.global.world.raws.creatures.all) do
  if raceCandidate.creature_id == args[3] then
   raceIndex = index
   race = raceCandidate
   break
  end
 end
 local caste
 local casteIndex
 if race then
  for index,casteCandidate in ipairs(race.caste) do
   if casteCandidate.caste_id == args[4] then
    caste = casteCandidate
    casteIndex = index
    break
   end
  end
 end
 if not race or not caste then
  print("shapechange error: couldn't find " .. args[3] .. " " .. args[4])
  return
 end
 transform(df.unit.find(tonumber(args[2])), raceIndex, casteIndex, args[5] and tonumber(args[5]))
end

