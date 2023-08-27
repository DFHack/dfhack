-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.

---@meta

---@class coord2d
---@field x integer
---@field y integer
df.coord2d = nil

---@class coord2d_path
---@field x integer[]
---@field y integer[]
df.coord2d_path = nil

---@class coord
---@field x integer
---@field y integer
---@field z integer
---@field unnamed_coord_4 coord2d
df.coord = nil

---@class coord_path
---@field x integer[]
---@field y integer[]
---@field z integer[]
df.coord_path = nil

---@enum tile_traffic
df.tile_traffic = {
    Normal = 0,
    Low = 1,
    High = 2,
    Restricted = 3,
}

---@enum tile_dig_designation
df.tile_dig_designation = {
    No = 0, -- no designation
    Default = 1, -- dig walls, remove stairs and ramps, gather plants, fell trees
    UpDownStair = 2,
    Channel = 3,
    Ramp = 4,
    DownStair = 5,
    UpStair = 6,
}

---@enum tile_liquid
df.tile_liquid = {
    Water = 0,
    Magma = 1,
}

---@alias tile_designation bitfield

---@enum tile_building_occ
df.tile_building_occ = {
    None = 0, -- no building
    Planned = 1, -- nothing built yet
    Passable = 2,
    Obstacle = 3, -- workshop tile; ~fortification
    Well = 4,
    Floored = 5, -- depot; lowered bridge
    Impassable = 6,
    Dynamic = 7, -- doors, grates, etc
}

---@alias tile_occupancy bitfield

---@alias block_flags bitfield

---@alias z_level_flags bitfield

---@enum tile_liquid_flow_dir
df.tile_liquid_flow_dir = {
    none = 0,
    south = 1,
    east = 2,
    northeast = 3,
    west = 4,
    northwest = 5,
    southeast = 6,
    southwest = 7,
    inv_8 = 8,
    inv_9 = 9,
    north = 10,
    inv_b = 11,
    inv_c = 12,
    inv_d = 13,
    inv_e = 14,
    inv_f = 15,
}

---@alias tile_liquid_flow bitfield

---@class tile_bitmask
---@field bits any[]
df.tile_bitmask = nil

---@class block_burrow
---@field id integer
---@field tile_bitmask tile_bitmask
---@field link block_burrow_link
df.block_burrow = nil

---@alias block_burrow_link block_burrow[]

---@class map_block
---@field flags bitfield
---@field block_events block_square_event[]
---@field block_burrows block_burrow_link
---@field local_feature integer index into world_data.region_map
---@field global_feature integer
---@field unk2 integer
---@field layer_depth integer uninitialized
---@field dsgn_check_cooldown integer
---@field default_liquid bitfield
---@field items any[]
---@field flows flow_info[]
---@field flow_pool flow_reuse_pool
---@field map_pos coord
---@field region_pos coord2d
---@field tiletype any[]
---@field designation any[]
---@field occupancy any[]
---@field fog_of_war any[] for adventure mode
---@field path_cost any[]
---@field path_tag any[]
---@field walkable any[]
---@field map_edge_distance any[]
---@field temperature_1 any[]
---@field temperature_2 any[]
---@field unk13 any[]
---@field liquid_flow any[]
---@field region_offset any[]
df.map_block = nil

---@alias cave_column_link cave_column[]

---@param file file_compressorst
function df.cave_column.write_file(file) end

---@param file file_compressorst
---@param loadversion save_version
function df.cave_column.read_file(file, loadversion) end

---@class cave_column
---@field unk_z1 integer
---@field unk_z2 integer
---@field unk_3 integer
---@field unk_4 bitfield
df.cave_column = nil

---@param file file_compressorst
function df.cave_column_rectangle.write_file(file) end

---@param file file_compressorst
---@param loadversion save_version
function df.cave_column_rectangle.read_file(file, loadversion) end

---@class cave_column_rectangle
---@field unk_1 integer
---@field unk_x1 integer
---@field unk_y1 integer
---@field unk_x2 integer
---@field unk_y2 integer
---@field z_shift integer
---@field unk_6 coord_path
---@field unk_7 bitfield
df.cave_column_rectangle = nil

