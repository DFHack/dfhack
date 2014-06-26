
local _ENV = mkmodule('onStrike')

local onReport = require 'plugins.onReport'
local utils = require 'utils'

debug = debug or true

triggers = triggers or {}

function getReportString(reportId)
 local report = df.report.find(reportId)
 local result = report.text
 local i = 1
 local report2 = df.report.find(reportId+i)
 while report2 and report2.flags.continuation do
  result = result .. ' ' .. report2.text
  i = i+1
  report2 = df.report.find(reportId+i)
 end
 return result
end

onReport.triggers.onStrike = function(reportId)
 local report = df.report.find(reportId)
 if report["type"] ~= df.announcement_type.COMBAT_STRIKE_DETAILS then
  return
 end
 if report.flags.continuation then
  return
 end
-- print('\n')
 local fighters = {}
 for unitId,_ in pairs(onReport.eventToDwarf[reportId]) do
  table.insert(fighters,unitId)
 end
 local reportString = getReportString(reportId)
 if #fighters ~= 2 then
  if debug then
   local ok = string.find(reportString,' skids along ')
   if not ok then 
    print('onStrike: #fighters = ' .. #fighters)
    print(reportString)
    df.global.pause_state = true
   end
  end
  return
 end
 local info = {}
 local count = 0
 fighters[1] = df.unit.find(fighters[1])
 fighters[2] = df.unit.find(fighters[2])
 local function getWound(fighterA,fighterB)
  local wound
  for i=#fighterB.body.wounds-1,0,-1 do
   local w = fighterB.body.wounds[i]
   if w.unit_id == fighterA.id and w.age <= 1 then
    wound = w
    break
   end
  end
  --[[name,_ = tryParse(reportString,getNames(fighterA))
  if not name then
   wound = nil
  end
  --]]
  return wound
 end
 local wound1 = getWound(fighters[1],fighters[2])
 local wound2 = getWound(fighters[2],fighters[1])
 local flying = string.find(reportString,'The flying')
 local name1
 local name2
 if flying then
  name1 = findAny(reportString,getNames(fighters[2]))
  name2 = findAny(reportString,getNames(fighters[1]))
 else
  name1 = tryParse(reportString,getNames(fighters[1]))
  name2 = tryParse(reportString,getNames(fighters[2]))
 end
 if name1 and wound1 and name2 and wound2 then
  if debug then
   print('ambiguous wounds: ' .. reportString)
   print('fighter1 = ' .. fighters[1].id)
   print('fighter2 = ' .. fighters[2].id)
   df.global.pause_state = true
  end
  return
 elseif not wound1 and not wound2 then
  local ok = fighters[1].flags1.dead or fighters[2].flags1.dead or string.find(reportString,' grabs ') or string.find(reportString,'snatches at') or string.find(reportString,'glances away!') or string.find(reportString, ' shakes ')
  if not ok and debug then
   print('neither wound works: ' .. reportString)
   print('fighter1 = ' .. fighters[1].id)
   print('fighter2 = ' .. fighters[2].id)
   df.global.pause_state = true
  end
  return
 elseif not name1 and not name2 and not string.find(reportString,'The flying ') then
  if debug then
   print('WTF?')
   print('fighter1 = ' .. fighters[1].id)
   print('fighter2 = ' .. fighters[2].id)
   df.global.pause_state = true
  end
  return
 elseif name1 and wound1 then
 else
  local temp = fighters[1]
  fighters[1] = fighters[2]
  fighters[2] = temp
 end
 local wound = wound1 or wound2
 
 --is it a weapon attack?
 local isWeaponAttack
 if getWeapon(fighters[1]) and string.find(reportString,getWeaponString(fighters[1])) then
  isWeaponAttack = true
 else
  isWeaponAttack = false
 end
 isWeaponAttack = isWeaponAttack or flying
 local weapon
 if isWeaponAttack then
  weapon = getWeapon(fighters[1])
 end
-- print('triggers')
 for _,trigger in pairs(triggers) do
--  print('trigger')
  trigger(fighters[1],fighters[2],weapon,wound)
 end
end

function myConcat(table1,table2)
 local result = {}
 for _,v in pairs(table1) do
  table.insert(result,v)
 end
 for _,v in pairs(table2) do
  table.insert(result,v)
 end
 return result
end

function getUnitAttackStrings(unit)
 local result = {}
 for _,attack in ipairs(unit.body.body_plan.attacks) do
  table.insert(result,attack.verb_3rd..' ')
 end
 return result
end

function getUnitAttack(unit,parsedAttack)
 for _,attack in ipairs(unit.body.body_plan.attacks) do
  if attack.verb_3rd..' ' == parsedAttack then
   return result
  end
 end
 return nil
end

function getWeapon(unit)
 function dumb(item)
--  print('\n')
--  print(item)
--  printall(item)
  if item.mode ~= df.unit_inventory_item.T_mode.Weapon then
   --print('item.mode ' .. item.mode .. ' /= Weapon ' .. df.unit_inventory_item.T_mode.Weapon)
   return false
  end
  if item.item._type ~= df.item_weaponst then
   --print('item.item._type ' .. item.item._type .. ' /= df.item_weaponst ' .. df.item_weaponst)
   return false
  end
  return true
 end
 for _,item in ipairs(unit.inventory) do
  if dumb(item) then
   return item.item
  end
 end
 return nil
end

function getWeaponAttackStrings(unit)
 local result = {}
 local weapon = getWeapon(unit)
 if not weapon then
  print('no weapon')
  return result
 end
 for _,attack in ipairs(weapon.subtype.attacks) do
  table.insert(result,attack.verb_3rd..' ')
 end
 return result
end

function getWeaponAttack(unit,parsedAttack)
 local weapon = getWeapon(unit)
 if not weapon then
  return nil
 end
 for _,attack in ipairs(weapon.subtype.attacks) do
  if attack.verb_3rd..' ' == parsedAttack then
   return attack
  end
 end
 return nil
end

function getNames(unit)
 local result = {}
 table.insert(result,unit.name.first_name .. ' ')
 table.insert(result,'The '..dfhack.units.getProfessionName(unit)..' ')
 table.insert(result,'the '..dfhack.units.getProfessionName(unit)..' ')
 table.insert(result,'The Stray '..dfhack.units.getProfessionName(unit)..' ')
 table.insert(result,'the stray '..dfhack.units.getProfessionName(unit)..' ')
 return result
end

function getWeaponString(unit,suffix)
 local weapon = getWeapon(unit)
 if not weapon then
  return ''
 end
 local material = getMaterialString(weapon)
 return material .. ' ' .. weapon.subtype.name .. (suffix or '')
end

function getMaterialString(item)
 local material = dfhack.matinfo.decode(item.mat_type,item.mat_index)
 return material.material.state_name[df.matter_state.Solid]
end

function findAny(parseString,strs)
 for _,str in ipairs(strs) do
  if string.find(parseString,str) then
   return str
  end
 end
 return nil
end

function tryParse(parseString,strs)
 for _,str in ipairs(strs) do
  if string.sub(parseString,1,#str) == str then
   --print('\n"' .. str .. '" matches "' .. parseString .. '"\n')
   return str,string.sub(parseString,#str+1,#parseString)
  end
  --print('\n"' .. str .. '" doesn\'t match "' .. parseString .. '"\n')
 end
 return nil,nil
end

return _ENV

