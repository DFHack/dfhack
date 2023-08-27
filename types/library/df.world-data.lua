-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.

---@meta

---@class world_site_unk130
---@field index integer
---@field unk_4 any[]
df.world_site_unk130 = nil

---@enum world_population_type
df.world_population_type = {
    Animal = 0,
    Vermin = 1,
    Unk2 = 2,
    VerminInnumerable = 3,
    ColonyInsect = 4,
    Tree = 5,
    Grass = 6,
    Bush = 7,
}

---@class embark_note
---@field tile integer
---@field fg_color integer
---@field bg_color integer
---@field name string
---@field pos coord2d
---@field left integer
---@field right integer
---@field top integer
---@field bottom integer
df.embark_note = nil

---@class world_population_ref
---@field region_x integer
---@field region_y integer
---@field feature_idx integer
---@field cave_id integer
---@field unk_28 integer
---@field population_idx integer
---@field depth layer_type Doesn't look correct. See -1, 0, 41, 172, 508, and 686 with critters visible in all caverns. Some dead, but the dorf on the surface isn't
df.world_population_ref = nil

---@class local_population
---@field type world_population_type
---@field unnamed_local_population_2 race | plant
---@field quantity integer
---@field quantity2 integer since v0.40.01
---@field flags bitfield
---@field population world_population_ref
---@field wp_unk_10 integer
---@field wp_unk_18 integer
---@field wp_unk_1c integer only set on subset of animals (including vermin). None seen on fresh embark
---@field unk_v47_1 integer set on same animals as wp_unk_1c and only seen 0, since v0.47.01
df.local_population = nil

---@class world_population
---@field type world_population_type
---@field unnamed_world_population_2 race | plant
---@field count_min integer
---@field count_max integer
---@field unk_c integer since v0.40.01
---@field owner integer
---@field unk_10 integer
---@field unk_14 integer
---@field unk_18 integer since v0.34.01
---@field unk_1c integer since v0.34.01
---@field unk_20 integer since v0.47.01
df.world_population = nil

---@class world_landmass
---@field name language_name
---@field index integer
---@field area integer
---@field min_x integer since v0.40.01
---@field max_x integer since v0.40.01
---@field min_y integer since v0.40.01
---@field max_y integer since v0.40.01
---@field unk_74 integer[]
---@field unk_84 integer[]
df.world_landmass = nil

---@enum world_region_type
df.world_region_type = {
    Swamp = 0,
    Desert = 1,
    Jungle = 2,
    Mountains = 3,
    Ocean = 4,
    Lake = 5,
    Glacier = 6,
    Tundra = 7,
    Steppe = 8,
    Hills = 9, -- Steppe and Hills share the same set of biomes, differing only in Drainage
}

---@class world_region
---@field name language_name
---@field index integer
---@field type world_region_type
---@field region_coords coord2d_path
---@field size integer Number of tiles in the region
---@field unk_98 integer
---@field unk_9c integer
---@field unk_a0 integer
---@field unk_a4 integer
---@field population world_population[]
---@field biome_tile_counts any[]
---@field tree_biomes any[]
---@field tree_tiles_1 any[]
---@field tree_tiles_2 any[]
---@field tree_tiles_good any[]
---@field tree_tiles_evil any[]
---@field tree_tiles_savage any[]
---@field dead_percentage integer % vegetation dead on embark. The number increases during world gen history, with the new ones always at 100%
---@field unk_1e5 boolean Probably optionally set only on good and evil regions during world gen. Number set increases during world gen history and can affect neutral.
---@field unk_1e6 boolean Probably optionally set only on neutral regions
---@field reanimating boolean Indicates that region interaction is reanimating
---@field unk_1e8 integer Number set increases during world gen history, since v0.47.01
---@field evil boolean
---@field good boolean
---@field lake_surface integer
---@field forces integer[] historical figure IDs of force deities associated with the region. Number set increases during civ placement
---@field unk_v47_2 integer Number set increases during civ placement
---@field mid_x integer
---@field mid_y integer
---@field min_x integer
---@field max_x integer
---@field min_y integer
---@field max_y integer
df.world_region = nil

