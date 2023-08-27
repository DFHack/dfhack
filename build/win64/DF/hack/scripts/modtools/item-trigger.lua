-- trigger commands based on attacks with certain items
--author expwnent
--based on itemsyndrome by Putnam
--equipment modes and combined trigger conditions added by AtomicChicken
local usage = [====[

modtools/item-trigger
=====================
This powerful tool triggers DFHack commands when a unit equips, unequips, or
attacks another unit with specified item types, specified item materials, or
specified item contaminants.

Arguments::

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
            RING
    -onStrike
        trigger the command on appropriate weapon strikes
    -onEquip mode
        trigger the command when someone equips an appropriate item
        Optionally, the equipment mode can be specified
        Possible values for mode:
            Hauled
            Weapon
            Worn
            Piercing
            Flask
            WrappedAround
            StuckIn
            InMouth
            Pet
            SewnInto
            Strapped
        multiple values can be specified simultaneously
        example: -onEquip [ Weapon Worn Hauled ]
    -onUnequip mode
        trigger the command when someone unequips an appropriate item
        see above note regarding 'mode' values
    -material mat
        trigger the commmand on items with the given material
        examples
            INORGANIC:IRON
            CREATURE:DWARF:BRAIN
            PLANT:OAK:WOOD
    -contaminant mat
        trigger the command for items with a given material contaminant
        examples
            INORGANIC:GOLD
            CREATURE:HUMAN:BLOOD
            PLANT:MUSHROOM_HELMET_PLUMP:DRINK
            WATER
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
]====]
local eventful = require 'plugins.eventful'
local utils = require 'utils'

itemTriggers = itemTriggers or {} --as:{_type:table,_array:{_type:table,triggers:{_type:table,itemType:string,material:string,contaminant:string},args:{_type:table,_array:{_type:table,checkAttackEvery:string,checkInventoryEvery:string,command:'string[]',itemType:string,onStrike:__arg,onEquip:__arg,onUnequip:__arg,material:string,contaminant:string}}}}
eventful.enableEvent(eventful.eventType.UNIT_ATTACK,1) -- this event type is cheap, so checking every tick is fine
eventful.enableEvent(eventful.eventType.INVENTORY_CHANGE,5) -- this is expensive, but you might still want to set it lower
eventful.enableEvent(eventful.eventType.UNLOAD,1)

eventful.onUnload.itemTrigger = function()
 itemTriggers = {}
end

function processTrigger(command)
 local command2 = {} --as:string[]
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

function getitemType(item)
 if item:getSubtype() ~= -1 and dfhack.items.getSubtypeDef(item:getType(),item:getSubtype()) then
  return dfhack.items.getSubtypeDef(item:getType(),item:getSubtype()).id
 else
  return df.item_type[item:getType()]
 end
end

function compareInvModes(reqMode,itemMode)
 if reqMode == nil then
  return
 end
 if not tonumber(reqMode) and df.unit_inventory_item.T_mode[itemMode] == tostring(reqMode) then
  return true
 elseif tonumber(reqMode) == itemMode then
  return true
 end
end

function checkMode(triggerArgs,data)
 local data = data --as:{_type:table,mode:string,modeType:number,attacker:df.unit,defender:df.unit,unit:df.unit,item:df.item,itemMat:dfhack.matinfo,contaminantMat:dfhack.matinfo}
 local mode = data.mode
 for _,argArray in ipairs(triggerArgs) do
  local modes = argArray --as:__arg[]
  if modes[tostring(mode)] then
   local modeType = data.modeType
   local reqModeType = modes[tostring(mode)]
   if #reqModeType == 1 then
    if compareInvModes(reqModeType,modeType) or compareInvModes(reqModeType[1],modeType) then
     utils.fillTable(argArray,data)
     processTrigger(argArray)
     utils.unfillTable(argArray,data)
    end
   elseif #reqModeType > 1 then
    for _,r in ipairs(reqModeType) do
     if compareInvModes(r,modeType) then
      utils.fillTable(argArray,data)
      processTrigger(argArray)
      utils.unfillTable(argArray,data)
     end
    end
   else
    utils.fillTable(argArray,data)
    processTrigger(argArray)
    utils.unfillTable(argArray,data)
   end
  end
 end
end

