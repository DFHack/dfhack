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
]]

function desc_of_item (textid, subtype)
    -- TODO: add placeholders, then content, for all vanilla items.
    if textid == "AMMO" then
        if subtype == "ITEM_AMMO_ARROWS" then
            return {"Ammunition for bows."}
        elseif subtype == "ITEM_AMMO_BOLTS" then
            return {"Ammunition for crossbows."}
        end
    elseif textid == "ARMOR" then
        if subtype == "ITEM_ARMOR_LEATHER" then
            return {"Leather armor is light and covers both arms and legs",
                    "in addition to body"}
        end
    elseif textid == "WEAPON" then
        if subtype == "ITEM_WEAPON_AXE_BATTLE" then
            return {"A battle axe is an edged weapon: essentially a sharp blade",
                    "mounted along the end of a short and heavy handle.", "",
                    "Dwarves can forge battle axes out of any weapon-grade metal,",
                    "though those with superior edge properties are more effective.", "",
                    "A battle axe may also be used as a tool for chopping down trees."}
        end
    elseif textid == "ANVIL" then
        return {"An essential component of the forge."}
    elseif textid == "ARMORSTAND" then
        return {"A rack for the storage of military equipment, specifically armor.",
                "It is required by some nobles, and can be used to create a barracks."}
    elseif textid == "BARREL" then
        return {"A hollow cylinder with a removable lid. It is used to hold liquids,",
                "food, and seeds. It can be made from metal or wood, and is replaceable",
                "with a rock pot. A barrel (or rock pot) is needed to brew drinks."}
    elseif textid == "BED" then
        return {"A pallet for dwarves to sleep on, which must be made from wood.",
                "It prevents the stress of sleeping on the ground, and can be used",
                "to designate a bedroom (used by one dwarf or couple), a dormitory",
                "(used by multiple dwarves), or a barracks (used by a military",
                "squad for training or sleep)."}
    elseif textid == "BIN" then
        return {"A container for the storage of ammunition, armor and weapons, bars,",
                "blocks, cloth and leather, coins, finished goods and gems. It can",
                "be used to carry multiple items to the Trade Depot at once.",
                "A bin can be made from wood or forged from metal."}
    elseif textid == "BOX" then
        return {"A container for storing dwarves' items. They are required by nobles,",
                "and will increase the value of rooms they are placed in. Also",
                "required to store hospital supplies. They can be made from stone or",
                "metal (coffers), wood (chests),or textiles or leather (bags)."}
    elseif textid == "BUCKET" then
        return {"A small cylindrical or conical container for holding and carrying",
                "small amounts of liquid such as water or lye. They are used by",
                "dwarves to give water to other dwarves, to store lye, and are",
                "required to build wells and certain workshops. They can be made",
                "from wood or metal."}
    elseif textid == "TRAPPARTS" then
        return {"Used to build traps, levers and other machines."}
    end
end