-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.

---@meta

---@enum resource_allotment_specifier_type
df.resource_allotment_specifier_type = {
    CROP = 0,
    STONE = 1,
    METAL = 2,
    WOOD = 3,
    ARMOR_BODY = 4,
    ARMOR_PANTS = 5,
    ARMOR_GLOVES = 6,
    ARMOR_BOOTS = 7,
    ARMOR_HELM = 8,
    CLOTHING_BODY = 9,
    CLOTHING_PANTS = 10,
    CLOTHING_GLOVES = 11,
    CLOTHING_BOOTS = 12,
    CLOTHING_HELM = 13,
    WEAPON_MELEE = 14,
    WEAPON_RANGED = 15,
    ANVIL = 16,
    GEMS = 17,
    THREAD = 18,
    CLOTH = 19,
    LEATHER = 20,
    QUIVER = 21,
    BACKPACK = 22,
    FLASK = 23,
    BAG = 24,
    TABLE = 25,
    CABINET = 26,
    CHAIR = 27,
    BOX = 28,
    BED = 29,
    CRAFTS = 30,
    MEAT = 31,
    BONE = 32,
    HORN = 33,
    SHELL = 34,
    TALLOW = 35,
    TOOTH = 36,
    PEARL = 37,
    SOAP = 38,
    EXTRACT = 39,
    CHEESE = 40,
    SKIN = 41,
    POWDER = 42,
    AMMO = 43,
}

---@return resource_allotment_specifier_type
function df.resource_allotment_specifier.getType() end

---@param file file_compressorst
function df.resource_allotment_specifier.write_file(file) end

---@param file file_compressorst
---@param loadversion save_version
function df.resource_allotment_specifier.read_file(file, loadversion) end

---@class resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
df.resource_allotment_specifier = nil

---@class resource_allotment_specifier_cropst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer index to world.raws.plant.all
---@field unk_4 integer
---@field unk_v40_01 integer since v0.40.01
---@field unk_5 integer[]
df.resource_allotment_specifier_cropst = nil

---@class resource_allotment_specifier_stonest
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
---@field unk_4 integer
---@field unk_5 integer
---@field unk_6 integer[]
df.resource_allotment_specifier_stonest = nil

---@class resource_allotment_specifier_metalst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
---@field unk_4 integer
---@field unk_5 integer[]
df.resource_allotment_specifier_metalst = nil

---@class resource_allotment_specifier_woodst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
---@field unk_4 integer
---@field unk_5 integer
---@field unk_6 integer
---@field unk_7 integer
---@field unk_8 integer
---@field unk_9 integer
df.resource_allotment_specifier_woodst = nil

---@class resource_allotment_specifier_armor_bodyst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
df.resource_allotment_specifier_armor_bodyst = nil

---@class resource_allotment_specifier_armor_pantsst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
df.resource_allotment_specifier_armor_pantsst = nil

---@class resource_allotment_specifier_armor_glovesst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
df.resource_allotment_specifier_armor_glovesst = nil

---@class resource_allotment_specifier_armor_bootsst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
df.resource_allotment_specifier_armor_bootsst = nil

---@class resource_allotment_specifier_armor_helmst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
df.resource_allotment_specifier_armor_helmst = nil

---@class resource_allotment_specifier_clothing_bodyst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
df.resource_allotment_specifier_clothing_bodyst = nil

---@class resource_allotment_specifier_clothing_pantsst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
df.resource_allotment_specifier_clothing_pantsst = nil

---@class resource_allotment_specifier_clothing_glovesst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
df.resource_allotment_specifier_clothing_glovesst = nil

---@class resource_allotment_specifier_clothing_bootsst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
df.resource_allotment_specifier_clothing_bootsst = nil

---@class resource_allotment_specifier_clothing_helmst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
df.resource_allotment_specifier_clothing_helmst = nil

---@class resource_allotment_specifier_weapon_meleest
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
df.resource_allotment_specifier_weapon_meleest = nil

---@class resource_allotment_specifier_weapon_rangedst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
df.resource_allotment_specifier_weapon_rangedst = nil