---@class map_block_column
---@field sink_level integer water at or above this level sinks into aquifer tiles
---@field beach_level integer water at this level disappears if above more water
---@field ground_level integer for coloring unallocated blocks
---@field unmined_glyphs any[]
---@field z_base integer
---@field cave_columns any[]
---@field column_rectangles cave_column_rectangle[]
---@field z_shift integer seems to be 0 originally, but updated when map is shifted
---@field flags any[] 0 process cave columns for caveins
---@field elevation any[] since v0.40.01
---@field map_pos coord2d top left in tiles
---@field unk_c3c integer uninitialized
---@field region_pos coord2d
---@field plants plant[] Only populated for the top left column in each mid level tile
df.map_block_column = nil

---@enum block_square_event_type
df.block_square_event_type = {
    mineral = 0,
    frozen_liquid = 1,
    world_construction = 2,
    material_spatter = 3,
    grass = 4,
    spoor = 5,
    item_spatter = 6,
    designation_priority = 7,
}

---@return block_square_event_type
function df.block_square_event.getType() end

---@param file file_compressorst
function df.block_square_event.write_file(file) end

---@param file file_compressorst
---@param loadversion save_version
function df.block_square_event.read_file(file, loadversion) end

---@return boolean
function df.block_square_event.isEmpty() end

---@param x integer
---@param y integer
---@param temperature integer
function df.block_square_event.checkTemperature(x, y, temperature) end

---@class block_square_event
df.block_square_event = nil

---@class block_square_event_mineralst
-- inherited from block_square_event
-- end block_square_event
---@field inorganic_mat integer
---@field tile_bitmask tile_bitmask
---@field flags bitfield
df.block_square_event_mineralst = nil

---@class block_square_event_frozen_liquidst
-- inherited from block_square_event
-- end block_square_event
---@field tiles any[]
---@field liquid_type any[]
df.block_square_event_frozen_liquidst = nil

---@class block_square_event_world_constructionst
-- inherited from block_square_event
-- end block_square_event
---@field construction_id integer
---@field tile_bitmask tile_bitmask
df.block_square_event_world_constructionst = nil

---@class block_square_event_material_spatterst
-- inherited from block_square_event
-- end block_square_event
---@field mat_type integer
---@field mat_index integer
---@field mat_state matter_state
---@field amount any[]
---@field min_temperature integer
---@field max_temperature integer
df.block_square_event_material_spatterst = nil

---@class block_square_event_grassst
-- inherited from block_square_event
-- end block_square_event
---@field plant_index integer
---@field amount any[]
df.block_square_event_grassst = nil

---@class block_square_event_spoorst
-- inherited from block_square_event
-- end block_square_event
---@field flags any[]
---@field unk_2 any[]
---@field unk_3 any[]
---@field race any[]
---@field caste any[]
---@field age any[] in half-seconds
---@field year integer
---@field year_tick integer
df.block_square_event_spoorst = nil

---@class block_square_event_item_spatterst
-- inherited from block_square_event
-- end block_square_event
---@field item_type item_type
---@field item_subtype integer
---@field mattype integer
---@field matindex integer
---@field unk1 integer
---@field amount any[]
---@field unk2 any[]
---@field temp1 integer
---@field temp2 integer
df.block_square_event_item_spatterst = nil

---@class block_square_event_designation_priorityst
-- inherited from block_square_event
-- end block_square_event
---@field priority any[]
df.block_square_event_designation_priorityst = nil

---@enum feature_type
df.feature_type = {
    outdoor_river = 0,
    cave = 1,
    pit = 2,
    magma_pool = 3,
    volcano = 4,
    deep_special_tube = 5,
    deep_surface_portal = 6,
    subterranean_from_layer = 7,
    magma_core_from_layer = 8,
    underworld_from_layer = 9,
}

---@return feature_type
function df.feature.getType() end

---@param file file_compressorst
function df.feature.write_file(file) end

---@param file file_compressorst
---@param loadversion save_version
function df.feature.read_file(file, loadversion) end

---@param x integer
---@param y integer
---@param z integer
function df.feature.shiftCoords(x, y, z) end

function df.feature.unnamed_method() end

