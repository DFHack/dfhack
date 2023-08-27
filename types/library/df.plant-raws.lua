-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.

---@meta

---@enum plant_raw_flags
df.plant_raw_flags = {
    SPRING = 0,
    SUMMER = 1,
    AUTUMN = 2,
    WINTER = 3,
    unk_4 = 4,
    SEED = 5,
    TREE_HAS_MUSHROOM_CAP = 6,
    DRINK = 7,
    EXTRACT_BARREL = 8,
    EXTRACT_VIAL = 9,
    EXTRACT_STILL_VIAL = 10,
    GENERATED = 11,
    THREAD = 12,
    MILL = 13,
    unk_14 = 14,
    unk_15 = 15,
    unk_16 = 16,
    unk_17 = 17,
    unk_18 = 18,
    unk_19 = 19,
    WET = 20,
    DRY = 21,
    BIOME_MOUNTAIN = 22,
    BIOME_GLACIER = 23,
    BIOME_TUNDRA = 24,
    BIOME_SWAMP_TEMPERATE_FRESHWATER = 25,
    BIOME_SWAMP_TEMPERATE_SALTWATER = 26,
    BIOME_MARSH_TEMPERATE_FRESHWATER = 27,
    BIOME_MARSH_TEMPERATE_SALTWATER = 28,
    BIOME_SWAMP_TROPICAL_FRESHWATER = 29,
    BIOME_SWAMP_TROPICAL_SALTWATER = 30,
    BIOME_SWAMP_MANGROVE = 31,
    BIOME_MARSH_TROPICAL_FRESHWATER = 32,
    BIOME_MARSH_TROPICAL_SALTWATER = 33,
    BIOME_FOREST_TAIGA = 34,
    BIOME_FOREST_TEMPERATE_CONIFER = 35,
    BIOME_FOREST_TEMPERATE_BROADLEAF = 36,
    BIOME_FOREST_TROPICAL_CONIFER = 37,
    BIOME_FOREST_TROPICAL_DRY_BROADLEAF = 38,
    BIOME_FOREST_TROPICAL_MOIST_BROADLEAF = 39,
    BIOME_GRASSLAND_TEMPERATE = 40,
    BIOME_SAVANNA_TEMPERATE = 41,
    BIOME_SHRUBLAND_TEMPERATE = 42,
    BIOME_GRASSLAND_TROPICAL = 43,
    BIOME_SAVANNA_TROPICAL = 44,
    BIOME_SHRUBLAND_TROPICAL = 45,
    BIOME_DESERT_BADLAND = 46,
    BIOME_DESERT_ROCK = 47,
    BIOME_DESERT_SAND = 48,
    BIOME_OCEAN_TROPICAL = 49,
    BIOME_OCEAN_TEMPERATE = 50,
    BIOME_OCEAN_ARCTIC = 51,
    BIOME_POOL_TEMPERATE_FRESHWATER = 52,
    BIOME_SUBTERRANEAN_WATER = 53,
    BIOME_SUBTERRANEAN_CHASM = 54,
    BIOME_SUBTERRANEAN_LAVA = 55,
    GOOD = 56,
    EVIL = 57,
    SAVAGE = 58,
    BIOME_POOL_TEMPERATE_BRACKISHWATER = 59,
    BIOME_POOL_TEMPERATE_SALTWATER = 60,
    BIOME_POOL_TROPICAL_FRESHWATER = 61,
    BIOME_POOL_TROPICAL_BRACKISHWATER = 62,
    BIOME_POOL_TROPICAL_SALTWATER = 63,
    BIOME_LAKE_TEMPERATE_FRESHWATER = 64,
    BIOME_LAKE_TEMPERATE_BRACKISHWATER = 65,
    BIOME_LAKE_TEMPERATE_SALTWATER = 66,
    BIOME_LAKE_TROPICAL_FRESHWATER = 67,
    BIOME_LAKE_TROPICAL_BRACKISHWATER = 68,
    BIOME_LAKE_TROPICAL_SALTWATER = 69,
    BIOME_RIVER_TEMPERATE_FRESHWATER = 70,
    BIOME_RIVER_TEMPERATE_BRACKISHWATER = 71,
    BIOME_RIVER_TEMPERATE_SALTWATER = 72,
    BIOME_RIVER_TROPICAL_FRESHWATER = 73,
    BIOME_RIVER_TROPICAL_BRACKISHWATER = 74,
    BIOME_RIVER_TROPICAL_SALTWATER = 75,
    TWIGS_SIDE_BRANCHES = 76,
    SAPLING = 77,
    TREE = 78,
    GRASS = 79,
    TWIGS_ABOVE_BRANCHES = 80,
    TWIGS_BELOW_BRANCHES = 81,
    TWIGS_SIDE_HEAVY_BRANCHES = 82,
    TWIGS_ABOVE_HEAVY_BRANCHES = 83,
    TWIGS_BELOW_HEAVY_BRANCHES = 84,
    TWIGS_SIDE_TRUNK = 85,
    TWIGS_ABOVE_TRUNK = 86,
    TWIGS_BELOW_TRUNK = 87,
}

