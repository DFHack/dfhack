--scripts/modtools/create-item.lua
--author expwnent
--creates an item of a given type and material
--[[=begin

modtools/create-item
====================
Replaces the `createitem` plugin, with standard
arguments. The other versions will be phased out in a later version.

=end]]
local utils = require 'utils'

validArgs = --[[validArgs or--]] utils.invert({
 'help',
 'creator',
 'material',
 'item',
-- 'creature',
-- 'caste',
 'leftHand',
 'rightHand',
 'quality'
})

organicTypes = organicTypes or utils.invert({
 df.item_type.REMAINS,
 df.item_type.FISH,
 df.item_type.FISH_RAW,
 df.item_type.VERMIN,
 df.item_type.PET,
 df.item_type.EGG,
})

badTypes = badTypes or utils.invert({
 df.item_type.CORPSE,
 df.item_type.CORPSEPIECE,
 df.item_type.FOOD,
})

function createItem(creatorID, item, material, leftHand, rightHand, quality)
 local itemQuality = quality and df.item_quality[quality]
 print(itemQuality)
 
 local creator = df.unit.find(creatorID)
 if not creator then
  error 'Invalid creator.'
 end
 
 if not item then
  error 'Invalid item.'
 end
 local itemType = dfhack.items.findType(item)
 if itemType == -1 then
  error 'Invalid item.'
 end
 local itemSubtype = dfhack.items.findSubtype(item)

 if organicTypes[itemType] then
  --TODO: look up creature and caste
  error 'Not yet supported.'
 end

 if badTypes[itemType] then
  error 'Not supported.'
 end

 if not material then
  error 'Invalid material.'
 end
 local materialInfo = dfhack.matinfo.find(material)
 if not materialInfo then
  error 'Invalid material.'
 end

 local item1 = dfhack.items.createItem(itemType, itemSubtype, materialInfo['type'], materialInfo.index, creator)
 local item = df.item.find(item1)
 if leftHand then
  item:setGloveHandedness(2)
 elseif rightHand then
  item:setGloveHandedness(1)
 end
 
 if itemQuality then
  item:setQuality(itemQuality)
 end
 --[[if matchingGloves or matchingShoes then
  if matchingGloves then
   item1 = df.item.find(item1)
   item1:setGloveHandedness(1);
  end
  local item2 = dfhack.items.createItem(itemType, itemSubtype, materialInfo['type'], materialInfo.index, creator)
  if matchingGloves then
   item2 = df.item.find(item2)
   item2:setGloveHandedness(2);
  end
 end --]]
 return item1
end

if moduleMode then
 return
end

local args = utils.processArgs({...}, validArgs)

if args.help then
 print(
[[scripts/modtools/create-item.lua
arguments:
    -help
        print this help message
    -creator id
        specify the id of the unit who will create the item, or \\LAST to indicate the unit with id df.global.unit_next_id-1
        examples:
            0
            2
            \\LAST
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

if args.creator == '\\LAST' then
  args.creator = tostring(df.global.unit_next_id-1)
end

createItem(tonumber(args.creator), args.item, args.material, args.leftHand, args.rightHand, args.quality)
