#pragma once

#include <array>
#include <string>
#include <vector>
#include "df/world_region_type.h"

using namespace std;
using std::array;
using std::ostringstream;
using std::string;
using std::vector;

#define fileresult_file_name "./data/init/embark_assistant_fileresult.txt"

namespace embark_assist {
    namespace defs {
        //  Survey types
        //
        enum class river_sizes : int8_t {
            None,
            Brook,
            Stream,
            Minor,
            Medium,
            Major
        };

        enum class aquifer_sizes : int8_t {
            NA,
            None,
            Light,
            None_Light,
            Heavy,
            None_Heavy,
            Light_Heavy,
            None_Light_Heavy
        };

        enum class tree_levels : int8_t {
            None,
            Very_Scarce,
            Scarce,
            Woodland,
            Heavily_Forested
        };

        struct mid_level_tile {
            aquifer_sizes aquifer = aquifer_sizes::NA;
            bool clay = false;
            bool sand = false;
            bool flux = false;
            bool coal = false;
            int8_t soil_depth;
            int8_t offset;
            int16_t elevation;
            bool river_present = false;
            int16_t river_elevation = 100;
            int8_t adamantine_level;  // -1 = none, 0 .. 3 = cavern 1 .. magma sea. Currently not used beyond present/absent.
            int8_t magma_level;  // -1 = none, 0 .. 3 = cavern 3 .. surface/volcano
            int8_t biome_offset;
            tree_levels trees;
            uint8_t savagery_level;  // 0 - 2
            uint8_t evilness_level;  // 0 - 2
            std::vector<bool> metals;
            std::vector<bool> economics;
            std::vector<bool> minerals;
        };

        typedef std::array<std::array<mid_level_tile, 16>, 16> mid_level_tiles;

        struct region_tile_datum {
            bool surveyed = false;
            aquifer_sizes aquifer = aquifer_sizes::NA;
            uint16_t clay_count = 0;
            uint16_t sand_count = 0;
            uint16_t flux_count = 0;
            uint16_t coal_count = 0;
            uint8_t min_region_soil = 10;
            uint8_t max_region_soil = 0;
            uint8_t max_waterfall = 0;
            river_sizes river_size;
            int16_t biome_index[10];  // Indexed through biome_offset; -1 = null, Index of region, [0] not used
            int16_t biome[10];        // Indexed through biome_offset; -1 = null, df::biome_type, [0] not used
            uint8_t biome_count;
            int16_t min_temperature[10];  // Indexed through biome_offset; -30000 = null, Urists - 10000, [0] not used
            int16_t max_temperature[10];  // Indexed through biome_offset; -30000 = null, Urists - 10000, [0] not used
            tree_levels min_tree_level = embark_assist::defs::tree_levels::Heavily_Forested;
            tree_levels max_tree_level = embark_assist::defs::tree_levels::None;
            bool blood_rain[10];
            bool blood_rain_possible;
            bool blood_rain_full;
            bool permanent_syndrome_rain[10];
            bool permanent_syndrome_rain_possible;
            bool permanent_syndrome_rain_full;
            bool temporary_syndrome_rain[10];
            bool temporary_syndrome_rain_possible;
            bool temporary_syndrome_rain_full;
            bool reanimating[10];
            bool reanimating_possible;
            bool reanimating_full;
            bool thralling[10];
            bool thralling_possible;
            bool thralling_full;
            uint16_t savagery_count[3];
            uint16_t evilness_count[3];
            std::vector<bool> metals;
            std::vector<bool> economics;
            std::vector<bool> minerals;
            std::vector<int16_t> neighbors;  //  entity_raw indices
            uint8_t necro_neighbors;
            mid_level_tile north_row[16];
            mid_level_tile south_row[16];
            mid_level_tile west_column[16];
            mid_level_tile east_column[16];
            uint8_t north_corner_selection[16]; //  0 - 3. For some reason DF stores everything needed for incursion
            uint8_t west_corner_selection[16];  //  detection in 17:th row/colum data in the region details except
                                                //  this info, so we have to go to neighboring world tiles to fetch it.
            df::world_region_type region_type[16][16];  //  Required for incursion override detection. We could store only the
                                                //  edges, but storing it for every tile allows for a unified fetching
                                                //  logic.
            int8_t north_row_biome_x[16];    //  "biome_x" data cached for the northern row for access from the north.
            int8_t west_column_biome_y[16];  //  "biome_y" data cached for the western row for access from the west.
        };