---@enum world_underground_region_type
df.world_underground_region_type = {
    Cavern = 0,
    MagmaSea = 1,
    Underworld = 2,
}
---@class world_underground_region
---@field world_underground_region_type world_underground_region_type
---@field name language_name
---@field index integer
---@field layer_depth integer 0-2 caves, 3 magma sea, 4 hell
---@field layer_depth_p1a integer +1
---@field layer_depth_p1b integer
---@field water integer
---@field unk_7c integer
---@field openness_min integer
---@field openness_max integer
---@field passage_density_min integer
---@field passage_density_max integer
---@field region_coords coord2d_path
---@field region_min_z integer[]
---@field region_max_z integer[]
---@field unk_c8 any[]
---@field feature_init feature_init
df.world_underground_region = nil

---@class world_river
---@field name language_name
---@field path coord2d_path
---@field flow integer[]
---@field exit_tile integer[]
---@field elevation integer[]
---@field end_pos coord2d
---@field flags any[]
df.world_river = nil

---@enum geo_layer_type
df.geo_layer_type = {
    SOIL = 0,
    SEDIMENTARY = 1,
    METAMORPHIC = 2,
    IGNEOUS_EXTRUSIVE = 3,
    IGNEOUS_INTRUSIVE = 4,
    SOIL_OCEAN = 5,
    SOIL_SAND = 6,
    SEDIMENTARY_OCEAN_SHALLOW = 7,
    SEDIMENTARY_OCEAN_DEEP = 8,
}

---@class world_geo_layer
---@field type geo_layer_type
---@field mat_index integer
---@field vein_mat integer[]
---@field vein_nested_in any[] Index of the other vein this one is nested in, or -1
---@field vein_type any[]
---@field vein_unk_38 integer[] density??
---@field top_height integer negative
---@field bottom_height integer
df.world_geo_layer = nil

---@class world_geo_biome
---@field unk1 integer
---@field index integer
---@field layers world_geo_layer[]
df.world_geo_biome = nil

---@class world_region_feature
---@field feature_idx integer
---@field layer integer
---@field region_tile_idx integer
---@field min_z integer
---@field max_z integer
---@field unk_c coord2d[]
---@field unk_28 integer
---@field seed integer looks random
---@field unk_30 any[]
---@field unk_38 integer[]
---@field top_layer_idx layer_type topmost cave layer the feature reaches
df.world_region_feature = nil

---@class world_region_details_rivers_horizontal
---@field y_min any[]
---@field y_max any[]
---@field active any[]
---@field elevation any[]

---@class world_region_details_rivers_vertical
---@field x_min any[]
---@field x_max any[]
---@field active any[]
---@field elevation any[]

---@class world_region_details_edges
---@field split_x any[] splits for horizontal edges, x=min y=max
---@field split_y any[] splits for vertical edges, x=min y=max
---@field biome_corner any[] 0=Reference is NW, 1=Reference is N, 2=Reference is W, 3=Reference is current tile
---@field biome_x any[] 0=Reference is N, 1=Reference is current tile (adopted by S edge to the N)
---@field biome_y any[] 0=Reference is W, 1=Reference is current tile (Adopted by E edge to the W)

---@class world_region_details
---@field biome any[]
---@field elevation any[]
---@field seed any[] looks random
---@field edges world_region_details_edges
---@field pos coord2d
---@field unk12e8 integer
---@field unk_1 integer
---@field unk_2 integer
---@field unk_3 integer
---@field unk_4 integer
---@field rivers_vertical world_region_details_rivers_vertical
---@field rivers_horizontal world_region_details_rivers_horizontal
---@field other_features any[]
---@field features any[]
---@field lava_stone integer
---@field unk_12 integer[] Might it be 256 * 9 int8_t, i.e. 1 per 16*16 block?. Never seen other than -1, though, since v0.40.01
---@field elevation2 any[]
---@field undef13 integer[]
---@field unnamed_world_region_details_20 integer
---@field unnamed_world_region_details_21 integer
df.world_region_details = nil

