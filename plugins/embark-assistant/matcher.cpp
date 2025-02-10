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
#include "df/world_region.h"
#include "df/world_region_details.h"
#include "df/world_region_type.h"

#include "matcher.h"
#include "survey.h"

using df::global::world;

namespace embark_assist {
    namespace matcher {

        //=======================================================================================

        struct matcher_info {
            bool savagery_found[3] = { false, false, false };
            bool evilness_found[3] = { false, false, false };
            uint8_t aquifer = embark_assist::defs::Clear_Aquifer_Bits;
            bool river_found = false;
            uint8_t max_waterfall = 0;
            uint16_t elevation;
            bool clay_found = false;
            bool sand_found = false;
            bool flux_found = false;
            bool coal_found = false;
            uint8_t max_soil = 0;
            bool uneven = false;
            int16_t min_temperature;
            int16_t max_temperature;
            embark_assist::defs::tree_levels min_trees_found = embark_assist::defs::tree_levels::Heavily_Forested;
            embark_assist::defs::tree_levels max_trees_found = embark_assist::defs::tree_levels::None;
            bool blood_rain_found = false;
            bool permanent_syndrome_rain_found = false;
            bool temporary_syndrome_rain_found = false;
            bool reanimation_found = false;
            bool thralling_found = false;
            uint8_t spire_count = 0;
            int8_t magma_level = -1;
            bool biomes[ENUM_LAST_ITEM(biome_type) + 1];
            bool region_types[ENUM_LAST_ITEM(world_region_type) + 1];
            uint8_t biome_count;
            bool metal_1;
            bool metal_2;
            bool metal_3;
            bool economic_1;
            bool economic_2;
            bool economic_3;
            bool mineral_1;
            bool mineral_2;
            bool mineral_3;
        };

        struct states {
            embark_assist::defs::mid_level_tiles mlt;
        };

        static states *state = nullptr;

        //=======================================================================================

        void process_embark_incursion(matcher_info *result,
            embark_assist::defs::world_tile_data *survey_results,
            embark_assist::defs::mid_level_tile_incursion_base *mlt,  // Note this is a single tile, as opposed to most usages of this variable name.
            embark_assist::defs::finders *finder,
            int16_t elevation,
            uint16_t x,
            uint16_t y,
            bool *failed_match) {

            df::world_data *world_data = world->world_data;

            // Savagery & Evilness
                {
                    result->savagery_found[mlt->savagery_level] = true;
                    result->evilness_found[mlt->evilness_level] = true;

                    embark_assist::defs::evil_savagery_ranges l = embark_assist::defs::evil_savagery_ranges::Low;
                    while (true) {
                        if (mlt->savagery_level == static_cast<uint8_t>(l)) {
                            if (finder->savagery[static_cast <int>(l)] ==
                                embark_assist::defs::evil_savagery_values::Absent) {
                                *failed_match = true;
                                return;
                            }
                        }
                        else {
                            if (finder->savagery[static_cast <int>(l)] ==
                                embark_assist::defs::evil_savagery_values::All) {
                                *failed_match = true;
                                return;
                            }
                        }

                        if (mlt->evilness_level == static_cast<uint8_t>(l)) {
                            if (finder->evilness[static_cast <int>(l)] ==
                                embark_assist::defs::evil_savagery_values::Absent) {
                                *failed_match = true;
                                return;
                            }
                        }
                        else {
                            if (finder->evilness[static_cast <int>(l)] ==
                                embark_assist::defs::evil_savagery_values::All) {
                                *failed_match = true;
                                return;
                            }
                        }

                        if (l == embark_assist::defs::evil_savagery_ranges::High) break;
                        l = static_cast <embark_assist::defs::evil_savagery_ranges>(static_cast<int8_t>(l) + 1);
                    }
                }

                //  Aquifer
                result->aquifer |= mlt->aquifer;

                switch (finder->aquifer) {
                case embark_assist::defs::aquifer_ranges::NA:
                    break;

                case embark_assist::defs::aquifer_ranges::None:
                    if (result->aquifer != embark_assist::defs::None_Aquifer_Bit) {
                        *failed_match = true;
                        return;
                    }
                    break;

                case embark_assist::defs::aquifer_ranges::At_Most_Light:
                case embark_assist::defs::aquifer_ranges::None_Plus_Light:
                    if (result->aquifer & embark_assist::defs::Heavy_Aquifer_Bit) {
                        *failed_match = true;
                        return;
                    }
                    break;

                case embark_assist::defs::aquifer_ranges::Light:
                    if (result->aquifer != embark_assist::defs::Light_Aquifer_Bit) {
                        *failed_match = true;
                        return;
                    }
                    break;

                case embark_assist::defs::aquifer_ranges::At_Least_Light:
                case embark_assist::defs::aquifer_ranges::Light_Plus_Heavy:
                    if (result->aquifer & embark_assist::defs::None_Aquifer_Bit) {
                        *failed_match = true;
                        return;
                    }
                    break;

                case embark_assist::defs::aquifer_ranges::None_Plus_Heavy:
                    if (result->aquifer & embark_assist::defs::Light_Aquifer_Bit) {
                        *failed_match = true;
                        return;
                    }
                    break;

                case embark_assist::defs::aquifer_ranges::None_Plus_At_Least_Light:
                case embark_assist::defs::aquifer_ranges::At_Most_Light_Plus_Heavy:
                case embark_assist::defs::aquifer_ranges::None_Light_Heavy:
                    break;

                case embark_assist::defs::aquifer_ranges::Heavy:
                    if (result->aquifer != embark_assist::defs::Heavy_Aquifer_Bit) {
                        *failed_match = true;
                        return;
                    }
                    break;
                }

                //  River & Waterfall. N/A for incursions.

                //  Flat
                if (finder->flat == embark_assist::defs::yes_no_ranges::Yes &&
                    result->elevation != mlt->elevation) {
                    *failed_match = true;
                    return;
                }

                // Clay
                if (mlt->clay) {
                    if (finder->clay == embark_assist::defs::present_absent_ranges::Absent) {
                        *failed_match = true;
                        return;
                    }
                    result->clay_found = true;
                }

                // Sand
                if (mlt->sand) {
                    if (finder->sand == embark_assist::defs::present_absent_ranges::Absent) {
                        *failed_match = true;
                        return;
                    }
                    result->sand_found = true;
                }

                // Flux. N/A for incursions.
                // Coal. N/A for incursions

                //  Min Soil
                if (finder->soil_min != embark_assist::defs::soil_ranges::NA &&
                    mlt->soil_depth < static_cast<uint16_t>(finder->soil_min) &&
                    finder->soil_min_everywhere == embark_assist::defs::all_present_ranges::All) {
                    *failed_match = true;
                    return;
                }

                if (result->max_soil < mlt->soil_depth) {
                    result->max_soil = mlt->soil_depth;
                }

                //  Max Soil
                if (finder->soil_max != embark_assist::defs::soil_ranges::NA &&
                    mlt->soil_depth > static_cast<uint16_t>(finder->soil_max)) {
                    *failed_match = true;
                    return;
                }

                //  Freezing
                if (result->min_temperature > survey_results->at(x).at(y).min_temperature[mlt->biome_offset]) {
                    result->min_temperature = survey_results->at(x).at(y).min_temperature[mlt->biome_offset];
                }

                if (result->max_temperature < survey_results->at(x).at(y).max_temperature[mlt->biome_offset]) {
                    result->max_temperature = survey_results->at(x).at(y).max_temperature[mlt->biome_offset];
                }

                if (result->min_temperature <= 0 &&
                    finder->freezing == embark_assist::defs::freezing_ranges::Never) {
                    *failed_match = true;
                    return;
                }

                if (result->max_temperature > 0 &&
                    finder->freezing == embark_assist::defs::freezing_ranges::Permanent) {
                    *failed_match = true;
                    return;
                }

                // Trees
                if (result->min_trees_found > mlt->trees) {
                    result->min_trees_found = mlt->trees;

                    switch (finder->min_trees) {
                    case embark_assist::defs::tree_ranges::NA:
                    case embark_assist::defs::tree_ranges::None:
                        break;

                    case embark_assist::defs::tree_ranges::Very_Scarce:
                        if (result->min_trees_found < embark_assist::defs::tree_levels::Very_Scarce) {
                            *failed_match = true;
                            return;
                        }
                        break;

                    case embark_assist::defs::tree_ranges::Scarce:
                        if (result->min_trees_found < embark_assist::defs::tree_levels::Scarce) {
                            *failed_match = true;
                            return;
                        }

                    case embark_assist::defs::tree_ranges::Woodland:
                        if (result->min_trees_found < embark_assist::defs::tree_levels::Woodland) {
                            *failed_match = true;
                            return;
                        }
                        break;

                    case embark_assist::defs::tree_ranges::Heavily_Forested:
                        if (result->min_trees_found < embark_assist::defs::tree_levels::Heavily_Forested) {
                            *failed_match = true;
                            return;
                        }
                        break;
                    }
                }

                if (result->max_trees_found < mlt->trees) {
                    result->max_trees_found = mlt->trees;

                    switch (finder->max_trees) {
                    case embark_assist::defs::tree_ranges::NA:
                    case embark_assist::defs::tree_ranges::Heavily_Forested:
                        break;

                    case embark_assist::defs::tree_ranges::None:
                        if (result->max_trees_found > embark_assist::defs::tree_levels::None) {
                            *failed_match = true;
                            return;
                        }
                        break;

                    case embark_assist::defs::tree_ranges::Very_Scarce:
                        if (result->max_trees_found > embark_assist::defs::tree_levels::Very_Scarce) {
                            *failed_match = true;
                            return;
                        }
                        break;

                    case embark_assist::defs::tree_ranges::Scarce:
                        if (result->max_trees_found > embark_assist::defs::tree_levels::Scarce) {
                            *failed_match = true;
                            return;
                        }
                        break;

                    case embark_assist::defs::tree_ranges::Woodland:
                        if (result->max_trees_found > embark_assist::defs::tree_levels::Woodland) {
                            *failed_match = true;
                            return;
                        }
                        break;
                    }
                }

                //  Blood Rain
                if (survey_results->at(x).at(y).blood_rain[mlt->biome_offset]) {
                    if (finder->blood_rain == embark_assist::defs::yes_no_ranges::No) {
                        *failed_match = true;
                        return;
                    }
                    result->blood_rain_found = true;
                }

                // Syndrome Rain, Permanent
                if (survey_results->at(x).at(y).permanent_syndrome_rain[mlt->biome_offset]) {
                    if (finder->syndrome_rain == embark_assist::defs::syndrome_rain_ranges::Temporary ||
                        finder->syndrome_rain == embark_assist::defs::syndrome_rain_ranges::Not_Permanent ||
                        finder->syndrome_rain == embark_assist::defs::syndrome_rain_ranges::None) {
                        *failed_match = true;
                        return;
                    }
                    result->permanent_syndrome_rain_found = true;
                }

                // Syndrome Rain, Temporary
                if (survey_results->at(x).at(y).temporary_syndrome_rain[mlt->biome_offset]) {
                    if (finder->syndrome_rain == embark_assist::defs::syndrome_rain_ranges::Permanent ||
                        finder->syndrome_rain == embark_assist::defs::syndrome_rain_ranges::None) {
                        *failed_match = true;
                        return;
                    }
                    result->temporary_syndrome_rain_found = true;
                }

                //  Reanmation
                if (survey_results->at(x).at(y).reanimating[mlt->biome_offset]) {
                    if (finder->reanimation == embark_assist::defs::reanimation_ranges::Thralling ||
                        finder->reanimation == embark_assist::defs::reanimation_ranges::None) {
                        *failed_match = true;
                        return;
                    }
                    result->reanimation_found = true;
                }

                //  Thralling
                if (survey_results->at(x).at(y).thralling[mlt->biome_offset]) {
                    if (finder->reanimation == embark_assist::defs::reanimation_ranges::Reanimation ||
                        finder->reanimation == embark_assist::defs::reanimation_ranges::Not_Thralling ||
                        finder->reanimation == embark_assist::defs::reanimation_ranges::None) {
                        *failed_match = true;
                        return;
                    }
                    result->thralling_found = true;
                }

                //  Spires. N/A for incursions
                //  Magma. N/A for incursions
                //  Biomes

                result->biomes[survey_results->at(x).at(y).biome[mlt->biome_offset]] = true;

                //  Region Type
                result->region_types[world_data->regions[survey_results->at(x).at(y).biome_index[mlt->biome_offset]]->type] = true;

                //  Metals. N/A for incursions
                //  Economics. N/A for incursions
                //  Neighbors. N/A for incursions
        }

