#include <Console.h>

#include <modules/Gui.h>

#include "DataDefs.h"
#include "df/biome_type.h"
#include "df/inorganic_raw.h"
#include "df/region_map_entry.h"
#include "df/viewscreen.h"
#include "df/viewscreen_choose_start_sitest.h"
#include "df/world.h"
#include "df/world_data.h"
#include "df/world_raws.h"
#include "df/world_region.h"
#include "df/world_region_type.h"

#include "matcher.h"
#include "survey.h"

using df::global::world;

namespace embark_assist {
    namespace matcher {

        //=======================================================================================

        //=======================================================================================

        bool embark_match(embark_assist::defs::world_tile_data *survey_results,
            embark_assist::defs::mid_level_tiles *mlt,
            uint16_t x,
            uint16_t y,
            uint16_t start_x,
            uint16_t start_y,
            embark_assist::defs::finders *finder) {

//            color_ostream_proxy out(Core::getInstance().getConsole());
            df::world_data *world_data = world->world_data;
            bool savagery_found[3] = { false, false, false };
            bool evilness_found[3] = { false, false, false };
            uint16_t aquifer_count = 0;
            bool river_found = false;
            bool waterfall_found = false;
            uint16_t river_elevation;
            uint16_t elevation = mlt->at(start_x).at(start_y).elevation;
            bool clay_found = false;
            bool sand_found = false;
            bool flux_found = false;
            uint8_t max_soil = 0;
            bool uneven = false;
            bool evil_weather_found = false;
            bool reanimation_found = false;
            bool thralling_found = false;
            bool biomes[ENUM_LAST_ITEM(biome_type) + 1];
            bool region_types[ENUM_LAST_ITEM(world_region_type) + 1];
            uint8_t biome_count;
            bool metal_1 = finder->metal_1 == -1;
            bool metal_2 = finder->metal_2 == -1;
            bool metal_3 = finder->metal_3 == -1;
            bool economic_1 = finder->economic_1 == -1;
            bool economic_2 = finder->economic_2 == -1;
            bool economic_3 = finder->economic_3 == -1;
            bool mineral_1 = finder->mineral_1 == -1;
            bool mineral_2 = finder->mineral_2 == -1;
            bool mineral_3 = finder->mineral_3 == -1;

            const uint16_t embark_size = finder->x_dim * finder->y_dim;

            if (finder->biome_count_min != -1 ||
                finder->biome_count_max != -1 ||
                finder->biome_1 != -1 ||
                finder->biome_2 != -1 ||
                finder->biome_3 != -1) {
                for (uint8_t i = 0; i <= ENUM_LAST_ITEM(biome_type); i++) biomes[i] = false;
            }

            for (uint8_t i = 0; i <= ENUM_LAST_ITEM(world_region_type); i++) region_types[i] = false;

            for (uint16_t i = start_x; i < start_x + finder->x_dim; i++) {
                for (uint16_t k = start_y; k < start_y + finder->y_dim; k++) {

                    // Savagery & Evilness
                    {
                        savagery_found[mlt->at(i).at(k).savagery_level] = true;
                        evilness_found[mlt->at(i).at(k).evilness_level] = true;

                        embark_assist::defs::evil_savagery_ranges l = embark_assist::defs::evil_savagery_ranges::Low;
                        while (true) {
                            if (mlt->at(i).at(k).savagery_level == static_cast<uint8_t>(l)) {
                                if (finder->savagery[static_cast <int>(l)] ==
                                    embark_assist::defs::evil_savagery_values::Absent) return false;
                            }
                            else {
                                if (finder->savagery[static_cast <int>(l)] ==
                                    embark_assist::defs::evil_savagery_values::All) return false;
                            }

                            if (mlt->at(i).at(k).evilness_level == static_cast<uint8_t>(l)) {
                                if (finder->evilness[static_cast <int>(l)] ==
                                    embark_assist::defs::evil_savagery_values::Absent) return false;
                            }
                            else {
                                if (finder->evilness[static_cast <int>(l)] ==
                                    embark_assist::defs::evil_savagery_values::All) return false;
                            }

                            if (l == embark_assist::defs::evil_savagery_ranges::High) break;
                            l = static_cast <embark_assist::defs::evil_savagery_ranges>(static_cast<int8_t>(l) + 1);
                        }
                    }

                    //  Aquifer
                    switch (finder->aquifer) {
                    case embark_assist::defs::aquifer_ranges::NA:
                        break;

                    case embark_assist::defs::aquifer_ranges::All:
                        if (!mlt->at(i).at(k).aquifer) return false;
                        aquifer_count++;
                        break;

                    case embark_assist::defs::aquifer_ranges::Present:
                    case embark_assist::defs::aquifer_ranges::Partial:
                    case embark_assist::defs::aquifer_ranges::Not_All:
                        if (mlt->at(i).at(k).aquifer) aquifer_count++;
                        break;

                    case embark_assist::defs::aquifer_ranges::Absent:
                        if (mlt->at(i).at(k).aquifer) return false;
                        break;
                    }

                    //  River & Waterfall
                    if (mlt->at(i).at(k).river_present) {
                        //  Actual size values were checked on the world tile level for min rivers
                        if (finder->max_river != embark_assist::defs::river_ranges::NA &&
                            finder->max_river < static_cast<embark_assist::defs::river_ranges>(survey_results->at(x).at(y).river_size)) return false;

                        if (river_found  && river_elevation != mlt->at(i).at(k).river_elevation) {
                            if (finder->waterfall == embark_assist::defs::yes_no_ranges::No) return false;
                            waterfall_found = true;
                        }
                        river_found = true;
                        river_elevation = mlt->at(i).at(k).river_elevation;
                    }

                    //  Flat
                    if (finder->flat == embark_assist::defs::yes_no_ranges::Yes &&
                        elevation != mlt->at(i).at(k).elevation) return false;

                    if (elevation != mlt->at(i).at(k).elevation) uneven = true;

                    // Clay
                    if (mlt->at(i).at(k).clay) {
                        if (finder->clay == embark_assist::defs::present_absent_ranges::Absent) return false;
                        clay_found = true;
                    }

                    // Sand
                    if (mlt->at(i).at(k).sand) {
                        if (finder->sand == embark_assist::defs::present_absent_ranges::Absent) return false;
                        sand_found = true;
                    }

                    // Flux
                    if (mlt->at(i).at(k).flux) {
                        if (finder->flux == embark_assist::defs::present_absent_ranges::Absent) return false;
                        flux_found = true;
                    }

                    //  Min Soil
                    if (finder->soil_min != embark_assist::defs::soil_ranges::NA &&
                        mlt->at(i).at(k).soil_depth < static_cast<uint16_t>(finder->soil_min) &&
                        finder->soil_min_everywhere == embark_assist::defs::all_present_ranges::All) return false;

                    if (max_soil < mlt->at(i).at(k).soil_depth) {
                        max_soil = mlt->at(i).at(k).soil_depth;
                    }

                    //  Max Soil
                    if (finder->soil_max != embark_assist::defs::soil_ranges::NA &&
                        mlt->at(i).at(k).soil_depth > static_cast<uint16_t>(finder->soil_max)) return false;

                    //  Evil Weather
                    if (survey_results->at(x).at(y).evil_weather[mlt->at(i).at(k).biome_offset]) {
                        if (finder->evil_weather == embark_assist::defs::yes_no_ranges::No) return false;
                        evil_weather_found = true;
                    }

                    //  Reanmation
                    if (survey_results->at(x).at(y).reanimating[mlt->at(i).at(k).biome_offset]) {
                        if (finder->reanimation == embark_assist::defs::yes_no_ranges::No) return false;
                        reanimation_found = true;
                    }

                    //  Thralling
                    if (survey_results->at(x).at(y).thralling[mlt->at(i).at(k).biome_offset]) {
                        if (finder->thralling == embark_assist::defs::yes_no_ranges::No) return false;
                        thralling_found = true;
                    }

                    //  Biomes
                    biomes[survey_results->at(x).at(y).biome[mlt->at(i).at(k).biome_offset]] = true;

                    //  Region Type
                    region_types[world_data->regions[survey_results->at(x).at(y).biome_index[mlt->at(i).at(k).biome_offset]]->type] = true;
                
                    //  Metals
                    metal_1 = metal_1 || mlt->at(i).at(k).metals[finder->metal_1];
                    metal_2 = metal_2 || mlt->at(i).at(k).metals[finder->metal_2];
                    metal_3 = metal_3 || mlt->at(i).at(k).metals[finder->metal_3];

                    //  Economics
                    economic_1 = economic_1 || mlt->at(i).at(k).economics[finder->economic_1];
                    economic_2 = economic_2 || mlt->at(i).at(k).economics[finder->economic_2];
                    economic_3 = economic_3 || mlt->at(i).at(k).economics[finder->economic_3];

                    //  Minerals
                    mineral_1 = mineral_1 || mlt->at(i).at(k).minerals[finder->mineral_1];
                    mineral_2 = mineral_2 || mlt->at(i).at(k).minerals[finder->mineral_2];
                    mineral_3 = mineral_3 || mlt->at(i).at(k).minerals[finder->mineral_3];
                }
            }

            //  Summary section, for all the stuff that require the complete picture
            //
            // Savagery & Evilness
            {
                embark_assist::defs::evil_savagery_ranges l = embark_assist::defs::evil_savagery_ranges::Low;

                while (true) {
                    if (finder->savagery[static_cast <int>(l)] ==
                        embark_assist::defs::evil_savagery_values::Present &&
                        !savagery_found[static_cast<int>(l)]) return false;

                    if (finder->evilness[static_cast <int>(l)] ==
                        embark_assist::defs::evil_savagery_values::Present &&
                        !evilness_found[static_cast<int>(l)]) return false;

                    if (l == embark_assist::defs::evil_savagery_ranges::High) break;
                    l = static_cast <embark_assist::defs::evil_savagery_ranges>(static_cast<int8_t>(l) + 1);
                }
            }

            //  Aquifer
            switch (finder->aquifer) {
            case embark_assist::defs::aquifer_ranges::NA:
            case embark_assist::defs::aquifer_ranges::All:    //  Checked above
            case embark_assist::defs::aquifer_ranges::Absent: //  Ditto
                break;

            case embark_assist::defs::aquifer_ranges::Present:
                if (aquifer_count == 0) return false;
                break;

            case embark_assist::defs::aquifer_ranges::Partial:
                if (aquifer_count == 0 || aquifer_count == embark_size) return false;
                break;

            case embark_assist::defs::aquifer_ranges::Not_All:
                if (aquifer_count == embark_size) return false;
                break;
            }

            //  River & Waterfall
            if (!river_found && finder->min_river > embark_assist::defs::river_ranges::None) return false;
            if (finder->waterfall == embark_assist::defs::yes_no_ranges::Yes && !waterfall_found) return false;

            //  Flat
            if (!uneven && finder->flat == embark_assist::defs::yes_no_ranges::No) return false;

            //  Clay
            if (finder->clay == embark_assist::defs::present_absent_ranges::Present && !clay_found) return false;

            //  Sand
            if (finder->sand == embark_assist::defs::present_absent_ranges::Present && !sand_found) return false;

            //  Flux
            if (finder->flux == embark_assist::defs::present_absent_ranges::Present && !flux_found) return false;

            //  Min Soil
            if (finder->soil_min != embark_assist::defs::soil_ranges::NA &&
                finder->soil_min_everywhere == embark_assist::defs::all_present_ranges::Present &&
                max_soil < static_cast<uint8_t>(finder->soil_min)) return false;

            //  Evil Weather
            if (finder->evil_weather == embark_assist::defs::yes_no_ranges::Yes && !evil_weather_found) return false;

            //  Reanimation
            if (finder->reanimation == embark_assist::defs::yes_no_ranges::Yes && !reanimation_found) return false;

            //  Thralling
            if (finder->thralling == embark_assist::defs::yes_no_ranges::Yes && !thralling_found) return false;

            //  Biomes
            if (finder->biome_count_min != -1 ||
                finder->biome_count_max != -1) {
                biome_count = 0;
                for (uint8_t i = 0; i <= ENUM_LAST_ITEM(biome_type); i++) {
                    if (biomes[i]) biome_count++;
                }

                if (biome_count < finder->biome_count_min ||
                    (finder->biome_count_max != -1 &&
                        finder->biome_count_max < biome_count)) return false;
            }

            if (finder->biome_1 != -1 && !biomes[finder->biome_1]) return false;
            if (finder->biome_2 != -1 && !biomes[finder->biome_2]) return false;
            if (finder->biome_3 != -1 && !biomes[finder->biome_3]) return false;

            //  Region Type
            if (finder->region_type_1 != -1 && !region_types[finder->region_type_1]) return false;
            if (finder->region_type_2 != -1 && !region_types[finder->region_type_2]) return false;
            if (finder->region_type_3 != -1 && !region_types[finder->region_type_3]) return false;

            //  Metals, Economics, and Minerals
            if (!metal_1 ||
                !metal_2 ||
                !metal_3 ||
                !economic_1 ||
                !economic_2 ||
                !economic_3 ||
                !mineral_1 ||
                !mineral_2 ||
                !mineral_3) return false;

            return true;
        }