---@enum region_map_entry_flags
df.region_map_entry_flags = {
    has_river = 0,
    tile_variant = 1,
    unk_2 = 2,
    has_site = 3,
    unk_4 = 4,
    river_up = 5,
    river_down = 6,
    river_right = 7,
    river_left = 8,
    discovered = 9,
    unk_10 = 10,
    unk_11 = 11,
    has_army = 12,
    is_peak = 13,
    is_lake = 14,
    is_brook = 15,
    has_road = 16,
    unk_17 = 17,
    unk_18 = 18,
    unk_19 = 19,
    unk_20 = 20,
    unk_21 = 21,
    unk_22 = 22,
    unk_23 = 23,
}

---@enum front_type
df.front_type = {
    front_none = 0,
    front_warm = 1,
    front_cold = 2,
    front_occluded = 3,
}

---@enum cumulus_type
df.cumulus_type = {
    cumulus_none = 0,
    cumulus_medium = 1,
    cumulus_multi = 2,
    cumulus_nimbus = 3,
}

---@enum stratus_type
df.stratus_type = {
    stratus_none = 0,
    stratus_alto = 1,
    stratus_proper = 2,
    stratus_nimbus = 3,
}

---@enum fog_type
df.fog_type = {
    fog_none = 0,
    fog_mist = 1,
    fog_normal = 2,
    fog_thick = 3,
}

---@class region_map_entry
---@field unk_0 integer
---@field finder_rank integer
---@field sites world_site[]
---@field flags region_map_entry_flags[]
---@field elevation integer 0-99=Ocean, 150+=Mountains, 100-149: all other biomes. Note that PSV elevation uses 100-299 for normal biomes, with range later cut to 1/4, and Mountains shifted down
---@field rainfall integer 0-100
---@field vegetation integer 0-100
---@field temperature integer Urists. 10000 Urists=0 Celsius. Urist steps equals Fahrenheit steps, which is equal to 5/9 Celsius steps
---@field evilness integer 0-32=Good, 33-65=Neutral, 66-100=Evil
---@field drainage integer 0-100
---@field volcanism integer 0-100
---@field savagery integer 0-32=Calm, 33-65=Neutral, 66-100=Savage
---@field air_temp integer
---@field air_x integer Toady: a velocity component?  I dont remember
---@field air_y integer
---@field clouds bitfield
---@field wind bitfield blows toward direction in morning
---@field snowfall integer 0-5000, humidity?
---@field salinity integer 0-100
---@field unk_3e coord
---@field unk_44 coord
---@field unk_4a coord
---@field region_id integer
---@field landmass_id integer
---@field geo_index integer
df.region_map_entry = nil

---@class entity_claim_mask
---@field map any[]
---@field width integer
---@field height integer
df.entity_claim_mask = nil

---@class moving_party
---@field pos coord2d global block x/y
---@field unk_4 integer
---@field unk_c integer
---@field unk_10 integer
---@field members any[]
---@field entity_id integer
---@field flags any[]
---@field unk_30 any[]
---@field unk_40 any[]
---@field unk_70 integer
---@field unk_72 integer
---@field unk_74 integer
---@field unk_7c integer
---@field region_id integer
---@field beast_id integer for FB
df.moving_party = nil

---@class world_object_data_unk_v43 probably used by Adventurer mode, since v0.43.01
---@field x integer[] probably MLT relative x coordinate
---@field y integer[] probably MLT relative y coordinate
---@field z integer[] probably z coordinate using the elevation coordinate system
---@field unk_4 integer[] 233/234 seen

---@class world_object_data_picked_growths also includes 'automatically picked' i.e. fallen fruit that becomes item_spatter. Doesn not seem to be used by Adventurer mode, since v0.40.14
---@field x integer[] 0 - 47, within the MLT
---@field y integer[] 0 - 47, within the MLT
---@field z integer[] z coordinate using the elevation coordinate system
---@field subtype integer[] subtype of the growth picked within the raws of the implicit plant
---@field density integer[] copy of the density field of the growth raws
---@field year integer[] presumably to know whether it's the current year's harvest or the previous one's