---@class feature
---@field population world_population[]
---@field irritation_level integer divide by 10k for attack chance, max 100k
---@field irritation_attacks integer maxes at 10?
---@field embark_pos coord2d_path
---@field min_map_z integer[]
---@field max_map_z integer[]
df.feature = nil

---@class feature_outdoor_riverst
-- inherited from feature
---@field population world_population[]
---@field irritation_level integer divide by 10k for attack chance, max 100k
---@field irritation_attacks integer maxes at 10?
---@field embark_pos coord2d_path
---@field min_map_z integer[]
---@field max_map_z integer[]
-- end feature
df.feature_outdoor_riverst = nil

---@class feature_cavest
-- inherited from feature
---@field population world_population[]
---@field irritation_level integer divide by 10k for attack chance, max 100k
---@field irritation_attacks integer maxes at 10?
---@field embark_pos coord2d_path
---@field min_map_z integer[]
---@field max_map_z integer[]
-- end feature
df.feature_cavest = nil

---@class feature_pitst
-- inherited from feature
---@field population world_population[]
---@field irritation_level integer divide by 10k for attack chance, max 100k
---@field irritation_attacks integer maxes at 10?
---@field embark_pos coord2d_path
---@field min_map_z integer[]
---@field max_map_z integer[]
-- end feature
df.feature_pitst = nil

---@class feature_magma_poolst
-- inherited from feature
---@field population world_population[]
---@field irritation_level integer divide by 10k for attack chance, max 100k
---@field irritation_attacks integer maxes at 10?
---@field embark_pos coord2d_path
---@field min_map_z integer[]
---@field max_map_z integer[]
-- end feature
---@field magma_fill_z integer
df.feature_magma_poolst = nil

---@class feature_volcanost
-- inherited from feature
---@field population world_population[]
---@field irritation_level integer divide by 10k for attack chance, max 100k
---@field irritation_attacks integer maxes at 10?
---@field embark_pos coord2d_path
---@field min_map_z integer[]
---@field max_map_z integer[]
-- end feature
---@field magma_fill_z integer
df.feature_volcanost = nil

---@class feature_deep_special_tubest
-- inherited from feature
---@field population world_population[]
---@field irritation_level integer divide by 10k for attack chance, max 100k
---@field irritation_attacks integer maxes at 10?
---@field embark_pos coord2d_path
---@field min_map_z integer[]
---@field max_map_z integer[]
-- end feature
df.feature_deep_special_tubest = nil

---@class feature_deep_surface_portalst
-- inherited from feature
---@field population world_population[]
---@field irritation_level integer divide by 10k for attack chance, max 100k
---@field irritation_attacks integer maxes at 10?
---@field embark_pos coord2d_path
---@field min_map_z integer[]
---@field max_map_z integer[]
-- end feature
df.feature_deep_surface_portalst = nil

---@class feature_subterranean_from_layerst
-- inherited from feature
---@field population world_population[]
---@field irritation_level integer divide by 10k for attack chance, max 100k
---@field irritation_attacks integer maxes at 10?
---@field embark_pos coord2d_path
---@field min_map_z integer[]
---@field max_map_z integer[]
-- end feature
df.feature_subterranean_from_layerst = nil

---@class feature_magma_core_from_layerst
-- inherited from feature
---@field population world_population[]
---@field irritation_level integer divide by 10k for attack chance, max 100k
---@field irritation_attacks integer maxes at 10?
---@field embark_pos coord2d_path
---@field min_map_z integer[]
---@field max_map_z integer[]
-- end feature
df.feature_magma_core_from_layerst = nil

---@class feature_underworld_from_layerst
-- inherited from feature
---@field population world_population[]
---@field irritation_level integer divide by 10k for attack chance, max 100k
---@field irritation_attacks integer maxes at 10?
---@field embark_pos coord2d_path
---@field min_map_z integer[]
---@field max_map_z integer[]
-- end feature
df.feature_underworld_from_layerst = nil

---@enum feature_init_flags
df.feature_init_flags = {
    unk_0 = 0,
    unk_1 = 1,
    unk_2 = 2,
    Discovered = 3,
}

---@enum layer_type
df.layer_type = {
    Surface = -1,
    Cavern1 = 0,
    Cavern2 = 1,
    Cavern3 = 2,
    MagmaSea = 3,
    Underworld = 4,
}