        struct geo_datum {
            uint8_t soil_size = 0;
            bool top_soil_only = true;
            bool top_soil_aquifer_only = true;
            bool aquifer_absent = true;
            bool clay_absent = true;
            bool sand_absent = true;
            bool flux_absent = true;
            bool coal_absent = true;
            std::vector<bool> possible_metals;
            std::vector<bool> possible_economics;
            std::vector<bool> possible_minerals;
        };

        typedef std::vector<geo_datum> geo_data;

        struct sites {
            uint8_t x;
            uint8_t y;
            char type;
        };

        struct site_infos {
            bool incursions_processed;
            aquifer_sizes aquifer;
            uint8_t min_soil;
            uint8_t max_soil;
            bool flat;
            uint8_t max_waterfall;
            bool clay;
            bool sand;
            bool flux;
            bool coal;
            bool blood_rain;
            bool permanent_syndrome_rain;
            bool temporary_syndrome_rain;
            bool reanimating;
            bool thralling;
            std::vector<uint16_t> metals;
            std::vector<uint16_t> economics;
            std::vector<uint16_t> minerals;
            //  Could add savagery, evilness, and biomes, but DF provides those easily.
            std::vector<int16_t> neighbors;  //  entity_raw indices
            uint8_t necro_neighbors;
        };

        typedef std::vector<sites> site_lists;

        typedef std::vector<std::vector<region_tile_datum>> world_tile_data;

        typedef bool mlt_matches[16][16];
        //  An embark region match is indicated by marking the top left corner
        //  tile as a match. Thus, the bottom and right side won't show matches
        //  unless the appropriate dimension has a width of 1.

        struct matches {
            bool preliminary_match;
            bool contains_match;
            mlt_matches mlt_match;
        };

        typedef std::vector<std::vector<matches>> match_results;

        //  matcher types
        //
        enum class evil_savagery_values : int8_t {
            NA = -1,
            All,
            Present,
            Absent
        };

        enum class evil_savagery_ranges : int8_t {
            Low,
            Medium,
            High
        };

        enum class aquifer_ranges : int8_t {
            NA = -1,
            None,
            At_Most_Light,
            None_Plus_Light,
            None_Plus_At_Least_Light,
            Light,
            At_Least_Light,
            None_Plus_Heavy,
            At_Most_Light_Plus_Heavy,
            Light_Plus_Heavy,
            None_Light_Heavy,
            Heavy
        };

        enum class river_ranges : int8_t {
            NA = -1,
            None,
            Brook,
            Stream,
            Minor,
            Medium,
            Major
        };

//  For possible future use. That's the level of data actually collected.
//        enum class adamantine_ranges : int8_t {
//            NA = -1,
//            Cavern_1,
//            Cavern_2,
//            Cavern_3,
//            Magma_Sea
//        };

        enum class magma_ranges : int8_t {
            NA = -1,
            Cavern_3,
            Cavern_2,
            Cavern_1,
            Volcano
        };

        enum class yes_no_ranges : int8_t {
            NA = -1,
            Yes,
            No
        };

        enum class all_present_ranges : int8_t {
            All,
            Present
        };
        enum class present_absent_ranges : int8_t {
            NA = -1,
            Present,
            Absent
        };

        enum class soil_ranges : int8_t {
            NA = -1,
            None,
            Very_Shallow,
            Shallow,
            Deep,
            Very_Deep
        };

        enum class syndrome_rain_ranges : int8_t {
            NA = -1,
            Any,
            Permanent,
            Temporary,
            Not_Permanent,
            None
        };

