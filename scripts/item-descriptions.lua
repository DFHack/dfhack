-- Holds custom descriptions for view-item-info
-- By PeridexisErrant

print("This is a content library; calling it does nothing.")

--[[
This script has a single function: to return a custom description for every
vanilla item in the game.

Having this as a separate script to "view-item-info.lua" allows either mods
to partially or fully replace the content.

If "raw/scripts/item-descriptions.lua" exists, it will entirely replace this one.
Instead, use "raw/scripts/more-item-descriptions.lua" to add content, or replace
descriptions on a case-by-case basis.  If an item description cannot be found in
the latter script, view-item-info will fall back to the former.

Lines should be about 70 characters for consistent presentation and to fit
on-screen.  Logical sections can be separated by an empty line ("").

All vanilla item IDs:

"AMULET", "ANIMALTRAP", "ANVIL", "ARMORSTAND", "BACKPACK", "BALLISTAARROWHEAD", "BALLISTAPARTS", "BAR", "BARREL", "BED", "BIN", "BLOCKS", "BOOK", "BOULDER", "BOX", "BRACELET", "BUCKET", "CABINET", "CAGE", "CATAPULTPARTS", "CHAIN", "CHAIR", "CLOTH", "COFFIN", "COIN", "COMMIT_EDI", "CROWN", "CRUTCH", "DOOR", "EARRING", "FETCH_", "FIGURINE", "FLASK", "FLOODGATE", "GOBLET", "GRATE", "HATCH_COVER", "ITEM_AMMO_ARROWS", "ITEM_AMMO_BOLTS", "ITEM_ARMOR_BREASTPLATE", "ITEM_ARMOR_CAPE", "ITEM_ARMOR_CLOAK", "ITEM_ARMOR_COAT", "ITEM_ARMOR_DRESS", "ITEM_ARMOR_LEATHER", "ITEM_ARMOR_MAIL_SHIRT", "ITEM_ARMOR_ROBE", "ITEM_ARMOR_SHIRT", "ITEM_ARMOR_TOGA", "ITEM_ARMOR_TUNIC", "ITEM_ARMOR_VEST", "ITEM_FOOD_BISCUITS", "ITEM_FOOD_ROAST", "ITEM_FOOD_STEW", "ITEM_GLOVES_GAUNTLETS", "ITEM_GLOVES_GLOVES", "ITEM_GLOVES_MITTENS", "ITEM_HELM_CAP", "ITEM_HELM_HELM", "ITEM_HELM_HOOD", "ITEM_HELM_MASK", "ITEM_HELM_SCARF_HEAD", "ITEM_HELM_TURBAN", "ITEM_HELM_VEIL_FACE", "ITEM_HELM_VEIL_HEAD", "ITEM_INSTRUMENT_DRUM", "ITEM_INSTRUMENT_FLUTE", "ITEM_INSTRUMENT_HARP", "ITEM_INSTRUMENT_PICCOLO", "ITEM_INSTRUMENT_TRUMPET", "ITEM_PANTS_BRAIES", "ITEM_PANTS_GREAVES", "ITEM_PANTS_LEGGINGS", "ITEM_PANTS_LOINCLOTH", "ITEM_PANTS_PANTS", "ITEM_PANTS_SKIRT", "ITEM_PANTS_SKIRT_LONG", "ITEM_PANTS_SKIRT_SHORT", "ITEM_PANTS_THONG", "ITEM_SHIELD_BUCKLER", "ITEM_SHIELD_SHIELD", "ITEM_SHOES_BOOTS", "ITEM_SHOES_BOOTS_LOW", "ITEM_SHOES_CHAUSSE", "ITEM_SHOES_SANDAL", "ITEM_SHOES_SHOES", "ITEM_SHOES_SOCKS", "ITEM_SIEGEAMMO_BALLISTA", "ITEM_TOOL_BOWL", "ITEM_TOOL_CAULDRON", "ITEM_TOOL_FORK_CARVING", "ITEM_TOOL_HIVE", "ITEM_TOOL_HONEYCOMB", "ITEM_TOOL_JUG", "ITEM_TOOL_KNIFE_BONING", "ITEM_TOOL_KNIFE_CARVING", "ITEM_TOOL_KNIFE_MEAT_CLEAVER", "ITEM_TOOL_KNIFE_SLICING", "ITEM_TOOL_LADLE", "ITEM_TOOL_LARGE_POT", "ITEM_TOOL_MINECART", "ITEM_TOOL_MORTAR", "ITEM_TOOL_NEST_BOX", "ITEM_TOOL_PESTLE", "ITEM_TOOL_POUCH", "ITEM_TOOL_SCALE_SHARD", "ITEM_TOOL_STEPLADDER", "ITEM_TOOL_WHEELBARROW", "ITEM_TOY_AXE", "ITEM_TOY_BOAT", "ITEM_TOY_HAMMER", "ITEM_TOY_MINIFORGE", "ITEM_TOY_PUZZLEBOX", "ITEM_TRAPCOMP_ENORMOUSCORKSCREW", "ITEM_TRAPCOMP_GIANTAXEBLADE", "ITEM_TRAPCOMP_LARGESERRATEDDISC", "ITEM_TRAPCOMP_MENACINGSPIKE", "ITEM_TRAPCOMP_SPIKEDBALL", "ITEM_WEAPON_AXE_BATTLE", "ITEM_WEAPON_AXE_GREAT", "ITEM_WEAPON_AXE_TRAINING", "ITEM_WEAPON_BLOWGUN", "ITEM_WEAPON_BOW", "ITEM_WEAPON_CROSSBOW", "ITEM_WEAPON_DAGGER_LARGE", "ITEM_WEAPON_FLAIL", "ITEM_WEAPON_HALBERD", "ITEM_WEAPON_HAMMER_WAR", "ITEM_WEAPON_MACE", "ITEM_WEAPON_MAUL", "ITEM_WEAPON_MORNINGSTAR", "ITEM_WEAPON_PICK", "ITEM_WEAPON_PIKE", "ITEM_WEAPON_SCIMITAR", "ITEM_WEAPON_SCOURGE", "ITEM_WEAPON_SPEAR", "ITEM_WEAPON_SPEAR_TRAINING", "ITEM_WEAPON_SWORD_2H", "ITEM_WEAPON_SWORD_LONG", "ITEM_WEAPON_SWORD_SHORT", "ITEM_WEAPON_SWORD_SHORT_TRAINING", "ITEM_WEAPON_WHIP", "MEAT", "MILLSTONE", "ORIG_", "ORTHOPEDIC_CAST", "PIPE_SECTION", "QUERN", "QUIVER", "RING", "ROCK", "ROUGH", "SCEPTER", "SKIN_TANNED", "SLAB", "SMALLGEM", "SPLINT", "STATUE", "TABLE", "THREAD", "TOTEM", "TRACTION_BENCH", "TRAPPARTS", "WEAPONRACK", "WINDOW", "WOOD"

]]

