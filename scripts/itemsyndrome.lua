-- itemsyndrome.lua
-- author: Putnam
-- edited by expwnent
-- Checks for inventory changes and applies or removes syndromes that items or their materials have. Use "disable" (minus quotes) to disable and "help" to get help.

eventful=eventful or require("plugins.eventful")
syndromeUtil = syndromeUtil or require("syndromeUtil")

local function printItemSyndromeHelp()
 print("Arguments (non case-sensitive):")
 print('    "help": displays this dialogue.')
 print(" ")
 print('    "disable": disables the script.')
 print(" ")
 print('    "debugon/debugoff": debug mode.')
 print(" ")
 print('    "contaminantson/contaminantsoff": toggles searching for contaminants.')
 print('    Disabling speeds itemsyndrome up greatly.')
 print('    "transformReEquipOn/TransformReEquipOff": toggles transformation auto-reequip.')
end

itemsyndromedebug=false

local args = {...}
for k,v in ipairs(args) do
 v=v:lower()
 if v == "help" then printItemSyndromeHelp() return end
 if v == "debugon" then itemsyndromedebug = true end
 if v == "debugoff" then itemsyndromedebug = false end
 if v == "contaminantson" then itemsyndromecontaminants = true end
 if v == "contaminantsoff" then itemsyndromecontaminants = false end
 if v == "transformreequipon" then transformationReEquip = true end
 if v == "transformreequipoff" then transformationReEquip = false end
end

local function getMaterial(item)
 --What does this line mean?
 local material = dfhack.matinfo.decode(item) and dfhack.matinfo.decode(item) or false
 if not material then return nil end
 if material.mode ~= "inorganic"  then
  return nil
 else
  return material.material --the "material" thing up there contains a bit more info which is all pretty important but impertinent, like the creature that the material comes from
 end
end

local function findItemSyndromeInorganic()
 local allInorganics = {}
 for matID,material in ipairs(df.global.world.raws.inorganics) do
  if string.find(material.id,"DFHACK_ITEMSYNDROME_MATERIAL_") then table.insert(allInorganics,matID) end --the last underscore is needed to prevent duped raws; I want good modder courtesy if it kills me, dammit!
 end
 if itemsyndromedebug then printall(allInorganics) end
 if #allInorganics>0 then return allInorganics else return nil end
end

local function getAllItemSyndromeMats(itemSyndromeMatIDs)
 local allActualInorganics = {}
 for _,itemSyndromeMatID in ipairs(itemSyndromeMatIDs) do
  table.insert(allActualInorganics,df.global.world.raws.inorganics[itemSyndromeMatID].material)
 end
 if itemsyndromedebug then printall(allActualInorganics) end
 return allActualInorganics
end

local function syndromeIsDfHackSyndrome(syndrome)
 for k,v in ipairs(syndrome.syn_class) do
  if v.value=="DFHACK_ITEM_SYNDROME" then 
   if itemsyndromedebug then print('Syndrome is DFHack syndrome, checking if creature is affected...') end
   return true 
  end
 end
 if itemsyndromedebug then print('Syndrome is not DFHack syndrome. Cancelling.') end
 return false
end

local function itemHasNoSubtype(item)
 return item:getSubtype()==-1
end

local function itemHasSyndrome(item)
 if itemHasNoSubtype(item) or not itemSyndromeMats then return nil end
 local allItemSyndromes={}
 for _,material in ipairs(itemSyndromeMats) do
  for __,syndrome in ipairs(material.syndrome) do
   if syndrome.syn_name == item.subtype.name then table.insert(allItemSyndromes,syndrome) end
  end
 end
 return #allItemSyndromes>0 and allItemSyndromes or false
end

local function getValidPositions(syndrome)
 local returnTable={AffectsHauler=false,AffectsStuckins=false,IsArmorOnly=false,IsWieldedOnly=false,OnlyAffectsStuckins=false}
 for k,v in ipairs(syndrome.syn_class) do
  if v.value:find('DFHACK') then
   if v.value=="DFHACK_AFFECTS_HAULER" then returnTable.AffectsHauler=true end
   if v.value=="DFHACK_AFFECTS_STUCKIN" then returnTable.AffectsStuckins=true end
   if v.value=="DFHACK_STUCKINS_ONLY" then returnTable.OnlyAffectsStuckins=true end    
   if v.value=="DFHACK_WIELDED_ONLY" then returnTable.IsWieldedOnly=true end
   if v.value=="DFHACK_ARMOR_ONLY" then returnTable.IsArmorOnly=true end
  end
 end
 return returnTable
end