        enum class reanimation_ranges : int8_t {
            NA = -1,
            Both,
            Any,
            Thralling,
            Reanimation,
            Not_Thralling,
            None
        };

        enum class freezing_ranges : int8_t {
            NA = -1,
            Permanent,
            At_Least_Partial,
            Partial,
            At_Most_Partial,
            Never
        };

        enum class tree_ranges : int8_t {
            NA = -1,
            None,
            Very_Scarce,  // DF dislays this with a different color but still the "scarce" text
            Scarce,
            Woodland,
            Heavily_Forested
        };

        struct neighbor {
            int16_t entity_raw;  //  entity_raw
            present_absent_ranges present;
        };

        struct finders {
            uint16_t x_dim;
            uint16_t y_dim;
            evil_savagery_values savagery[static_cast<int8_t>(evil_savagery_ranges::High) + 1];
            evil_savagery_values evilness[static_cast<int8_t>(evil_savagery_ranges::High) + 1];
            aquifer_ranges aquifer;
            river_ranges min_river;
            river_ranges max_river;
            int8_t min_waterfall; // N/A(-1), Absent, 1-50
            yes_no_ranges flat;
            present_absent_ranges clay;
            present_absent_ranges sand;
            present_absent_ranges flux;
            present_absent_ranges coal;
            soil_ranges soil_min;
            all_present_ranges soil_min_everywhere;
            soil_ranges soil_max;
            freezing_ranges freezing;
            yes_no_ranges blood_rain;  //  Will probably blow up with the magic release arcs...
            syndrome_rain_ranges syndrome_rain;
            reanimation_ranges reanimation;
            int8_t spire_count_min; // N/A(-1), 0-9
            int8_t spire_count_max; // N/A(-1), 0-9
            magma_ranges magma_min;
            magma_ranges magma_max;
            int8_t biome_count_min; // N/A(-1), 1-9
            int8_t biome_count_max; // N/A(-1), 1-9
            int8_t region_type_1;   // N/A(-1), df::world_region_type
            int8_t region_type_2;   // N/A(-1), df::world_region_type
            int8_t region_type_3;   // N/A(-1), df::world_region_type
            int8_t biome_1;         // N/A(-1), df::biome_type
            int8_t biome_2;         // N/A(-1), df::biome_type
            int8_t biome_3;         // N/A(-1), df::biome_type
            tree_ranges min_trees;
            tree_ranges max_trees;
            int16_t metal_1;        // N/A(-1), 0-max_inorganic;
            int16_t metal_2;        // N/A(-1), 0-max_inorganic;
            int16_t metal_3;        // N/A(-1), 0-max_inorganic;
            int16_t economic_1;     // N/A(-1), 0-max_inorganic;
            int16_t economic_2;     // N/A(-1), 0-max_inorganic;
            int16_t economic_3;     // N/A(-1), 0-max_inorganic;
            int16_t mineral_1;      // N/A(-1), 0-max_inorganic;
            int16_t mineral_2;      // N/A(-1), 0-max_inorganic;
            int16_t mineral_3;      // N/A(-1), 0-max_inorganic;
            int8_t min_necro_neighbors; // N/A(-1), 0 - 9, where 9 = 9+
            int8_t max_necro_neighbors; // N/A(-1), 0 - 9, where 9 = 9+
            int8_t min_civ_neighbors; // N/A(-1), 0 - 9, where 9 = 9+
            int8_t max_civ_neighbors; // N/A(-1), 0 - 9, where 9 = 9+
            std::vector<neighbor> neighbors;
        };

        struct match_iterators {
            bool active;
            uint16_t x;  //  x position of focus when iteration started so we can return it.
            uint16_t y;  //  y
            uint16_t i;
            uint16_t k;
            bool x_right;
            bool y_down;
            bool inhibit_x_turn;
            bool inhibit_y_turn;
            uint16_t target_location_x;
            uint16_t target_location_y;
            uint16_t count;
            finders finder;
        };

        typedef void(*find_callbacks) (embark_assist::defs::finders finder);
    }
}