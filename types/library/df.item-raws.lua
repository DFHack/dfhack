-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.

---@meta

---@enum item_type
df.item_type = {
    NONE = -1,
    BAR = 0, -- Bars, such as metal, fuel, or soap.
    SMALLGEM = 1, -- Cut gemstones usable in jewelers workshop
    BLOCKS = 2, -- Blocks of any kind.
    ROUGH = 3, -- Rough gemstones.
    BOULDER = 4, -- Raw mined stone.
    WOOD = 5, -- Wooden logs.
    DOOR = 6, -- Doors.
    FLOODGATE = 7, -- Floodgates.
    BED = 8, -- Beds.
    CHAIR = 9, -- Chairs and thrones.
    CHAIN = 10, -- Restraints.
    FLASK = 11, -- Flasks.
    GOBLET = 12, -- Goblets.
    INSTRUMENT = 13, -- Musical instruments.
    TOY = 14, -- Toys.
    WINDOW = 15, -- Glass windows.
    CAGE = 16, -- Cages.
    BARREL = 17, -- Barrels.
    BUCKET = 18, -- Buckets.
    ANIMALTRAP = 19, -- Animal traps.
    TABLE = 20, -- Tables.
    COFFIN = 21, -- Coffins.
    STATUE = 22, -- Statues.
    CORPSE = 23, -- Corpses. Does not have a material.
    WEAPON = 24, -- Weapons.
    ARMOR = 25, -- Armor and clothing worn on the upper body.
    SHOES = 26, -- Armor and clothing worn on the feet.
    SHIELD = 27, -- Shields and bucklers.
    HELM = 28, -- Armor and clothing worn on the head.
    GLOVES = 29, -- Armor and clothing worn on the hands.
    BOX = 30, -- Chests (wood), coffers (stone), and boxes (glass).
    BAG = 31, -- Bags (cloth or leather).
    BIN = 32, -- Bins.
    ARMORSTAND = 33, -- Armor stands.
    WEAPONRACK = 34, -- Weapon racks.
    CABINET = 35, -- Cabinets.
    FIGURINE = 36, -- Figurines.
    AMULET = 37, -- Amulets.
    SCEPTER = 38, -- Scepters.
    AMMO = 39, -- Ammunition for hand-held weapons.
    CROWN = 40, -- Crowns.
    RING = 41, -- Rings.
    EARRING = 42, -- Earrings.
    BRACELET = 43, -- Bracelets.
    GEM = 44, -- Large gems.
    ANVIL = 45, -- Anvils.
    CORPSEPIECE = 46, -- Body parts. Does not have a material.
    REMAINS = 47, -- Dead vermin bodies. Material is CREATURE_ID:CASTE.
    MEAT = 48, -- Butchered meat.
    FISH = 49, -- Prepared fish. Material is CREATURE_ID:CASTE.
    FISH_RAW = 50, -- Unprepared fish. Material is CREATURE_ID:CASTE.
    VERMIN = 51, -- Live vermin. Material is CREATURE_ID:CASTE.
    PET = 52, -- Tame vermin. Material is CREATURE_ID:CASTE.
    SEEDS = 53, -- Seeds from plants.
    PLANT = 54, -- Plants.
    SKIN_TANNED = 55, -- Tanned skins.
    PLANT_GROWTH = 56, -- Assorted plant growths, including leaves and berries
    THREAD = 57, -- Thread gathered from webs or made at the farmers workshop.
    CLOTH = 58, -- Cloth made at the loom.
    TOTEM = 59, -- Skull totems.
    PANTS = 60, -- Armor and clothing worn on the legs.
    BACKPACK = 61, -- Backpacks.
    QUIVER = 62, -- Quivers.
    CATAPULTPARTS = 63, -- Catapult parts.
    BALLISTAPARTS = 64, -- Ballista parts.
    SIEGEAMMO = 65, -- Siege engine ammunition.
    BALLISTAARROWHEAD = 66, -- Ballista arrow heads.
    TRAPPARTS = 67, -- Mechanisms.
    TRAPCOMP = 68, -- Trap components.
    DRINK = 69, -- Alcoholic drinks.
    POWDER_MISC = 70, -- Powders such as flour, gypsum plaster, dye, or sand.
    CHEESE = 71, -- Pieces of cheese.
    FOOD = 72, -- Prepared meals. Subtypes come from item_food.txt
    LIQUID_MISC = 73, -- Liquids such as water, lye, and extracts.
    COIN = 74, -- Coins.
    GLOB = 75, -- Fat, tallow, pastes/pressed objects, and small bits of molten rock/metal.
    ROCK = 76, -- Small rocks (usually sharpened and/or thrown in adventurer mode)
    PIPE_SECTION = 77, -- Pipe sections.
    HATCH_COVER = 78, -- Hatch covers.
    GRATE = 79, -- Grates.
    QUERN = 80, -- Querns.
    MILLSTONE = 81, -- Millstones.
    SPLINT = 82, -- Splints.
    CRUTCH = 83, -- Crutches.
    TRACTION_BENCH = 84, -- Traction benches.
    ORTHOPEDIC_CAST = 85, -- Casts
    TOOL = 86, -- Tools.
    SLAB = 87, -- Slabs.
    EGG = 88, -- Eggs. Material is CREATURE_ID:CASTE.
    BOOK = 89, -- Books.
    SHEET = 90, -- Sheets of paper
    BRANCH = 91, -- Tree branches
}