---@return feature_type
function df.feature_init.getType() end

---@param file file_compressorst
---@param include_feature boolean
function df.feature_init.write_file(file, include_feature) end

---@param file file_compressorst
---@param loadversion save_version
---@param include_feature boolean
function df.feature_init.read_file(file, loadversion, include_feature) end

---@return feature
function df.feature_init.createFeature() end

---@return feature
function df.feature_init.recreateFeature() end -- destroyFeature(), then createFeature()

function df.feature_init.destroyFeature() end

---@return feature
function df.feature_init.getFeature() end

---@param mat_type integer
---@param mat_index integer
function df.feature_init.getMaterial(mat_type, mat_index) end

---@return boolean
function df.feature_init.unnamed_method() end

---@return boolean
function df.feature_init.unnamed_method() end

---@return boolean
function df.feature_init.unnamed_method() end

---@return boolean
function df.feature_init.unnamed_method() end

---@param foreground integer
---@param background integer
---@param bright integer
function df.feature_init.getColor(foreground, background, bright) end

---@param name string
function df.feature_init.getName(name) end

---@return boolean
function df.feature_init.isWater() end

---@return boolean
function df.feature_init.isSubterranean() end

---@return boolean
function df.feature_init.isMagma() end

---@return boolean
function df.feature_init.isChasm() end

---@return boolean
function df.feature_init.isLayer() end

---@return boolean
function df.feature_init.unnamed_method() end

---@return integer
function df.feature_init.getLayer() end

---@class feature_init
---@field flags feature_init_flags[]
---@field alterations feature_alteration[]
---@field start_x integer
---@field start_y integer
---@field end_x integer
---@field end_y integer
---@field start_depth layer_type
---@field end_depth layer_type
df.feature_init = nil

---@class feature_init_outdoor_riverst
-- inherited from feature_init
---@field flags feature_init_flags[]
---@field alterations feature_alteration[]
---@field start_x integer
---@field start_y integer
---@field end_x integer
---@field end_y integer
---@field start_depth layer_type
---@field end_depth layer_type
-- end feature_init
---@field feature feature_outdoor_riverst
df.feature_init_outdoor_riverst = nil

---@class feature_init_cavest
-- inherited from feature_init
---@field flags feature_init_flags[]
---@field alterations feature_alteration[]
---@field start_x integer
---@field start_y integer
---@field end_x integer
---@field end_y integer
---@field start_depth layer_type
---@field end_depth layer_type
-- end feature_init
---@field feature feature_cavest
df.feature_init_cavest = nil

---@class feature_init_pitst
-- inherited from feature_init
---@field flags feature_init_flags[]
---@field alterations feature_alteration[]
---@field start_x integer
---@field start_y integer
---@field end_x integer
---@field end_y integer
---@field start_depth layer_type
---@field end_depth layer_type
-- end feature_init
---@field feature feature_pitst
df.feature_init_pitst = nil

---@class feature_init_magma_poolst
-- inherited from feature_init
---@field flags feature_init_flags[]
---@field alterations feature_alteration[]
---@field start_x integer
---@field start_y integer
---@field end_x integer
---@field end_y integer
---@field start_depth layer_type
---@field end_depth layer_type
-- end feature_init
---@field feature feature_magma_poolst
df.feature_init_magma_poolst = nil

---@class feature_init_volcanost
-- inherited from feature_init
---@field flags feature_init_flags[]
---@field alterations feature_alteration[]
---@field start_x integer
---@field start_y integer
---@field end_x integer
---@field end_y integer
---@field start_depth layer_type
---@field end_depth layer_type
-- end feature_init
---@field feature feature_volcanost
df.feature_init_volcanost = nil

---@class feature_init_deep_special_tubest
-- inherited from feature_init
---@field flags feature_init_flags[]
---@field alterations feature_alteration[]
---@field start_x integer
---@field start_y integer
---@field end_x integer
---@field end_y integer
---@field start_depth layer_type
---@field end_depth layer_type
-- end feature_init
---@field mat_type integer
---@field mat_index integer
---@field feature feature_deep_special_tubest
df.feature_init_deep_special_tubest = nil

