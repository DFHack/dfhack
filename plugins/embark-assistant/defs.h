#pragma once

#include <array>
#include <string>
#include <vector>

using namespace std;
using std::array;
using std::ostringstream;
using std::string;
using std::vector;

namespace embark_assist {
    namespace defs {
        //  Survey types
        //
        enum class river_sizes {
            None,
            Brook,
            Stream,
            Minor,
            Medium,
            Major
        };

        struct mid_level_tile {
            bool aquifer = false;
            bool clay = false;
            bool sand = false;
            bool flux = false;
            int8_t soil_depth;
            int8_t offset;
            int16_t elevation;
            bool river_present = false;
            int16_t river_elevation = 100;
            int8_t biome_offset;
            uint8_t savagery_level;  // 0 - 2
            uint8_t evilness_level;  // 0 - 2
            std::vector<bool> metals;
            std::vector<bool> economics;
            std::vector<bool> minerals;
        };

        typedef std::array<std::array<mid_level_tile, 16>, 16> mid_level_tiles;
//        typedef mid_level_tile mid_level_tiles[16][16];

        struct region_tile_datum {
            bool surveyed = false;
            uint16_t aquifer_count = 0;
            uint16_t clay_count = 0;
            uint16_t sand_count = 0;
            uint16_t flux_count = 0;
            uint8_t min_region_soil = 10;
            uint8_t max_region_soil = 0;
            bool waterfall = false;

            river_sizes river_size;
            int16_t biome_index[10];  // Indexed through biome_offset; -1 = null, Index of region, [0] not used
            int16_t biome[10];        // Indexed through biome_offset; -1 = null, df::biome_type, [0] not used
            uint8_t biome_count;
            bool evil_weather[10];
            bool evil_weather_possible;
            bool evil_weather_full;
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
        };

        struct geo_datum {
            uint8_t soil_size = 0;
            bool top_soil_only = true;
            bool top_soil_aquifer_only = true;
            bool aquifer_absent = true;
            bool clay_absent = true;
            bool sand_absent = true;
            bool flux_absent = true;
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
            bool aquifer;
            bool aquifer_full;
            uint8_t min_soil;
            uint8_t max_soil;
            bool flat;
            bool waterfall;
            bool clay;
            bool sand;
            bool flux;
            std::vector<uint16_t> metals;
            std::vector<uint16_t> economics;
            std::vector<uint16_t> minerals;
            //  Could add savagery, evilness, and biomes, but DF provides those easily.
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
            All,
            Present,
            Partial,
            Not_All,
            Absent
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

        /*  //  Future possible enhancement
        enum class freezing_ranges : int8_t {
        NA = -1,
        Permanent,
        At_Least_Partial,
        Partial,
        At_Most_Partial,
        Never
        };
        */

        struct finders {
            uint16_t x_dim;
            uint16_t y_dim;
            evil_savagery_values savagery[static_cast<int8_t>(evil_savagery_ranges::High) + 1];
            evil_savagery_values evilness[static_cast<int8_t>(evil_savagery_ranges::High) + 1];
            aquifer_ranges aquifer;
            river_ranges min_river;
            river_ranges max_river;
            yes_no_ranges waterfall;
            yes_no_ranges flat;
            present_absent_ranges clay;
            present_absent_ranges sand;
            present_absent_ranges flux;
            soil_ranges soil_min;
            all_present_ranges soil_min_everywhere;
            soil_ranges soil_max;
            /*freezing_ranges freezing;*/
            yes_no_ranges evil_weather;  //  Will probably blow up with the magic release arcs...
            yes_no_ranges reanimation;
            yes_no_ranges thralling;
            int8_t biome_count_min; // N/A(-1), 1-9
            int8_t biome_count_max; // N/A(-1), 1-9
            int8_t region_type_1;   // N/A(-1), df::world_region_type
            int8_t region_type_2;   // N/A(-1), df::world_region_type
            int8_t region_type_3;   // N/A(-1), df::world_region_type
            int8_t biome_1;         // N/A(-1), df::biome_type
            int8_t biome_2;         // N/A(-1), df::biome_type
            int8_t biome_3;         // N/A(-1), df::biome_type
            int16_t metal_1;        // N/A(-1), 0-max_inorganic;
            int16_t metal_2;        // N/A(-1), 0-max_inorganic;
            int16_t metal_3;        // N/A(-1), 0-max_inorganic;
            int16_t economic_1;     // N/A(-1), 0-max_inorganic;
            int16_t economic_2;     // N/A(-1), 0-max_inorganic;
            int16_t economic_3;     // N/A(-1), 0-max_inorganic;
            int16_t mineral_1;      // N/A(-1), 0-max_inorganic;
            int16_t mineral_2;      // N/A(-1), 0-max_inorganic;
            int16_t mineral_3;      // N/A(-1), 0-max_inorganic;
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
            uint16_t count;
            finders finder;
        };

        typedef void(*find_callbacks) (embark_assist::defs::finders finder);
    }
}