---@class weapon_attack
---@field edged boolean
---@field contact integer
---@field penetration integer
---@field velocity_mult integer
---@field verb_2nd string
---@field verb_3rd string
---@field noun string
---@field prepare integer since v0.40.01
---@field recover integer since v0.40.01
---@field flags bitfield since v0.40.01
df.weapon_attack = nil

---@enum itemdef_flags
df.itemdef_flags = {
    GENERATED = 0,
}

function df.itemdef.unnamed_method() end

---@param arg_0 integer
---@param arg_1 integer
---@param arg_2 integer
---@param arg_3 integer
---@param arg_4 integer
function df.itemdef.parseRaws(arg_0, arg_1, arg_2, arg_3, arg_4) end

function df.itemdef.categorize() end -- add to world.raws.itemdefs.whatever

function df.itemdef.finalize() end -- calculate stuff like base value

function df.itemdef.unnamed_method() end

---@class itemdef
---@field id string
---@field subtype integer
---@field base_flags itemdef_flags[] since v0.34.08
---@field source_hfid integer since v0.40.01
---@field source_enid integer since v0.42.01
---@field raw_strings string[]
---@field unnamed_itemdef_7 integer
---@field unnamed_itemdef_8 integer
df.itemdef = nil

---@enum ammo_flags
df.ammo_flags = {
    HAS_EDGE_ATTACK = 0,
}

---@class itemdef_ammost
-- inherited from itemdef
---@field id string
---@field subtype integer
---@field base_flags itemdef_flags[] since v0.34.08
---@field source_hfid integer since v0.40.01
---@field source_enid integer since v0.42.01
---@field raw_strings string[]
---@field unnamed_itemdef_7 integer
---@field unnamed_itemdef_8 integer
-- end itemdef
---@field name string
---@field name_plural string
---@field adjective string since v0.40.01
---@field ammo_class string
---@field flags ammo_flags[]
---@field size integer divided by 10
---@field value integer
---@field attacks weapon_attack[]
---@field unnamed_itemdef_ammost_9 integer[]
---@field unnamed_itemdef_ammost_10 any[] items pointed to are 8 bytes
df.itemdef_ammost = nil