local function itemIsInValidPosition(item_inv, syndrome)
 local item = getValidPositions(syndrome)
 if not item_inv then print("You shouldn't see this error! At all! Putnam f'd up! Tell him off!") return false end
 local isInValidPosition=not ((item_inv.mode == 0 and not item.AffectsHauler) or (item_inv.mode == 7 and not item.AffectsStuckins) or (item_inv.mode ~= 2 and item.IsArmorOnly) or (item_inv.mode ~=1 and item.IsWieldedOnly) or (item_inv.mode ~=7 and item.OnlyAffectsStuckins))
 if itemsyndromedebug then print(isInValidPosition and 'Item is in correct position.' or 'Item is not in correct position.') end
 return isInValidPosition
end

local function syndromeIsTransformation(syndrome)
 for _,effect in ipairs(syndrome.ce) do
  if df.creature_interaction_effect_body_transformationst:is_instance(effect) then return true end
 end
 return false
end

local function rememberInventory(unit)
 local invCopy = {}
 for inv_id,item_inv in ipairs(unit.inventory) do
  invCopy[inv_id+1] = {}
  local itemToWorkOn = invCopy[inv_id+1]
  itemToWorkOn.item = item_inv.item
  itemToWorkOn.mode = item_inv.mode
  itemToWorkOn.body_part_id = item_inv.body_part_id
 end
 return invCopy
end

local function moveAllToInventory(unit,invTable)
 for _,item_inv in ipairs(invTable) do
  dfhack.items.moveToInventory(item_inv.item,unit,item_inv.mode,item_inv.body_part_id)
 end
end

local function syndromeIsOnUnequip(syndrome)
 for k,v in ipairs(syndrome.syn_class) do
  if v.value:upper()=='DFHACK_ON_UNEQUIP' then return true end
 end
 return false
end

local function addOrRemoveSyndromeDepending(unit,old_equip,new_equip,syndrome)
 local item_inv=new_equip or old_equip
 if not syndromeIsDfHackSyndrome(syndrome) then
  return
 end
 local equippedOld = itemIsInValidPosition(old_equip,syndrome)
 local equippedNew = itemIsInValidPosition(new_equip,syndrome)
 if equippedOld == equippedNew then
  return
 end
 local isOnEquip = not syndromeIsOnUnequip(syndrome)
 local apply = (isOnEquip and equippedNew) or (not isOnEquip and not equippedNew)
 if apply then
  syndromeUtil.infectWithSyndrome(unit,syndrome,syndromeUtil.ResetPolicy.ResetDuration)
 else
  syndromeUtil.eraseSyndrome(unit,syndrome)
 end
end

eventful.enableEvent(eventful.eventType.INVENTORY_CHANGE,5)

eventful.onInventoryChange.itemsyndrome=function(unit_id,item_id,old_equip,new_equip)
 local item = df.item.find(item_id)
 --if not item then return false end
 if not item then
  return
 end
 local unit = df.unit.find(unit_id)
 if unit.flags1.dead then return false end
 if itemsyndromedebug then print("Checking unit #" .. unit_id) end
 local transformation = false
 if itemsyndromedebug then print("checking item #" .. item_id .." on unit #" .. unit_id) end
 local itemMaterial=getMaterial(item)
 local function manageSyndromes(syndromes)
  for k,syndrome in ipairs(syndromes) do
   if itemsyndromedebug then print("item has a syndrome, checking if syndrome is valid for application...") end
   if syndromeIsTransformation(syndrome) then
    --unitInventory = rememberInventory(unit)
    rememberInventory(unit)
    transformation = true
   end
   addOrRemoveSyndromeDepending(unit,old_equip,new_equip,syndrome)
  end
 end
 if itemMaterial then
  manageSyndromes(itemMaterial.syndrome)
 end
 local itemSyndromes = itemHasSyndrome(item)
 if itemSyndromes then
  if itemsyndromedebug then print("Item itself has a syndrome, checking if item is in correct position and creature is affected") end
  manageSyndromes(itemSyndromes)
 end
 if itemsyndromecontaminants and item.contaminants then
  if itemsyndromedebug then print("Item has contaminants. Checking for syndromes...") end
  for _,contaminant in ipairs(item.contaminants) do
   local contaminantMaterial=getMaterial(contaminant)
   if contaminantMaterial then
    manageSyndromes(contaminantMaterial.syndrome)
   end
  end
 end
 if transformation and transformationReEquip then dfhack.timeout(2,"ticks",function() moveAllToInventory(unit,unitInventory) end) end
end

dfhack.onStateChange.itemsyndrome=function(code)
 if code==SC_WORLD_LOADED then
  itemSyndromeMatIDs = findItemSyndromeInorganic()
  if itemSyndromeMatIDs then itemSyndromeMats = getAllItemSyndromeMats(itemSyndromeMatIDs) end
 end
end

if disable then
 eventful.onInventoryChange.itemsyndrome=nil
 print("Disabled itemsyndrome.")
 disable = false
else
 print("Enabled itemsyndrome.")
end