---@class feature_init_deep_surface_portalst
-- inherited from feature_init
---@field flags feature_init_flags[]
---@field alterations feature_alteration[]
---@field start_x integer
---@field start_y integer
---@field end_x integer
---@field end_y integer
---@field start_depth layer_type
---@field end_depth layer_type
-- end feature_init
---@field mat_type integer
---@field mat_index integer
---@field feature feature_deep_surface_portalst
df.feature_init_deep_surface_portalst = nil

---@class feature_init_subterranean_from_layerst
-- inherited from feature_init
---@field flags feature_init_flags[]
---@field alterations feature_alteration[]
---@field start_x integer
---@field start_y integer
---@field end_x integer
---@field end_y integer
---@field start_depth layer_type
---@field end_depth layer_type
-- end feature_init
---@field layer integer
---@field feature feature_subterranean_from_layerst
df.feature_init_subterranean_from_layerst = nil

---@class feature_init_magma_core_from_layerst
-- inherited from feature_init
---@field flags feature_init_flags[]
---@field alterations feature_alteration[]
---@field start_x integer
---@field start_y integer
---@field end_x integer
---@field end_y integer
---@field start_depth layer_type
---@field end_depth layer_type
-- end feature_init
---@field layer integer
---@field feature feature_magma_core_from_layerst
df.feature_init_magma_core_from_layerst = nil

---@class feature_init_underworld_from_layerst
-- inherited from feature_init
---@field flags feature_init_flags[]
---@field alterations feature_alteration[]
---@field start_x integer
---@field start_y integer
---@field end_x integer
---@field end_y integer
---@field start_depth layer_type
---@field end_depth layer_type
-- end feature_init
---@field layer integer
---@field mat_type integer
---@field mat_index integer
---@field feature feature_underworld_from_layerst
df.feature_init_underworld_from_layerst = nil

---@enum feature_alteration_type
df.feature_alteration_type = {
    new_pop_max = 0,
    new_lava_fill_z = 1,
}

---@return feature_alteration_type
function df.feature_alteration.getType() end

---@param file file_compressorst
function df.feature_alteration.write_file(file) end

---@param file file_compressorst
---@param loadversion save_version
function df.feature_alteration.read_file(file, loadversion) end

---@class feature_alteration
df.feature_alteration = nil

---@class feature_alteration_new_pop_maxst
-- inherited from feature_alteration
-- end feature_alteration
---@field unk_1 integer
---@field unk_2 integer
df.feature_alteration_new_pop_maxst = nil

---@class feature_alteration_new_lava_fill_zst
-- inherited from feature_alteration
-- end feature_alteration
---@field magma_fill_z integer
df.feature_alteration_new_lava_fill_zst = nil

---@enum world_construction_type
df.world_construction_type = {
    ROAD = 0,
    TUNNEL = 1,
    BRIDGE = 2,
    WALL = 3,
}

---@return world_construction_type
function df.world_construction_square.getType() end

---@param file file_compressorst
function df.world_construction_square.write_file(file) end

---@param file file_compressorst
---@param loadversion save_version
function df.world_construction_square.read_file(file, loadversion) end

---@class world_construction_square
---@field region_pos coord2d
---@field construction_id integer
---@field embark_x integer[]
---@field embark_y integer[]
---@field embark_unk integer[]
---@field embark_z integer[]
df.world_construction_square = nil

---@class world_construction_square_roadst
-- inherited from world_construction_square
---@field region_pos coord2d
---@field construction_id integer
---@field embark_x integer[]
---@field embark_y integer[]
---@field embark_unk integer[]
---@field embark_z integer[]
-- end world_construction_square
---@field item_type item_type
---@field item_subtype integer
---@field mat_type integer
---@field mat_index integer
df.world_construction_square_roadst = nil

---@class world_construction_square_tunnelst
-- inherited from world_construction_square
---@field region_pos coord2d
---@field construction_id integer
---@field embark_x integer[]
---@field embark_y integer[]
---@field embark_unk integer[]
---@field embark_z integer[]
-- end world_construction_square
df.world_construction_square_tunnelst = nil