---@enum armor_general_flags
df.armor_general_flags = {
    SOFT = 0,
    HARD = 1,
    METAL = 2,
    BARRED = 3,
    SCALED = 4,
    LEATHER = 5,
    SHAPED = 6,
    CHAIN_METAL_TEXT = 7,
    STRUCTURAL_ELASTICITY_WOVEN_THREAD = 8,
    STRUCTURAL_ELASTICITY_CHAIN_METAL = 9,
    STRUCTURAL_ELASTICITY_CHAIN_ALL = 10,
}

---@class armor_properties
---@field flags armor_general_flags[]
---@field layer integer
---@field layer_size integer
---@field layer_permit integer
---@field coverage integer
df.armor_properties = nil

---@enum armor_flags
df.armor_flags = {
    METAL_ARMOR_LEVELS = 0,
}

---@class itemdef_armorst
-- inherited from itemdef
---@field id string
---@field subtype integer
---@field base_flags itemdef_flags[] since v0.34.08
---@field source_hfid integer since v0.40.01
---@field source_enid integer since v0.42.01
---@field raw_strings string[]
---@field unnamed_itemdef_7 integer
---@field unnamed_itemdef_8 integer
-- end itemdef
---@field name string
---@field name_plural string
---@field name_preplural string
---@field material_placeholder string
---@field adjective string since v0.40.01
---@field value integer
---@field armorlevel integer
---@field ubstep integer
---@field lbstep integer
---@field material_size integer
---@field props armor_properties
---@field flags armor_flags[]
---@field unnamed_itemdef_armorst_13 integer
---@field unnamed_itemdef_armorst_14 integer
---@field unnamed_itemdef_armorst_15 any[] items pointed to are 8 bytes
df.itemdef_armorst = nil

---@class itemdef_foodst
-- inherited from itemdef
---@field id string
---@field subtype integer
---@field base_flags itemdef_flags[] since v0.34.08
---@field source_hfid integer since v0.40.01
---@field source_enid integer since v0.42.01
---@field raw_strings string[]
---@field unnamed_itemdef_7 integer
---@field unnamed_itemdef_8 integer
-- end itemdef
---@field name string
---@field level integer
---@field unnamed_itemdef_foodst_3 integer
---@field unnamed_itemdef_foodst_4 integer
df.itemdef_foodst = nil

---@enum gloves_flags
df.gloves_flags = {
    METAL_ARMOR_LEVELS = 0,
}

---@class itemdef_glovesst
-- inherited from itemdef
---@field id string
---@field subtype integer
---@field base_flags itemdef_flags[] since v0.34.08
---@field source_hfid integer since v0.40.01
---@field source_enid integer since v0.42.01
---@field raw_strings string[]
---@field unnamed_itemdef_7 integer
---@field unnamed_itemdef_8 integer
-- end itemdef
---@field name string
---@field name_plural string
---@field adjective string since v0.40.01
---@field value integer
---@field armorlevel integer
---@field upstep integer
---@field flags gloves_flags[]
---@field material_size integer
---@field props armor_properties
---@field unnamed_itemdef_glovesst_10 integer
---@field unnamed_itemdef_glovesst_11 integer
---@field unnamed_itemdef_glovesst_12 any[] items pointed to are 8 bytes
df.itemdef_glovesst = nil

---@enum helm_flags
df.helm_flags = {
    METAL_ARMOR_LEVELS = 0,
}

---@class itemdef_helmst
-- inherited from itemdef
---@field id string
---@field subtype integer
---@field base_flags itemdef_flags[] since v0.34.08
---@field source_hfid integer since v0.40.01
---@field source_enid integer since v0.42.01
---@field raw_strings string[]
---@field unnamed_itemdef_7 integer
---@field unnamed_itemdef_8 integer
-- end itemdef
---@field name string
---@field name_plural string
---@field adjective string since v0.40.01
---@field value integer
---@field armorlevel integer
---@field flags helm_flags[]
---@field material_size integer
---@field props armor_properties
---@field unnamed_itemdef_helmst_9 integer
---@field unnamed_itemdef_helmst_10 integer
---@field unnamed_itemdef_helmst_11 any[] items pointed to are 8 bytes
df.itemdef_helmst = nil