---@class world_object_data
---@field id integer World MLT of the data according to: i + x * 16 + k * 16 * world_width + y * 256 * world_width, where (x, y) is the world tile and (i, k) the MLT within it
---@field altered_items integer[] world_data_subid
---@field offloaded_items any[]
---@field unk_24 integer[]
---@field unk_34 integer[]
---@field unk_44 integer[]
---@field unk_54 integer[]
---@field unk_64 integer[]
---@field altered_buildings integer[] world_data_subid
---@field offloaded_buildings any[]
---@field unk_94 any[]
---@field creation_zone_alterations creation_zone_pwg_alterationst[] since v0.40.01
---@field unk_v40_1 integer since v0.40.01
---@field year integer
---@field year_tick integer
---@field picked_growths world_object_data_picked_growths also includes 'automatically picked' i.e. fallen fruit that becomes item_spatter. Doesn not seem to be used by Adventurer mode, since v0.40.14
---@field unk_v43 world_object_data_unk_v43 probably used by Adventurer mode, since v0.43.01
df.world_object_data = nil

---@enum mountain_peak_flags
df.mountain_peak_flags = {
    is_volcano = 0,
}

---@class world_mountain_peak
---@field name language_name
---@field pos coord2d
---@field flags mountain_peak_flags[]
---@field height integer
df.world_mountain_peak = nil

---@class world_data_unk_482f8 since v0.40.01
---@field unk_1 integer[]
---@field unk_2 integer
---@field unk_3 integer
---@field unk_4 integer
---@field unk_5 integer
---@field unk_6 integer
---@field unk_7 integer
---@field unk_8 integer

---@class world_data_constructions
---@field width integer
---@field height integer
---@field map any[]
---@field list world_construction[]
---@field next_id integer

---@class world_data_unk_b4
---@field world_width2 integer
---@field world_height2 integer
---@field unk_1 any[] align(width,4)*height
---@field unk_2 any[] align(width,4)*height
---@field unk_3 any[] width*height
---@field unk_4 any[] align(width,4)*height

