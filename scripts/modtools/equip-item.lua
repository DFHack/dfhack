-- modtools/equip-item.lua
-- equip an item on a unit with a particular body part
--[[=begin

modtools/equip-item
===================
Force a unit to equip an item with a particular body part; useful in
conjunction with the ``create`` scripts above.  See also `forceequip`.

=end]]
local utils = require 'utils'

function equipItem(unit, item, bodyPart, mode)
  --it is assumed that the item is on the ground
  item.flags.on_ground = false
  item.flags.in_inventory = true
  local block = dfhack.maps.getTileBlock(item.pos)
  local occupancy = block.occupancy[item.pos.x%16][item.pos.y%16]
  for k,v in ipairs(block.items) do
    --local blockItem = df.item.find(v)
    if v == item.id then
      block.items:erase(k)
      break
    end
  end
  local foundItem = false
  for k,v in ipairs(block.items) do
    local blockItem = df.item.find(v)
    if blockItem.pos.x == item.pos.x and blockItem.pos.y == item.pos.y then
      foundItem = true
      break
    end
  end
  if not foundItem then
    occupancy.item = false
  end
  
  local inventoryItem = df.unit_inventory_item:new()
  inventoryItem.item = item
  inventoryItem.mode = mode
  inventoryItem.body_part_id = bodyPart
  unit.inventory:insert(#unit.inventory,inventoryItem)
  
end

validArgs = --[[validArgs or--]] utils.invert({
  'help',
  'unit',
  'item',
  'bodyPart',
  'mode'
})

if moduleMode then
  return
end


local args = utils.processArgs({...}, validArgs)

if args.help then
  print(
[[scripts/modtools/equip-item.lua
arguments:
    -help
        print this help message
 ]])
  return
end

local unitId = tonumber(args.unit) or ((args.unit == '\\LAST') and (df.global.unit_next_id-1))
local unit = df.unit.find(unitId)
if not unit then
  error('invalid unit!', args.unit)
end

local itemId = tonumber(args.item) or ((args.item == '\\LAST') and (df.global.item_next_id-1))
local item = df.item.find(itemId)
if not item then
  error('invalid item!', args.item)
end

local bodyPartName = args.bodyPart
local creature_raw = df.global.world.raws.creatures.all[unit.race]
local caste_raw = creature_raw.caste[unit.caste]
local body_info = caste_raw.body_info

local partId
local part
for k,v in ipairs(body_info.body_parts) do
  if v.token == bodyPartName then
    partId = k
    part = v
    break
  end
end

if not part then
  error('invalid body part name: ', bodyPartName)
end

local mode = args.mode
mode = df.unit_inventory_item.T_mode[mode]

equipItem(unit, item, partId, mode)