---@enum instrument_flags
df.instrument_flags = {
    INDEFINITE_PITCH = 0,
    PLACED_AS_BUILDING = 1,
    METAL_MAT = 2,
    STONE_MAT = 3,
    WOOD_MAT = 4,
    GLASS_MAT = 5,
    CERAMIC_MAT = 6,
    SHELL_MAT = 7,
    BONE_MAT = 8,
}

---@class itemdef_instrumentst
-- inherited from itemdef
---@field id string
---@field subtype integer
---@field base_flags itemdef_flags[] since v0.34.08
---@field source_hfid integer since v0.40.01
---@field source_enid integer since v0.42.01
---@field raw_strings string[]
---@field unnamed_itemdef_7 integer
---@field unnamed_itemdef_8 integer
-- end itemdef
---@field name string
---@field name_plural string
---@field flags instrument_flags[]
---@field music_skill job_skill
---@field size integer
---@field value integer
---@field material_size integer
---@field pieces instrument_piece[]
---@field dominant_instrument_piece string
---@field pitch_range_min integer
---@field pitch_range_max integer
---@field volume_mb_min integer
---@field volume_mb_max integer
---@field sound_production sound_production_type[]
---@field sound_production_parm1 string[]
---@field sound_production_parm2 string[]
---@field unk_100 integer[]
---@field unk_110 integer[]
---@field pitch_choice pitch_choice_type[]
---@field pitch_choice_parm1 string[]
---@field pitch_choice_parm2 string[]
---@field unk_150 integer[]
---@field unk_160 integer[]
---@field tuning tuning_type[]
---@field tuning_parm string[]
---@field unk_190 integer[]
---@field registers instrument_register[]
---@field timbre timbre_type[]
---@field description string
df.itemdef_instrumentst = nil

---@enum sound_production_type
df.sound_production_type = {
    PLUCKED_BY_BP = 0,
    PLUCKED = 1,
    BOWED = 2,
    STRUCK_BY_BP = 3,
    STRUCK = 4,
    VIBRATE_BP_AGAINST_OPENING = 5,
    BLOW_AGAINST_FIPPLE = 6,
    BLOW_OVER_OPENING_SIDE = 7,
    BLOW_OVER_OPENING_END = 8,
    BLOW_OVER_SINGLE_REED = 9,
    BLOW_OVER_DOUBLE_REED = 10,
    BLOW_OVER_FREE_REED = 11,
    STRUCK_TOGETHER = 12,
    SHAKEN = 13,
    SCRAPED = 14,
    FRICTION = 15,
    RESONATOR = 16,
    BAG_OVER_REED = 17,
    AIR_OVER_REED = 18,
    AIR_OVER_FREE_REED = 19,
    AIR_AGAINST_FIPPLE = 20,
}

---@enum pitch_choice_type
df.pitch_choice_type = {
    MEMBRANE_POSITION = 0,
    SUBPART_CHOICE = 1,
    KEYBOARD = 2,
    STOPPING_FRET = 3,
    STOPPING_AGAINST_BODY = 4,
    STOPPING_HOLE = 5,
    STOPPING_HOLE_KEY = 6,
    SLIDE = 7,
    HARMONIC_SERIES = 8,
    VALVE_ROUTES_AIR = 9,
    BP_IN_BELL = 10,
    FOOT_PEDALS = 11,
}

---@enum tuning_type
df.tuning_type = {
    PEGS = 0,
    ADJUSTABLE_BRIDGES = 1,
    CROOKS = 2,
    TIGHTENING = 3,
    LEVERS = 4,
}