        //=======================================================================================

        void mid_level_tile_match(embark_assist::defs::world_tile_data *survey_results,
            embark_assist::defs::mid_level_tiles *mlt,
            uint16_t x,
            uint16_t y,
            embark_assist::defs::finders *finder,
            embark_assist::defs::match_results *match_results) {

//            color_ostream_proxy out(Core::getInstance().getConsole());
            bool match = false;

            for (uint16_t i = 0; i < 16; i++) {
                for (uint16_t k = 0; k < 16; k++) {
                    if (i < 16 - finder->x_dim + 1 && k < 16 - finder->y_dim + 1) {
                        match_results->at(x).at(y).mlt_match[i][k] = embark_match(survey_results, mlt, x, y, i, k, finder);
                        match = match || match_results->at(x).at(y).mlt_match[i][k];
                    }
                    else {
                        match_results->at(x).at(y).mlt_match[i][k] = false;
                    }
                }
            }
            match_results->at(x).at(y).contains_match = match;
            match_results->at(x).at(y).preliminary_match = false;
        }

        //=======================================================================================

        bool world_tile_match(embark_assist::defs::world_tile_data *survey_results,
            uint16_t x,
            uint16_t y,
            embark_assist::defs::finders *finder) {

//            color_ostream_proxy out(Core::getInstance().getConsole());
            df::world_data *world_data = world->world_data;
            embark_assist::defs::region_tile_datum *tile = &survey_results->at(x).at(y);
            const uint16_t embark_size = finder->x_dim * finder->y_dim;
            uint16_t count;
            bool found;

            if (tile->surveyed) {
                //  Savagery
                for (uint8_t i = 0; i < 3; i++)
                {
                    switch (finder->savagery[i]) {
                    case embark_assist::defs::evil_savagery_values::NA:
                        break;  //  No restriction

                    case embark_assist::defs::evil_savagery_values::All:
                        if (tile->savagery_count[i] < embark_size) return false;
                        break;

                    case embark_assist::defs::evil_savagery_values::Present:
                        if (tile->savagery_count[i] == 0) return false;
                        break;

                    case embark_assist::defs::evil_savagery_values::Absent:
                        if (tile->savagery_count[i] > 256 - embark_size) return false;
                        break;
                    }
                }

                // Evilness
                for (uint8_t i = 0; i < 3; i++)
                {
                    switch (finder->evilness[i]) {
                    case embark_assist::defs::evil_savagery_values::NA:
                        break;  //  No restriction

                    case embark_assist::defs::evil_savagery_values::All:
                        if (tile->evilness_count[i] < embark_size) return false;
                        break;

                    case embark_assist::defs::evil_savagery_values::Present:
                        if (tile->evilness_count[i] == 0) return false;
                        break;

                    case embark_assist::defs::evil_savagery_values::Absent:
                        if (tile->evilness_count[i] > 256 - embark_size) return false;
                        break;
                    }
                }

                // Aquifer
                switch (finder->aquifer) {
                case embark_assist::defs::aquifer_ranges::NA:
                    break;  //  No restriction

                case embark_assist::defs::aquifer_ranges::All:
                    if (tile->aquifer_count < 256 - embark_size) return false;
                    break;

                case embark_assist::defs::aquifer_ranges::Present:
                    if (tile->aquifer_count == 0) return false;
                    break;

                case embark_assist::defs::aquifer_ranges::Partial:
                    if (tile->aquifer_count == 0 ||
                        tile->aquifer_count == 256) return false;
                    break;

                case embark_assist::defs::aquifer_ranges::Not_All:
                    if (tile->aquifer_count == 256) return false;
                    break;

                case embark_assist::defs::aquifer_ranges::Absent:
                    if (tile->aquifer_count > 256 - embark_size) return false;
                    break;
                }

                // River size. Every tile has riverless tiles, so max rivers has to be checked on the detailed level.
                switch (tile->river_size) {
                case embark_assist::defs::river_sizes::None:
                    if (finder->min_river > embark_assist::defs::river_ranges::None) return false;
                    break;

                case embark_assist::defs::river_sizes::Brook:
                    if (finder->min_river > embark_assist::defs::river_ranges::Brook) return false;
                    break;

                case embark_assist::defs::river_sizes::Stream:
                    if (finder->min_river > embark_assist::defs::river_ranges::Stream) return false;
                    break;

                case embark_assist::defs::river_sizes::Minor:
                    if (finder->min_river > embark_assist::defs::river_ranges::Minor) return false;
                    break;

                case embark_assist::defs::river_sizes::Medium:
                    if (finder->min_river > embark_assist::defs::river_ranges::Medium) return false;
                    break;

                case embark_assist::defs::river_sizes::Major:
                    if (finder->max_river != embark_assist::defs::river_ranges::NA) return false;
                    break;
                }

                //  Waterfall
                switch (finder->waterfall) {
                case embark_assist::defs::yes_no_ranges::NA:
                    break;  //  No restriction

                case embark_assist::defs::yes_no_ranges::Yes:
                    if (!tile->waterfall) return false;
                    break;

                case embark_assist::defs::yes_no_ranges::No:
                    if (tile->waterfall &&
                        embark_size == 256) return false;
                    break;
                }

                //  Flat. No world tile checks. Need to look at the details

                //  Clay
                switch (finder->clay) {
                case embark_assist::defs::present_absent_ranges::NA:
                    break;  //  No restriction

                case embark_assist::defs::present_absent_ranges::Present:
                    if (tile->clay_count == 0) return false;
                    break;
                case embark_assist::defs::present_absent_ranges::Absent:
                    if (tile->clay_count > 256 - embark_size) return false;
                    break;
                }

                //  Sand
                switch (finder->sand) {
                case embark_assist::defs::present_absent_ranges::NA:
                    break;  //  No restriction

                case embark_assist::defs::present_absent_ranges::Present:
                    if (tile->sand_count == 0) return false;
                    break;
                case embark_assist::defs::present_absent_ranges::Absent:
                    if (tile->sand_count > 256 - embark_size) return false;
                    break;
                }

                //  Flux
                switch (finder->flux) {
                case embark_assist::defs::present_absent_ranges::NA:
                    break;  //  No restriction

                case embark_assist::defs::present_absent_ranges::Present:
                    if (tile->flux_count == 0) return false;
                    break;
                case embark_assist::defs::present_absent_ranges::Absent:
                    if (tile->flux_count > 256 - embark_size) return false;
                    break;
                }

                //  Soil Min
                switch (finder->soil_min) {
                case embark_assist::defs::soil_ranges::NA:
                case embark_assist::defs::soil_ranges::None:
                    break;  //  No restriction

                case embark_assist::defs::soil_ranges::Very_Shallow:
                    if (tile->max_region_soil < 1) return false;
                    break;

                case embark_assist::defs::soil_ranges::Shallow:
                    if (tile->max_region_soil < 2) return false;
                    break;

                case embark_assist::defs::soil_ranges::Deep:
                    if (tile->max_region_soil < 3) return false;
                    break;

                case embark_assist::defs::soil_ranges::Very_Deep:
                    if (tile->max_region_soil < 4) return false;
                    break;
                }

                //  soil_min_everywhere only applies on the detailed level

                //  Soil Max
                switch (finder->soil_max) {
                case embark_assist::defs::soil_ranges::NA:
                case embark_assist::defs::soil_ranges::Very_Deep:
                    break;  //  No restriction

                case embark_assist::defs::soil_ranges::None:
                    if (tile->min_region_soil > 0) return false;
                    break;

                case embark_assist::defs::soil_ranges::Very_Shallow:
                    if (tile->min_region_soil > 1) return false;
                    break;

                case embark_assist::defs::soil_ranges::Shallow:
                    if (tile->min_region_soil > 2) return false;
                    break;

                case embark_assist::defs::soil_ranges::Deep:
                    if (tile->min_region_soil > 3) return false;
                    break;
                }

                //  Evil Weather
                switch (finder->evil_weather) {
                case embark_assist::defs::yes_no_ranges::NA:
                    break;  //  No restriction

                case embark_assist::defs::yes_no_ranges::Yes:
                    if (!tile->evil_weather_possible) return false;
                    break;

                case embark_assist::defs::yes_no_ranges::No:
                    if (tile->evil_weather_full) return false;
                    break;
                }

                //  Reanimating
                switch (finder->reanimation) {
                case embark_assist::defs::yes_no_ranges::NA:
                    break;  //  No restriction

                case embark_assist::defs::yes_no_ranges::Yes:
                    if (!tile->reanimating_possible) return false;
                    break;

                case embark_assist::defs::yes_no_ranges::No:
                    if (tile->reanimating_full) return false;
                    break;
                }

                //  Thralling
                switch (finder->thralling) {
                case embark_assist::defs::yes_no_ranges::NA:
                    break;  //  No restriction

                case embark_assist::defs::yes_no_ranges::Yes:
                    if (!tile->thralling_possible) return false;
                    break;

                case embark_assist::defs::yes_no_ranges::No:
                    if (tile->thralling_full) return false;
                    break;
                }

                //  Biome Count Min (Can't do anything with Max at this level)
                if (finder->biome_count_min > tile->biome_count) return false;

                //  Region Type 1
                if (finder->region_type_1 != -1) {
                    found = false;

                    for (uint8_t k = 1; k < 10; k++) {
                        if (tile->biome_index[k] != -1) {
                            if (world_data->regions[tile->biome_index[k]]->type == finder->region_type_1) {
                                found = true;
                                break;
                            }
                        }

                        if (found) break;
                    }

                    if (!found) return false;
                }

                //  Region Type 2
                if (finder->region_type_2 != -1) {
                    found = false;

                    for (uint8_t k = 1; k < 10; k++) {
                        if (tile->biome_index[k] != -1) {
                            if (world_data->regions[tile->biome_index[k]]->type == finder->region_type_2) {
                                found = true;
                                break;
                            }
                        }

                        if (found) break;
                    }

                    if (!found) return false;
                }

                //  Region Type 3
                if (finder->region_type_3 != -1) {
                    found = false;

                    for (uint8_t k = 1; k < 10; k++) {
                        if (tile->biome_index[k] != -1) {
                            if (world_data->regions[tile->biome_index[k]]->type == finder->region_type_3) {
                                found = true;
                                break;
                            }
                        }

                        if (found) break;
                    }

                    if (!found) return false;
                }

                //  Biome 1
                if (finder->biome_1 != -1) {
                    found = false;

                    for (uint8_t i = 1; i < 10; i++) {
                        if (tile->biome[i] == finder->biome_1) {
                            found = true;
                            break;
                        }
                    }

                    if (!found) return false;
                }

                //  Biome 2
                if (finder->biome_2 != -1) {
                    found = false;

                    for (uint8_t i = 1; i < 10; i++) {
                        if (tile->biome[i] == finder->biome_2) {
                            found = true;
                            break;
                        }
                    }

                    if (!found) return false;
                }

                //  Biome 3
                if (finder->biome_3 != -1) {
                    found = false;

                    for (uint8_t i = 1; i < 10; i++) {
                        if (tile->biome[i] == finder->biome_3) {
                            found = true;
                            break;
                        }
                    }

                    if (!found) return false;
                }

                if (finder->metal_1 != -1 ||
                    finder->metal_2 != -1 ||
                    finder->metal_3 != -1 ||
                    finder->economic_1 != -1 ||
                    finder->economic_2 != -1 ||
                    finder->economic_3 != -1 ||
                    finder->mineral_1 != -1 ||
                    finder->mineral_2 != -1 ||
                    finder->mineral_3 != -1) {
                    count = 0;
                    bool metal_1 = finder->metal_1 == -1;
                    bool metal_2 = finder->metal_2 == -1;
                    bool metal_3 = finder->metal_3 == -1;
                    bool economic_1 = finder->economic_1 == -1;
                    bool economic_2 = finder->economic_2 == -1;
                    bool economic_3 = finder->economic_3 == -1;
                    bool mineral_1 = finder->mineral_1 == -1;
                    bool mineral_2 = finder->mineral_2 == -1;
                    bool mineral_3 = finder->mineral_3 == -1;

                    metal_1 = metal_1 || tile->metals[finder->metal_1];
                    metal_2 = metal_2 || tile->metals[finder->metal_2];
                    metal_3 = metal_3 || tile->metals[finder->metal_3];
                    economic_1 = economic_1 || tile->economics[finder->economic_1];
                    economic_2 = economic_2 || tile->economics[finder->economic_2];
                    economic_3 = economic_3 || tile->economics[finder->economic_3];
                    mineral_1 = mineral_1 || tile->minerals[finder->mineral_1];
                    mineral_2 = mineral_2 || tile->minerals[finder->mineral_2];
                    mineral_3 = mineral_3 || tile->minerals[finder->mineral_3];

                    if (!metal_1 ||
                        !metal_2 ||
                        !metal_3 ||
                        !economic_1 ||
                        !economic_2 ||
                        !economic_3 ||
                        !mineral_1 ||
                        !mineral_2 ||
                        !mineral_3) return false;
                }
            }
            else {  //  Not surveyed
                    //  Savagery
                for (uint8_t i = 0; i < 3; i++)
                {
                    switch (finder->savagery[i]) {
                    case embark_assist::defs::evil_savagery_values::NA:
                        break;  //  No restriction

                    case embark_assist::defs::evil_savagery_values::All:
                        if (tile->savagery_count[i] == 0) return false;
                        break;

                    case embark_assist::defs::evil_savagery_values::Present:
                        if (tile->savagery_count[i] == 0) return false;
                        break;

                    case embark_assist::defs::evil_savagery_values::Absent:
                        if (tile->savagery_count[i] == 256) return false;
                        break;
                    }
                }

                // Evilness
                for (uint8_t i = 0; i < 3; i++)
                {
                    switch (finder->evilness[i]) {
                    case embark_assist::defs::evil_savagery_values::NA:
                        break;  //  No restriction

                    case embark_assist::defs::evil_savagery_values::All:
                        if (tile->evilness_count[i] == 0) return false;
                        break;

                    case embark_assist::defs::evil_savagery_values::Present:
                        if (tile->evilness_count[i] == 0) return false;
                        break;

                    case embark_assist::defs::evil_savagery_values::Absent:
                        if (tile->evilness_count[i] == 256) return false;
                        break;
                    }
                }

                // Aquifer
                switch (finder->aquifer) {
                case embark_assist::defs::aquifer_ranges::NA:
                    break;  //  No restriction

                case embark_assist::defs::aquifer_ranges::All:
                    if (tile->aquifer_count == 0) return false;
                    break;

                case embark_assist::defs::aquifer_ranges::Present:
                    if (tile->aquifer_count == 0) return false;
                    break;

                case embark_assist::defs::aquifer_ranges::Partial:
                    if (tile->aquifer_count == 0 ||
                        tile->aquifer_count == 256) return false;
                    break;

                case embark_assist::defs::aquifer_ranges::Not_All:
                    if (tile->aquifer_count == 256) return false;
                    break;

                case embark_assist::defs::aquifer_ranges::Absent:
                    if (tile->aquifer_count == 256) return false;
                    break;
                }

                // River size
                switch (tile->river_size) {
                case embark_assist::defs::river_sizes::None:
                    if (finder->min_river > embark_assist::defs::river_ranges::None) return false;
                    break;

                case embark_assist::defs::river_sizes::Brook:
                    if (finder->min_river > embark_assist::defs::river_ranges::Brook) return false;
                    break;

                case embark_assist::defs::river_sizes::Stream:
                    if (finder->min_river > embark_assist::defs::river_ranges::Stream) return false;
                    break;

                case embark_assist::defs::river_sizes::Minor:
                    if (finder->min_river > embark_assist::defs::river_ranges::Minor) return false;
                    break;

                case embark_assist::defs::river_sizes::Medium:
                    if (finder->min_river > embark_assist::defs::river_ranges::Medium) return false;
                    break;

                case embark_assist::defs::river_sizes::Major:
                    if (finder->max_river != embark_assist::defs::river_ranges::NA) return false;
                    break;
                }

                //  Waterfall
                if (finder->waterfall == embark_assist::defs::yes_no_ranges::Yes &&
                    tile->river_size == embark_assist::defs::river_sizes::None) return false;

                //  Flat. No world tile checks. Need to look at the details

                //  Clay
                switch (finder->clay) {
                case embark_assist::defs::present_absent_ranges::NA:
                    break;  //  No restriction

                case embark_assist::defs::present_absent_ranges::Present:
                    if (tile->clay_count == 0) return false;
                    break;
                case embark_assist::defs::present_absent_ranges::Absent:
                    if (tile->clay_count == 256) return false;
                    break;
                }

                //  Sand
                switch (finder->sand) {
                case embark_assist::defs::present_absent_ranges::NA:
                    break;  //  No restriction

                case embark_assist::defs::present_absent_ranges::Present:
                    if (tile->sand_count == 0) return false;
                    break;
                case embark_assist::defs::present_absent_ranges::Absent:
                    if (tile->sand_count == 256) return false;
                    break;
                }

                //  Flux
                switch (finder->flux) {
                case embark_assist::defs::present_absent_ranges::NA:
                    break;  //  No restriction

                case embark_assist::defs::present_absent_ranges::Present:
                    if (tile->flux_count == 0) return false;
                    break;
                case embark_assist::defs::present_absent_ranges::Absent:
                    if (tile->flux_count == 256) return false;
                    break;
                }

                //  Soil Min
                switch (finder->soil_min) {
                case embark_assist::defs::soil_ranges::NA:
                case embark_assist::defs::soil_ranges::None:
                    break;  //  No restriction

                case embark_assist::defs::soil_ranges::Very_Shallow:
                    if (tile->max_region_soil < 1) return false;
                    break;

                case embark_assist::defs::soil_ranges::Shallow:
                    if (tile->max_region_soil < 2) return false;
                    break;

                case embark_assist::defs::soil_ranges::Deep:
                    if (tile->max_region_soil < 3) return false;
                    break;

                case embark_assist::defs::soil_ranges::Very_Deep:
                    if (tile->max_region_soil < 4) return false;
                    break;
                }

                //  soil_min_everywhere only applies on the detailed level

                //  Soil Max
                //  Can't say anything as the preliminary data isn't reliable

                //  Evil Weather
                switch (finder->evil_weather) {
                case embark_assist::defs::yes_no_ranges::NA:
                    break;  //  No restriction

                case embark_assist::defs::yes_no_ranges::Yes:
                    if (!tile->evil_weather_possible) return false;
                    break;

                case embark_assist::defs::yes_no_ranges::No:
                    if (tile->evil_weather_full) return false;
                    break;
                }

                //  Reanimating
                switch (finder->reanimation) {
                case embark_assist::defs::yes_no_ranges::NA:
                    break;  //  No restriction

                case embark_assist::defs::yes_no_ranges::Yes:
                    if (!tile->reanimating_possible) return false;
                    break;

                case embark_assist::defs::yes_no_ranges::No:
                    if (tile->reanimating_full) return false;
                    break;
                }

                //  Thralling
                switch (finder->thralling) {
                case embark_assist::defs::yes_no_ranges::NA:
                    break;  //  No restriction

                case embark_assist::defs::yes_no_ranges::Yes:
                    if (!tile->thralling_possible) return false;
                    break;

                case embark_assist::defs::yes_no_ranges::No:
                    if (tile->thralling_full) return false;
                    break;
                }

                //  Biome Count Min (Can't do anything with Max at this level)
                if (finder->biome_count_min > tile->biome_count) return false;

                //  Region Type 1
                if (finder->region_type_1 != -1) {
                    found = false;

                    for (uint8_t k = 1; k < 10; k++) {
                        if (tile->biome_index[k] != -1) {
                            if (world_data->regions[tile->biome_index[k]]->type == finder->region_type_1) {
                                found = true;
                                break;
                            }
                        }

                        if (found) break;
                    }

                    if (!found) return false;
                }

                //  Region Type 2
                if (finder->region_type_2 != -1) {
                    found = false;

                    for (uint8_t k = 1; k < 10; k++) {
                        if (tile->biome_index[k] != -1) {
                            if (world_data->regions[tile->biome_index[k]]->type == finder->region_type_2) {
                                found = true;
                                break;
                            }
                        }

                        if (found) break;
                    }

                    if (!found) return false;
                }

                //  Region Type 3
                if (finder->region_type_3 != -1) {
                    found = false;

                    for (uint8_t k = 1; k < 10; k++) {
                        if (tile->biome_index[k] != -1) {
                            if (world_data->regions[tile->biome_index[k]]->type == finder->region_type_3) {
                                found = true;
                                break;
                            }
                        }

                        if (found) break;
                    }

                    if (!found) return false;
                }

                //  Biome 1
                if (finder->biome_1 != -1) {
                    found = false;

                    for (uint8_t i = 1; i < 10; i++) {
                        if (tile->biome[i] == finder->biome_1) {
                            found = true;
                            break;
                        }
                    }

                    if (!found) return false;
                }

                //  Biome 2
                if (finder->biome_2 != -1) {
                    found = false;

                    for (uint8_t i = 1; i < 10; i++) {
                        if (tile->biome[i] == finder->biome_2) {
                            found = true;
                            break;
                        }
                    }

                    if (!found) return false;
                }

                //  Biome 3
                if (finder->biome_3 != -1) {
                    found = false;

                    for (uint8_t i = 1; i < 10; i++) {
                        if (tile->biome[i] == finder->biome_3) {
                            found = true;
                            break;
                        }
                    }

                    if (!found) return false;
                }

                if (finder->metal_1 != -1 ||
                    finder->metal_2 != -1 ||
                    finder->metal_3 != -1 ||
                    finder->economic_1 != -1 ||
                    finder->economic_2 != -1 ||
                    finder->economic_3 != -1 ||
                    finder->mineral_1 != -1 ||
                    finder->mineral_2 != -1 ||
                    finder->mineral_3 != -1) {
                    count = 0;
                    bool metal_1 = finder->metal_1 == -1;
                    bool metal_2 = finder->metal_2 == -1;
                    bool metal_3 = finder->metal_3 == -1;
                    bool economic_1 = finder->economic_1 == -1;
                    bool economic_2 = finder->economic_2 == -1;
                    bool economic_3 = finder->economic_3 == -1;
                    bool mineral_1 = finder->mineral_1 == -1;
                    bool mineral_2 = finder->mineral_2 == -1;
                    bool mineral_3 = finder->mineral_3 == -1;

                    metal_1 = metal_1 || tile->metals[finder->metal_1];
                    metal_2 = metal_2 || tile->metals[finder->metal_2];
                    metal_3 = metal_3 || tile->metals[finder->metal_3];
                    economic_1 = economic_1 || tile->economics[finder->economic_1];
                    economic_2 = economic_2 || tile->economics[finder->economic_2];
                    economic_3 = economic_3 || tile->economics[finder->economic_3];
                    mineral_1 = mineral_1 || tile->minerals[finder->mineral_1];
                    mineral_2 = mineral_2 || tile->minerals[finder->mineral_2];
                    mineral_3 = mineral_3 || tile->minerals[finder->mineral_3];

                    if (!metal_1 ||
                        !metal_2 ||
                        !metal_3 ||
                        !economic_1 ||
                        !economic_2 ||
                        !economic_3 ||
                        !mineral_1 ||
                        !mineral_2 ||
                        !mineral_3) return false;
                }
            }
            return true;
        }

