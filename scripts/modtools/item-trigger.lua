--scripts/modtools/attack-trigger.lua
--author expwnent
--based on itemsyndrome by Putnam
--triggers scripts when a unit attacks another with a weapon type, a weapon of a particular material, or a weapon contaminated with a particular material, or when a unit equips/unequips a particular item type, an item of a particular material, or an item contaminated with a particular material
--[[=begin

modtools/item-trigger
=====================
This powerful tool triggers DFHack commands when a unit equips, unequips, or
attacks another unit with specified item types, specified item materials, or
specified item contaminants.

=end]]
local eventful = require 'plugins.eventful'
local utils = require 'utils'

itemTriggers = itemTriggers or {}
materialTriggers = materialTriggers or {}
contaminantTriggers = contaminantTriggers or {}

eventful.enableEvent(eventful.eventType.UNIT_ATTACK,1) -- this event type is cheap, so checking every tick is fine
eventful.enableEvent(eventful.eventType.INVENTORY_CHANGE,5) --this is expensive, but you might still want to set it lower
eventful.enableEvent(eventful.eventType.UNLOAD,1)

eventful.onUnload.itemTrigger = function()
 itemTriggers = {}
 materialTriggers = {}
 contaminantTriggers = {}
end

function processTrigger(command)
 local command2 = {}
 for i,arg in ipairs(command.command) do
  if arg == '\\ATTACKER_ID' then
   command2[i] = '' .. command.attacker.id
  elseif arg == '\\DEFENDER_ID' then
   command2[i] = '' .. command.defender.id
  elseif arg == '\\ITEM_MATERIAL' then
   command2[i] = command.itemMat:getToken()
  elseif arg == '\\ITEM_MATERIAL_TYPE' then
   command2[i] = command.itemMat['type']
  elseif arg == '\\ITEM_MATERIAL_INDEX' then
   command2[i] = command.itemMat.index
  elseif arg == '\\ITEM_ID' then
   command2[i] = '' .. command.item.id
  elseif arg == '\\ITEM_TYPE' then
   command2[i] = command.itemType
  elseif arg == '\\CONTAMINANT_MATERIAL' then
   command2[i] = command.contaminantMat:getToken()
  elseif arg == '\\CONTAMINANT_MATERIAL_TYPE' then
   command2[i] = command.contaminantMat['type']
  elseif arg == '\\CONTAMINANT_MATERIAL_INDEX' then
   command2[i] = command.contaminantMat.index
  elseif arg == '\\MODE' then
   command2[i] = command.mode
  elseif arg == '\\UNIT_ID' then
   command2[i] = command.unit.id
  elseif string.sub(arg,1,1) == '\\' then
   command2[i] = string.sub(arg,2)
  else
   command2[i] = arg
  end
 end
 dfhack.run_command(table.unpack(command2))
end

function handler(table)
 local itemMat = dfhack.matinfo.decode(table.item)
 local itemMatStr = itemMat:getToken()
 local itemType = dfhack.items.getSubtypeDef(table.item:getType(),table.item:getSubtype()).id
 table.itemMat = itemMat
 table.itemType = itemType

 for _,command in ipairs(itemTriggers[itemType] or {}) do
  if command[table.mode] then
   utils.fillTable(command,table)
   processTrigger(command)
   utils.unfillTable(command,table)
  end
 end

 for _,command in ipairs(materialTriggers[itemMatStr] or {}) do
  if command[table.mode] then
   utils.fillTable(command,table)
   processTrigger(command)
   utils.unfillTable(command,table)
  end
 end

 for _,contaminant in ipairs(table.item.contaminants or {}) do
  local contaminantMat = dfhack.matinfo.decode(contaminant.mat_type, contaminant.mat_index)
  local contaminantStr = contaminantMat:getToken()
  table.contaminantMat = contaminantMat
  for _,command in ipairs(contaminantTriggers[contaminantStr] or {}) do
   utils.fillTable(command,table)
   processTrigger(command)
   utils.unfillTable(command,table)
  end
  table.contaminantMat = nil
 end
end

function equipHandler(unit, item, isEquip)
 local mode = (isEquip and 'onEquip') or (not isEquip and 'onUnequip')

 local table = {}
 table.mode = mode
 table.item = df.item.find(item)
 table.unit = df.unit.find(unit)
 handler(table)