---@enum timbre_type
df.timbre_type = {
    CLEAR = 0,
    NOISY = 1,
    FULL = 2,
    THIN = 3,
    ROUND = 4,
    SHARP = 5,
    SMOOTH = 6,
    CHOPPY = 7,
    STEADY = 8,
    EVOLVING = 9,
    STRONG = 10,
    DELICATE = 11,
    BRIGHT = 12,
    GRACEFUL = 13,
    SPARSE = 14,
    BREATHY = 15,
    STRAINED = 16,
    BROAD = 17,
    LIGHT = 18,
    MELLOW = 19,
    WOBBLING = 20,
    FOCUSED = 21,
    EVEN = 22,
    FLUID = 23,
    VIBRATING = 24,
    QUAVERING = 25,
    EERIE = 26,
    FRAGILE = 27,
    BRITTLE = 28,
    PURE_ = 29,
    PIERCING = 30,
    STRIDENT = 31,
    WAVERING = 32,
    HARSH = 33,
    REEDY = 34,
    NASAL = 35,
    BUZZY = 36,
    ROUGH = 37,
    WARM = 38,
    RUGGED = 39,
    HEAVY = 40,
    FLAT = 41,
    DARK = 42,
    CRISP = 43,
    SONOROUS = 44,
    WATERY = 45,
    GENTLE = 46,
    SLICING = 47,
    LIQUID = 48,
    RAUCOUS = 49,
    BREEZY = 50,
    RASPY = 51,
    WISPY = 52,
    SHRILL = 53,
    MUDDY = 54,
    RICH = 55,
    DULL = 56,
    FLOATING = 57,
    RINGING = 58,
    RESONANT = 59,
    SWEET = 60,
    RIPPLING = 61,
    SPARKLING = 62,
}

---@class instrument_piece
---@field type string
---@field id string
---@field index integer
---@field name string
---@field name_plural string
---@field flags bitfield
df.instrument_piece = nil

---@class instrument_register
---@field pitch_range_min integer
---@field pitch_range_max integer
---@field timbres timbre_type[]
df.instrument_register = nil

---@enum pants_flags
df.pants_flags = {
    METAL_ARMOR_LEVELS = 0,
}

---@class itemdef_pantsst
-- inherited from itemdef
---@field id string
---@field subtype integer
---@field base_flags itemdef_flags[] since v0.34.08
---@field source_hfid integer since v0.40.01
---@field source_enid integer since v0.42.01
---@field raw_strings string[]
---@field unnamed_itemdef_7 integer
---@field unnamed_itemdef_8 integer
-- end itemdef
---@field name string
---@field name_plural string
---@field name_preplural string
---@field material_placeholder string
---@field adjective string since v0.40.01
---@field value integer
---@field armorlevel integer
---@field flags pants_flags[]
---@field material_size integer
---@field lbstep integer
---@field props armor_properties
---@field unnamed_itemdef_pantsst_12 integer
---@field unnamed_itemdef_pantsst_13 integer
---@field unnamed_itemdef_pantsst_14 any[] items pointed to are 8 bytes
df.itemdef_pantsst = nil

---@class itemdef_shieldst
-- inherited from itemdef
---@field id string
---@field subtype integer
---@field base_flags itemdef_flags[] since v0.34.08
---@field source_hfid integer since v0.40.01
---@field source_enid integer since v0.42.01
---@field raw_strings string[]
---@field unnamed_itemdef_7 integer
---@field unnamed_itemdef_8 integer
-- end itemdef
---@field name string
---@field name_plural string
---@field adjective string since v0.40.01
---@field value integer
---@field blockchance integer
---@field armorlevel integer
---@field upstep integer
---@field material_size integer
---@field unnamed_itemdef_shieldst_9 integer
---@field unnamed_itemdef_shieldst_10 integer
---@field unnamed_itemdef_shieldst_11 any[] items pointed to are 8 bytes
df.itemdef_shieldst = nil

---@enum shoes_flags
df.shoes_flags = {
    METAL_ARMOR_LEVELS = 0,
}