        //=======================================================================================


        void process_embark_incursion_mid_level_tile(uint8_t from_direction,
            matcher_info *result,
            embark_assist::defs::world_tile_data *survey_results,
            embark_assist::defs::mid_level_tiles *mlt,
            embark_assist::defs::finders *finder,
            uint16_t x,
            uint16_t y,
            uint8_t i,
            uint8_t k,
            bool *failed_match) {
            int8_t fetch_i = i;
            int8_t fetch_k = k;
            int16_t fetch_x = x;
            int16_t fetch_y = y;
            df::world_data *world_data = world->world_data;

            //  Logic can be implemented with modulo and division, but that's harder to read.
            switch (from_direction) {
            case embark_assist::defs::directions::Northwest:
                fetch_i = i - 1;
                fetch_k = k - 1;
                break;

            case embark_assist::defs::directions::North:
                fetch_k = k - 1;
                break;

            case embark_assist::defs::directions::Northeast:
                fetch_i = i + 1;
                fetch_k = k - 1;
                break;

            case embark_assist::defs::directions::West:
                fetch_i = i - 1;
                break;

            case embark_assist::defs::directions::Center:
                return;   //  Own tile provides the data, so there's no incursion.
                break;

            case embark_assist::defs::directions::East:
                fetch_i = i + 1;
                break;

            case embark_assist::defs::directions::Southwest:
                fetch_i = i - 1;
                fetch_k = k + 1;
                break;

            case embark_assist::defs::directions::South:
                fetch_k = k + 1;
                break;

            case embark_assist::defs::directions::Southeast:
                fetch_i = i + 1;
                fetch_k = k + 1;
            }

            if (fetch_i < 0) {
                fetch_x = x - 1;
            }
            else if (fetch_i > 15) {
                fetch_x = x + 1;
            }

            if (fetch_k < 0) {
                fetch_y = y - 1;
            }
            else if (fetch_k > 15) {
                fetch_y = y + 1;
            }

            if (fetch_x < 0 ||
                fetch_x == world_data->world_width ||
                fetch_y < 0 ||
                fetch_y == world_data->world_height) {
                return;  //  We're at the world edge, so no incursions from the outside.
            }

            if (!survey_results->at(fetch_x).at(fetch_y).surveyed) {
                // If the data has been collected, incursion processing should be performed to evaluate whether a match actually is present.
                // but if it hasn't we need to return with a failed_match
                *failed_match = true;
                return;
            }

            if (fetch_k < 0) {
                if (fetch_i < 0) {
                    process_embark_incursion(result,
                        survey_results,
                        &survey_results->at(fetch_x).at(fetch_y).south_row[15],
                        finder,
                        mlt->at(i).at(k).elevation,
                        fetch_x,
                        fetch_y,
                        failed_match);
                }
                else if (fetch_i > 15) {
                    process_embark_incursion(result,
                        survey_results,
                        &survey_results->at(fetch_x).at(fetch_y).south_row[0],
                        finder,
                        mlt->at(i).at(k).elevation,
                        fetch_x,
                        fetch_y,
                        failed_match);
                }
                else {
                    process_embark_incursion(result,
                        survey_results,
                        &survey_results->at(fetch_x).at(fetch_y).south_row[fetch_i],
                        finder,
                        mlt->at(i).at(k).elevation,
                        fetch_x,
                        fetch_y,
                        failed_match);
                }
            }
            else if (fetch_k > 15) {
                if (fetch_i < 0) {
                    process_embark_incursion(result,
                        survey_results,
                        &survey_results->at(fetch_x).at(fetch_y).north_row[15],
                        finder,
                        mlt->at(i).at(k).elevation,
                        fetch_x,
                        fetch_y,
                        failed_match);
                }
                else if (fetch_i > 15) {
                    process_embark_incursion(result,
                        survey_results,
                        &survey_results->at(fetch_x).at(fetch_y).north_row[0],
                        finder,
                        mlt->at(i).at(k).elevation,
                        fetch_x,
                        fetch_y,
                        failed_match);
                }
                else {
                    process_embark_incursion(result,
                        survey_results,
                        &survey_results->at(fetch_x).at(fetch_y).north_row[fetch_i],
                        finder,
                        mlt->at(i).at(k).elevation,
                        fetch_x,
                        fetch_y,
                        failed_match);
                }
            }
            else {
                if (fetch_i < 0) {
                    process_embark_incursion(result,
                        survey_results,
                        &survey_results->at(fetch_x).at(fetch_y).east_column[fetch_k],
                        finder,
                        mlt->at(i).at(k).elevation,
                        fetch_x,
                        fetch_y,
                        failed_match);
                }
                else if (fetch_i > 15) {
                    process_embark_incursion(result,
                        survey_results,
                        &survey_results->at(fetch_x).at(fetch_y).west_column[fetch_k],
                        finder,
                        mlt->at(i).at(k).elevation,
                        fetch_x,
                        fetch_y,
                        failed_match);
                }
                else {
                    process_embark_incursion(result,
                        survey_results,
                        &mlt->at(fetch_i).at(fetch_k),
                        finder,
                        mlt->at(i).at(k).elevation,
                        fetch_x,
                        fetch_y,
                        failed_match);
                }
            }
        }

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
            matcher_info result;
            result.elevation = mlt->at(start_x).at(start_y).elevation;
            result.min_temperature = survey_results->at(x).at(y).min_temperature[mlt->at(start_x).at(start_y).biome_offset];
            result.max_temperature = survey_results->at(x).at(y).max_temperature[mlt->at(start_x).at(start_y).biome_offset];
            result.metal_1 = finder->metal_1 == -1;
            result.metal_2 = finder->metal_2 == -1;
            result.metal_3 = finder->metal_3 == -1;
            result.economic_1 = finder->economic_1 == -1;
            result.economic_2 = finder->economic_2 == -1;
            result.economic_3 = finder->economic_3 == -1;
            result.mineral_1 = finder->mineral_1 == -1;
            result.mineral_2 = finder->mineral_2 == -1;
            result.mineral_3 = finder->mineral_3 == -1;
            bool failed_match = false;

            if (finder->biome_count_min != -1 ||
                finder->biome_count_max != -1 ||
                finder->biome_1 != -1 ||
                finder->biome_2 != -1 ||
                finder->biome_3 != -1) {
                for (uint8_t i = 0; i <= ENUM_LAST_ITEM(biome_type); i++) result.biomes[i] = false;
            }

            for (uint8_t i = 0; i <= ENUM_LAST_ITEM(world_region_type); i++) result.region_types[i] = false;