        //=======================================================================================

        uint32_t preliminary_world_match(embark_assist::defs::world_tile_data *survey_results,
            embark_assist::defs::finders *finder,
            embark_assist::defs::match_results *match_results) {
            //            color_ostream_proxy out(Core::getInstance().getConsole());
            uint32_t count = 0;
            for (uint16_t i = 0; i < world->worldgen.worldgen_parms.dim_x; i++) {
                for (uint16_t k = 0; k < world->worldgen.worldgen_parms.dim_y; k++) {
                    match_results->at(i).at(k).preliminary_match =
                        world_tile_match(survey_results, i, k, finder);
                    if (match_results->at(i).at(k).preliminary_match) count++;
                    match_results->at(i).at(k).contains_match = false;
                }
            }

            return count;
        }

        //=======================================================================================

        void match_world_tile(embark_assist::defs::geo_data *geo_summary,
            embark_assist::defs::world_tile_data *survey_results,
            embark_assist::defs::finders *finder,
            embark_assist::defs::match_results *match_results,
            uint16_t x,
            uint16_t y) {

//            color_ostream_proxy out(Core::getInstance().getConsole());
            embark_assist::defs::mid_level_tiles mlt;

            embark_assist::survey::survey_mid_level_tile(geo_summary,
                survey_results,
                &mlt);

            mid_level_tile_match(survey_results,
                &mlt,
                x,
                y,
                finder,
                match_results);
        }
    }
}

