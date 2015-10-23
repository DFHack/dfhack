--scripts/modtools/invader-item-destroyer.lua
--author expwnent
--configurably deletes invader items when they die
--[[=begin

modtools/invader-item-destroyer
===============================
This tool configurably destroys invader items to prevent clutter or to prevent
the player from getting tools exclusive to certain races.

=end]]
local eventful = require 'plugins.eventful'
local utils = require 'utils'

--invaders = invaders or {}
entities = entities or {}
items = items or {}

allEntities = allEntities or false
allItems = allitems or true

eventful.enableEvent(eventful.eventType.UNLOAD,1)
eventful.onUnload.invaderItemDestroyer = function()
 entities = {}
 items = {}
 allEntities = false
 allItems = true
end

eventful.enableEvent(eventful.eventType.UNIT_DEATH, 1) --requires iterating through all units
eventful.onUnitDeath.invaderItemDestroyer = function(unitId)
 local unit = df.unit.find(unitId)
 if not unit then
  return
 end

 local entity = df.historical_entity.find(unit.civ_id)
 if not allEntities and not entity then
  return
 end

 if not allEntities and not entities[entity.entity_raw.code] then
  return
 end

 if dfhack.units.isCitizen(unit) then
  return
 end

 local function forEach(item)
  if not allItems and not items[dfhack.items.getSubtypeDef(item:getType(),item:getSubtype()).id] then
   return
  end
  if not (item.flags.foreign and item.flags.forbid) then
   return
  end
  if item.pos.x ~= unit.pos.x then
   return
  end
  if item.pos.y ~= unit.pos.y then
   return
  end
  if item.pos.z ~= unit.pos.z then
   return
  end
  item.flags.garbage_collect = true
  item.flags.forbid = true
  item.flags.hidden = true
 end

 for _,item in ipairs(unit.inventory) do
  local item2 = df.item.find(item.item)
  forEach(item2)
 end
 --for each item on the ground
 local block = dfhack.maps.getTileBlock(unit.pos.x, unit.pos.y, unit.pos.z)
 for _,item in ipairs(block.items) do
  local item2 = df.item.find(item)
  forEach(item2)
 end
end
--[[eventful.onUnitDeath.invaderItemDestroyer = function(unit)
 if invaders[unit] then
  print ('Invader ' .. unit .. ' dies.')
 end
 for _,item in ipairs(invaders[unit] or {}) do
  local item2 = df.item.find(item)
  if item2 then
   print ('deleting item ' .. item)
   item2.flags.garbage_collect = true
   item2.flags.forbid = true
   item2.flags.hidden = true
   item2.flags.encased = true
  end
 end
 invaders[unit] = nil
 --TODO: delete corpses?
end]]

validArgs = validArgs or utils.invert({
 'clear',
 'help',
 'allRaces',
 'allEntities',
 'allItems',
 'item',
 'entity',
 'race',
})

local args = utils.processArgs({...}, validArgs)

if args.clear then
 entities = {}
 items = {}
 allEntities = false
 allItems = true
end

if args.help then
 print([[scripts/modtools/invader-item-destroyer.lua usage
arguments:
    -help
        print this help message
    -clear
        reset all registered data
    -allEntities [true/false]
        set whether it should delete items from invaders from any civ
    -allItems [true/false]
        set whether it should delete all invader items regardless of type when an appropriate invader dies
    -item itemdef
        set a particular itemdef to be destroyed when an invader from an appropriate civ dies
        examples:
            ITEM_WEAPON_PICK
    -entity entityName
        set a particular entity up so that its invaders destroy their items shortly after death
        examples:
            MOUNTAIN
            EVIL
]])
 return
end

if args.allEntities then
 if args.allEntities == 'true' then
  allEntities = true
 else
  allEntities = false
 end
end
if args.allItems then
 if args.allItems == 'true' then
  allItems = true
 else
  allItems = false
 end
end

if args.item then
 local itemType
 for _,itemdef in ipairs(df.global.world.raws.itemdefs.all) do
  if itemdef.id == args.item then
   itemType = itemdef.id
   break
  end
 end
 if not itemType then
  error ('Invalid item type: ' .. args.item)
 end
 items[itemType] = true
end
if args.entity then
 local success
 for _,entity in ipairs(df.global.world.entities.all) do
  if entity.entity_raw.code == args.entity then
   success = true
   break
  end
 end
 if not success then
  error 'Invalid entity'
 end
 entities[args.entity] = true
end

