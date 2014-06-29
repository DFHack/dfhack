--attackTrigger.lua
--author expwnent
--triggers scripts when a unit attacks another with a weapon type, a weapon of a particular material

local eventful = require 'plugins.eventful'
eventful.enableEvent(eventful.eventType.UNIT_ATTACK,1)

itemTriggers = itemTriggers or {}
materialTriggers = materialTriggers or {}
itemMaterialTriggers = itemMaterialTriggers or {}

local function processTrigger(command, attacker, defender)
 local command2 = {}
 for i,arg in ipairs(command) do
  if arg == '\\ATTACKER' then
   command2[i] = '' .. attacker.id
  elseif arg == '\\DEFENDER' then
   command2[i] = '' .. defender.id
  else
   command2[i] = arg
  end
 end
 print(dfhack.run_command(table.unpack(command2)))
end

eventful.onUnitAttack.attackTrigger = function(attacker,defender,wound)
 attacker = df.unit.find(attacker)
 defender = df.unit.find(defender)
 
 if not attacker then
  return
 end
 
 local attackerWeapon
 for _,item in ipairs(attacker.inventory) do
  if item.mode == df.unit_inventory_item.T_mode.Weapon then
   attackerWeapon = item.item
   break
  end
 end
 
 if not attackerWeapon then
  return
 end
 
 local weaponType = attackerWeapon.subtype.subtype --attackerWeapon.subtype.id
 
 if itemTriggers[weaponType] then
  for _,command in pairs(itemTriggers[weaponType]) do
   processTrigger(command,attacker,defender)
  end
 end
 
-- if materialTriggers[attackerWeapon.mat_type] and materialTriggers[attackerWeapon.mat_type][attackerWeapon.mat_index] then
 if materialTriggers[attackerWeapon.mat_index] then
  for _,command in pairs(materialTriggers[attackerWeapon.mat_index]) do
   processTrigger(command,attacker,defender)
  end
 end
 
-- if itemMaterialTriggers[weaponType] and itemMaterialTriggers[weaponType][attackerWeapon.mat_type] and itemMaterialTriggers[weaponType][attackerWeapon.mat_type][attackerWeapon.mat_index] then
--  for _,command in pairs(itemMaterialTriggers[weaponType][attackerWeapon.mat_type][attackerWeapon.mat_index]) do
--   processTrigger(command,attacker,defender)
--  end
-- end
end

local args = {...}

local i = 1
local command
local weaponType
local material
while i <= #args do
 if command then
  table.insert(command, args[i])
  i = i+1
 else
  if args[i] == '-weaponType' then
   weaponType = args[i+1]
   i = i+2
  elseif args[i] == '-material' then
   material = args[i+1]
   i = i+2
  elseif args[i] == '-command' then
   command = {}
   i = i+1
  elseif args[i] == '-clear' then
   itemTriggers = {}
   materialTriggers = {}
   i = i+1
  else
   error('Invalid arguments to attackTrigger.lua.')
  end
 end
end

if weaponType then
 local temp
 for _,itemdef in ipairs(df.global.world.raws.itemdefs.weapons) do
  if itemdef.id == weaponType then
   temp = itemdef.subtype
   break
  end
 end
 if not temp then
  error 'Could not find weapon type.'
 end
 weaponType = temp
end

if material then
 local i = 0
 while true do
  local mat = dfhack.matinfo.decode(0,i)
  if not mat then
   error 'Could not find material.'
  end
  if mat.inorganic.id == material then
   material = mat.index
   break
  end
  i = i+1
 end
end

if material then
 if not materialTriggers[material] then
  materialTriggers[material] = {}
 end
 table.insert(materialTriggers[material],command)
-- table.insert(materialTriggers[material],function() 
--  print(dfhack.run_command(command))
-- end)
elseif weaponType then
 if not itemTriggers[weaponType] then
  itemTriggers[weaponType] = {}
 end
 table.insert(itemTriggers[weaponType],command)
end