//=======================================================================================
//  Visible operations
//=======================================================================================

void embark_assist::matcher::move_cursor(uint16_t x, uint16_t y) {
    //            color_ostream_proxy out(Core::getInstance().getConsole());
    auto screen = Gui::getViewscreenByType<df::viewscreen_choose_start_sitest>(0);
    uint16_t original_x = screen->location.region_pos.x;
    uint16_t original_y = screen->location.region_pos.y;

    uint16_t large_x = std::abs(original_x - x) / 10;
    uint16_t small_x = std::abs(original_x - x) % 10;
    uint16_t large_y = std::abs(original_y - y) / 10;
    uint16_t small_y = std::abs(original_y - y) % 10;

    while (large_x > 0 || large_y > 0) {
        if (large_x > 0 && large_y > 0) {
            if (original_x - x > 0 && original_y - y > 0) {
                screen->feed_key(df::interface_key::CURSOR_UPLEFT_FAST);
            }
            else if (original_x - x > 0 && original_y - y < 0) {
                screen->feed_key(df::interface_key::CURSOR_DOWNLEFT_FAST);
            }
            else if (original_y - y > 0) {
                screen->feed_key(df::interface_key::CURSOR_UPRIGHT_FAST);
            }
            else {
                screen->feed_key(df::interface_key::CURSOR_DOWNRIGHT_FAST);
            }
            large_x--;
            large_y--;
        }
        else if (large_x > 0) {
            if (original_x - x > 0) {
                screen->feed_key(df::interface_key::CURSOR_LEFT_FAST);
            }
            else {
                screen->feed_key(df::interface_key::CURSOR_RIGHT_FAST);
            }
            large_x--;
        }
        else {
            if (original_y - y > 0) {
                screen->feed_key(df::interface_key::CURSOR_UP_FAST);
            }
            else {
                screen->feed_key(df::interface_key::CURSOR_DOWN_FAST);
            }
            large_y--;
        }
    }

    while (small_x > 0 || small_y > 0) {
        if (small_x > 0 && small_y > 0) {
            if (original_x - x > 0 && original_y - y > 0) {
                screen->feed_key(df::interface_key::CURSOR_UPLEFT);
            }
            else if (original_x - x > 0 && original_y - y < 0) {
                screen->feed_key(df::interface_key::CURSOR_DOWNLEFT);
            }
            else if (original_y - y > 0) {
                screen->feed_key(df::interface_key::CURSOR_UPRIGHT);
            }
            else {
                screen->feed_key(df::interface_key::CURSOR_DOWNRIGHT);
            }
            small_x--;
            small_y--;
        }
        else if (small_x > 0) {
            if (original_x - x > 0) {
                screen->feed_key(df::interface_key::CURSOR_LEFT);
            }
            else {
                screen->feed_key(df::interface_key::CURSOR_RIGHT);
            }
            small_x--;
        }
        else {
            if (original_y - y > 0) {
                screen->feed_key(df::interface_key::CURSOR_UP);
            }
            else {
                screen->feed_key(df::interface_key::CURSOR_DOWN);
            }
            small_y--;
        }
    }
}