end

eventful.onInventoryChange.equipmentTrigger = function(unit, item, item_old, item_new)
 if item_old and item_new then
  return
 end

 local isEquip = item_new and not item_old
 equipHandler(unit,item,isEquip)
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

 local table = {}
 table.attacker = attacker
 table.defender = defender
 table.item = attackerWeapon
 table.mode = 'onStrike'
 handler(table)
end

validArgs = validArgs or utils.invert({
 'clear',
 'help',
 'checkAttackEvery',
 'checkInventoryEvery',
 'command',
 'itemType',
 'onStrike',
 'onEquip',
 'onUnequip',
 'material',
 'contaminant',
})
local args = utils.processArgs({...}, validArgs)

if args.help then
 print([[scripts/modtools/item-trigger.lua usage
arguments:
    -help
        print this help message
    -clear
        clear all registered triggers
    -checkAttackEvery n
        check the attack event at least every n ticks
    -checkInventoryEvery n
        check inventory event at least every n ticks
    -itemType type
        trigger the command for items of this type
        examples:
            ITEM_WEAPON_PICK
    -onStrike
        trigger the command when someone strikes someone with an appropriate weapon
    -onEquip
        trigger the command when someone equips an appropriate item
    -onUnequip
        trigger the command when someone unequips an appropriate item
    -material mat
        trigger the commmand on items with the given material
        examples
            INORGANIC:IRON
            CREATURE_MAT:DWARF:BRAIN
            PLANT_MAT:MUSHROOM_HELMET_PLUMP:DRINK
    -contaminant mat
        trigger the command on items with a given material contaminant
        examples
            INORGANIC:IRON
            CREATURE_MAT:DWARF:BRAIN
            PLANT_MAT:MUSHROOM_HELMET_PLUMP:DRINK
    -command [ commandStrs ]
        specify the command to be executed
        commandStrs
            \\ATTACKER_ID
            \\DEFENDER_ID
            \\ITEM_MATERIAL
            \\ITEM_MATERIAL_TYPE
            \\ITEM_ID
            \\ITEM_TYPE
            \\CONTAMINANT_MATERIAL
            \\CONTAMINANT_MATERIAL_TYPE
            \\CONTAMINANT_MATERIAL_INDEX
            \\MODE
            \\UNIT_ID
            \\anything -> \anything
            anything -> anything
]])
 return
end

if args.clear then
 itemTriggers = {}
 materialTriggers = {}
 contaminantTriggers = {}
end

if args.checkAttackEvery then
 if not tonumber(args.checkAttackEvery) then
  error('checkAttackEvery must be a number')
 end
 eventful.enableEvent(eventful.eventType.UNIT_ATTACK,tonumber(args.checkAttackEvery))
end

if args.checkInventoryEvery then
 if not tonumber(args.checkInventoryEvery) then
  error('checkInventoryEvery must be a number')
 end
 eventful.enableEvent(eventful.eventType.INVENTORY_CHANGE,tonumber(args.checkInventoryEvery))
end

if not args.command then
 if not args.clear then
  error 'specify a command'
 end
 return
end

if args.itemType then
 local temp
 for _,itemdef in ipairs(df.global.world.raws.itemdefs.all) do
  if itemdef.id == args.itemType then
   temp = args.itemType --itemdef.subtype
   break
  end
 end
 if not temp then
  error 'Could not find item type.'
 end
 args.itemType = temp
end

local numConditions = (args.material and 1 or 0) + (args.itemType and 1 or 0) + (args.contaminant and 1 or 0)
if numConditions > 1 then
 error 'too many conditions defined: not (yet) supported (pester expwnent if you want it)'
elseif numConditions == 0 then
 error 'specify a material, weaponType, or contaminant'
end

if args.material then
 if not materialTriggers[args.material] then
  materialTriggers[args.material] = {}
 end
 table.insert(materialTriggers[args.material],args)
elseif args.itemType then
 if not itemTriggers[args.itemType] then
  itemTriggers[args.itemType] = {}
 end
 table.insert(itemTriggers[args.itemType],args)
elseif args.contaminant then
 if not contaminantTriggers[args.contaminant] then
  contaminantTriggers[args.contaminant] = {}
 end
 table.insert(contaminantTriggers[args.contaminant],args)
end