---@class itemdef_shoesst
-- inherited from itemdef
---@field id string
---@field subtype integer
---@field base_flags itemdef_flags[] since v0.34.08
---@field source_hfid integer since v0.40.01
---@field source_enid integer since v0.42.01
---@field raw_strings string[]
---@field unnamed_itemdef_7 integer
---@field unnamed_itemdef_8 integer
-- end itemdef
---@field name string
---@field name_plural string
---@field adjective string since v0.40.01
---@field value integer
---@field armorlevel integer
---@field upstep integer
---@field flags shoes_flags[]
---@field material_size integer
---@field props armor_properties
---@field unnamed_itemdef_shoesst_10 integer
---@field unnamed_itemdef_shoesst_11 integer
---@field unnamed_itemdef_shoesst_12 any[] items pointed to are 8 bytes
df.itemdef_shoesst = nil

---@class itemdef_siegeammost
-- inherited from itemdef
---@field id string
---@field subtype integer
---@field base_flags itemdef_flags[] since v0.34.08
---@field source_hfid integer since v0.40.01
---@field source_enid integer since v0.42.01
---@field raw_strings string[]
---@field unnamed_itemdef_7 integer
---@field unnamed_itemdef_8 integer
-- end itemdef
---@field name string
---@field name_plural string
---@field ammo_class string
---@field unnamed_itemdef_siegeammost_4 integer[]
---@field unnamed_itemdef_siegeammost_5 any[] items pointed to are 8 bytes
df.itemdef_siegeammost = nil

---@enum tool_flags
df.tool_flags = {
    HARD_MAT = 0,
    METAL_MAT = 1,
    HAS_EDGE_ATTACK = 2,
    METAL_WEAPON_MAT = 3,
    UNIMPROVABLE = 4,
    SOFT_MAT = 5,
    WOOD_MAT = 6,
    INVERTED_TILE = 7,
    FURNITURE = 8,
    LEATHER_MAT = 9,
    SILK_MAT = 10,
    THREAD_PLANT_MAT = 11,
    GLASS_MAT = 12,
    CERAMIC_MAT = 13,
    STONE_MAT = 14,
    SHELL_MAT = 15,
    BONE_MAT = 16,
    NO_DEFAULT_JOB = 17,
    INCOMPLETE_ITEM = 18,
    SHEET_MAT = 19,
}

---@enum tool_uses
df.tool_uses = {
    NONE = -1,
    LIQUID_COOKING = 0,
    LIQUID_SCOOP = 1,
    GRIND_POWDER_RECEPTACLE = 2,
    GRIND_POWDER_GRINDER = 3,
    MEAT_CARVING = 4,
    MEAT_BONING = 5,
    MEAT_SLICING = 6,
    MEAT_CLEAVING = 7,
    HOLD_MEAT_FOR_CARVING = 8,
    MEAL_CONTAINER = 9,
    LIQUID_CONTAINER = 10,
    FOOD_STORAGE = 11,
    HIVE = 12,
    NEST_BOX = 13,
    SMALL_OBJECT_STORAGE = 14,
    TRACK_CART = 15,
    HEAVY_OBJECT_HAULING = 16,
    STAND_AND_WORK_ABOVE = 17,
    ROLL_UP_SHEET = 18,
    PROTECT_FOLDED_SHEETS = 19,
    CONTAIN_WRITING = 20,
    BOOKCASE = 21,
    DISPLAY_OBJECT = 22,
    PLACE_OFFERING = 23,
    DIVINATION = 24,
    GAMES_OF_CHANCE = 25,
}