function checkForTrigger(data)
 local itemTypeStr = data.itemType
 local itemMatStr = data.itemMat:getToken()
 local contaminantStr
 if data.contaminantMat then
  contaminantStr = data.contaminantMat:getToken()
 end
 for _,triggerBundle in ipairs(itemTriggers) do
  local count = 0
  local trigger = triggerBundle['triggers']
  local triggerCount = 0
  if trigger['itemType'] then
   triggerCount = triggerCount + 1
  end
  if trigger['material'] then
   triggerCount = triggerCount + 1
  end
  if trigger['contaminant'] then
   triggerCount = triggerCount + 1
  end
  if itemTypeStr and trigger['itemType'] == itemTypeStr then
   count = count+1
  end
  if itemMatStr and trigger['material'] == itemMatStr then
   count = count+1
  end
  if contaminantStr and trigger['contaminant'] == contaminantStr then
   count = count+1
  end
  if count == triggerCount then
   checkMode(triggerBundle['args'],data)
  end
 end
end

function checkForDuplicates(args)
 for k,triggerBundle in ipairs(itemTriggers) do
  local count = 0
  local trigger = triggerBundle['triggers']
  if trigger['itemType'] == args.itemType then
   count = count+1
  end
  if trigger['material'] == args.material then
   count = count+1
  end
  if trigger['contaminant'] == args.contaminant then
   count = count+1
  end
  if count == 3 then--counts nil values too
   return k
  end
 end
end

function handler(data)
 local itemMat = dfhack.matinfo.decode(data.item)
 local itemType = getitemType(data.item)
 data.itemMat = itemMat
 data.itemType = itemType

 if data.item.contaminants and #data.item.contaminants > 0 then --hint:df.item_actual
  for _,contaminant in ipairs(data.item.contaminants or {}) do --hint:df.item_actual
   local contaminantMat = dfhack.matinfo.decode(contaminant.mat_type, contaminant.mat_index)
   data.contaminantMat = contaminantMat
   checkForTrigger(data)
   data.contaminantMat = nil
  end
 else
  checkForTrigger(data)
 end
end

function equipHandler(unit, item, mode, modeType)
 local data = {}
 data.mode = tostring(mode)
 data.modeType = tonumber(modeType)
 data.item = df.item.find(item)
 data.unit = df.unit.find(unit)
 if data.item and data.unit then -- they must both be not nil or errors will occur after this point with instant reactions.
  handler(data)
 end
end

function modeHandler(unit, item, modeOld, modeNew)
 local mode
 local modeType
 if modeOld then
  mode = "onUnequip"
  modeType = modeOld
  equipHandler(unit, item, mode, modeType)
 end
 if modeNew then
  mode = "onEquip"
  modeType = modeNew
  equipHandler(unit, item, mode, modeType)
 end
end

eventful.onInventoryChange.equipmentTrigger = function(unit, item, item_old, item_new)
 local modeOld = (item_old and item_old.mode)
 local modeNew = (item_new and item_new.mode)
 if modeOld ~= modeNew then
  modeHandler(unit,item,modeOld,modeNew)
 end
end

eventful.onUnitAttack.attackTrigger = function(attacker,defender,wound)
 attacker = df.unit.find(attacker) --luacheck: retype
 defender = df.unit.find(defender) --luacheck: retype

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

 local data = {}
 data.attacker = attacker
 data.defender = defender
 data.item = attackerWeapon
 data.mode = 'onStrike'
 handler(data)
end

local validArgs = utils.invert({
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
 print(usage)
 return
end

if args.clear then
 itemTriggers = {}
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

if args.itemType and dfhack.items.findType(args.itemType) == -1 then
 local temp
 for _,itemdef in ipairs(df.global.world.raws.itemdefs.all) do
  if itemdef.id == args.itemType then
   temp = args.itemType--itemdef.subtype
   break
  end
 end
 if not temp then
  error 'Could not find item type.'
 end
 args.itemType = temp
end

local numConditions = (args.material and 1 or 0) + (args.itemType and 1 or 0) + (args.contaminant and 1 or 0)
if numConditions == 0 then
 error 'Specify at least one material, itemType or contaminant.'
end

local index
if #itemTriggers > 0 then
 index = checkForDuplicates(args)
end

if not index then
 index = #itemTriggers+1
 itemTriggers[index] = {}
 local triggerArray = {}
 if args.itemType then
  triggerArray['itemType'] = args.itemType
 end
 if args.material then
  triggerArray['material'] = args.material
 end
 if args.contaminant then
  triggerArray['contaminant'] = args.contaminant
 end
 itemTriggers[index]['triggers'] = triggerArray
end

if not itemTriggers[index]['args'] then
 itemTriggers[index]['args'] = {}
end
local triggerArgs = itemTriggers[index]['args']
args.itemType = nil
args.material = nil
args.contaminant = nil
table.insert(triggerArgs,args)
