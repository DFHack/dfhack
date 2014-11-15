--lua/syndrome-util.lua
--author expwnent
--some utilities for adding syndromes to units

local _ENV = mkmodule("syndrome-util")
local utils = require("utils")

function findUnitSyndrome(unit,syn_id)
 for index,syndrome in ipairs(unit.syndromes.active) do
  if syndrome['type'] == syn_id then
   return syndrome
  end
 end
 return nil
end

--usage: syndromeUtil.ResetPolicy.DoNothing, syndromeUtil.ResetPolicy.ResetDuration, etc
ResetPolicy = ResetPolicy or utils.invert({
 "DoNothing",
 "ResetDuration",
 "AddDuration",
 "NewInstance"
})

function eraseSyndrome(unit,syndromeId,oldestFirst)
 local i1
 local iN
 local d
 if oldestFirst then
  i1 = 0
  iN = #unit.syndromes.active-1
  d = 1
 else
  i1 = #unit.syndromes.active-1
  iN = 0
  d = -1
 end
 local syndromes = unit.syndromes.active
 for i=i1,iN,d do
  if syndromes[i]['type'] == syndromeId then
   syndromes:erase(i)
   return true
  end
 end
 return false
end

local function eraseSyndromeClassHelper(unit,synclass)
 for i,unitSyndrome in ipairs(unit.syndromes.active) do
  local syndrome = df.syndrome.find(unitSyndrome.type)
  for _,class in ipairs(syndrome.syn_class) do
   if class.value == synclass then
    unit.syndromes.active:erase(i)
    return true
   end
  end
 end
 return false
end

function eraseSyndromeClass(unit,synclass)
 local count=0
 while eraseSyndromeClassHelper(unit,synclass) do
  count = count+1
 end
 return count
end

function eraseSyndromes(unit,syndromeId)
 local count=0
 while eraseSyndrome(unit,syndromeId,true) do
  count = count+1
 end
 return count
end

--target is a df.unit, syndrome is a df.syndrome, resetPolicy is one of syndromeUtil.ResetPolicy
--if the target has an instance of the syndrome already, the reset policy takes effect
--returns true if the unit did not have the syndrome before calling and false otherwise
function infectWithSyndrome(target,syndrome,resetPolicy)
 local oldSyndrome = findUnitSyndrome(target,syndrome.id)
 if oldSyndrome == nil or resetPolicy == nil or resetPolicy == ResetPolicy.NewInstance then
  local unitSyndrome = df.unit_syndrome:new()
  unitSyndrome.type = syndrome.id
  unitSyndrome.year = df.global.cur_year
  unitSyndrome.year_time = df.global.cur_year_tick
  unitSyndrome.ticks = 0
  unitSyndrome.wound_id = -1
  for k,v in ipairs(syndrome.ce) do
   local symptom = df.unit_syndrome.T_symptoms:new()
   symptom.quantity = 0
   symptom.delay = 0
   symptom.ticks = 0
   symptom.flags.active = true
   unitSyndrome.symptoms:insert("#",symptom)
  end
  target.syndromes.active:insert("#",unitSyndrome)
 elseif resetPolicy == ResetPolicy.DoNothing then
 elseif resetPolicy == ResetPolicy.ResetDuration then
  for k,symptom in ipairs(oldSyndrome.symptoms) do
   symptom.ticks = 0
  end
  oldSyndrome.ticks = 0
 elseif resetPolicy == ResetPolicy.AddDuration then
  for k,symptom in ipairs(oldSyndrome.symptoms) do
   --really it's syndrome.ce[k].end, but lua doesn't like that because keywords
   if syndrome.ce[k]["end"] ~= -1 then
    symptom.ticks = symptom.ticks - syndrome.ce[k]["end"]
   end
  end
 else qerror("Bad reset policy: " .. resetPolicy)
 end
 return (oldSyndrome == nil)
end

function isValidTarget(unit,syndrome)
 --mostly copied from itemsyndrome, which is based on autoSyndrome
 if
      #syndrome.syn_affected_class==0
  and #syndrome.syn_affected_creature==0
  and #syndrome.syn_affected_caste==0
  and #syndrome.syn_immune_class==0
  and #syndrome.syn_immune_creature==0
  and #syndrome.syn_immune_caste==0
 then
  return true
 end
 local affected = false
 local unitRaws = df.creature_raw.find(unit.race)
 local casteRaws = unitRaws.caste[unit.caste]
 local unitRaceName = unitRaws.creature_id
 local unitCasteName = casteRaws.caste_id
 local unitClasses = casteRaws.creature_class
 for _,unitClass in ipairs(unitClasses) do
  for _,syndromeClass in ipairs(syndrome.syn_affected_class) do
   if unitClass.value==syndromeClass.value then
    affected = true
   end
  end
 end
 for caste,creature in ipairs(syndrome.syn_affected_creature) do
  local affectedCreature = creature.value
  local affectedCaste = syndrome.syn_affected_caste[caste].value
  if affectedCreature == unitRaceName and (affectedCaste == unitCasteName or affectedCaste == "ALL") then
   affected = true
  end
 end
 for _,unitClass in ipairs(unitClasses) do
  for _,syndromeClass in ipairs(syndrome.syn_immune_class) do
   if unitClass.value == syndromeClass.value then
    affected = false
   end
  end
 end
 for caste,creature in ipairs(syndrome.syn_immune_creature) do
  local immuneCreature = creature.value
  local immuneCaste = syndrome.syn_immune_caste[caste].value
  if immuneCreature == unitRaceName and (immuneCaste == unitCasteName or immuneCaste == "ALL") then
   affected = false
  end
 end
 return affected
end

function infectWithSyndromeIfValidTarget(target,syndrome,resetPolicy)
 if isValidTarget(target,syndrome) then
  infectWithSyndrome(target,syndrome,resetPolicy)
  return true
 else
  return false
 end
end

return _ENV