---@class plant_raw_material_defs
---@field type any[]
---@field idx integer[]
---@field str any[]

---@class plant_raw_colors
---@field picked_color integer[]
---@field dead_picked_color integer[]
---@field shrub_color integer[]
---@field dead_shrub_color integer[]
---@field seed_color integer[]
---@field tree_color integer[]
---@field dead_tree_color integer[]
---@field sapling_color integer[]
---@field dead_sapling_color integer[]
---@field grass_colors_0 integer[]
---@field grass_colors_1 integer[]
---@field grass_colors_2 integer[]

---@class plant_raw_tiles
---@field picked_tile integer
---@field dead_picked_tile integer
---@field shrub_tile integer
---@field dead_shrub_tile integer
---@field tree_tile integer unused
---@field dead_tree_tile integer unused
---@field sapling_tile integer
---@field dead_sapling_tile integer
---@field grass_tiles integer[]
---@field alt_grass_tiles integer[]
---@field tree_tiles integer[]
---@field unk_v50_1 integer[]

---@class plant_raw
---@field id string
---@field index integer since v0.40.01
---@field raws string[] since v0.40.01
---@field flags plant_raw_flags[]
---@field name string
---@field name_plural string
---@field adj string
---@field seed_singular string
---@field seed_plural string
---@field leaves_singular string unused
---@field leaves_plural string unused
---@field source_hfid integer
---@field unk_v4201_1 integer since v0.42.01
---@field unk1 integer
---@field unk2 integer
---@field tiles plant_raw_tiles
---@field growdur integer
---@field value integer
---@field colors plant_raw_colors
---@field alt_period integer[]
---@field shrub_drown_level integer
---@field tree_drown_level integer
---@field sapling_drown_level integer
---@field frequency integer
---@field clustersize integer
---@field prefstring string[]
---@field material material[]
---@field material_defs plant_raw_material_defs
---@field underground_depth_min integer
---@field underground_depth_max integer
---@field growths plant_growth[]
---@field root_name string
---@field trunk_name string
---@field heavy_branch_name string
---@field light_branch_name string
---@field twig_name string
---@field cap_name string
---@field trunk_period integer
---@field heavy_branch_density integer
---@field light_branch_density integer
---@field max_trunk_height integer
---@field heavy_branch_radius integer
---@field light_branch_radius integer
---@field trunk_branching integer
---@field max_trunk_diameter integer
---@field trunk_width_period integer
---@field cap_period integer
---@field cap_radius integer
---@field root_density integer
---@field root_radius integer
---@field stockpile_growths integer[] indices of edible growths that are marked with STOCKPILE_PLANT_GROWTH
---@field stockpile_growth_flags any[]
df.plant_raw = nil

---@enum plant_material_def
df.plant_material_def = {
    basic_mat = 0,
    tree = 1,
    drink = 2,
    seed = 3,
    thread = 4,
    mill = 5,
    extract_vial = 6,
    extract_barrel = 7,
    extract_still_vial = 8,
}

---@class plant_growth
---@field id string
---@field name string
---@field name_plural string
---@field str_growth_item string[]
---@field item_type item_type
---@field item_subtype integer
---@field mat_type integer
---@field mat_index integer
---@field prints plant_growth_print[]
---@field unk_v50_1 integer
---@field unk_v50_2 integer
---@field unk_v50_3 integer
---@field unk_v50_4 integer
---@field unk_v50_5 integer
---@field timing_1 integer
---@field timing_2 integer
---@field locations bitfield
---@field density integer
---@field behavior bitfield
---@field trunk_height_perc_1 integer
---@field trunk_height_perc_2 integer
df.plant_growth = nil

---@class plant_growth_print
---@field priority integer final token in list
---@field tile_growth integer
---@field tile_item integer
---@field color integer[]
---@field timing_start integer
---@field timing_end integer
df.plant_growth_print = nil