---@class resource_allotment_specifier_ammost
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
df.resource_allotment_specifier_ammost = nil

---@class resource_allotment_specifier_anvilst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
df.resource_allotment_specifier_anvilst = nil

---@class resource_allotment_specifier_gemsst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
df.resource_allotment_specifier_gemsst = nil

---@class resource_allotment_specifier_threadst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
---@field unk_4 integer
df.resource_allotment_specifier_threadst = nil

---@class resource_allotment_specifier_clothst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
---@field unk_4 integer
---@field unk_5 integer
---@field unk_6 integer
---@field unk_7 integer
---@field unk_8 integer
df.resource_allotment_specifier_clothst = nil

---@class resource_allotment_specifier_leatherst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
---@field unk_4 integer
---@field unk_5 integer
---@field unk_6 integer
---@field unk_7 integer
---@field unk_8 integer
---@field unk_9 integer
---@field unk_10 integer
---@field unk_11 integer
---@field unk_12 integer
---@field unk_13 integer
df.resource_allotment_specifier_leatherst = nil

---@class resource_allotment_specifier_quiverst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
df.resource_allotment_specifier_quiverst = nil

---@class resource_allotment_specifier_backpackst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
df.resource_allotment_specifier_backpackst = nil

---@class resource_allotment_specifier_flaskst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
df.resource_allotment_specifier_flaskst = nil

---@class resource_allotment_specifier_bagst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
df.resource_allotment_specifier_bagst = nil

---@class resource_allotment_specifier_tablest
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
df.resource_allotment_specifier_tablest = nil

---@class resource_allotment_specifier_cabinetst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
df.resource_allotment_specifier_cabinetst = nil

---@class resource_allotment_specifier_chairst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
df.resource_allotment_specifier_chairst = nil

---@class resource_allotment_specifier_boxst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
df.resource_allotment_specifier_boxst = nil

---@class resource_allotment_specifier_bedst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
df.resource_allotment_specifier_bedst = nil

---@class resource_allotment_specifier_craftsst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
df.resource_allotment_specifier_craftsst = nil

---@class resource_allotment_specifier_meatst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
df.resource_allotment_specifier_meatst = nil

---@class resource_allotment_specifier_bonest
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
---@field unk_4 integer
df.resource_allotment_specifier_bonest = nil

---@class resource_allotment_specifier_hornst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
---@field unk_4 integer
df.resource_allotment_specifier_hornst = nil

---@class resource_allotment_specifier_shellst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
---@field unk_4 integer
df.resource_allotment_specifier_shellst = nil

---@class resource_allotment_specifier_tallowst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
df.resource_allotment_specifier_tallowst = nil

---@class resource_allotment_specifier_toothst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
---@field unk_4 integer
df.resource_allotment_specifier_toothst = nil

---@class resource_allotment_specifier_pearlst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
df.resource_allotment_specifier_pearlst = nil

---@class resource_allotment_specifier_soapst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
df.resource_allotment_specifier_soapst = nil

---@class resource_allotment_specifier_extractst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
---@field unk_4 integer
---@field mat_type2 integer
---@field mat_index2 integer
---@field unk_5 integer uninitialized
df.resource_allotment_specifier_extractst = nil

---@class resource_allotment_specifier_cheesest
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
df.resource_allotment_specifier_cheesest = nil

---@class resource_allotment_specifier_skinst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
---@field mat_type2 integer
---@field mat_index2 integer
---@field unk_4 integer
df.resource_allotment_specifier_skinst = nil

---@class resource_allotment_specifier_powderst
-- inherited from resource_allotment_specifier
---@field unk_1 integer
---@field unk_2 integer since v0.34.01-04
---@field unk_3 integer since v0.34.01-04
-- end resource_allotment_specifier
---@field mat_type integer
---@field mat_index integer
---@field unk_4 integer
df.resource_allotment_specifier_powderst = nil

---@class resource_allotment_data
---@field index integer
---@field resource_allotments any[]
---@field unk1 integer
---@field unk2 integer
---@field unk3 integer
---@field unk_650 integer
---@field unk_654 any[]
df.resource_allotment_data = nil


