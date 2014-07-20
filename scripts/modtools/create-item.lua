--scripts/modtools/create-item.lua
--author expwnent
--creates an item of a given type and material

local utils = require 'utils'

validArgs = validArgs or utils.invert({
 'help',
 'creator',
 'material',
 'item',
-- 'creature',
-- 'caste',
 'matchingGloves',
 'matchingShoes'
})

local args = utils.processArgs({...}, validArgs)

if args.help then
 print(
[[scripts/modtools/create-item.lua
arguments:
    -help
        print this help message
    -creator id
        specify the id of the unit who will create the item
        examples:
            0
            2
    -material matstring
        specify the material of the item to be created
        examples:
            INORGANIC:IRON
            CREATURE_MAT:DWARF:BRAIN
            PLANT_MAT:MUSHROOM_HELMET_PLUMP:DRINK
    -item itemstr
        specify the itemdef of the item to be created
        examples:
            WEAPON:ITEM_WEAPON_PICK
    -matchingShoes
        create two of this item
    -matchingGloves
        create two of this item, and set handedness appropriately
 ]])
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
if itemType == -1 then
 error 'Invalid item.'
end
local itemSubtype = dfhack.items.findSubtype(args.item)

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