            for (uint16_t i = start_x; i < start_x + finder->x_dim; i++) {
                for (uint16_t k = start_y; k < start_y + finder->y_dim; k++) {

                    // Savagery & Evilness
                    {
                        result.savagery_found[mlt->at(i).at(k).savagery_level] = true;
                        result.evilness_found[mlt->at(i).at(k).evilness_level] = true;

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
                    result.aquifer |= mlt->at(i).at(k).aquifer;

                    switch (finder->aquifer) {
                    case embark_assist::defs::aquifer_ranges::NA:
                        break;

                    case embark_assist::defs::aquifer_ranges::None:
                        if (result.aquifer != embark_assist::defs::None_Aquifer_Bit) return false;
                        break;

                    case embark_assist::defs::aquifer_ranges::At_Most_Light:
                    case embark_assist::defs::aquifer_ranges::None_Plus_Light:
                        if (result.aquifer & embark_assist::defs::Heavy_Aquifer_Bit) return false;
                        break;

                    case embark_assist::defs::aquifer_ranges::Light:
                        if (result.aquifer != embark_assist::defs::Light_Aquifer_Bit) return false;
                        break;

                    case embark_assist::defs::aquifer_ranges::At_Least_Light:
                    case embark_assist::defs::aquifer_ranges::Light_Plus_Heavy:
                        if (result.aquifer & embark_assist::defs::None_Aquifer_Bit) return false;
                        break;

                    case embark_assist::defs::aquifer_ranges::None_Plus_Heavy:
                        if (result.aquifer & embark_assist::defs::Light_Aquifer_Bit) return false;
                        break;

                    case embark_assist::defs::aquifer_ranges::None_Plus_At_Least_Light:
                    case embark_assist::defs::aquifer_ranges::At_Most_Light_Plus_Heavy:
                    case embark_assist::defs::aquifer_ranges::None_Light_Heavy:
                        break;

                    case embark_assist::defs::aquifer_ranges::Heavy:
                        if (result.aquifer != embark_assist::defs::Heavy_Aquifer_Bit) return false;
                        break;
                    }

                    //  River & Waterfall
                    if (mlt->at(i).at(k).river_size != embark_assist::defs::river_sizes::None) {
                        if (finder->min_river != embark_assist::defs::river_ranges::NA &&
                            finder->min_river > static_cast<embark_assist::defs::river_ranges>(mlt->at(i).at(k).river_size)) return false;

                        if (finder->max_river != embark_assist::defs::river_ranges::NA &&
                            finder->max_river < static_cast<embark_assist::defs::river_ranges>(mlt->at(i).at(k).river_size)) return false;

                        if (i < start_x + finder->x_dim - 2 &&
                            mlt->at(i + 1).at(k).river_size != embark_assist::defs::river_sizes::None &&
                            abs(mlt->at(i).at(k).river_elevation - mlt->at(i + 1).at(k).river_elevation) > result.max_waterfall) {
                            if (finder->min_waterfall == 0) return false;  // 0 = Absent
                            result.max_waterfall =
                                abs(mlt->at(i).at(k).river_elevation - mlt->at(i + 1).at(k).river_elevation);
                        }

                        if (k < start_y + finder->y_dim - 2 &&
                            mlt->at(i).at(k + 1).river_size != embark_assist::defs::river_sizes::None &&
                            abs(mlt->at(i).at(k).river_elevation - mlt->at(i).at(k + 1).river_elevation) > result.max_waterfall) {
                            if (finder->min_waterfall == 0) return false;  // 0 = Absent
                            result.max_waterfall =
                                abs(mlt->at(i).at(k).river_elevation - mlt->at(i).at(k + 1).river_elevation);
                        }

                        result.river_found = true;
                    }

                    //  Flat
                    if (finder->flat == embark_assist::defs::yes_no_ranges::Yes &&
                        result.elevation != mlt->at(i).at(k).elevation) return false;

                    if (result.elevation != mlt->at(i).at(k).elevation) result.uneven = true;

                    // Clay
                    if (mlt->at(i).at(k).clay) {
                        if (finder->clay == embark_assist::defs::present_absent_ranges::Absent) return false;
                        result.clay_found = true;
                    }

                    // Sand
                    if (mlt->at(i).at(k).sand) {
                        if (finder->sand == embark_assist::defs::present_absent_ranges::Absent) return false;
                        result.sand_found = true;
                    }

                    // Flux
                    if (mlt->at(i).at(k).flux) {
                        if (finder->flux == embark_assist::defs::present_absent_ranges::Absent) return false;
                        result.flux_found = true;
                    }

                    // Coal
                    if (mlt->at(i).at(k).coal) {
                        if (finder->coal == embark_assist::defs::present_absent_ranges::Absent) return false;
                        result.coal_found = true;
                    }

                    //  Min Soil
                    if (finder->soil_min != embark_assist::defs::soil_ranges::NA &&
                        mlt->at(i).at(k).soil_depth < static_cast<uint16_t>(finder->soil_min) &&
                        finder->soil_min_everywhere == embark_assist::defs::all_present_ranges::All) return false;

                    if (result.max_soil < mlt->at(i).at(k).soil_depth) {
                        result.max_soil = mlt->at(i).at(k).soil_depth;
                    }

                    //  Max Soil
                    if (finder->soil_max != embark_assist::defs::soil_ranges::NA &&
                        mlt->at(i).at(k).soil_depth > static_cast<uint16_t>(finder->soil_max)) return false;

                    //  Freezing
                    if (result.min_temperature > survey_results->at(x).at(y).min_temperature[mlt->at(i).at(k).biome_offset]) {
                        result.min_temperature = survey_results->at(x).at(y).min_temperature[mlt->at(i).at(k).biome_offset];
                    }

                    if (result.max_temperature < survey_results->at(x).at(y).max_temperature[mlt->at(i).at(k).biome_offset]) {
                        result.max_temperature = survey_results->at(x).at(y).max_temperature[mlt->at(i).at(k).biome_offset];
                    }

                    if (result.min_temperature <= 0 &&
                        finder->freezing == embark_assist::defs::freezing_ranges::Never) return false;

                    if (result.max_temperature > 0 &&
                        finder->freezing == embark_assist::defs::freezing_ranges::Permanent) return false;


                    // Trees
                    if (result.min_trees_found > mlt->at(i).at(k).trees) {
                        result.min_trees_found = mlt->at(i).at(k).trees;

                        switch (finder->min_trees) {
                        case embark_assist::defs::tree_ranges::NA:
                        case embark_assist::defs::tree_ranges::None:
                            break;

                        case embark_assist::defs::tree_ranges::Very_Scarce:
                            if (result.min_trees_found < embark_assist::defs::tree_levels::Very_Scarce) return false;
                            break;

                        case embark_assist::defs::tree_ranges::Scarce:
                            if (result.min_trees_found < embark_assist::defs::tree_levels::Scarce) return false;
                            break;

                        case embark_assist::defs::tree_ranges::Woodland:
                            if (result.min_trees_found < embark_assist::defs::tree_levels::Woodland) return false;
                            break;

                        case embark_assist::defs::tree_ranges::Heavily_Forested:
                            if (result.min_trees_found < embark_assist::defs::tree_levels::Heavily_Forested) return false;
                            break;
                        }
                    }

                    if (result.max_trees_found < mlt->at(i).at(k).trees) {
                        result.max_trees_found = mlt->at(i).at(k).trees;

                        switch (finder->max_trees) {
                        case embark_assist::defs::tree_ranges::NA:
                        case embark_assist::defs::tree_ranges::Heavily_Forested:
                            break;

                        case embark_assist::defs::tree_ranges::None:
                            if (result.max_trees_found > embark_assist::defs::tree_levels::None) return false;
                            break;

                        case embark_assist::defs::tree_ranges::Very_Scarce:
                            if (result.max_trees_found > embark_assist::defs::tree_levels::Very_Scarce) return false;
                            break;

                        case embark_assist::defs::tree_ranges::Scarce:
                            if (result.max_trees_found > embark_assist::defs::tree_levels::Scarce) return false;
                            break;

                        case embark_assist::defs::tree_ranges::Woodland:
                            if (result.max_trees_found > embark_assist::defs::tree_levels::Woodland) return false;
                            break;
                        }
                    }

                    //  Blood Rain
                    if (survey_results->at(x).at(y).blood_rain[mlt->at(i).at(k).biome_offset]) {
                        if (finder->blood_rain == embark_assist::defs::yes_no_ranges::No) return false;
                        result.blood_rain_found = true;
                    }

                    // Syndrome Rain, Permanent
                    if (survey_results->at(x).at(y).permanent_syndrome_rain[mlt->at(i).at(k).biome_offset]) {
                        if (finder->syndrome_rain == embark_assist::defs::syndrome_rain_ranges::Temporary ||
                            finder->syndrome_rain == embark_assist::defs::syndrome_rain_ranges::Not_Permanent ||
                            finder->syndrome_rain == embark_assist::defs::syndrome_rain_ranges::None) return false;
                        result.permanent_syndrome_rain_found = true;
                    }

                    // Syndrome Rain, Temporary
                    if (survey_results->at(x).at(y).temporary_syndrome_rain[mlt->at(i).at(k).biome_offset]) {
                        if (finder->syndrome_rain == embark_assist::defs::syndrome_rain_ranges::Permanent ||
                            finder->syndrome_rain == embark_assist::defs::syndrome_rain_ranges::None) return false;
                        result.temporary_syndrome_rain_found = true;
                    }

                    //  Reanmation
                    if (survey_results->at(x).at(y).reanimating[mlt->at(i).at(k).biome_offset]) {
                        if (finder->reanimation == embark_assist::defs::reanimation_ranges::Thralling ||
                            finder->reanimation == embark_assist::defs::reanimation_ranges::None) return false;
                        result.reanimation_found = true;
                    }

                    //  Thralling
                    if (survey_results->at(x).at(y).thralling[mlt->at(i).at(k).biome_offset]) {
                        if (finder->reanimation == embark_assist::defs::reanimation_ranges::Reanimation ||
                            finder->reanimation == embark_assist::defs::reanimation_ranges::Not_Thralling ||
                            finder->reanimation == embark_assist::defs::reanimation_ranges::None) return false;
                        result.thralling_found = true;
                    }

                    //  Spires
                    if (mlt->at(i).at(k).adamantine_level != -1) {
                        result.spire_count++;

                        if (finder->spire_count_max != -1 &&
                            finder->spire_count_max < result.spire_count) return false;
                    }

                    //  Magma
                    if (mlt->at(i).at(k).magma_level != -1) {
                        if (mlt->at(i).at(k).magma_level > result.magma_level)
                        {
                            result.magma_level = mlt->at(i).at(k).magma_level;
                            if (finder->magma_max != embark_assist::defs::magma_ranges::NA &&
                                static_cast<int8_t>(finder->magma_max) < result.magma_level) return false;
                        }
                    }

                    //  Biomes
                    result.biomes[survey_results->at(x).at(y).biome[mlt->at(i).at(k).biome_offset]] = true;

                    //  Region Type
                    result.region_types[world_data->regions[survey_results->at(x).at(y).biome_index[mlt->at(i).at(k).biome_offset]]->type] = true;

                    //  Metals
                    result.metal_1 = result.metal_1 || mlt->at(i).at(k).metals[finder->metal_1];
                    result.metal_2 = result.metal_2 || mlt->at(i).at(k).metals[finder->metal_2];
                    result.metal_3 = result.metal_3 || mlt->at(i).at(k).metals[finder->metal_3];

                    //  Economics
                    result.economic_1 = result.economic_1 || mlt->at(i).at(k).economics[finder->economic_1];
                    result.economic_2 = result.economic_2 || mlt->at(i).at(k).economics[finder->economic_2];
                    result.economic_3 = result.economic_3 || mlt->at(i).at(k).economics[finder->economic_3];

                    //  Minerals
                    result.mineral_1 = result.mineral_1 || mlt->at(i).at(k).minerals[finder->mineral_1];
                    result.mineral_2 = result.mineral_2 || mlt->at(i).at(k).minerals[finder->mineral_2];
                    result.mineral_3 = result.mineral_3 || mlt->at(i).at(k).minerals[finder->mineral_3];
                }
            }

            //  Take incursions into account.

            for (int8_t i = start_x; i < start_x + finder->x_dim; i++) {

                //  NW corner, north row
                if ((i == 0 && start_y == 0 && x - 1 >= 0 && y - 1 >= 0 && !survey_results->at(x - 1).at(y - 1).surveyed) ||
                    (i == 0 && x - 1 >= 0 && !survey_results->at(x - 1).at(y).surveyed) ||
                    (start_y == 0 && y - 1 >= 0 && !survey_results->at(x).at(y - 1).surveyed)) {
                    failed_match = true;
                }
                else {
                    process_embark_incursion_mid_level_tile
                    (embark_assist::survey::translate_corner(survey_results,
                        embark_assist::defs::directions::Center,
                        x,
                        y,
                        i,
                        start_y),
                        &result,
                        survey_results,
                        mlt,
                        finder,
                        x,
                        y,
                        i,
                        start_y,
                        &failed_match);
                }

                //  N edge, north row
                if (start_y == 0 && y - 1 >= 0 && !survey_results->at(x).at(y - 1).surveyed) {
                    failed_match = true;
                }
                else {
                    process_embark_incursion_mid_level_tile
                    (embark_assist::survey::translate_ns_edge(survey_results,
                        true,
                        x,
                        y,
                        i,
                        start_y),
                        &result,
                        survey_results,
                        mlt,
                        finder,
                        x,
                        y,
                        i,
                        start_y,
                        &failed_match);
                }

                //  NE corner, north row
                if ((i == 15 && start_y == 0 && x + 1 < world_data->world_width && y - 1 >= 0 && !survey_results->at(x + 1).at(y - 1).surveyed) ||
                    (i == 15 && x + 1 < world_data->world_width && !survey_results->at(x + 1).at(y).surveyed) ||
                    (start_y == 0 && y - 1 >= 0 && !survey_results->at(x).at(y - 1).surveyed)) {
                    failed_match = true;
                }
                else {
                    process_embark_incursion_mid_level_tile
                    (embark_assist::survey::translate_corner(survey_results,
                        embark_assist::defs::directions::East,
                        x,
                        y,
                        i,
                        start_y),
                        &result,
                        survey_results,
                        mlt,
                        finder,
                        x,
                        y,
                        i,
                        start_y,
                        &failed_match);
                }

                //  SW corner, south row
                if ((i == 0 && start_y + finder->y_dim == 16 && x - 1 >= 0 && y + 1 < world_data->world_height && !survey_results->at(x - 1).at(y + 1).surveyed) ||
                    (i == 0 && x - 1 >= 0 && !survey_results->at(x - 1).at(y).surveyed) ||
                    (start_y + finder->y_dim == 16 && y + 1 < world_data->world_height && !survey_results->at(x).at(y + 1).surveyed)) {
                    failed_match = true;
                }
                else {
                    process_embark_incursion_mid_level_tile
                    (embark_assist::survey::translate_corner(survey_results,
                        embark_assist::defs::directions::South,
                        x,
                        y,
                        i,
                        start_y + finder->y_dim - 1),
                        &result,
                        survey_results,
                        mlt,
                        finder,
                        x,
                        y,
                        i,
                        start_y + finder->y_dim - 1,
                        &failed_match);
                }

                //  S edge, south row
                if (start_y + finder->y_dim == 16 && y + 1 < world_data->world_height && !survey_results->at(x).at(y + 1).surveyed) {
                    failed_match = true;
                }
                else {
                    process_embark_incursion_mid_level_tile
                    (embark_assist::survey::translate_ns_edge(survey_results,
                        false,
                        x,
                        y,
                        i,
                        start_y + finder->y_dim - 1),
                        &result,
                        survey_results,
                        mlt,
                        finder,
                        x,
                        y,
                        i,
                        start_y + finder->y_dim - 1,
                        &failed_match);
                }

                //  SE corner south row
                if ((i == 15 && start_y + finder->y_dim == 16 && x + 1 < world_data->world_width && y + 1 < world_data->world_height && !survey_results->at(x + 1).at(y + 1).surveyed) ||
                    (i == 15 && x + 1 < world_data->world_width && !survey_results->at(x + 1).at(y).surveyed) ||
                    (start_y + finder->y_dim == 16 && y + 1 < world_data->world_height && !survey_results->at(x).at(y + 1).surveyed)) {
                    failed_match = true;
                }
                else {
                    process_embark_incursion_mid_level_tile
                    (embark_assist::survey::translate_corner(survey_results,
                        embark_assist::defs::directions::Southeast,
                        x,
                        y,
                        i,
                        start_y + finder->y_dim - 1),
                        &result,
                        survey_results,
                        mlt,
                        finder,
                        x,
                        y,
                        i,
                        start_y + finder->y_dim - 1,
                        &failed_match);
                }

                if (failed_match) return false;
           }

           for (int8_t k = start_y; k < start_y + finder->y_dim; k++) {
                // NW corner, west side
               if ((start_x == 0 && x - 1 >= 0 && !survey_results->at(x - 1).at(y).surveyed)) {
                   failed_match = true;
               }
               else if (k > start_y) { //    We've already covered the NW corner of the NW, with its complications.
                   process_embark_incursion_mid_level_tile
                   (embark_assist::survey::translate_corner(survey_results,
                       embark_assist::defs::directions::Center,
                       x,
                       y,
                       start_x,
                       k),
                       &result,
                       survey_results,
                       mlt,
                       finder,
                       x,
                       y,
                       start_x,
                       k,
                       &failed_match);
               }

               // W edge, west side
               if (start_x == 0 && x - 1 >= 0 && !survey_results->at(x - 1).at(y).surveyed) {
                   failed_match = true;
               }
               else {
                   process_embark_incursion_mid_level_tile
                   (embark_assist::survey::translate_ew_edge(survey_results,
                       true,
                       x,
                       y,
                       start_x,
                       k),
                       &result,
                       survey_results,
                       mlt,
                       finder,
                       x,
                       y,
                       start_x,
                       k,
                       &failed_match);
               }

                // SW corner, west side
               if (start_x == 0 && x - 1 >= 0 && !survey_results->at(x - 1).at(y).surveyed) {
                   failed_match = true;
               }
               else if (k < start_y + finder->y_dim - 1) { //  We've already covered the SW corner of the SW tile, with its complicatinons.
                   process_embark_incursion_mid_level_tile
                   (embark_assist::survey::translate_corner(survey_results,
                       embark_assist::defs::directions::South,
                       x,
                       y,
                       start_x,
                       k),
                       &result,
                       survey_results,
                       mlt,
                       finder,
                       x,
                       y,
                       start_x,
                       k,
                       &failed_match);
               }

                // NE corner, east side
               if ((start_x + finder->x_dim == 16 && x + 1 < world_data->world_width && !survey_results->at(x + 1).at(y).surveyed)) {
                   failed_match = true;
               }
               else if (k > start_y) { //  We've already covered the NE tile's NE corner, with its complications.
                   process_embark_incursion_mid_level_tile
                   (embark_assist::survey::translate_corner(survey_results,
                       embark_assist::defs::directions::East,
                       x,
                       y,
                       start_x + finder->x_dim - 1,
                       k),
                       &result,
                       survey_results,
                       mlt,
                       finder,
                       x,
                       y,
                       start_x + finder->x_dim - 1,
                       k,
                       &failed_match);
               }

               // E edge, east side
               if (start_x + finder->y_dim == 16 && x + 1 < world_data->world_width && !survey_results->at(x + 1).at(y).surveyed) {
                   failed_match = true;
               }
               else {
                   process_embark_incursion_mid_level_tile
                   (embark_assist::survey::translate_ew_edge(survey_results,
                       false,
                       x,
                       y,
                       start_x + finder->x_dim - 1,
                       k),
                       &result,
                       survey_results,
                       mlt,
                       finder,
                       x,
                       y,
                       start_x + finder->x_dim - 1,
                       k,
                       &failed_match);
               }

                // SE corner, east side
               if (start_x + finder->x_dim == 16 && x + 1 < world_data->world_width && !survey_results->at(x + 1).at(y).surveyed) {
                   failed_match = true;
               }
               else if (k < start_y + finder->y_dim - 1) { //  We've already covered the SE tile's SE corner, with its complications.
                   process_embark_incursion_mid_level_tile
                   (embark_assist::survey::translate_corner(survey_results,
                       embark_assist::defs::directions::Southeast,
                       x,
                       y,
                       start_x + finder->x_dim - 1,
                       k),
                       &result,
                       survey_results,
                       mlt,
                       finder,
                       x,
                       y,
                       start_x + finder->x_dim - 1,
                       k,
                       &failed_match);
               }

                if (failed_match) return false;
           }

            //  Summary section, for all the stuff that require the complete picture
            //
            // Savagery & Evilness
            {
                embark_assist::defs::evil_savagery_ranges l = embark_assist::defs::evil_savagery_ranges::Low;

                while (true) {
                    if (finder->savagery[static_cast <int>(l)] ==
                        embark_assist::defs::evil_savagery_values::Present &&
                        !result.savagery_found[static_cast<int>(l)]) return false;

                    if (finder->evilness[static_cast <int>(l)] ==
                        embark_assist::defs::evil_savagery_values::Present &&
                        !result.evilness_found[static_cast<int>(l)]) return false;

                    if (l == embark_assist::defs::evil_savagery_ranges::High) break;
                    l = static_cast <embark_assist::defs::evil_savagery_ranges>(static_cast<int8_t>(l) + 1);
                }
            }

            //  Aquifer
            switch (finder->aquifer) {
            case embark_assist::defs::aquifer_ranges::NA:
            case embark_assist::defs::aquifer_ranges::None:          //  Checked above
            case embark_assist::defs::aquifer_ranges::Light:         //  Ditto
            case embark_assist::defs::aquifer_ranges::Heavy:         //  Ditto
            case embark_assist::defs::aquifer_ranges::At_Most_Light: //  Ditto
            case embark_assist::defs::aquifer_ranges::At_Least_Light://  Ditto
                break;

            case embark_assist::defs::aquifer_ranges::None_Plus_Light:
                if (result.aquifer != (embark_assist::defs::None_Aquifer_Bit | embark_assist::defs::Light_Aquifer_Bit)) return false;
                break;

            case embark_assist::defs::aquifer_ranges::None_Plus_At_Least_Light:
                if (result.aquifer != (embark_assist::defs::None_Aquifer_Bit | embark_assist::defs::Light_Aquifer_Bit) &&
                    result.aquifer != (embark_assist::defs::None_Aquifer_Bit | embark_assist::defs::Heavy_Aquifer_Bit)) return false;
                break;

            case embark_assist::defs::aquifer_ranges::None_Plus_Heavy:
                if (result.aquifer != (embark_assist::defs::None_Aquifer_Bit | embark_assist::defs::Heavy_Aquifer_Bit)) return false;
                break;

            case embark_assist::defs::aquifer_ranges::At_Most_Light_Plus_Heavy:
                if (result.aquifer != (embark_assist::defs::None_Aquifer_Bit | embark_assist::defs::Heavy_Aquifer_Bit) &&
                    result.aquifer != (embark_assist::defs::Light_Aquifer_Bit | embark_assist::defs::Heavy_Aquifer_Bit)) return false;
                break;

            case embark_assist::defs::aquifer_ranges::Light_Plus_Heavy:
                if (result.aquifer != (embark_assist::defs::Light_Aquifer_Bit | embark_assist::defs::Heavy_Aquifer_Bit)) return false;
                break;

            case embark_assist::defs::aquifer_ranges::None_Light_Heavy:
                if (result.aquifer != (embark_assist::defs::None_Aquifer_Bit | embark_assist::defs::Light_Aquifer_Bit | embark_assist::defs::Heavy_Aquifer_Bit)) return false;
                break;
            }

            //  River & Waterfall
            if (!result.river_found && finder->min_river > embark_assist::defs::river_ranges::None) return false;
            if (result.max_waterfall < finder->min_waterfall) return false;  // N/A = -1 is always smaller, so no additional check needed.

            //  Flat
            if (!result.uneven && finder->flat == embark_assist::defs::yes_no_ranges::No) return false;

            //  Clay
            if (finder->clay == embark_assist::defs::present_absent_ranges::Present && !result.clay_found) return false;

            //  Sand
            if (finder->sand == embark_assist::defs::present_absent_ranges::Present && !result.sand_found) return false;

            //  Flux
            if (finder->flux == embark_assist::defs::present_absent_ranges::Present && !result.flux_found) return false;

            //  Coal
            if (finder->coal == embark_assist::defs::present_absent_ranges::Present && !result.coal_found) return false;

            //  Min Soil
            if (finder->soil_min != embark_assist::defs::soil_ranges::NA &&
                finder->soil_min_everywhere == embark_assist::defs::all_present_ranges::Present &&
                result.max_soil < static_cast<uint8_t>(finder->soil_min)) return false;

            //  Freezing
            if (finder->freezing == embark_assist::defs::freezing_ranges::At_Least_Partial &&
                result.min_temperature > 0) return false;

            if (finder->freezing == embark_assist::defs::freezing_ranges::Partial &&
                (result.min_temperature > 0 ||
                    result.max_temperature <= 0)) return false;

            if (finder->freezing == embark_assist::defs::freezing_ranges::At_Most_Partial &&
                result.max_temperature <= 0) return false;

            //  Trees. Mismatches already dealt with

            //  Blood Rain
            if (finder->blood_rain == embark_assist::defs::yes_no_ranges::Yes && !result.blood_rain_found) return false;

            //  Syndrome Rain
            if (finder->syndrome_rain == embark_assist::defs::syndrome_rain_ranges::Any && !result.permanent_syndrome_rain_found && !result.temporary_syndrome_rain_found) return false;
            if (finder->syndrome_rain == embark_assist::defs::syndrome_rain_ranges::Permanent && !result.permanent_syndrome_rain_found) return false;
            if (finder->syndrome_rain == embark_assist::defs::syndrome_rain_ranges::Temporary && !result.temporary_syndrome_rain_found) return false;

            //  Reanimation
            if (finder->reanimation == embark_assist::defs::reanimation_ranges::Both && !(result.reanimation_found && result.thralling_found)) return false;
            if (finder->reanimation == embark_assist::defs::reanimation_ranges::Any && !result.reanimation_found && !result.thralling_found) return false;
            if (finder->reanimation == embark_assist::defs::reanimation_ranges::Thralling && !result.thralling_found) return false;
            if (finder->reanimation == embark_assist::defs::reanimation_ranges::Reanimation && !result.reanimation_found) return false;

            //  Spires
            if (finder->spire_count_min != -1 && finder->spire_count_min > result.spire_count) return false;
            if (finder->spire_count_max != -1 && finder->spire_count_max < result.spire_count) return false;

            //  Magma
            if (// finder->magma_min != embark_assist::defs::magma_ranges::NA &&  //  This check is redundant.
                finder->magma_min > static_cast<embark_assist::defs::magma_ranges>(result.magma_level)) return false;

            //  Biomes
            if (finder->biome_count_min != -1 ||
                finder->biome_count_max != -1) {
                result.biome_count = 0;
                for (uint8_t i = 0; i <= ENUM_LAST_ITEM(biome_type); i++) {
                    if (result.biomes[i]) result.biome_count++;
                }

                if (result.biome_count < finder->biome_count_min ||
                    (finder->biome_count_max != -1 &&
                        finder->biome_count_max < result.biome_count)) return false;
            }

            if (finder->biome_1 != -1 && !result.biomes[finder->biome_1]) return false;
            if (finder->biome_2 != -1 && !result.biomes[finder->biome_2]) return false;
            if (finder->biome_3 != -1 && !result.biomes[finder->biome_3]) return false;

            //  Region Type
            if (finder->region_type_1 != -1 && !result.region_types[finder->region_type_1]) return false;
            if (finder->region_type_2 != -1 && !result.region_types[finder->region_type_2]) return false;
            if (finder->region_type_3 != -1 && !result.region_types[finder->region_type_3]) return false;

            //  Metals, Economics, and Minerals
            if (!result.metal_1 ||
                !result.metal_2 ||
                !result.metal_3 ||
                !result.economic_1 ||
                !result.economic_2 ||
                !result.economic_3 ||
                !result.mineral_1 ||
                !result.mineral_2 ||
                !result.mineral_3) return false;

            //  Necro neighbors  //  Checked at the world tile level
            //  Civ neighbors
            //  Civs

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
            bool world_tile_match = true;
            embark_assist::defs::region_tile_datum *world_tile = &survey_results->at(x).at(y);

            //  These world tile checks require that the MLTs have been surveyed to update the world tile data
            //  Necro Neighbors
            if (finder->min_necro_neighbors > world_tile->necro_neighbors) world_tile_match = false;
            if (finder->max_necro_neighbors != -1 && finder->max_necro_neighbors < world_tile->necro_neighbors) world_tile_match = false;

            //  Civ Neighbors
            if (finder->min_civ_neighbors > (int16_t)world_tile->neighbors.size()) world_tile_match = false;
            if (finder->max_civ_neighbors != -1 && finder->max_civ_neighbors < (int8_t)world_tile->neighbors.size()) world_tile_match = false;

            //  Specific Neighbors
            for (uint16_t i = 0; i < finder->neighbors.size(); i++) {
                switch (finder->neighbors[i].present) {
                case embark_assist::defs::present_absent_ranges::NA:
                    break;

                case embark_assist::defs::present_absent_ranges::Present:
                {
                    bool found = false;

                    for (uint16_t k = 0; k < world_tile->neighbors.size(); k++) {
                        if (finder->neighbors[i].entity_raw == world_tile->neighbors[k]) {
                            found = true;
                            break;
                        }
                    }

                    if (!found) world_tile_match = false;

                    break;
                }

                case embark_assist::defs::present_absent_ranges::Absent:
                    for (uint16_t k = 0; k < world_tile->neighbors.size(); k++) {
                        if (finder->neighbors[i].entity_raw == world_tile->neighbors[k]) {
                            world_tile_match = false;
                            break;
                        }
                    }

                    break;
                }
            }

            for (uint16_t i = 0; i < 16; i++) {
                for (uint16_t k = 0; k < 16; k++) {
                    if (world_tile_match && i < 16 - finder->x_dim + 1 && k < 16 - finder->y_dim + 1) {
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

            bool trace = false;
            color_ostream_proxy out(Core::getInstance().getConsole());
            df::world_data *world_data = world->world_data;
            embark_assist::defs::region_tile_datum *tile = &survey_results->at(x).at(y);
            const uint16_t embark_size = finder->x_dim * finder->y_dim;
            bool found;

            if (tile->surveyed) {
                //  Savagery
                for (uint8_t i = 0; i < 3; i++)
                {
                    switch (finder->savagery[i]) {
                    case embark_assist::defs::evil_savagery_values::NA:
                        break;  //  No restriction

                    case embark_assist::defs::evil_savagery_values::All:
                        if (tile->savagery_count[i] < embark_size) {
                            if (trace) out.print("matcher::world_tile_match: Savagery All (%i, %i)\n", x, y);
                            return false;
                        }
                        break;

                    case embark_assist::defs::evil_savagery_values::Present:
                        if (tile->savagery_count[i] == 0 && !tile->neighboring_savagery[i]) {
                            if (trace) out.print("matcher::world_tile_match: Savagery Present (%i, %i)\n", x, y);
                            return false;
                        }
                        break;

                    case embark_assist::defs::evil_savagery_values::Absent:
                        if (tile->savagery_count[i] > 256 - embark_size) {
                            if (trace) out.print("matcher::world_tile_match: Savagery Absent (%i, %i)\n", x, y);
                            return false;
                        }
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
                        if (tile->evilness_count[i] < embark_size) {
                            if (trace) out.print("matcher::world_tile_match: Evil All (%i, %i)\n", x, y);
                            return false;
                        }
                        break;

                    case embark_assist::defs::evil_savagery_values::Present:
                        if (tile->evilness_count[i] == 0 && !tile->neighboring_evilness[i]) {
                            if (trace) out.print("matcher::world_tile_match: Evil Present (%i, %i)\n", x, y);
                            return false;
                        }
                        break;

                    case embark_assist::defs::evil_savagery_values::Absent:
                        if (tile->evilness_count[i] > 256 - embark_size) {
                            if (trace) out.print("matcher::world_tile_match: Evil Absent (%i, %i)\n", x, y);
                            return false;
                        }
                        break;
                    }
                }

                // Aquifer
                // Survey can only augment results with incursions, but single results can't be achieved without the native tile matching it,
                // so extra checks are needed only for combinations. Also note that while two incursions won't be sufficient to actually get a match,
                // the important thing here is not to miss matches due to aborting too early.
                switch (finder->aquifer) {
                case embark_assist::defs::aquifer_ranges::NA:
                    break;

                case embark_assist::defs::aquifer_ranges::None:
                    if (!(tile->aquifer & embark_assist::defs::None_Aquifer_Bit)) {
                        if (trace) out.print("matcher::world_tile_match: Aquifer None (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::aquifer_ranges::At_Most_Light:
                    if (tile->aquifer == embark_assist::defs::Heavy_Aquifer_Bit) {
                        if (trace) out.print("matcher::world_tile_match: Aquifer At_Most_Light (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::aquifer_ranges::None_Plus_Light:
                    if (!(tile->aquifer & embark_assist::defs::None_Aquifer_Bit) ||
                        !(tile->aquifer & embark_assist::defs::Light_Aquifer_Bit)) {
                        if (trace) out.print("matcher::world_tile_match: Aquifer None_Plus_Light (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::aquifer_ranges::None_Plus_At_Least_Light:
                    if (!(tile->aquifer & embark_assist::defs::None_Aquifer_Bit) ||
                        (tile->aquifer == embark_assist::defs::None_Aquifer_Bit)) {
                        if (trace) out.print("matcher::world_tile_match: Aquifer None_Plus_At_Least_Light (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::aquifer_ranges::Light:
                    if (!(tile->aquifer & embark_assist::defs::Light_Aquifer_Bit)) {
                        if (trace) out.print("matcher::world_tile_match: Aquifer Light (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::aquifer_ranges::At_Least_Light:
                    if (tile->aquifer == embark_assist::defs::None_Aquifer_Bit) {
                        if (trace) out.print("matcher::world_tile_match: Aquifer At_Least_Light (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::aquifer_ranges::None_Plus_Heavy:
                    if (!(tile->aquifer & embark_assist::defs::None_Aquifer_Bit) ||
                        !(tile->aquifer & embark_assist::defs::Heavy_Aquifer_Bit)) {
                        if (trace) out.print("matcher::world_tile_match: Aquifer None_Plus_Heavy (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::aquifer_ranges::At_Most_Light_Plus_Heavy:
                    if (tile->aquifer == embark_assist::defs::Heavy_Aquifer_Bit ||
                        !(tile->aquifer & embark_assist::defs::Heavy_Aquifer_Bit)) {
                        if (trace) out.print("matcher::world_tile_match: Aquifer At_Most_Light_Plus_Heavy (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::aquifer_ranges::Light_Plus_Heavy:
                    if (!(tile->aquifer & embark_assist::defs::Light_Aquifer_Bit) ||
                        !(tile->aquifer & embark_assist::defs::Heavy_Aquifer_Bit)) {
                        if (trace) out.print("matcher::world_tile_match: Aquifer Light_Plus_Heavy (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::aquifer_ranges::None_Light_Heavy:
                    if (tile->aquifer !=
                        (embark_assist::defs::None_Aquifer_Bit | embark_assist::defs::Light_Aquifer_Bit | embark_assist::defs::Heavy_Aquifer_Bit)) {
                        if (trace) out.print("matcher::world_tile_match: Aquifer None_Light_Heavy (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::aquifer_ranges::Heavy:
                    if (!(tile->aquifer & embark_assist::defs::Heavy_Aquifer_Bit)) {
                        if (trace) out.print("matcher::world_tile_match: Aquifer Heavy (%i, %i)\n", x, y);
                        return false;
                    }
                    break;
                }

                // River size. Every tile has riverless tiles, so max rivers has to be checked on the detailed level.
                switch (tile->max_river_size) {
                case embark_assist::defs::river_sizes::None:
                    if (finder->min_river > embark_assist::defs::river_ranges::None) {
                        if (trace) out.print("matcher::world_tile_match: River_Size None (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::river_sizes::Brook:
                    if (finder->min_river > embark_assist::defs::river_ranges::Brook) {
                        if (trace) out.print("matcher::world_tile_match: River_Size Brook (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::river_sizes::Stream:
                    if (finder->min_river > embark_assist::defs::river_ranges::Stream) {
                        if (trace) out.print("matcher::world_tile_match: River_Size Stream (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::river_sizes::Minor:
                    if (finder->min_river > embark_assist::defs::river_ranges::Minor) {
                        if (trace) out.print("matcher::world_tile_match: River_Size Mino (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::river_sizes::Medium:
                    if (finder->min_river > embark_assist::defs::river_ranges::Medium) {
                        if (trace) out.print("matcher::world_tile_match: River_Size Medium (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::river_sizes::Major:
                    if (finder->max_river != embark_assist::defs::river_ranges::NA) {
                        if (trace) out.print("matcher::world_tile_match: River_Size Major (%i, %i)\n", x, y);
                        return false;
                    }
                    break;
                }

                //  Waterfall
                if (finder->min_waterfall > tile->max_waterfall) {  //  N/A = -1 is always smaller
                    if (trace) out.print("matcher::world_tile_match: Waterfall (%i, %i), finder: %i, tile: %i\n", x, y, finder->min_waterfall, tile->max_waterfall);
                    return false;
                }
                if (finder->min_waterfall == 0 &&  // Absent
                    embark_size == 256 &&
                    tile->max_waterfall > 0) {
                    if (trace) out.print("matcher::world_tile_match: Waterfall 2 (%i, %i)\n", x, y);
                    return false;
                }

                //  Flat. No world tile checks. Need to look at the details

                //  Clay
                switch (finder->clay) {
                case embark_assist::defs::present_absent_ranges::NA:
                    break;  //  No restriction

                case embark_assist::defs::present_absent_ranges::Present:
                    if (tile->clay_count == 0 &&
                        !tile->neighboring_clay) {
                        if (trace) out.print("matcher::world_tile_match: Clay Present (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::present_absent_ranges::Absent:
                    if (tile->clay_count > 256 - embark_size) {
                        if (trace) out.print("matcher::world_tile_match: Clay Absent (%i, %i)\n", x, y);
                        return false;
                    }
                    break;
                }

                //  Sand
                switch (finder->sand) {
                case embark_assist::defs::present_absent_ranges::NA:
                    break;  //  No restriction

                case embark_assist::defs::present_absent_ranges::Present:
                    if (tile->sand_count == 0 &&
                        !tile->neighboring_sand) {
                        if (trace) out.print("matcher::world_tile_match: Sand Present (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::present_absent_ranges::Absent:
                    if (tile->sand_count > 256 - embark_size) {
                        if (trace) out.print("matcher::world_tile_match: Sand Absent (%i, %i)\n", x, y);
                        return false;
                    }
                    break;
                }

                //  Flux
                switch (finder->flux) {
                case embark_assist::defs::present_absent_ranges::NA:
                    break;  //  No restriction

                case embark_assist::defs::present_absent_ranges::Present:
                    if (tile->flux_count == 0) {
                        if (trace) out.print("matcher::world_tile_match: Flux Present (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::present_absent_ranges::Absent:
                    if (tile->flux_count > 256 - embark_size) {
                        if (trace) out.print("matcher::world_tile_match: Flux Absent (%i, %i)\n", x, y);
                        return false;
                    }
                    break;
                }

                //  Coal
                switch (finder->coal) {
                case embark_assist::defs::present_absent_ranges::NA:
                    break;  //  No restriction

                case embark_assist::defs::present_absent_ranges::Present:
                    if (tile->coal_count == 0) {
                        if (trace) out.print("matcher::world_tile_match: Coal Present (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::present_absent_ranges::Absent:
                    if (tile->coal_count > 256 - embark_size) {
                        if (trace) out.print("matcher::world_tile_match: Coal Absent (%i, %i)\n", x, y);
                        return false;
                    }
                    break;
                }

                //  Soil Min
                switch (finder->soil_min) {
                case embark_assist::defs::soil_ranges::NA:
                case embark_assist::defs::soil_ranges::None:
                    break;  //  No restriction

                case embark_assist::defs::soil_ranges::Very_Shallow:
                    if (tile->max_region_soil < 1) {
                        if (trace) out.print("matcher::world_tile_match: Soil Min Very Shallow (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::soil_ranges::Shallow:
                    if (tile->max_region_soil < 2) {
                        if (trace) out.print("matcher::world_tile_match: Soil Min Shallow (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::soil_ranges::Deep:
                    if (tile->max_region_soil < 3) {
                        if (trace) out.print("matcher::world_tile_match: Soil Min Deep (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::soil_ranges::Very_Deep:
                    if (tile->max_region_soil < 4) {
                        if (trace) out.print("matcher::world_tile_match: Soil Min Very Deep (%i, %i)\n", x, y);
                        return false;
                    }
                    break;
                }

                //  soil_min_everywhere only applies on the detailed level

                //  Soil Max
                switch (finder->soil_max) {
                case embark_assist::defs::soil_ranges::NA:
                case embark_assist::defs::soil_ranges::Very_Deep:
                    break;  //  No restriction

                case embark_assist::defs::soil_ranges::None:
                    if (tile->min_region_soil > 0) {
                        if (trace) out.print("matcher::world_tile_match: Soil_Max None (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::soil_ranges::Very_Shallow:
                    if (tile->min_region_soil > 1) {
                        if (trace) out.print("matcher::world_tile_match: Soil_Max Very_Shallow (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::soil_ranges::Shallow:
                    if (tile->min_region_soil > 2) {
                        if (trace) out.print("matcher::world_tile_match: Soil_Max Shallow (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::soil_ranges::Deep:
                    if (tile->min_region_soil > 3) {
                        if (trace) out.print("matcher::world_tile_match: Soil_Max Deep (%i, %i)\n", x, y);
                        return false;
                    }
                    break;
                }

                //  Freezing
                if (finder->freezing != embark_assist::defs::freezing_ranges::NA)
                {
                    int16_t max_max_temperature = tile->max_temperature[5];
                    int16_t min_max_temperature = tile->max_temperature[5];
                    int16_t max_min_temperature = tile->min_temperature[5];
                    int16_t min_min_temperature = tile->min_temperature[5];

                    for (uint8_t i = 1; i < 10; i++) {
                        if (tile->max_temperature[i] > max_max_temperature) {
                            max_max_temperature = tile->max_temperature[i];
                        }

                        if (tile->max_temperature[i] != - 30000 &&
                            tile->max_temperature[i] < min_max_temperature) {
                            min_max_temperature = tile->max_temperature[i];
                        }

                        if (tile->min_temperature[i] != -30000 &&
                            tile->min_temperature[i] < min_min_temperature) {
                            min_min_temperature = tile->min_temperature[i];
                        }

                        if (tile->min_temperature[i] > max_min_temperature) {
                            max_min_temperature = tile->min_temperature[i];
                        }
                    }

                    switch (finder->freezing) {
                    case embark_assist::defs::freezing_ranges::NA:
                        break;  //  Excluded above, but the Travis complains if it's not here.

                    case embark_assist::defs::freezing_ranges::Permanent:
                        if (min_max_temperature > 0) {
                            if (trace) out.print("matcher::world_tile_match: Freezing Permanent (%i, %i)\n", x, y);
                            return false;
                        }
                        break;

                    case embark_assist::defs::freezing_ranges::At_Least_Partial:
                        if (min_min_temperature > 0) {
                            if (trace) out.print("matcher::world_tile_match: Freezing At_Lest_Partial (%i, %i)\n", x, y);
                            return false;
                        }
                        break;

                    case embark_assist::defs::freezing_ranges::Partial:
                        if (min_min_temperature > 0 ||
                            max_max_temperature <= 0) {
                            if (trace) out.print("matcher::world_tile_match: Freezing Partial (%i, %i)\n", x, y);
                            return false;
                        }
                        break;

                    case embark_assist::defs::freezing_ranges::At_Most_Partial:
                        if (max_max_temperature <= 0) {
                            if (trace) out.print("matcher::world_tile_match: Freezing At Most_Partial (%i, %i)\n", x, y);
                            return false;
                        }
                        break;

                    case embark_assist::defs::freezing_ranges::Never:
                        if (max_min_temperature <= 0) {
                            if (trace) out.print("matcher::world_tile_match: Freezing Never (%i, %i)\n", x, y);
                            return false;
                        }
                        break;
                    }
                }

                //  Trees
                switch (finder->min_trees) {
                case embark_assist::defs::tree_ranges::NA:
                case embark_assist::defs::tree_ranges::None:
                    break;

                case embark_assist::defs::tree_ranges::Very_Scarce:
                    if (tile->max_tree_level < embark_assist::defs::tree_levels::Very_Scarce) {
                        if (trace) out.print("matcher::world_tile_match: Min_Trees Very_Scarce (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::tree_ranges::Scarce:
                    if (tile->max_tree_level < embark_assist::defs::tree_levels::Scarce) {
                        if (trace) out.print("matcher::world_tile_match: Min_Trees Scarce (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::tree_ranges::Woodland:
                    if (tile->max_tree_level < embark_assist::defs::tree_levels::Woodland) {
                        if (trace) out.print("matcher::world_tile_match: Min_Trees Woodland (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::tree_ranges::Heavily_Forested:
                    if (tile->max_tree_level < embark_assist::defs::tree_levels::Heavily_Forested) {
                        if (trace) out.print("matcher::world_tile_match: Min_Trees Heavily_Forested (%i, %i)\n", x, y);
                        return false;
                    }
                    break;
                }

                switch (finder->max_trees) {
                case embark_assist::defs::tree_ranges::NA:
                case embark_assist::defs::tree_ranges::Heavily_Forested:
                    break;

                case embark_assist::defs::tree_ranges::None:
                    if (tile->min_tree_level > embark_assist::defs::tree_levels::None) {
                        if (trace) out.print("matcher::world_tile_match: Max_Trees None (%i, %i)\n", x, y);
                        return false;
                    }
                    break;


                case embark_assist::defs::tree_ranges::Very_Scarce:
                    if (tile->min_tree_level > embark_assist::defs::tree_levels::Very_Scarce) {
                        if (trace) out.print("matcher::world_tile_match: Max_Trees Very_Scarce (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::tree_ranges::Scarce:
                    if (tile->min_tree_level > embark_assist::defs::tree_levels::Scarce) {
                        if (trace) out.print("matcher::world_tile_match: Max_Trees Scarce (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::tree_ranges::Woodland:
                    if (tile->min_tree_level > embark_assist::defs::tree_levels::Woodland) {
                        if (trace) out.print("matcher::world_tile_match: Max_Trees Woodland (%i, %i)\n", x, y);
                        return false;
                    }
                    break;
                }

                //  Blood Rain
                switch (finder->blood_rain) {
                case embark_assist::defs::yes_no_ranges::NA:
                    break;  //  No restriction

                case embark_assist::defs::yes_no_ranges::Yes:
                    if (!tile->blood_rain_possible) {
                        if (trace) out.print("matcher::world_tile_match: Blood_Rain Yes (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::yes_no_ranges::No:
                    if (tile->blood_rain_full) {
                        if (trace) out.print("matcher::world_tile_match: Blood_Rain No (%i, %i)\n", x, y);
                        return false;
                    }
                    break;
                }

                //  Syndrome Rain
                switch (finder->syndrome_rain) {
                case embark_assist::defs::syndrome_rain_ranges::NA:
                    break;  //  No restriction

                case embark_assist::defs::syndrome_rain_ranges::Any:
                    if (!tile->permanent_syndrome_rain_possible && !tile->temporary_syndrome_rain_possible) {
                        if (trace) out.print("matcher::world_tile_match: Syndrome_Rain Any (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::syndrome_rain_ranges::Permanent:
                    if (!tile->permanent_syndrome_rain_possible) {
                        if (trace) out.print("matcher::world_tile_match: Syndrome_Rain Permanent (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::syndrome_rain_ranges::Temporary:
                    if (!tile->temporary_syndrome_rain_possible) {
                        if (trace) out.print("matcher::world_tile_match: Syndrome_Rain Temporary (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::syndrome_rain_ranges::Not_Permanent:
                    if (tile->permanent_syndrome_rain_full) {
                        if (trace) out.print("matcher::world_tile_match: Syndrome_Rain Not_Permanent (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::syndrome_rain_ranges::None:
                    if (tile->permanent_syndrome_rain_full || tile->temporary_syndrome_rain_full) {
                        if (trace) out.print("matcher::world_tile_match: Syndrome_Rain None (%i, %i)\n", x, y);
                        return false;
                    }
                    break;
                }

                //  Reanimating
                switch (finder->reanimation) {
                case embark_assist::defs::reanimation_ranges::NA:
                    break;  //  No restriction

                case embark_assist::defs::reanimation_ranges::Both:
                    if (!tile->reanimating_possible || !tile->thralling_possible) {
                        if (trace) out.print("matcher::world_tile_match: Reanimation Both (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::reanimation_ranges::Any:
                    if (!tile->reanimating_possible && !tile->thralling_possible) {
                        if (trace) out.print("matcher::world_tile_match: Reanimation Any (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::reanimation_ranges::Thralling:
                    if (!tile->thralling_possible) {
                        if (trace) out.print("matcher::world_tile_match: Reanimation Thralling (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::reanimation_ranges::Reanimation:
                    if (!tile->reanimating_possible) {
                        if (trace) out.print("matcher::world_tile_match: Reanimation Reanimation (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::reanimation_ranges::Not_Thralling:
                    if (tile->thralling_full) {
                        if (trace) out.print("matcher::world_tile_match: Reanimation Not_Thralling (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::reanimation_ranges::None:
                    if (tile->reanimating_full || tile->thralling_full) {
                        if (trace) out.print("matcher::world_tile_match: Reanimation None (%i, %i)\n", x, y);
                        return false;
                    }
                    break;
                }

                //  Spire Count Min/Max
                //  Magma Min/Max
                //  Biome Count Min (Can't do anything with Max at this level)
                if (finder->biome_count_min > tile->biome_count) return false;

                //  Region Type 1
                if (finder->region_type_1 != -1) {
                    if (!tile->neighboring_region_types[finder->region_type_1]) {
                        if (trace) out.print("matcher::world_tile_match: Region_Type_1 (%i, %i)\n", x, y);
                        return false;
                    }
                }

                //  Region Type 2
                if (finder->region_type_2 != -1) {
                    if (!tile->neighboring_region_types[finder->region_type_2]) {
                        if (trace) out.print("matcher::world_tile_match: Region_Type_2 (%i, %i)\n", x, y);
                        return false;
                    }
                }

                //  Region Type 3
                if (finder->region_type_3 != -1) {
                    if (!tile->neighboring_region_types[finder->region_type_3]) {
                        if (trace) out.print("matcher::world_tile_match: Region_Type_3 (%i, %i)\n", x, y);
                        return false;
                    }
                }

                //  Biome 1
                if (finder->biome_1 != -1) {
                    if (!tile->neighboring_biomes[finder->biome_1]) {
                        if (trace) out.print("matcher::world_tile_match: Biome_1 (%i, %i)\n", x, y);
                        return false;
                    }
                }

                //  Biome 2
                if (finder->biome_2 != -1) {
                    if (!tile->neighboring_biomes[finder->biome_2]) {
                        if (trace) out.print("matcher::world_tile_match: Biome_2 (%i, %i)\n", x, y);
                        return false;
                    }
                }

                //  Biome 3
                if (finder->biome_3 != -1) {
                    if (!tile->neighboring_biomes[finder->biome_3]) {
                        if (trace) out.print("matcher::world_tile_match: Biome_3 (%i, %i)\n", x, y);
                        return false;
                    }
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
                        !mineral_3) {
                        if (trace) out.print("matcher::world_tile_match: Metal/Economic/Mineral (%i, %i)\n", x, y);
                        return false;
                    }
                }

                //  Necro Neighbors
                if (finder->min_necro_neighbors > tile->necro_neighbors) {
                    if (trace) out.print("matcher::world_tile_match: Necro_Neighbors 1 (%i, %i)\n", x, y);
                    return false;
                }
                if (finder->max_necro_neighbors < tile->necro_neighbors &&
                    finder->max_necro_neighbors != -1) {
                    if (trace) out.print("matcher::world_tile_match: Necro_Neighbors 2 (%i, %i)\n", x, y);
                    return false;
                }

                //  Civ Neighbors
                if (finder->min_civ_neighbors > (int16_t)tile->neighbors.size()) {
                    if (trace) out.print("matcher::world_tile_match: Civ_Neighbors 1 (%i, %i), %i, %i\n", x, y, finder->min_civ_neighbors, (int)tile->neighbors.size());
                    return false;
                }
                if (finder->max_civ_neighbors < (int8_t)tile->neighbors.size() &&
                    finder->max_civ_neighbors != -1) {
                    if (trace) out.print("matcher::world_tile_match: Civ_Neighbors 2 (%i, %i)\n", x, y);
                    return false;
                }

                //  Specific Neighbors
                for (uint16_t i = 0; i < finder->neighbors.size(); i++) {
                    switch (finder->neighbors[i].present) {
                    case embark_assist::defs::present_absent_ranges::NA:
                        break;

                    case embark_assist::defs::present_absent_ranges::Present:
                    {
                        bool found = false;

                        for (uint16_t k = 0; k < tile->neighbors.size(); k++) {
                            if (finder->neighbors[i].entity_raw == tile->neighbors[k]) {
                                found = true;
                                break;
                            }
                        }

                        if (!found) {
                            if (trace) out.print("matcher::world_tile_match: Specific Neighbors Present (%i, %i)\n", x, y);
                            return false;
                        }

                        break;
                    }

                    case embark_assist::defs::present_absent_ranges::Absent:
                        for (uint16_t k = 0; k < tile->neighbors.size(); k++) {
                            if (finder->neighbors[i].entity_raw == tile->neighbors[k]) {
                                if (trace) out.print("matcher::world_tile_match: Specific Neighbors Absent (%i, %i)\n", x, y);
                                return false;
                            }
                        }

                        break;
                    }
                }
            }
            else {  //  Not surveyed. This code is essentially obsolete size we're now surveying every tile the first time,
                    //  but it does still provide a hint about the number of matching world tiles, and thus is kept.
                    //  If designed this way originally the effort to code it wouldn't have been made, though...

                    //  Savagery
                for (uint8_t i = 0; i < 3; i++)
                {
                    switch (finder->savagery[i]) {
                    case embark_assist::defs::evil_savagery_values::NA:
                        break;  //  No restriction

                    case embark_assist::defs::evil_savagery_values::All:
                        if (tile->savagery_count[i] == 0) {
                            if (trace) out.print("matcher::world_tile_match: NS Savagery All (%i, %i)\n", x, y);
                            return false;
                        }
                        break;

                    case embark_assist::defs::evil_savagery_values::Present:
                        if (tile->savagery_count[i] == 0) {
                            if (trace) out.print("matcher::world_tile_match: NS Savagery Present (%i, %i)\n", x, y);
                            return false;
                        }
                        break;

                    case embark_assist::defs::evil_savagery_values::Absent:
                        if (tile->savagery_count[i] == 256) {
                            if (trace) out.print("matcher::world_tile_match: NS Savagery Absent (%i, %i)\n", x, y);
                            return false;
                        }
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
                        if (tile->evilness_count[i] == 0) {
                            if (trace) out.print("matcher::world_tile_match: NS Evil All (%i, %i)\n", x, y);
                            return false;
                        }
                        break;

                    case embark_assist::defs::evil_savagery_values::Present:
                        if (tile->evilness_count[i] == 0) {
                            if (trace) out.print("matcher::world_tile_match: NS Evil Present (%i, %i)\n", x, y);
                            return false;
                        }
                        break;

                    case embark_assist::defs::evil_savagery_values::Absent:
                        if (tile->evilness_count[i] == 256) {
                            if (trace) out.print("matcher::world_tile_match: NS Evil Absent (%i, %i)\n", x, y);
                            return false;
                        }
                        break;
                    }
                }

                //  Aquifer  //  Requires survey

                //  River size  //  Requires survey

                //  Waterfall  //  Requires survey

                //  Flat. No world tile checks. Need to look at the details

                //  Clay
                //  With no preliminary survey we don't know if incursions might bring clay, so we can't really exclude any tiles.

                //  Sand
                //  Same as with clay.

                //  Flux
                switch (finder->flux) {
                case embark_assist::defs::present_absent_ranges::NA:
                    break;  //  No restriction

                case embark_assist::defs::present_absent_ranges::Present:
                    if (tile->flux_count == 0) {
                        if (trace) out.print("matcher::world_tile_match: NS Flux Present (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::present_absent_ranges::Absent:
                    if (tile->flux_count == 256) {
                        if (trace) out.print("matcher::world_tile_match: NS Flux Absent (%i, %i)\n", x, y);
                        return false;
                    }
                    break;
                }

                //  Coal
                switch (finder->coal) {
                case embark_assist::defs::present_absent_ranges::NA:
                    break;  //  No restriction

                case embark_assist::defs::present_absent_ranges::Present:
                    if (tile->coal_count == 0) {
                        if (trace) out.print("matcher::world_tile_match: NS Coal Present (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::present_absent_ranges::Absent:
                    if (tile->coal_count == 256) {
                        if (trace) out.print("matcher::world_tile_match: NS Coal Absent (%i, %i)\n", x, y);
                        return false;
                    }
                    break;
                }

                //  Soil Min
                switch (finder->soil_min) {
                case embark_assist::defs::soil_ranges::NA:
                case embark_assist::defs::soil_ranges::None:
                    break;  //  No restriction

                case embark_assist::defs::soil_ranges::Very_Shallow:
                    if (tile->max_region_soil < 1) {
                        if (trace) out.print("matcher::world_tile_match: NS Soil_Min Very_Shallow (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::soil_ranges::Shallow:
                    if (tile->max_region_soil < 2) {
                        if (trace) out.print("matcher::world_tile_match: NS Soil_Min Shallow (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::soil_ranges::Deep:
                    if (tile->max_region_soil < 3) {
                        if (trace) out.print("matcher::world_tile_match: NS Soil_Min Deep (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::soil_ranges::Very_Deep:
                    if (tile->max_region_soil < 4) {
                        if (trace) out.print("matcher::world_tile_match: NS Soil_Min Very_Deep (%i, %i)\n", x, y);
                        return false;
                    }
                    break;
                }

                //  soil_min_everywhere only applies on the detailed level

                //  Soil Max
                //  Can't say anything as the preliminary data isn't reliable

                //  Blood Rain
                switch (finder->blood_rain) {
                case embark_assist::defs::yes_no_ranges::NA:
                    break;  //  No restriction

                case embark_assist::defs::yes_no_ranges::Yes:
                    if (!tile->blood_rain_possible) {
                        if (trace) out.print("matcher::world_tile_match: NS Blood_Rain Yes (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::yes_no_ranges::No:
                    if (tile->blood_rain_full) {
                        if (trace) out.print("matcher::world_tile_match: NS Blood_Rain No (%i, %i)\n", x, y);
                        return false;
                    }
                    break;
                }

                //  Trees  //  Requires survey

                //  Syndrome Rain
                switch (finder->syndrome_rain) {
                case embark_assist::defs::syndrome_rain_ranges::NA:
                    break;  //  No restriction

                case embark_assist::defs::syndrome_rain_ranges::Any:
                    if (!tile->permanent_syndrome_rain_possible && !tile->temporary_syndrome_rain_possible) {
                        if (trace) out.print("matcher::world_tile_match: NS Syndrome_Rain Any (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::syndrome_rain_ranges::Permanent:
                    if (!tile->permanent_syndrome_rain_possible) {
                        if (trace) out.print("matcher::world_tile_match: NS Syndrome_Rain Permanent (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::syndrome_rain_ranges::Temporary:
                    if (!tile->temporary_syndrome_rain_possible) {
                        if (trace) out.print("matcher::world_tile_match: NS Syndrome_Rain Temporary (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::syndrome_rain_ranges::Not_Permanent:
                    if (tile->permanent_syndrome_rain_full) {
                        if (trace) out.print("matcher::world_tile_match: NS Syndrome_Rain Not_Permanent (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::syndrome_rain_ranges::None:
                    if (tile->permanent_syndrome_rain_full || tile->temporary_syndrome_rain_full) {
                        if (trace) out.print("matcher::world_tile_match: NS Syndrome_Rain None (%i, %i)\n", x, y);
                        return false;
                    }
                    break;
                }

                //  Reanimating
                switch (finder->reanimation) {
                case embark_assist::defs::reanimation_ranges::NA:
                    break;  //  No restriction

                case embark_assist::defs::reanimation_ranges::Both:
                    if (!tile->reanimating_possible || !tile->thralling_possible) {
                        if (trace) out.print("matcher::world_tile_match: NS Reanimating Both (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::reanimation_ranges::Any:
                    if (!tile->reanimating_possible && !tile->thralling_possible) {
                        if (trace) out.print("matcher::world_tile_match: NS Reanimating Any (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::reanimation_ranges::Thralling:
                    if (!tile->thralling_possible) {
                        if (trace) out.print("matcher::world_tile_match: NS Reanimating Thralling (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::reanimation_ranges::Reanimation:
                    if (!tile->reanimating_possible) {
                        if (trace) out.print("matcher::world_tile_match:NS Reanimating Reanimating (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::reanimation_ranges::Not_Thralling:
                    if (tile->thralling_full) {
                        if (trace) out.print("matcher::world_tile_match: NS Reanimating Not_Thralling (%i, %i)\n", x, y);
                        return false;
                    }
                    break;

                case embark_assist::defs::reanimation_ranges::None:
                    if (tile->reanimating_full || tile->thralling_full) {
                        if (trace) out.print("matcher::world_tile_match: NS Reanimating None (%i, %i)\n", x, y);
                        return false;
                    }
                    break;
                }

                //  Spire Count Min/Max
                //  Magma Min/Max
                //  Biome Count Min (Can't do anything with Max at this level)
                if (finder->biome_count_min > tile->biome_count) {
                    if (trace) out.print("matcher::world_tile_match: NS Biome_Count (%i, %i)\n", x, y);
                    return false;
                }

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

                    if (!found) {
                        if (trace) out.print("matcher::world_tile_match: NS Region_Type_1 (%i, %i)\n", x, y);
                        return false;
                    }
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

                    if (!found) {
                        if (trace) out.print("matcher::world_tile_match: NS Region_Type_2 (%i, %i)\n", x, y);
                        return false;
                    }
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

                    if (!found) {
                        if (trace) out.print("matcher::world_tile_match: NS Region_Type_3 (%i, %i)\n", x, y);
                        return false;
                    }
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

                    if (!found) {
                        if (trace) out.print("matcher::world_tile_match: NS Biome_1 (%i, %i)\n", x, y);
                        return false;
                    }
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

                    if (!found) {
                        if (trace) out.print("matcher::world_tile_match: NS Biome_2 (%i, %i)\n", x, y);
                        return false;
                    }
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

                    if (!found) {
                        if (trace) out.print("matcher::world_tile_match: NS Biome_3 (%i, %i)\n", x, y);
                        return false;
                    }
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
                        !mineral_3) {
                        if (trace) out.print("matcher::world_tile_match: NS Metal/Economic/Mineral (%i, %i)\n", x, y);
                        return false;
                    }
                }

                //  Necro Neighbors  //  Can't evaluate these without having collected the info.
                //  Civ Neighbors
                //  Specific Neighbors
            }
            return true;
        }

        //=======================================================================================

        uint32_t preliminary_world_match(embark_assist::defs::world_tile_data *survey_results,
            embark_assist::defs::finders *finder,
            embark_assist::defs::match_results *match_results) {
//                        color_ostream_proxy out(Core::getInstance().getConsole());
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

            embark_assist::survey::survey_mid_level_tile(geo_summary,
                survey_results,
                &state->mlt);

            mid_level_tile_match(survey_results,
                &state->mlt,
                x,
                y,
                finder,
                match_results);
        }

        //=======================================================================================

        void merge_incursion_into_world_tile(embark_assist::defs::region_tile_datum* current,
            embark_assist::defs::region_tile_datum* target_tile,
            embark_assist::defs::mid_level_tile_incursion_base* target_mlt) {
            df::world_data* world_data = world->world_data;

            current->aquifer |= target_mlt->aquifer;
            current->neighboring_sand |= target_mlt->sand;
            current->neighboring_clay |= target_mlt->clay;
            if (current->min_region_soil > target_mlt->soil_depth) current->min_region_soil = target_mlt->soil_depth;
            if (current->max_region_soil < target_mlt->soil_depth) current->max_region_soil = target_mlt->soil_depth;
            current->neighboring_biomes[target_tile->biome[target_mlt->biome_offset]] = true;
            current->neighboring_region_types[world_data->regions[target_tile->biome_index[target_mlt->biome_offset]]->type] = true;
        }
    }
}

//=======================================================================================
//  Visible operations
//=======================================================================================

void embark_assist::matcher::setup() {
    embark_assist::matcher::state = new(embark_assist::matcher::states);
    embark_assist::survey::initiate(&state->mlt);
}

//=======================================================================================

void embark_assist::matcher::shutdown() {
    delete state;
    state = nullptr;
}

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
    df::world_data* world_data = world->world_data;
    auto screen = Gui::getViewscreenByType<df::viewscreen_choose_start_sitest>(0);
    uint16_t x_end;
    uint16_t y_end;
    bool turn = false;
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

        if (iterator->finder.min_waterfall > 0 &&
            iterator->finder.max_river == embark_assist::defs::river_ranges::None) {
            out.printerr("matcher::find: Will never find any waterfalls with None as max river\n");
            return 0;
        }

        if (iterator->finder.soil_max < iterator->finder.soil_min &&
            iterator->finder.soil_max != embark_assist::defs::soil_ranges::NA) {
            out.printerr("matcher::find: Will never find any matches with max soil < min soil\n");
            return 0;
        }

        if (iterator->finder.spire_count_max < iterator->finder.spire_count_min &&
            iterator->finder.spire_count_max != -1) {
            out.printerr("matcher::find: Will never find any matches with max spires < min spires\n");
            return 0;
        }

        if (iterator->finder.magma_max < iterator->finder.magma_min &&
            iterator->finder.magma_max != embark_assist::defs::magma_ranges::NA) {
            out.printerr("matcher::find: Will never find any matches with max magma < min magma\n");
            return 0;
        }

        if (iterator->finder.biome_count_max < iterator->finder.biome_count_min &&
            iterator->finder.biome_count_max != -1) {
            out.printerr("matcher::find: Will never find any matches with max biomes < min biomes\n");
            return 0;
        }

        if (iterator->finder.min_trees > iterator->finder.max_trees &&
            iterator->finder.max_trees != embark_assist::defs::tree_ranges::NA) {
            out.printerr("matcher::find: Will never find any matches with max trees < min trees\n");
            return 0;
        }

        if (iterator->finder.min_necro_neighbors > iterator->finder.max_necro_neighbors &&
            iterator->finder.max_necro_neighbors != -1) {
            out.printerr("matcher::find: Will never find any matches with max necro neighbors < min necro neighbors\n");
            return 0;
        }

        if (iterator->finder.min_civ_neighbors > iterator->finder.max_civ_neighbors &&
            iterator->finder.max_civ_neighbors != -1) {
            out.printerr("matcher::find: Will never find any matches with max civ neighbors < min civ neighbors\n");
            return 0;
        }

        if (iterator->finder.min_civ_neighbors != -1 ||
            iterator->finder.max_civ_neighbors != -1) {
            int16_t present_count = 0;
            int16_t absent_count = 0;

            for (size_t i = 0; i < iterator->finder.neighbors.size(); i++) {
                switch (iterator->finder.neighbors[i].present) {
                case embark_assist::defs::present_absent_ranges::NA:
                    break;

                case embark_assist::defs::present_absent_ranges::Present:
                    present_count++;
                    break;

                case embark_assist::defs::present_absent_ranges::Absent:
                    absent_count++;
                    break;
                }
            }

            if (iterator->finder.max_civ_neighbors != -1 && present_count > iterator->finder.max_civ_neighbors) {
                out.printerr("matcher::find: Will never find any matches with specified neighbors > max civ neighbors\n");
                return 0;
            }

            if (iterator->finder.min_civ_neighbors != -1 && (int8_t)iterator->finder.neighbors.size() - absent_count < iterator->finder.min_civ_neighbors) {
                out.printerr("matcher::find: Will never find any matches with total possible neighbors - excluded neighbors < min civ neighbors\n");
                return 0;
            }
        }

        preliminary_matches = preliminary_world_match(survey_results, &iterator->finder, match_results);

        if (preliminary_matches == 0) {
            out.printerr("matcher::find: Preliminarily matching World Tiles: %i\n", preliminary_matches);
            return 0;
        }
        else {
            out.print("matcher::find: Preliminarily matching World Tiles: %i\n", preliminary_matches);
        }

        while (screen->location.region_pos.x != 0 || screen->location.region_pos.y != 0) {
            screen->feed_key(df::interface_key::CURSOR_UPLEFT_FAST);
        }
        iterator->target_location_x = 0;
        iterator->target_location_y = 0;
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
            if (!survey_results->at(iterator->target_location_x).at(iterator->target_location_y).surveyed ||
                match_results->at(iterator->target_location_x).at(iterator->target_location_y).preliminary_match) {
                move_cursor(iterator->target_location_x, iterator->target_location_y);

                match_world_tile(geo_summary,
                    survey_results,
                    &iterator->finder,
                    match_results,
                    iterator->target_location_x,
                    iterator->target_location_y);
                if (match_results->at(iterator->target_location_x).at(iterator->target_location_y).contains_match) {
                    iterator->count++;
                }
            }
            else {
                for (uint16_t n = 0; n < 16; n++) {
                    for (uint16_t p = 0; p < 16; p++) {
                        match_results->at(iterator->target_location_x).at(iterator->target_location_y).mlt_match[n][p] = false;
                    }
                }
            }
            //  End of payload section

            if (m != y_end) {
                if (iterator->y_down) {
                    if (iterator->target_location_y < world->worldgen.worldgen_parms.dim_y - 1) iterator->target_location_y++;
                }
                else {
                    if (iterator->target_location_y > 0) iterator->target_location_y--;
                }
            }
            else {
                if (iterator->target_location_x != 0 &&
                    iterator->target_location_x != world->worldgen.worldgen_parms.dim_x - 1) {
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
                        if (iterator->target_location_y < world->worldgen.worldgen_parms.dim_y - 1) iterator->target_location_y++;
                    }
                    else {
                        if (iterator->target_location_y > 0) iterator->target_location_y--;
                    }
                }
            }
        }

        if (iterator->x_right) {  //  Won't do anything at the edge, so we don't bother filter those cases.
            if (iterator->target_location_x < world->worldgen.worldgen_parms.dim_x - 1) iterator->target_location_x++;
        }
        else {
            if (iterator->target_location_x > 0) iterator->target_location_x--;
        }

        if (!iterator->x_right &&
            iterator->target_location_x == 0) {
            turn = !turn;

            if (turn) {
                iterator->x_right = true;
            }
        }
        else if (iterator->x_right &&
            iterator->target_location_x == world->worldgen.worldgen_parms.dim_x - 1) {
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
            // if the cursor was positioned in the lower right corner before the search it has to be moved to a neighbouring tile manually
            // to force another call to embark_update when all (incursion) data is finally collected to make sure this specific world tile is properly reevaluated
            // see the embark_update() in embark-assistant
            if (iterator->x == world->worldgen.worldgen_parms.dim_x - 1 && iterator->y == world->worldgen.worldgen_parms.dim_y - 1) {
                embark_assist::matcher::move_cursor(iterator->x - 1, iterator->y);
            }
            embark_assist::matcher::move_cursor(iterator->x, iterator->y);

            if (!survey_results->at(0).at(0).survey_completed) {  // Every world tile has gone through preliminary survey, so add possible incursion resources to each tile.
                for (uint16_t i = 0; i < world->worldgen.worldgen_parms.dim_x; i++) {
                    for (uint16_t k = 0; k < world->worldgen.worldgen_parms.dim_y; k++) {
                        embark_assist::defs::region_tile_datum* current = &survey_results->at(i).at(k);

                        for (uint8_t l = 0; l < 16; l++) {
                            //  Start with the north row west corners

                            switch (embark_assist::survey::translate_corner(survey_results,
                                embark_assist::defs::directions::Center,
                                i,
                                k,
                                l,
                                0)) {

                            case embark_assist::defs::directions::Northwest:
                            {
                                if (l == 0) {
                                    embark_assist::defs::region_tile_datum* target_tile = &survey_results->at(i - 1).at(k - 1);
                                    merge_incursion_into_world_tile(current, target_tile, &target_tile->south_row[15]);
                                }
                                else {
                                    embark_assist::defs::region_tile_datum* target_tile = &survey_results->at(i).at(k - 1);
                                    merge_incursion_into_world_tile(current, target_tile, &target_tile->south_row[l - 1]);
                                }
                                break;
                            }

                            case embark_assist::defs::directions::North:
                            {
                                embark_assist::defs::region_tile_datum* target_tile = &survey_results->at(i).at(k - 1);
                                merge_incursion_into_world_tile(current, target_tile, &target_tile->south_row[l]);
                                break;
                            }

                            case embark_assist::defs::directions::West:
                            {
                                if (l == 0) {
                                    embark_assist::defs::region_tile_datum* target_tile = &survey_results->at(i - 1).at(k);
                                    merge_incursion_into_world_tile(current, target_tile, &target_tile->east_column[0]);
                                }
                                break;
                            }
                                // Only legal remaining result is Center, and we don't need to do anything for that case.
                            }

                            //  North row edges

                            if (embark_assist::survey::translate_ns_edge(survey_results,
                                true,
                                i,
                                k,
                                l,
                                0) == embark_assist::defs::directions::North) {

                                embark_assist::defs::region_tile_datum* target_tile = &survey_results->at(i).at(k - 1);
                                merge_incursion_into_world_tile(current, target_tile, &target_tile->south_row[l]);
                            }

                            //  North row east corners
                            switch (embark_assist::survey::translate_corner(survey_results,
                                embark_assist::defs::directions::East,
                                i,
                                k,
                                l,
                                0)) {

                            case embark_assist::defs::directions::North:
                            {
                                embark_assist::defs::region_tile_datum* target_tile = &survey_results->at(i).at(k - 1);
                                merge_incursion_into_world_tile(current, target_tile, &target_tile->south_row[l]);
                                break;
                            }
                            case embark_assist::defs::directions::Northeast:
                            {
                                if (l == 15) {
                                    embark_assist::defs::region_tile_datum* target_tile = &survey_results->at(i + 1).at(k - 1);
                                    merge_incursion_into_world_tile(current, target_tile, &target_tile->south_row[0]);
                                }
                                else {
                                    embark_assist::defs::region_tile_datum* target_tile = &survey_results->at(i).at(k - 1);
                                    merge_incursion_into_world_tile(current, target_tile, &target_tile->south_row[l + 1]);
                                }
                                break;
                            }

                            case embark_assist::defs::directions::East:
                            {
                                if (l == 15) {
                                    embark_assist::defs::region_tile_datum* target_tile = &survey_results->at(i + 1).at(k);
                                    merge_incursion_into_world_tile(current, target_tile, &target_tile->west_column[0]);
                                }
                                break;
                            }

                                // Only legal remaining result is Center, and we don't need to do anything for that case.
                            }
                            //  West column

                            if (embark_assist::survey::translate_ew_edge(survey_results,
                                true,
                                i,
                                k,
                                0,
                                l) == embark_assist::defs::directions::West) {

                                embark_assist::defs::region_tile_datum* target_tile = &survey_results->at(i - 1).at(k);
                                merge_incursion_into_world_tile(current, target_tile, &target_tile->east_column[l]);
                            }

                            //  East column

                            if (embark_assist::survey::translate_ew_edge(survey_results,
                                false,
                                i,
                                k,
                                15,
                                l) == embark_assist::defs::directions::East) {
                                embark_assist::defs::region_tile_datum* target_tile = &survey_results->at(i + 1).at(k);
                                merge_incursion_into_world_tile(current, target_tile, &target_tile->west_column[l]);
                            }

                            //  South row west corners

                            switch (embark_assist::survey::translate_corner(survey_results,
                                embark_assist::defs::directions::South,
                                i,
                                k,
                                l,
                                15)) {

                            case embark_assist::defs::directions::West:
                            {
                                if (l == 0) {
                                    embark_assist::defs::region_tile_datum* target_tile = &survey_results->at(i - 1).at(k);
                                    merge_incursion_into_world_tile(current, target_tile, &target_tile->east_column[15]);
                                }
                                break;
                            }

                            case embark_assist::defs::directions::Southwest:
                            {
                                if (l == 0) {
                                    embark_assist::defs::region_tile_datum* target_tile = &survey_results->at(i - 1).at(k + 1);
                                    merge_incursion_into_world_tile(current, target_tile, &target_tile->north_row[15]);
                                }
                                else {
                                    embark_assist::defs::region_tile_datum* target_tile = &survey_results->at(i).at(k + 1);
                                    merge_incursion_into_world_tile(current, target_tile, &target_tile->north_row[l - 1]);
                                }
                                break;
                            }

                            case embark_assist::defs::directions::South:
                            {
                                embark_assist::defs::region_tile_datum* target_tile = &survey_results->at(i).at(k + 1);
                                merge_incursion_into_world_tile(current, target_tile, &target_tile->north_row[l]);
                                break;
                            }
                                // Only legal remaining result is Center, and we don't need to do anything for that case.
                            }

                            //  South row edges

                            if (embark_assist::survey::translate_ns_edge(survey_results,
                                false,
                                i,
                                k,
                                l,
                                15) == embark_assist::defs::directions::South) {

                                embark_assist::defs::region_tile_datum* target_tile = &survey_results->at(i).at(k + 1);
                                merge_incursion_into_world_tile(current, target_tile, &target_tile->north_row[l]);
                            }

                            //  South row east corners

                            switch (embark_assist::survey::translate_corner(survey_results,
                                embark_assist::defs::directions::Southeast,
                                i,
                                k,
                                l,
                                15)) {

                            case embark_assist::defs::directions::East:
                            {
                                if (l == 15) {
                                    embark_assist::defs::region_tile_datum* target_tile = &survey_results->at(i + 1).at(k);
                                    merge_incursion_into_world_tile(current, target_tile, &target_tile->west_column[15]);
                                }
                                break;
                            }

                            case embark_assist::defs::directions::South:
                            {
                                embark_assist::defs::region_tile_datum* target_tile = &survey_results->at(i).at(k + 1);
                                merge_incursion_into_world_tile(current, target_tile, &target_tile->north_row[l]);
                                break;
                            }

                            case embark_assist::defs::directions::Southeast:
                            {
                                if (l == 15) {
                                    embark_assist::defs::region_tile_datum* target_tile = &survey_results->at(i + 1).at(k + 1);
                                    merge_incursion_into_world_tile(current, target_tile, &target_tile->north_row[0]);
                                }
                                else {
                                    embark_assist::defs::region_tile_datum* target_tile = &survey_results->at(i).at(k + 1);
                                    merge_incursion_into_world_tile(current, target_tile, &target_tile->north_row[l + 1]);
                                }
                                break;
                            }
                            }
                        }

                        //  The complete set of biomes and region types is stored in the "neighboring" elements, which is a little misleading.
                        for (uint8_t l = 0; l < 10; l++) {
                            if (current->biome_index[l] != -1) {
                                current->neighboring_biomes[current->biome[l]] = true;
                                current->neighboring_region_types[world_data->regions[current->biome_index[l]]->type] = true;
                            }
                        }

                        current->biome_count = 0;
                        for (uint8_t l = 0; l <= ENUM_LAST_ITEM(biome_type); l++) {
                            if (current->neighboring_biomes[l]) current->biome_count++;
                        }

                        survey_results->at(i).at(k).survey_completed = true;  //  A bit wasteful to add a flag to every entry when only the very first one is ever read...
                    }
                }
            }
        }
    }

    return iterator->count;
}
