-- Script to add unknown crafting recipes to the player's civ.
--[====[
add-recipe
==========
Adds unknown weapon and armor crafting recipes to your civ.
E.g. some civilizations never learn to craft high boots. This script can
help with that, and more. Only weapons, armor, and tools are currently supported;
things such as instruments are not. Available options:

* ``add-recipe all`` adds *all* available weapons and armor, including exotic items
  like blowguns, two-handed swords, and capes.

* ``add-recipe native`` adds only native (but unknown) crafting recipes. Civilizations
  pick randomly from a pool of possible recipes, which means not all civs get
  high boots, for instance. This command gives you all the recipes your
  civilisation could have gotten.

* ``add-recipe single <item token>`` adds a single item by the given
  item token. For example::

    add-recipe single SHOES:ITEM_SHOES_BOOTS
]====]

local itemDefs = df.global.world.raws.itemdefs
local resources = df.historical_entity.find(df.global.plotinfo.civ_id).resources
local civ = df.historical_entity.find(df.global.plotinfo.civ_id).entity_raw

if (resources == nil) then
  qerror("Could not find entity resources")
elseif (itemDefs == nil) then
  qerror("Could not find item raws!")
elseif (civ == nil) then
  qerror("Could not find civilization type!")
end

--array that categorises the various containers we need to access per type
--each type is represented by an array with the following elements:
  --1: the currently known recipes to the player civ
  --2: the native (non-exotic) recipes to the player civ
  --3: all possible recipes including exotic
local categories = { --as:{1:'number[]',2:'number[]',3:'df.itemdef[]'}[]
  weapon = {resources.weapon_type, civ.equipment.weapon_id, itemDefs.weapons },
  shield = {resources.shield_type, civ.equipment.shield_id, itemDefs.shields },
  ammo = {resources.ammo_type, civ.equipment.ammo_id, itemDefs.ammo },
  armor = {resources.armor_type, civ.equipment.armor_id, itemDefs.armor },
  gloves = {resources.gloves_type, civ.equipment.gloves_id, itemDefs.gloves },
  shoes = {resources.shoes_type, civ.equipment.shoes_id, itemDefs.shoes },
  helm = {resources.helm_type, civ.equipment.helm_id, itemDefs.helms },
  pants = {resources.pants_type, civ.equipment.pants_id, itemDefs.pants },
  tool = {resources.tool_type, civ.equipment.tool_id, itemDefs.tools}
}

local diggers = resources.digger_type

function checkKnown(known, itemType)
  --checks if the item with the given subtype is already known to the civ
  --params:
    --known: the resources field for the civ
    --itemType: the item subtype we're checking
  for _, id in ipairs(known) do
    if id == itemType then
      return true
    end
  end
  return false
end

function checkNative(native, itemType)
  --checks if the given itemtype is native to the player's civ
  --params:
    --native: the list of native item types
    --itemType: the item subtype we're checking
  for _, id in ipairs(native) do
    if id == itemType then
      return true
    end
  end
  return false
end

function addItems(category, exotic)
  --makes all available recipes of the given category known to the player's civ
  --params:
    --category: the category of items we're adding
    --exotic: whether to add exotic items
  --returns: list of item objects that were added
  local known = category[1]
  local native = category[2]
  local all = category[3]
  local added = {} --as:df.itemdef[]

  for _, item in ipairs(all) do
    local item = item --as:df.itemdef_weaponst
    local subtype = item.subtype
    local itemOk = false

    --check if it's a training weapon
    local t1, t2 = pcall(function () return item.flags.TRAINING == false end)
    local training = not(not t1 or t2)

    --we don't want procedural items with adjectives such as "wavy spears"
    --(because they don't seem to be craftable even if added)
    --nor do we want known items or training items (because adding training
    --items seems to allow them to be made out of metals)
    if (item.adjective == "" and not training and not checkKnown(known, subtype)) then
      itemOk = true
    end

    if (not exotic and not checkNative(native, subtype)) then
      itemOk = false
    end

    --check that the weapon we're adding is not already known to the civ as
    --a digging implement so picks don't get duplicated
    if (checkKnown(diggers, subtype)) then
      itemOk = false
    end

    if (itemOk) then
      known:insert('#', subtype)
      table.insert(added, item)
    end
  end

  return added
end


function printItems(itemList)
  for _, v in ipairs(itemList) do
    local v = v --as:df.itemdef_weaponst
    print("Added recipe " .. v.id .. " (" .. v.name .. ")")
  end
end

function addAllItems(exotic)
  printItems(addItems(categories.weapon, exotic))
  printItems(addItems(categories.shield, exotic))
  printItems(addItems(categories.ammo, exotic))
  printItems(addItems(categories.armor, exotic))
  printItems(addItems(categories.gloves, exotic))
  printItems(addItems(categories.shoes, exotic))
  printItems(addItems(categories.helm, exotic))
  printItems(addItems(categories.pants, exotic))
  printItems(addItems(categories.tool, exotic))
end

function addSingleItem(itemstring)
  local itemType = nil  --The category of the item, the word before the ":"
  local itemId = nil    --The item within that category, what goes after the ":"

  if (itemstring ~= nil) then
    itemType, itemId = string.match(itemstring, "(.*):(.*)")
  else
    print("Usage: add-recipe single <item name> | Example: add-recipe single SHOES:ITEM_SHOES_BOOTS")
    return
  end
  if (itemType == nil or itemId == nil) then
    print("Usage: add-recipe single <item name> | Example: add-recipe single SHOES:ITEM_SHOES_BOOTS")
    return
  end
  local category = categories[string.lower(itemType)]
  if (category == nil) then return end
  local known = category[1]
  local all = category[3]

  local addedItem = nil
  --assume the user knows what they're doing, so no need for sanity checks
  for _, item in ipairs(all) do
    if (item.id == itemId) then
      known:insert('#', item.subtype)
      addedItem = item
      break
    end
  end

  if (addedItem ~= nil) then
    local addedItem = addedItem --as:df.itemdef_weaponst
    print("Added recipe " .. addedItem.id .. " (" .. addedItem.name .. ")")
  else
    print("Could not add recipe: invalid item name")
  end
end

local args = {...}
local cmd = args[1]

if (cmd == "all") then
  addAllItems(true)
elseif (cmd == "native") then
  addAllItems(false)
elseif (cmd == "single") then
  addSingleItem(args[2])
else
  print("Available options:\n"
        .."all:    adds all supported crafting recipes.\n"
        .."native: adds only unknown native recipes (eg. high boots for "
        .."some dwarves)\n"
        .."single: adds a specific item by itemstring (eg. "
        .."SHOES:ITEM_SHOES_BOOTS)")
end
