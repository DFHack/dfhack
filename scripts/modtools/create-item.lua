--create-item.lua
--author expwnent
--creates an item of a given type and material

local utils = require 'utils'

validArgs = --[[validArgs or]] utils.invert({
 'help',
 'creator',
 'material',
 'item',
 'creature',
 'caste',
 'matchingGloves',
 'matchingShoes'
})

local args = utils.processArgs({...}, validArgs)

if args.help then
 --TODO: help string
 return
end

if not args.creator or not tonumber(args.creator) or not df.unit.find(tonumber(args.creator)) then
 error 'Invalid creator.'
end
args.creator = df.unit.find(tonumber(args.creator))
if not args.creator then
 error 'Invalid creator.'
end

if not args.item then
 error 'Invalid item.'
end
local itemType = dfhack.items.findType(args.item)
local itemSubtype = dfhack.items.findSubtype(args.item)
if itemType == -1 then
 error 'Invalid item.'
end

organicTypes = organicTypes or utils.invert({
 df.item_type.REMAINS,
 df.item_type.FISH,
 df.item_type.FISH_RAW,
 df.item_type.VERMIN,
 df.item_type.PET,
 df.item_type.EGG,
})

if organicTypes[itemType] then
 --TODO: look up creature and caste
 error 'Not yet supported.'
end

badTypes = badTypes or utils.invert({
 df.item_type.CORPSE,
 df.item_type.CORPSEPIECE,
 df.item_type.FOOD,
})

if badTypes[itemType] then
 error 'Not supported.'
end

if not args.material then
 error 'Invalid material.'
end
args.material = dfhack.matinfo.find(args.material)
if not args.material then
 error 'Invalid material.'
end

local item1 = dfhack.items.createItem(itemType, itemSubtype, args.material['type'], args.material.index, args.creator)
if args.matchingGloves or args.matchingShoes then
 if args.matchingGloves then
  item1 = df.item.find(item1)
  item1:setGloveHandedness(1);
 end
 local item2 = dfhack.items.createItem(itemType, itemSubtype, args.material['type'], args.material.index, args.creator)
 if args.matchingGloves then
  item2 = df.item.find(item2)
  item2:setGloveHandedness(2);
 end
end