---@class itemdef_toolst
-- inherited from itemdef
---@field id string
---@field subtype integer
---@field base_flags itemdef_flags[] since v0.34.08
---@field source_hfid integer since v0.40.01
---@field source_enid integer since v0.42.01
---@field raw_strings string[]
---@field unnamed_itemdef_7 integer
---@field unnamed_itemdef_8 integer
-- end itemdef
---@field name string
---@field name_plural string
---@field flags tool_flags[]
---@field value integer
---@field tile integer
---@field tool_use any[]
---@field adjective string
---@field size integer
---@field skill_melee job_skill
---@field skill_ranged job_skill
---@field ranged_ammo string
---@field two_handed integer
---@field minimum_size integer
---@field material_size integer
---@field attacks weapon_attack[]
---@field shoot_force integer
---@field shoot_maxvel integer
---@field container_capacity integer
---@field shape_category_str string[] since v0.47.01
---@field shape_category integer[] since v0.47.01
---@field description string since v0.42.01
---@field default_improvements any[] since v0.42.01
---@field unnamed_itemdef_toolst_23 integer[]
---@field unnamed_itemdef_toolst_24 integer[]
---@field unnamed_itemdef_toolst_25 integer[]
---@field unnamed_itemdef_toolst_26 any[]
---@field unnamed_itemdef_toolst_27 any[]
df.itemdef_toolst = nil

---@enum toy_flags
df.toy_flags = {
    HARD_MAT = 0,
}

---@class itemdef_toyst
-- inherited from itemdef
---@field id string
---@field subtype integer
---@field base_flags itemdef_flags[] since v0.34.08
---@field source_hfid integer since v0.40.01
---@field source_enid integer since v0.42.01
---@field raw_strings string[]
---@field unnamed_itemdef_7 integer
---@field unnamed_itemdef_8 integer
-- end itemdef
---@field name string
---@field name_plural string
---@field flags toy_flags[]
---@field unnamed_itemdef_toyst_4 integer
---@field unnamed_itemdef_toyst_5 integer
---@field unnamed_itemdef_toyst_6 integer
---@field unnamed_itemdef_toyst_7 integer
---@field unnamed_itemdef_toyst_8 integer
---@field unnamed_itemdef_toyst_9 any[]
df.itemdef_toyst = nil

---@enum trapcomp_flags
df.trapcomp_flags = {
    IS_SCREW = 0,
    IS_SPIKE = 1,
    WOOD = 2,
    METAL = 3,
    HAS_EDGE_ATTACK = 4,
}

---@class itemdef_trapcompst
-- inherited from itemdef
---@field id string
---@field subtype integer
---@field base_flags itemdef_flags[] since v0.34.08
---@field source_hfid integer since v0.40.01
---@field source_enid integer since v0.42.01
---@field raw_strings string[]
---@field unnamed_itemdef_7 integer
---@field unnamed_itemdef_8 integer
-- end itemdef
---@field name string
---@field name_plural string
---@field adjective string
---@field size integer
---@field value integer
---@field hits integer
---@field material_size integer
---@field flags trapcomp_flags[]
---@field attacks weapon_attack[]
---@field unnamed_itemdef_trapcompst_10 integer[]
---@field unnamed_itemdef_trapcompst_11 any[]
df.itemdef_trapcompst = nil

---@enum weapon_flags
df.weapon_flags = {
    CAN_STONE = 0,
    HAS_EDGE_ATTACK = 1,
    TRAINING = 2,
}

---@class itemdef_weaponst
-- inherited from itemdef
---@field id string
---@field subtype integer
---@field base_flags itemdef_flags[] since v0.34.08
---@field source_hfid integer since v0.40.01
---@field source_enid integer since v0.42.01
---@field raw_strings string[]
---@field unnamed_itemdef_7 integer
---@field unnamed_itemdef_8 integer
-- end itemdef
---@field name string
---@field name_plural string
---@field adjective string
---@field size integer
---@field value integer
---@field skill_melee job_skill
---@field skill_ranged job_skill
---@field ranged_ammo string
---@field two_handed integer
---@field minimum_size integer
---@field material_size integer
---@field flags weapon_flags[]
---@field attacks weapon_attack[]
---@field shoot_force integer
---@field shoot_maxvel integer
---@field unnamed_itemdef_weaponst_16 integer[]
---@field unnamed_itemdef_weaponst_17 any[]
df.itemdef_weaponst = nil