---@enum world_data_flip_latitude
df.world_data_flip_latitude = {
    None = -1,
    North = 0,
    South = 1,
    Both = 2,
}
---@class world_data
---@field name language_name name of the world
---@field unk1 integer[]
---@field next_site_id integer
---@field next_site_unk130_id integer
---@field next_resource_allotment_id integer
---@field next_breed_id integer
---@field next_battlefield_id integer since v0.34.01
---@field unk_v34_1 integer since v0.34.01
---@field world_width integer
---@field world_height integer
---@field unk_78 integer
---@field moon_phase integer
---@field world_data_flip_latitude world_data_flip_latitude
---@field flip_longitude integer
---@field unk_84 integer
---@field unk_86 integer
---@field unk_88 integer
---@field unk_8a integer
---@field unk_v34_2 integer since v0.34.01
---@field unk_v34_3 integer since v0.34.01
---@field unk_b4 world_data_unk_b4
---@field region_details world_region_details[]
---@field adv_region_x integer
---@field adv_region_y integer
---@field adv_emb_x integer
---@field adv_emb_y integer
---@field unk_x1 integer
---@field unk_y1 integer
---@field unk_x2 integer
---@field unk_y2 integer
---@field constructions world_data_constructions
---@field entity_claims1 entity_claim_mask
---@field entity_claims2 entity_claim_mask
---@field sites world_site[]
---@field site_unk130 world_site_unk130[]
---@field resource_allotments resource_allotment_data[]
---@field breeds breed[]
---@field battlefields battlefield[] since v0.34.01
---@field region_weather region_weather[] since v0.34.01
---@field object_data world_object_data[] since v0.34.01
---@field landmasses world_landmass[]
---@field regions world_region[]
---@field underground_regions world_underground_region[]
---@field geo_biomes world_geo_biome[]
---@field mountain_peaks world_mountain_peak[]
---@field rivers world_river[]
---@field region_map any[]
---@field unk_1c4 any[]
---@field unnamed_world_data_49 integer
---@field unnamed_world_data_50 integer
---@field unk_1c8 integer
---@field embark_notes embark_note[]
---@field unk_1dc any[]
---@field unk_1e0 any[]
---@field unk_1e4 any[]
---@field unk_1e8 any[]
---@field unk_1ec any[]
---@field unk_1f0 any[]
---@field unk_1 integer since v0.40.01
---@field unk_2 integer since v0.40.01
---@field unk_3 integer since v0.40.01
---@field unk_4 integer since v0.40.01
---@field unk_5 integer since v0.40.01
---@field unk_6 integer since v0.40.01
---@field unk_7 integer since v0.40.01
---@field unk_8 integer since v0.40.01
---@field unk_9 integer since v0.40.01
---@field unk_10 integer since v0.40.01
---@field unk_11 integer since v0.40.01
---@field unk_12 integer since v0.40.01
---@field unk_13 integer since v0.40.01
---@field unk_14 integer since v0.40.01
---@field unk_15 integer since v0.40.01
---@field unk_16 integer since v0.40.01
---@field pad_1 integer
---@field unk_17 integer since v0.40.01
---@field unk_18 integer since v0.40.01
---@field active_site world_site[]
---@field feature_map any[]
---@field old_sites integer[]
---@field old_site_x integer[]
---@field old_site_y integer[]
---@field land_rgns coord2d_path
---@field unk_260 integer
---@field unk_264 integer
---@field unk_268 integer
---@field unk_26c integer
---@field unk_270 integer
---@field unk_274 any[]
---@field unk_482f8 world_data_unk_482f8 since v0.40.01
df.world_data = nil

---@class breed
---@field id integer
---@field unk_4 integer
---@field unk_8 any[]
---@field unk_18 any[]
---@field unk_28 any[]
df.breed = nil

---@class battlefield
---@field id integer
---@field sapient_deaths any[] Seems to be by squad. Trolls/Blizzard Men not counted
---@field hfs_killed integer[] some victims are not listed, for some reason, and culled HFs can be present
---@field x1 integer
---@field y1 integer
---@field x2 integer
---@field y2 integer
---@field unk_34 integer wouldn't be surprised if it was layer, based on other structure layouts, but no non -1 found
---@field event_collections integer[]
df.battlefield = nil

---@enum region_weather_type
df.region_weather_type = {
    CreepingGas = 0,
    CreepingVapor = 1, -- doesn't seem to be generated by DF, but appears if hacked
    CreepingDust = 2,
    FallingMaterial = 3, -- a.k.a. rain, both blood and syndrome, but not regular
}

---@class region_weather only evil weather, not the regular kind
---@field id integer
---@field type region_weather_type Creeping Gas/Vapor/Dust='cloud' below, FallingMaterial='rain'
---@field mat_type integer
---@field mat_index integer
---@field announcement boolean Guess based on seeing it appear for an entry when hitting the embark, resulting in an announcement
---@field region_x integer world tile, used with evil rain. Probably uninitialized with cloud
---@field region_y integer world tile, used with evil rain. Probably uninitialized with cloud
---@field world_in_game_x integer used with evil clouds, indicating global in-game coordinates
---@field world_in_game_y integer used with evil clouds, indicating global in-game coordinates
---@field world_in_game_z integer probably never used, as weather appears on the surface
---@field cloud_x_movement integer -1/0/1, indicating the movement per 10 ticks in X direction. Uninitialized for rain
---@field cloud_y_movement integer -1/0/1, indicating the movement per 10 ticks in Y direction. Uninitialized for rain
---@field remaining_duration integer ticks down 1 every 10 ticks. Removed some time after reaching 0. Cloud duration seems to start with a fairly large, but somewhat random value
---@field region_id integer Set for clouds, -1 for rain
df.region_weather = nil