---@class world_construction_square_bridgest
-- inherited from world_construction_square
---@field region_pos coord2d
---@field construction_id integer
---@field embark_x integer[]
---@field embark_y integer[]
---@field embark_unk integer[]
---@field embark_z integer[]
-- end world_construction_square
---@field road_id integer guess
---@field item_type item_type
---@field item_subtype integer
---@field mat_type integer
---@field mat_index integer
df.world_construction_square_bridgest = nil

---@class world_construction_square_wallst
-- inherited from world_construction_square
---@field region_pos coord2d
---@field construction_id integer
---@field embark_x integer[]
---@field embark_y integer[]
---@field embark_unk integer[]
---@field embark_z integer[]
-- end world_construction_square
---@field item_type item_type
---@field item_subtype integer
---@field mat_type integer
---@field mat_index integer
df.world_construction_square_wallst = nil

---@return world_construction_type
function df.world_construction.getType() end

---@return language_name
function df.world_construction.getName() end

---@param file file_compressorst
function df.world_construction.write_file(file) end

---@param file file_compressorst
---@param loadversion save_version
function df.world_construction.read_file(file, loadversion) end

---@class world_construction
---@field id integer
---@field square_obj world_construction_square[]
---@field square_pos coord2d_path
df.world_construction = nil

---@class world_construction_roadst
-- inherited from world_construction
---@field id integer
---@field square_obj world_construction_square[]
---@field square_pos coord2d_path
-- end world_construction
---@field name language_name
df.world_construction_roadst = nil

---@class world_construction_tunnelst
-- inherited from world_construction
---@field id integer
---@field square_obj world_construction_square[]
---@field square_pos coord2d_path
-- end world_construction
---@field name language_name
df.world_construction_tunnelst = nil

---@class world_construction_bridgest
-- inherited from world_construction
---@field id integer
---@field square_obj world_construction_square[]
---@field square_pos coord2d_path
-- end world_construction
---@field name language_name
df.world_construction_bridgest = nil

---@class world_construction_wallst
-- inherited from world_construction
---@field id integer
---@field square_obj world_construction_square[]
---@field square_pos coord2d_path
-- end world_construction
---@field name language_name
df.world_construction_wallst = nil

---@enum biome_type
df.biome_type = {
    MOUNTAIN = 0,
    GLACIER = 1,
    TUNDRA = 2,
    SWAMP_TEMPERATE_FRESHWATER = 3,
    SWAMP_TEMPERATE_SALTWATER = 4,
    MARSH_TEMPERATE_FRESHWATER = 5,
    MARSH_TEMPERATE_SALTWATER = 6,
    SWAMP_TROPICAL_FRESHWATER = 7,
    SWAMP_TROPICAL_SALTWATER = 8,
    SWAMP_MANGROVE = 9,
    MARSH_TROPICAL_FRESHWATER = 10,
    MARSH_TROPICAL_SALTWATER = 11,
    FOREST_TAIGA = 12,
    FOREST_TEMPERATE_CONIFER = 13,
    FOREST_TEMPERATE_BROADLEAF = 14,
    FOREST_TROPICAL_CONIFER = 15,
    FOREST_TROPICAL_DRY_BROADLEAF = 16,
    FOREST_TROPICAL_MOIST_BROADLEAF = 17,
    GRASSLAND_TEMPERATE = 18,
    SAVANNA_TEMPERATE = 19,
    SHRUBLAND_TEMPERATE = 20,
    GRASSLAND_TROPICAL = 21,
    SAVANNA_TROPICAL = 22,
    SHRUBLAND_TROPICAL = 23,
    DESERT_BADLAND = 24,
    DESERT_ROCK = 25,
    DESERT_SAND = 26,
    OCEAN_TROPICAL = 27,
    OCEAN_TEMPERATE = 28,
    OCEAN_ARCTIC = 29,
    POOL_TEMPERATE_FRESHWATER = 30,
    POOL_TEMPERATE_BRACKISHWATER = 31,
    POOL_TEMPERATE_SALTWATER = 32,
    POOL_TROPICAL_FRESHWATER = 33,
    POOL_TROPICAL_BRACKISHWATER = 34,
    POOL_TROPICAL_SALTWATER = 35,
    LAKE_TEMPERATE_FRESHWATER = 36,
    LAKE_TEMPERATE_BRACKISHWATER = 37,
    LAKE_TEMPERATE_SALTWATER = 38,
    LAKE_TROPICAL_FRESHWATER = 39,
    LAKE_TROPICAL_BRACKISHWATER = 40,
    LAKE_TROPICAL_SALTWATER = 41,
    RIVER_TEMPERATE_FRESHWATER = 42,
    RIVER_TEMPERATE_BRACKISHWATER = 43,
    RIVER_TEMPERATE_SALTWATER = 44,
    RIVER_TROPICAL_FRESHWATER = 45,
    RIVER_TROPICAL_BRACKISHWATER = 46,
    RIVER_TROPICAL_SALTWATER = 47,
    SUBTERRANEAN_WATER = 48,
    SUBTERRANEAN_CHASM = 49,
    SUBTERRANEAN_LAVA = 50,
}