function desc_of_item (ID)
    if ID == "ANVIL" then
        return {"An essential component of the forge."}
    elseif ID == "ARMORSTAND" then
        return {"A rack for the storage of military equipment, specifically armor.",
                "It is required by some nobles, and can be used to create a barracks."}
    elseif ID == "BARREL" then
        return {"A hollow cylinder with a removable lid. It is used to hold liquids,",
                "food, and seeds. It can be made from metal or wood, and is replaceable",
                "with a rock pot. A barrel (or rock pot) is needed to brew drinks."}
    elseif ID == "BED" then
        return {"A pallet for dwarves to sleep on, which must be made from wood.",
                "It prevents the stress of sleeping on the ground, and can be used",
                "to designate a bedroom (used by one dwarf or couple), a dormitory",
                "(used by multiple dwarves), or a barracks (used by a military",
                "squad for training or sleep)."}
    elseif ID == "BIN" then
        return {"A container for the storage of ammunition, armor and weapons, bars,",
                "blocks, cloth and leather, coins, finished goods and gems. It can",
                "be used to carry multiple items to the Trade Depot at once.",
                "A bin can be made from wood or forged from metal."}
    elseif ID == "BOX" then
        return {"A container for storing dwarves' items. They are required by nobles,",
                "and will increase the value of rooms they are placed in. Also",
                "required to store hospital supplies. They can be made from stone or",
                "metal (coffers), wood (chests),or textiles or leather (bags)."}
    elseif ID == "BUCKET" then
        return {"A small cylindrical or conical container for holding and carrying",
                "small amounts of liquid such as water or lye. They are used by",
                "dwarves to give water to other dwarves, to store lye, and are",
                "required to build wells and certain workshops. They can be made",
                "from wood or metal."}
    elseif ID == "ITEM_AMMO_ARROWS" then
        return {"Ammunition for bows."}
    elseif ID == "ITEM_AMMO_BOLTS" then
        return {"Ammunition for crossbows."}
    elseif ID == "ITEM_ARMOR_LEATHER" then
        return {"Leather armor is light and covers both arms and legs",
                "in addition to body"}
    elseif ID == "ITEM_WEAPON_AXE_BATTLE" then
        return {"A battle axe is an edged weapon: essentially a sharp blade",
                "mounted along the end of a short and heavy handle.", "",
                "Dwarves can forge battle axes out of any weapon-grade metal,",
                "though those with superior edge properties are more effective.", "",
                "A battle axe may also be used as a tool for chopping down trees."}
    elseif ID == "TRAPPARTS" then
        return {"Used to build traps, levers and other machines."}
    end
end