//=======================================================================================

uint16_t embark_assist::matcher::find(embark_assist::defs::match_iterators *iterator,
    embark_assist::defs::geo_data *geo_summary,
    embark_assist::defs::world_tile_data *survey_results,
    embark_assist::defs::match_results *match_results) {

    color_ostream_proxy out(Core::getInstance().getConsole());
    auto screen = Gui::getViewscreenByType<df::viewscreen_choose_start_sitest>(0);
    uint16_t x_end;
    uint16_t y_end;
    bool turn;
    uint16_t count;
    uint16_t preliminary_matches;

    if (!iterator->active) {
        embark_assist::survey::clear_results(match_results);

        //  Static check for impossible requirements
        //
        count = 0;
        for (uint8_t i = 0; i < 3; i++) {
            if (iterator->finder.evilness[i] == embark_assist::defs::evil_savagery_values::All) {
                count++;
            }
        }

        if (count > 1) {
            out.printerr("matcher::find: Will never find any due to multiple All evilness requirements\n");
            return 0;
        }

        count = 0;
        for (uint8_t i = 0; i < 3; i++) {
            if (iterator->finder.savagery[i] == embark_assist::defs::evil_savagery_values::All) {
                count++;
            }
        }

        if (count > 1) {
            out.printerr("matcher::find: Will never find any due to multiple All savagery requirements\n");
            return 0;
        }

        if (iterator->finder.max_river < iterator->finder.min_river &&
            iterator->finder.max_river != embark_assist::defs::river_ranges::NA) {
            out.printerr("matcher::find: Will never find any due to max river < min river\n");
            return 0;
        }

        if (iterator->finder.waterfall == embark_assist::defs::yes_no_ranges::Yes &&
            iterator->finder.max_river == embark_assist::defs::river_ranges::None) {
            out.printerr("matcher::find: Will never find any waterfalls with None as max river\n");
            return 0;
        }

        if (iterator->finder.soil_max < iterator->finder.soil_min &&
            iterator->finder.soil_max != embark_assist::defs::soil_ranges::NA) {
            out.printerr("matcher::find: Will never find any matches with max soil < min soil\n");
            return 0;
        }

        if (iterator->finder.biome_count_max < iterator->finder.biome_count_min &&
            iterator->finder.biome_count_max != -1) {
            out.printerr("matcher::find: Will never find any matches with max biomes < min biomes\n");
            return 0;
        }

        preliminary_matches = preliminary_world_match(survey_results, &iterator->finder, match_results);

        if (preliminary_matches == 0) {
            out.printerr("matcher::find: Preliminarily matching world tiles: %i\n", preliminary_matches);
            return 0;
        }
        else {
            out.print("matcher::find: Preliminarily matching world tiles: %i\n", preliminary_matches);
        }

        while (screen->location.region_pos.x != 0 || screen->location.region_pos.y != 0) {
            screen->feed_key(df::interface_key::CURSOR_UPLEFT_FAST);
        }
        iterator->active = true;
        iterator->i = 0;
        iterator->k = 0;
        iterator->x_right = true;
        iterator->y_down = true;
        iterator->inhibit_x_turn = false;
        iterator->inhibit_y_turn = false;
        iterator->count = 0;
    }

    if ((iterator->k == world->worldgen.worldgen_parms.dim_x / 16 && iterator->x_right) ||
        (iterator->k == 0 && !iterator->x_right)) {
        x_end = 0;
    }
    else {
        x_end = 15;
    }

    if (iterator->i == world->worldgen.worldgen_parms.dim_y / 16) {
        y_end = 0;
    }
    else {
        y_end = 15;
    }

    for (uint16_t l = 0; l <= x_end; l++) {
        for (uint16_t m = 0; m <= y_end; m++) {
            //  This is where the payload goes
            if (match_results->at(screen->location.region_pos.x).at(screen->location.region_pos.y).preliminary_match) {
                match_world_tile(geo_summary,
                    survey_results,
                    &iterator->finder,
                    match_results,
                    screen->location.region_pos.x,
                    screen->location.region_pos.y);
                if (match_results->at(screen->location.region_pos.x).at(screen->location.region_pos.y).contains_match) {
                    iterator->count++;
                }
            }
            else {
                for (uint16_t n = 0; n < 16; n++) {
                    for (uint16_t p = 0; p < 16; p++) {
                        match_results->at(screen->location.region_pos.x).at(screen->location.region_pos.y).mlt_match[n][p] = false;
                    }
                }
            }
            //  End of payload section

            if (m != y_end) {
                if (iterator->y_down) {
                    screen->feed_key(df::interface_key::CURSOR_DOWN);
                }
                else {
                    screen->feed_key(df::interface_key::CURSOR_UP);
                }
            }
            else {
                if (screen->location.region_pos.x != 0 &&
                    screen->location.region_pos.x != world->worldgen.worldgen_parms.dim_x - 1) {
                    turn = true;
                }
                else {
                    iterator->inhibit_y_turn = !iterator->inhibit_y_turn;
                    turn = iterator->inhibit_y_turn;
                }

                if (turn) {
                    iterator->y_down = !iterator->y_down;
                }
                else {
                    if (iterator->y_down) {
                        screen->feed_key(df::interface_key::CURSOR_DOWN);
                    }
                    else {
                        screen->feed_key(df::interface_key::CURSOR_UP);
                    }
                }
            }
        }

        if (iterator->x_right) {  //  Won't do anything at the edge, so we don't bother filter those cases.
            screen->feed_key(df::interface_key::CURSOR_RIGHT);
        }
        else {
            screen->feed_key(df::interface_key::CURSOR_LEFT);
        }

        if (!iterator->x_right &&
            screen->location.region_pos.x == 0) {
            turn = !turn;

            if (turn) {
                iterator->x_right = true;
            }
        }
        else if (iterator->x_right &&
            screen->location.region_pos.x == world->worldgen.worldgen_parms.dim_x - 1) {
            turn = !turn;

            if (turn) {
                iterator->x_right = false;
            }
        }
    }
    //        }

    iterator->k++;
    if (iterator->k > world->worldgen.worldgen_parms.dim_x / 16)
    {
        iterator->k = 0;
        iterator->i++;
        iterator->active = !(iterator->i > world->worldgen.worldgen_parms.dim_y / 16);

        if (!iterator->active) {
            embark_assist::matcher::move_cursor(iterator->x, iterator->y);
        }
    }

    return iterator->count;
}