---@alias construction_flags bitfield

---@class construction
---@field pos coord
---@field item_type item_type
---@field item_subtype integer
---@field mat_type integer
---@field mat_index integer
---@field flags bitfield
---@field original_tile tiletype
df.construction = nil

---@enum flow_type
df.flow_type = {
    Miasma = 0,
    Steam = 1, -- only if mat_type=1
    Mist = 2,
    MaterialDust = 3,
    MagmaMist = 4,
    Smoke = 5,
    Dragonfire = 6,
    Fire = 7,
    Web = 8,
    MaterialGas = 9,
    MaterialVapor = 10,
    OceanWave = 11,
    SeaFoam = 12,
    ItemCloud = 13,
}

---@class flow_info
---@field type flow_type
---@field mat_type integer
---@field mat_index integer
---@field density integer
---@field pos coord
---@field dest coord
---@field expanding boolean
---@field reuse boolean
---@field guide_id integer
df.flow_info = nil

---@class flow_reuse_pool
---@field reuse_idx integer
---@field flags bitfield
df.flow_reuse_pool = nil

---@enum flow_guide_type
df.flow_guide_type = {
    TrailingFlow = 0,
    ItemCloud = 1,
}

---@return flow_guide_type
function df.flow_guide.getType() end

---@param x integer
---@param y integer
---@param z integer
function df.flow_guide.shiftCoords(x, y, z) end

---@param file file_compressorst
function df.flow_guide.write_file(file) end

---@param file file_compressorst
---@param loadversion save_version
function df.flow_guide.read_file(file, loadversion) end

---@param arg_0 flow_info
function df.flow_guide.unnamed_method(arg_0) end

---@class flow_guide
---@field id integer
---@field unk_8 integer
df.flow_guide = nil

---@class flow_guide_trailing_flowst
-- inherited from flow_guide
---@field id integer
---@field unk_8 integer
-- end flow_guide
---@field unk_1 coord[]
df.flow_guide_trailing_flowst = nil

---@class flow_guide_item_cloudst
-- inherited from flow_guide
---@field id integer
---@field unk_8 integer
-- end flow_guide
---@field item_type item_type
---@field item_subtype integer
---@field mattype integer
---@field matindex integer
---@field unk_18 integer
---@field unk_1c integer
---@field unk_1 coord[]
df.flow_guide_item_cloudst = nil

---@class effect_info
---@field id integer assigned during Save
---@field job job
---@field type integer 2 = falling into chasm
---@field foreground integer
---@field background integer
---@field bright integer
---@field pos coord
---@field timer integer
df.effect_info = nil

---@enum region_block_event_type
df.region_block_event_type = {
    SphereField = 0,
}

---@return region_block_event_type
function df.region_block_eventst.getType() end

---@param file file_compressorst
function df.region_block_eventst.write_file(file) end

---@param file file_compressorst
---@param loadversion save_version
function df.region_block_eventst.read_file(file, loadversion) end

---@return boolean
function df.region_block_eventst.unnamed_method() end

---@class region_block_eventst
df.region_block_eventst = nil

---@class region_block_event_sphere_fieldst
-- inherited from region_block_eventst
-- end region_block_eventst
---@field unk_1 integer[]
df.region_block_event_sphere_fieldst = nil


