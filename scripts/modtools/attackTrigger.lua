--attackTrigger.lua
--author expwnent
--triggers scripts when a unit attacks another with a weapon type, a weapon of a particular material

local eventful = require 'plugins.eventful'
local utils = require 'utils'
eventful.enableEvent(eventful.eventType.UNIT_ATTACK,1)

itemTriggers = itemTriggers or {}
materialTriggers = materialTriggers or {}
--itemMaterialTriggers = itemMaterialTriggers or {}

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

local args = utils.processArgs(...)

if args.clear then
 itemTriggers = {}
 materialTriggers = {}
end

if not args.command then
 if not args.clear then
  error 'specify a command'
 end
 return
end

if args.weaponType then
 local temp
 for _,itemdef in ipairs(df.global.world.raws.itemdefs.weapons) do
  if itemdef.id == args.weaponType then
   temp = itemdef.subtype
   break
  end
 end
 if not temp then
  error 'Could not find weapon type.'
 end
 args.weaponType = temp
end

if args.material then
 local i = 0
 while true do
  local mat = dfhack.matinfo.decode(0,i)
  if not mat then
   error('Could not find material "' .. args.material .. '" after '..i..' materials.')
  end
  if mat.inorganic.id == args.material then
   args.material = mat.index
   break
  end
  i = i+1
 end
end

if args.material and args.weaponType then
 error 'both material and weaponType defined'
end

if args.material then
 if not materialTriggers[args.material] then
  materialTriggers[args.material] = {}
 end
 table.insert(materialTriggers[args.material],args.command)
elseif args.weaponType then
 if not itemTriggers[args.weaponType] then
  itemTriggers[args.weaponType] = {}
 end
 table.insert(itemTriggers[args.weaponType],args.command)
end

