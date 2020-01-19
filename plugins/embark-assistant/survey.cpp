#include <math.h>
#include <vector>

#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>

#include <modules/Gui.h>
#include "modules/Materials.h"
#include "modules/Maps.h"

#include "DataDefs.h"
#include "df/builtin_mats.h"
#include "df/coord2d.h"
#include "df/creature_interaction_effect.h"
#include "df/creature_interaction_effect_display_symbolst.h"
#include "df/creature_interaction_effect_type.h"
#include "df/feature_init.h"
#include "df/feature_init_deep_special_tubest.h"
#include "df/feature_init_magma_poolst.h"
#include "df/feature_init_volcanost.h"
#include "df/feature_type.h"
#include "df/inorganic_flags.h"
#include "df/inorganic_raw.h"
#include "df/interaction.h"
#include "df/interaction_effect.h"
#include "df/interaction_effect_type.h"
#include "df/interaction_effect_animatest.h"
#include "df/interaction_instance.h"
#include "df/interaction_source.h"
#include "df/interaction_source_regionst.h"
#include "df/interaction_source_type.h"
#include "df/interaction_target.h"
#include "df/interaction_target_corpsest.h"
#include "df/interaction_target_materialst.h"
#include "df/material_common.h"
#include "df/reaction.h"
#include "df/reaction_product.h"
#include "df/reaction_product_itemst.h"
#include "df/reaction_product_type.h"
#include "df/region_map_entry.h"
#include "df/syndrome.h"
#include "df/viewscreen.h"
#include "df/viewscreen_choose_start_sitest.h"
#include "df/world.h"
#include "df/world_data.h"
#include "df/world_geo_biome.h"
#include "df/world_geo_layer.h"
#include "df/world_raws.h"
#include "df/world_region.h"
#include "df/world_region_details.h"
#include "df/world_region_feature.h"
#include "df/world_region_type.h"
#include "df/world_river.h"
#include "df/world_site.h"
#include "df/world_site_type.h"
#include "df/world_underground_region.h"

#include "defs.h"
#include "survey.h"

using namespace DFHack;
using namespace df::enums;
using namespace Gui;

using df::global::world;

namespace embark_assist {
    namespace survey {
        struct states {
            uint16_t clay_reaction = -1;
            uint16_t flux_reaction = -1;
            std::vector<uint16_t> coals;
            uint16_t x;
            uint16_t y;
            uint8_t local_min_x;
            uint8_t local_min_y;
            uint8_t local_max_x;
            uint8_t local_max_y;
            uint16_t max_inorganic;
        };

        static states *state;

        //=======================================================================================

        bool geo_survey(embark_assist::defs::geo_data *geo_summary) {
            color_ostream_proxy out(Core::getInstance().getConsole());
            df::world_data *world_data = world->world_data;
            auto reactions = df::reaction::get_vector();
            bool non_soil_found;
            uint16_t size;

            for (uint16_t i = 0; i < reactions.size(); i++) {
                if (reactions[i]->code == "MAKE_CLAY_BRICKS") {
                    state->clay_reaction = i;
                }

                if (reactions[i]->code == "PIG_IRON_MAKING") {
                    state->flux_reaction = i;
                }
            }

            if (state->clay_reaction == -1) {
                out.printerr("The reaction 'MAKE_CLAY_BRICKS' was not found, so clay can't be identified.\n");
            }

            if (state->flux_reaction == -1) {
                out.printerr("The reaction 'PIG_IRON_MAKING' was not found, so flux can't be identified.\n");
            }

            for (uint16_t i = 0; i < world->raws.inorganics.size(); i++) {
                for (uint16_t k = 0; k < world->raws.inorganics[i]->economic_uses.size(); k++) {
                    for (uint16_t l = 0; l < world->raws.reactions.reactions[world->raws.inorganics[i]->economic_uses[k]]->products.size(); l++) {
                        df::reaction_product_itemst *product = static_cast<df::reaction_product_itemst*>(world->raws.reactions.reactions[world->raws.inorganics[i]->economic_uses[k]]->products[l]);

                        if (product->mat_type == df::builtin_mats::COAL) {
                            state->coals.push_back(i);
                            break;
                        }
                    }
                }
            }

            for (uint16_t i = 0; i < world_data->geo_biomes.size(); i++) {
                geo_summary->at(i).possible_metals.resize(state->max_inorganic);
                geo_summary->at(i).possible_economics.resize(state->max_inorganic);
                geo_summary->at(i).possible_minerals.resize(state->max_inorganic);

                non_soil_found = true;
                df::world_geo_biome *geo = world_data->geo_biomes[i];

                for (uint16_t k = 0; k < geo->layers.size() && k < 16; k++) {
                    df::world_geo_layer *layer = geo->layers[k];

                    if (layer->type == df::geo_layer_type::SOIL ||
                        layer->type == df::geo_layer_type::SOIL_SAND) {
                        geo_summary->at(i).soil_size += layer->top_height - layer->bottom_height + 1;

                        if (world->raws.inorganics[layer->mat_index]->flags.is_set(df::inorganic_flags::SOIL_SAND)) {
                            geo_summary->at(i).sand_absent = false;
                        }

                        if (non_soil_found) {
                            geo_summary->at(i).top_soil_only = false;
                        }
                    }
                    else {
                        non_soil_found = true;
                    }

                    geo_summary->at(i).possible_minerals[layer->mat_index] = true;

                    size = (uint16_t)world->raws.inorganics[layer->mat_index]->metal_ore.mat_index.size();

                    for (uint16_t l = 0; l < size; l++) {
                        geo_summary->at(i).possible_metals.at(world->raws.inorganics[layer->mat_index]->metal_ore.mat_index[l]) = true;
                    }

                    size = (uint16_t)world->raws.inorganics[layer->mat_index]->economic_uses.size();
                    if (size != 0) {
                        geo_summary->at(i).possible_economics[layer->mat_index] = true;

                        for (uint16_t l = 0; l < size; l++) {
                            if (world->raws.inorganics[layer->mat_index]->economic_uses[l] == state->clay_reaction) {
                                geo_summary->at(i).clay_absent = false;
                            }

                            if (world->raws.inorganics[layer->mat_index]->economic_uses[l] == state->flux_reaction) {
                                geo_summary->at(i).flux_absent = false;
                            }
                        }
                    }

                    for (uint16_t l = 0; l < state->coals.size(); l++) {
                        if (layer->mat_index == state->coals[l]) {
                            geo_summary->at(i).coal_absent = false;
                            break;
                        }
                    }

                    size = (uint16_t)layer->vein_mat.size();

                    for (uint16_t l = 0; l < size; l++) {
                        auto vein = layer->vein_mat[l];
                        geo_summary->at(i).possible_minerals[vein] = true;

                        for (uint16_t m = 0; m < world->raws.inorganics[vein]->metal_ore.mat_index.size(); m++) {
                            geo_summary->at(i).possible_metals.at(world->raws.inorganics[vein]->metal_ore.mat_index[m]) = true;
                        }

                        if (world->raws.inorganics[vein]->economic_uses.size() != 0) {
                            geo_summary->at(i).possible_economics[vein] = true;

                            for (uint16_t m = 0; m < world->raws.inorganics[vein]->economic_uses.size(); m++) {
                                if (world->raws.inorganics[vein]->economic_uses[m] == state->clay_reaction) {
                                    geo_summary->at(i).clay_absent = false;
                                }

                                if (world->raws.inorganics[vein]->economic_uses[m] == state->flux_reaction) {
                                    geo_summary->at(i).flux_absent = false;
                                }
                            }

                            for (uint16_t m = 0; m < state->coals.size(); m++) {
                                if (vein == state->coals[m]) {
                                    geo_summary->at(i).coal_absent = false;
                                    break;
                                }
                            }

                        }
                    }

                    if (layer->bottom_height <= -3 &&
                        world->raws.inorganics[layer->mat_index]->flags.is_set(df::inorganic_flags::AQUIFER)) {
                        geo_summary->at(i).aquifer_absent = false;
                    }

                    if (non_soil_found == true) {
                        geo_summary->at(i).top_soil_aquifer_only = false;
                    }
                }
            }
            return true;
        }


        //=================================================================================

        void survey_rivers(embark_assist::defs::world_tile_data *survey_results) {
            //            color_ostream_proxy out(Core::getInstance().getConsole());
            df::world_data *world_data = world->world_data;
            int16_t x;
            int16_t y;

            for (uint16_t i = 0; i < world_data->rivers.size(); i++) {
                for (uint16_t k = 0; k < world_data->rivers[i]->path.x.size(); k++) {
                    x = world_data->rivers[i]->path.x[k];
                    y = world_data->rivers[i]->path.y[k];

                    if (world_data->rivers[i]->flow[k] < 5000) {
                        if (world_data->region_map[x][y].flags.is_set(df::region_map_entry_flags::is_brook)) {
                            survey_results->at(x).at(y).river_size = embark_assist::defs::river_sizes::Brook;
                        }
                        else {
                            survey_results->at(x).at(y).river_size = embark_assist::defs::river_sizes::Stream;
                        }
                    }
                    else if (world_data->rivers[i]->flow[k] < 10000) {
                        survey_results->at(x).at(y).river_size = embark_assist::defs::river_sizes::Minor;
                    }
                    else if (world_data->rivers[i]->flow[k] < 20000) {
                        survey_results->at(x).at(y).river_size = embark_assist::defs::river_sizes::Medium;
                    }
                    else {
                        survey_results->at(x).at(y).river_size = embark_assist::defs::river_sizes::Major;
                    }
                }

                x = world_data->rivers[i]->end_pos.x;
                y = world_data->rivers[i]->end_pos.y;

                //  Make the guess the river size for the end is the same as the tile next to the end. Note that DF
                //  doesn't actually recognize this tile as part of the river in the pre embark river name display.
                //  We also assume the is_river/is_brook flags are actually set properly for the end tile.
                //
                if (x >= 0 && y >= 0 && x < world->worldgen.worldgen_parms.dim_x && y < world->worldgen.worldgen_parms.dim_y) {
                    if (survey_results->at(x).at(y).river_size == embark_assist::defs::river_sizes::None) {
                        if (world_data->rivers[i]->path.x.size() &&
                            world_data->rivers[i]->flow[world_data->rivers[i]->path.x.size() - 1] < 5000) {
                            if (world_data->region_map[x][y].flags.is_set(df::region_map_entry_flags::is_brook)) {
                                survey_results->at(x).at(y).river_size = embark_assist::defs::river_sizes::Brook;
                            }
                            else {
                                survey_results->at(x).at(y).river_size = embark_assist::defs::river_sizes::Stream;
                            }
                        }
                        else if (world_data->rivers[i]->flow[world_data->rivers[i]->path.x.size() - 1] < 10000) {
                            survey_results->at(x).at(y).river_size = embark_assist::defs::river_sizes::Minor;
                        }
                        else if (world_data->rivers[i]->flow[world_data->rivers[i]->path.x.size() - 1] < 20000) {
                            survey_results->at(x).at(y).river_size = embark_assist::defs::river_sizes::Medium;
                        }
                        else {
                            survey_results->at(x).at(y).river_size = embark_assist::defs::river_sizes::Major;
                        }
                    }
                }
            }
        }

        //=================================================================================

        void survey_evil_weather(embark_assist::defs::world_tile_data *survey_results) {
            df::world_data *world_data = world->world_data;

            for (uint16_t i = 0; i < world->interaction_instances.all.size(); i++) {
                auto interaction = world->raws.interactions[world->interaction_instances.all[i]->interaction_id];
                uint16_t region_index = world->interaction_instances.all[i]->region_index;
                bool blood_rain = false;
                bool permanent_syndrome_rain = false;
                bool temporary_syndrome_rain = false;
                bool thralling = false;
                bool reanimating = false;

                if (interaction->sources.size() &&
                    interaction->sources[0]->getType() == df::interaction_source_type::REGION) {
                    for (uint16_t k = 0; k < interaction->targets.size(); k++) {
                        if (interaction->targets[k]->getType() == df::interaction_target_type::CORPSE) {
                            for (uint16_t l = 0; l < interaction->effects.size(); l++) {
                                if (interaction->effects[l]->getType() == df::interaction_effect_type::ANIMATE) {
                                    reanimating = true;
                                    break;
                                }
                            }
                        }
                        else if (interaction->targets[k]->getType() == df::interaction_target_type::MATERIAL) {
                            df::interaction_target_materialst* material = virtual_cast<df::interaction_target_materialst>(interaction->targets[k]);
                            if (material && DFHack::MaterialInfo(material->mat_type, material->mat_index).isInorganic()) {
                                for (const auto &syndrome : world->raws.inorganics[material->mat_index]->material.syndrome) {
                                    for (const auto &ce : syndrome->ce) {
                                        df::creature_interaction_effect_type ce_type = ce->getType();
                                        if (ce_type == df::creature_interaction_effect_type::FLASH_TILE) {
                                            //  Using this as a proxy. There seems to be a group of 4 effects for thralls:
                                            //  display symbol, flash symbol, phys att change and one more.
                                            thralling = true;
                                        }
                                        else if (ce_type == df::creature_interaction_effect_type::PAIN ||
                                            ce_type == df::creature_interaction_effect_type::SWELLING ||
                                            ce_type == df::creature_interaction_effect_type::OOZING ||
                                            ce_type == df::creature_interaction_effect_type::BRUISING ||
                                            ce_type == df::creature_interaction_effect_type::BLISTERS ||
                                            ce_type == df::creature_interaction_effect_type::NUMBNESS ||
                                            ce_type == df::creature_interaction_effect_type::PARALYSIS ||
                                            ce_type == df::creature_interaction_effect_type::FEVER ||
                                            ce_type == df::creature_interaction_effect_type::BLEEDING ||
                                            ce_type == df::creature_interaction_effect_type::COUGH_BLOOD ||
                                            ce_type == df::creature_interaction_effect_type::VOMIT_BLOOD ||
                                            ce_type == df::creature_interaction_effect_type::NAUSEA ||
                                            ce_type == df::creature_interaction_effect_type::UNCONSCIOUSNESS ||
                                            ce_type == df::creature_interaction_effect_type::NECROSIS ||
                                            ce_type == df::creature_interaction_effect_type::IMPAIR_FUNCTION ||
                                            ce_type == df::creature_interaction_effect_type::DROWSINESS ||
                                            ce_type == df::creature_interaction_effect_type::DIZZINESS ||
                                            ce_type == df::creature_interaction_effect_type::ERRATIC_BEHAVIOR) {  // Doubtful if possible for region.
                                            if (ce->end == -1) {
                                                permanent_syndrome_rain = true;
                                            }
                                            else {
                                                temporary_syndrome_rain = true;
                                            }
                                        }
                                    }
                                }
                            }
                            else {  // If not inorganic it's always blood, as far as known.
                                blood_rain = true;
                            }
                        }
                    }
                }

                for (uint16_t k = 0; k < world_data->regions[region_index]->region_coords.size(); k++) {
                    auto &results = survey_results->at(world_data->regions[region_index]->region_coords[k].x).at(world_data->regions[region_index]->region_coords[k].y);
                    results.blood_rain[5] = blood_rain;
                    results.permanent_syndrome_rain[5] = permanent_syndrome_rain;
                    results.temporary_syndrome_rain[5] = temporary_syndrome_rain;
                    results.reanimating[5] = reanimating;
                    results.thralling[5] = thralling;
                }
            }

            for (uint16_t i = 0; i < world->worldgen.worldgen_parms.dim_x; i++) {
                for (uint16_t k = 0; k < world->worldgen.worldgen_parms.dim_y; k++) {
                    auto &results = survey_results->at(i).at(k);
                    results.blood_rain_possible = false;
                    results.permanent_syndrome_rain_possible = false;
                    results.temporary_syndrome_rain_possible = false;
                    results.reanimating_possible = false;
                    results.thralling_possible = false;
                    results.blood_rain_full = true;
                    results.permanent_syndrome_rain_full = true;
                    results.temporary_syndrome_rain_full = true;
                    results.reanimating_full = true;
                    results.thralling_full = true;

                    for (uint8_t l = 1; l < 10; l++) {
                        if (results.biome_index[l] != -1) {
                            df::coord2d adjusted = apply_offset(i, k, l);
                            results.blood_rain[l] = survey_results->at(adjusted.x).at(adjusted.y).blood_rain[5];
                            results.permanent_syndrome_rain[l] = survey_results->at(adjusted.x).at(adjusted.y).permanent_syndrome_rain[5];
                            results.temporary_syndrome_rain[l] = survey_results->at(adjusted.x).at(adjusted.y).temporary_syndrome_rain[5];
                            results.reanimating[l] = survey_results->at(adjusted.x).at(adjusted.y).reanimating[5];
                            results.thralling[l] = survey_results->at(adjusted.x).at(adjusted.y).thralling[5];

                            if (results.blood_rain[l]) {
                                results.blood_rain_possible = true;
                            }
                            else {
                                results.blood_rain_full = false;
                            }

                            if (results.permanent_syndrome_rain[l]) {
                                results.permanent_syndrome_rain_possible = true;
                            }
                            else {
                                results.permanent_syndrome_rain_full = false;
                            }

                            if (results.temporary_syndrome_rain[l]) {
                                results.temporary_syndrome_rain_possible = true;
                            }
                            else {
                                results.temporary_syndrome_rain_full = false;
                            }

                            if (results.reanimating[l]) {
                                results.reanimating_possible = true;
                            }
                            else {
                                results.reanimating_full = false;
                            }

                            if (results.thralling[l]) {
                                results.thralling_possible = true;
                            }
                            else {
                                results.thralling_full = false;
                            }
                        }
                    }
                }
            }
        }

        //=================================================================================

        int16_t min_temperature(int16_t max_temperature, uint16_t latitude) {
            uint16_t divisor;
            uint16_t steps;
            uint16_t lat;

            if (world->world_data->flip_latitude == df::world_data::T_flip_latitude::None) {
                return max_temperature;
            }

            else if (world->world_data->flip_latitude == df::world_data::T_flip_latitude::North ||
                world->world_data->flip_latitude == df::world_data::T_flip_latitude::South) {
                steps = world->world_data->world_height / 2;

                if (latitude > steps) {
                    lat = world->world_data->world_height - 1 - latitude;
                }
                else
                {
                    lat = latitude;
                }
            }
            else {  // Both
                steps = world->world_data->world_height / 4;

                if (latitude < steps) {
                    lat = latitude;
                }
                else if (latitude <= steps * 2) {
                    lat = steps * 2 - latitude;
                }
                else if (latitude <= steps * 3) {
                    lat = latitude - steps * 2;
                }
                else {
                    lat = world->world_data->world_height - latitude;
                }

            }

            if (world->world_data->world_height == 17) {
                divisor = (57 / steps * lat + 0.4);
            }
            else if (world->world_data->world_height == 33) {
                divisor = (61 / steps * lat + 0.1);
            }
            else if (world->world_data->world_height == 65) {
                divisor = (63 / steps * lat);
            }
            else if (world->world_data->world_height == 129 ||
                world->world_data->world_height == 257) {
                divisor = (64 / steps * lat);
            }
            else {
                return max_temperature; // Not any standard world height. No formula available
            }

            return max_temperature - ceil(divisor * 3 / 4);
        }

        //=================================================================================

        void process_embark_incursion(embark_assist::defs::site_infos *site_info,
            embark_assist::defs::world_tile_data *survey_results,
            embark_assist::defs::mid_level_tile *mlt,  // Note this is a single tile, as opposed to most usages of this variable name.
            int16_t elevation,
            uint16_t x,
            uint16_t y) {

            if (mlt->aquifer) {
                site_info->aquifer = true;
            }
            else {
                site_info->aquifer_full = false;
            }

            if (mlt->soil_depth < site_info->min_soil) {
                site_info->min_soil = mlt->soil_depth;
            }

            if (mlt->soil_depth > site_info->max_soil) {
                site_info->max_soil = mlt->soil_depth;
            }

            if (elevation != mlt->elevation) {
                site_info->flat = false;
            }

            if (mlt->clay) {
                site_info->clay = true;
            }

            if (mlt->sand) {
                site_info->sand = true;
            }

            if (survey_results->at(x).at(y).blood_rain [mlt->biome_offset]) {
                site_info->blood_rain = true;
            }

            if (survey_results->at(x).at(y).permanent_syndrome_rain[mlt->biome_offset]) {
                site_info->permanent_syndrome_rain = true;
            }

            if (survey_results->at(x).at(y).temporary_syndrome_rain[mlt->biome_offset]) {
                site_info->temporary_syndrome_rain = true;
            }

            if (survey_results->at(x).at(y).reanimating[mlt->biome_offset]) {
                site_info->reanimating = true;
            }

            if (survey_results->at(x).at(y).thralling[mlt->biome_offset]) {
                site_info->thralling = true;
            }
        }

        //=================================================================================

        void process_embark_incursion_mid_level_tile(uint8_t from_direction,
            embark_assist::defs::site_infos *site_info,
            embark_assist::defs::world_tile_data *survey_results,
            embark_assist::defs::mid_level_tiles *mlt,
            uint8_t i,
            uint8_t k) {

            int8_t fetch_i = i;
            int8_t fetch_k = k;
            int16_t fetch_x = state->x;
            int16_t fetch_y = state->y;
            df::world_data *world_data = world->world_data;

            //  Logic can be implemented with modulo and division, but that's harder to read.
            switch (from_direction) {
            case 0:
                fetch_i = i - 1;
                fetch_k = k - 1;
                break;

            case 1:
                fetch_k = k - 1;
                break;

            case 2:
                fetch_i = i + 1;
                fetch_k = k - 1;
                break;

            case 3:
                fetch_i = i - 1;
                break;

            case 4:
                return;   //  Own tile provides the data, so there's no incursion.
                break;

            case 5:
                fetch_i = i + 1;
                break;

            case 6:
                fetch_i = i - 1;
                fetch_k = k + 1;
                break;

            case 7:
                fetch_k = k + 1;
                break;

            case 8:
                fetch_i = i + 1;
                fetch_k = k + 1;
                break;
            }

            if (fetch_i < 0) {
                fetch_x = state->x - 1;
            }
            else if (fetch_i > 15) {
                fetch_x = state->x + 1;
            }

            if (fetch_k < 0) {
                fetch_y = state->y - 1;
            }
            else if (fetch_k > 15) {
                fetch_y = state->y + 1;
            }

            if (fetch_x < 0 ||
                fetch_x == world_data->world_width ||
                fetch_y < 0 ||
                fetch_y == world_data->world_height) {
                return;  //  We're at the world edge, so no incursions from the outside.
            }

            if (fetch_k < 0) {
                if (fetch_i < 0) {
                    process_embark_incursion(site_info,
                        survey_results,
                        &survey_results->at(fetch_x).at(fetch_y).south_row[15],
                        mlt->at(i).at(k).elevation,
                        fetch_x,
                        fetch_y);
                }
                else if (fetch_i > 15) {
                    process_embark_incursion(site_info,
                        survey_results,
                        &survey_results->at(fetch_x).at(fetch_y).south_row[0],
                        mlt->at(i).at(k).elevation,
                        fetch_x,
                        fetch_y);
                }
                else {
                    process_embark_incursion(site_info,
                        survey_results,
                        &survey_results->at(fetch_x).at(fetch_y).south_row[i],
                        mlt->at(i).at(k).elevation,
                        fetch_x,
                        fetch_y);
                }
            }
            else if (fetch_k > 15) {
                if (fetch_i < 0) {
                    process_embark_incursion(site_info,
                        survey_results,
                        &survey_results->at(fetch_x).at(fetch_y).north_row[15],
                        mlt->at(i).at(k).elevation,
                        fetch_x,
                        fetch_y);
                }
                else if (fetch_i > 15) {
                    process_embark_incursion(site_info,
                        survey_results,
                        &survey_results->at(fetch_x).at(fetch_y).north_row[0],
                        mlt->at(i).at(k).elevation,
                        fetch_x,
                        fetch_y);
                }
                else {
                    process_embark_incursion(site_info,
                        survey_results,
                        &survey_results->at(fetch_x).at(fetch_y).north_row[i],
                        mlt->at(i).at(k).elevation,
                        fetch_x,
                        fetch_y);
                }
            }
            else {
                if (fetch_i < 0) {
                    process_embark_incursion(site_info,
                        survey_results,
                        &survey_results->at(fetch_x).at(fetch_y).east_column[k],
                        mlt->at(i).at(k).elevation,
                        fetch_x,
                        fetch_y);
                }
                else if (fetch_i > 15) {
                    process_embark_incursion(site_info,
                        survey_results,
                        &survey_results->at(fetch_x).at(fetch_y).west_column[k],
                        mlt->at(i).at(k).elevation,
                        fetch_x,
                        fetch_y);
                }
                else {
                    process_embark_incursion(site_info,
                        survey_results,
                        &mlt->at(fetch_i).at(fetch_k),
                        mlt->at(i).at(k).elevation,
                        fetch_x,
                        fetch_y);
                }
            }
        }
    }
}

//=================================================================================
//  Exported operations
//=================================================================================

void embark_assist::survey::setup(uint16_t max_inorganic) {
    embark_assist::survey::state = new(embark_assist::survey::states);
    embark_assist::survey::state->max_inorganic = max_inorganic;
}

//=================================================================================

df::coord2d embark_assist::survey::get_last_pos() {
    return{ embark_assist::survey::state->x, embark_assist::survey::state->y };
}

//=================================================================================

void embark_assist::survey::initiate(embark_assist::defs::mid_level_tiles *mlt) {
    for (uint8_t i = 0; i < 16; i++) {
        for (uint8_t k = 0; k < 16; k++) {
            mlt->at(i).at(k).metals.resize(state->max_inorganic);
            mlt->at(i).at(k).economics.resize(state->max_inorganic);
            mlt->at(i).at(k).minerals.resize(state->max_inorganic);
        }
    }
}

//=================================================================================

void embark_assist::survey::clear_results(embark_assist::defs::match_results *match_results) {
    for (uint16_t i = 0; i < world->worldgen.worldgen_parms.dim_x; i++) {
        for (uint16_t k = 0; k < world->worldgen.worldgen_parms.dim_y; k++) {
            match_results->at(i).at(k).preliminary_match = false;
            match_results->at(i).at(k).contains_match = false;

            for (uint16_t l = 0; l < 16; l++) {
                for (uint16_t m = 0; m < 16; m++) {
                    match_results->at(i).at(k).mlt_match[l][m] = false;
                }
            }
        }
    }
}

//=================================================================================

void embark_assist::survey::high_level_world_survey(embark_assist::defs::geo_data *geo_summary,
    embark_assist::defs::world_tile_data *survey_results) {
//    color_ostream_proxy out(Core::getInstance().getConsole());

    int16_t temperature;
    bool negative;

    embark_assist::survey::geo_survey(geo_summary);
    for (uint16_t i = 0; i < world->worldgen.worldgen_parms.dim_x; i++) {
        for (uint16_t k = 0; k < world->worldgen.worldgen_parms.dim_y; k++) {
            df::coord2d adjusted;
            df::world_data *world_data = world->world_data;
            uint16_t geo_index;
            uint16_t sav_ev;
            uint8_t offset_count = 0;
            auto &results = survey_results->at(i).at(k);
            results.surveyed = false;
            results.aquifer_count = 0;
            results.clay_count = 0;
            results.sand_count = 0;
            results.flux_count = 0;
            results.coal_count = 0;
            results.min_region_soil = 10;
            results.max_region_soil = 0;
            results.max_waterfall = 0;
            results.savagery_count[0] = 0;
            results.savagery_count[1] = 0;
            results.savagery_count[2] = 0;
            results.evilness_count[0] = 0;
            results.evilness_count[1] = 0;
            results.evilness_count[2] = 0;
            results.metals.resize(state->max_inorganic);
            results.economics.resize(state->max_inorganic);
            results.minerals.resize(state->max_inorganic);
            //  Evil weather and rivers are handled in later operations. Should probably be merged into one.

            for (uint8_t l = 1; l < 10; l++)
            {
                adjusted = apply_offset(i, k, l);
                if (adjusted.x != i || adjusted.y != k || l == 5) {
                    offset_count++;

                    results.biome_index[l] = world_data->region_map[adjusted.x][adjusted.y].region_id;
                    results.biome[l] = DFHack::Maps::GetBiomeTypeWithRef(adjusted.x, adjusted.y, k);
                    temperature = world_data->region_map[adjusted.x][adjusted.y].temperature;
                    negative = temperature < 0;

                    if (negative) {
                        temperature = -temperature;
                    }

                    results.max_temperature[l] = (temperature / 4) * 3;
                    if (temperature % 4 > 1) {
                        results.max_temperature[l] = results.max_temperature[l] + temperature % 4 - 1;
                    }

                    if (negative) {
                        results.max_temperature[l] = -results.max_temperature[l];
                    }

                    results.min_temperature[l] = min_temperature(results.max_temperature[l], adjusted.y);
                    geo_index = world_data->region_map[adjusted.x][adjusted.y].geo_index;

                    if (!geo_summary->at(geo_index).aquifer_absent) results.aquifer_count++;
                    if (!geo_summary->at(geo_index).clay_absent) results.clay_count++;
                    if (!geo_summary->at(geo_index).sand_absent) results.sand_count++;
                    if (!geo_summary->at(geo_index).flux_absent) results.flux_count++;
                    if (!geo_summary->at(geo_index).coal_absent) results.coal_count++;

                    if (geo_summary->at(geo_index).soil_size < results.min_region_soil)
                        results.min_region_soil = geo_summary->at(geo_index).soil_size;

                    if (geo_summary->at(geo_index).soil_size > results.max_region_soil)
                        results.max_region_soil = geo_summary->at(geo_index).soil_size;

                    sav_ev = world_data->region_map[adjusted.x][adjusted.y].savagery / 33;
                    if (sav_ev == 3) sav_ev = 2;
                    results.savagery_count[sav_ev]++;

                    sav_ev = world_data->region_map[adjusted.x][adjusted.y].evilness / 33;
                    if (sav_ev == 3) sav_ev = 2;
                    results.evilness_count[sav_ev]++;

                    for (uint16_t m = 0; m < state->max_inorganic; m++) {
                        if (geo_summary->at(geo_index).possible_metals[m]) results.metals[m] = true;
                        if (geo_summary->at(geo_index).possible_economics[m]) results.economics[m] = true;
                        if (geo_summary->at(geo_index).possible_minerals[m]) results.minerals[m] = true;
                    }
                }
                else {
                    results.biome_index[l] = -1;
                    results.biome[l] = -1;
                    results.max_temperature[l] = -30000;
                    results.min_temperature[l] = -30000;
                }
            }

            results.biome_count = 0;
            for (uint8_t l = 1; l < 10; l++) {
                if (results.biome[l] != -1) results.biome_count++;
            }

            if (results.aquifer_count == offset_count) results.aquifer_count = 256;
            if (results.clay_count == offset_count) results.clay_count = 256;
            if (results.sand_count == offset_count) results.sand_count = 256;
            if (results.flux_count == offset_count) results.flux_count = 256;
            if (results.coal_count == offset_count) results.coal_count = 256;

            for (uint8_t l = 0; l < 3; l++) {
                if (results.savagery_count[l] == offset_count) results.savagery_count[l] = 256;
                if (results.evilness_count[l] == offset_count) results.evilness_count[l] = 256;
            }
        }
    }

    embark_assist::survey::survey_rivers(survey_results);
    embark_assist::survey::survey_evil_weather(survey_results);
}

//=================================================================================

void embark_assist::survey::survey_mid_level_tile(embark_assist::defs::geo_data *geo_summary,
    embark_assist::defs::world_tile_data *survey_results,
    embark_assist::defs::mid_level_tiles *mlt) {
//                color_ostream_proxy out(Core::getInstance().getConsole());
    auto screen = Gui::getViewscreenByType<df::viewscreen_choose_start_sitest>(0);
    int16_t x = screen->location.region_pos.x;
    int16_t y = screen->location.region_pos.y;
    embark_assist::defs::region_tile_datum *tile = &survey_results->at(x).at(y);
    int8_t max_soil_depth;
    int8_t offset;
    int16_t elevation;
    int16_t last_bottom;
    int16_t top_z;
    int16_t base_z;
    int16_t min_z = 0;  //  Initialized to silence warning about potential usage of uninitialized data.
    int16_t bottom_z;
    df::coord2d adjusted;
    df::world_data *world_data = world->world_data;
    df::world_region_details *details = world_data->region_details[0];
    df::region_map_entry *world_tile = &world_data->region_map[x][y];
    std::vector <df::world_region_feature *> features;
    uint8_t soil_erosion;
    uint16_t end_check_l;
    uint16_t end_check_m;
    uint16_t end_check_n;

    for (uint16_t i = 0; i < state->max_inorganic; i++) {
        tile->metals[i] = 0;
        tile->economics[i] = 0;
        tile->minerals[i] = 0;
    }

    for (uint8_t i = 0; i < 16; i++) {
        for (uint8_t k = 0; k < 16; k++) {
            mlt->at(i).at(k).metals.resize(state->max_inorganic);
            mlt->at(i).at(k).economics.resize(state->max_inorganic);
            mlt->at(i).at(k).minerals.resize(state->max_inorganic);
        }
    }

    for (uint8_t i = 1; i < 10; i++) survey_results->at(x).at(y).biome_index[i] = -1;

    for (uint8_t i = 0; i < 16; i++) {
        for (uint8_t k = 0; k < 16; k++) {
            max_soil_depth = -1;

            offset = details->biome[i][k];
            adjusted = apply_offset(x, y, offset);

            if (adjusted.x != x || adjusted.y != y)
            {
                mlt->at(i).at(k).biome_offset = offset;
            }
            else
            {
                mlt->at(i).at(k).biome_offset = 5;
            }

            survey_results->at(x).at(y).biome_index[mlt->at(i).at(k).biome_offset] =
                world_data->region_map[adjusted.x][adjusted.y].region_id;

            mlt->at(i).at(k).savagery_level = world_data->region_map[adjusted.x][adjusted.y].savagery / 33;
            if (mlt->at(i).at(k).savagery_level == 3) {
                mlt->at(i).at(k).savagery_level = 2;
            }
            mlt->at(i).at(k).evilness_level = world_data->region_map[adjusted.x][adjusted.y].evilness / 33;
            if (mlt->at(i).at(k).evilness_level == 3) {
                mlt->at(i).at(k).evilness_level = 2;
            }

            elevation = details->elevation[i][k];

            // Special biome adjustments
            if (!world_data->region_map[adjusted.x][adjusted.y].flags.is_set(region_map_entry_flags::is_lake)) {
                if (world_data->region_map[adjusted.x][adjusted.y].elevation >= 150) {  //  Mountain
                    max_soil_depth = 0;

                }
                else if (world_data->region_map[adjusted.x][adjusted.y].elevation < 100) {  // Ocean
                    if (elevation == 99) {
                        elevation = 98;
                    }

                    if ((world_data->geo_biomes[world_data->region_map[x][y].geo_index]->unk1 == 4 ||
                        world_data->geo_biomes[world_data->region_map[x][y].geo_index]->unk1 == 5) &&
                        details->unk12e8 < 500) {
                        max_soil_depth = 0;
                    }
                }
            }

            base_z = elevation - 1;
            features = details->features[i][k];
            std::map<int, int> layer_bottom, layer_top;
            mlt->at(i).at(k).adamantine_level = -1;
            mlt->at(i).at(k).magma_level = -1;

            end_check_l = static_cast<uint16_t>(features.size());
            for (size_t l = 0; l < end_check_l; l++) {
                auto feature = features[l];

                if (feature->feature_idx != -1) {
                    switch (world_data->feature_map[x / 16][y / 16].features->feature_init[x % 16][y % 16][feature->feature_idx]->getType())
                    {
                    case df::feature_type::deep_special_tube:
                        mlt->at(i).at(k).adamantine_level = world_data->feature_map[x / 16][y / 16].features->feature_init[x % 16][y % 16][feature->feature_idx]->start_depth;
                        break;

                    case df::feature_type::magma_pool:
                        mlt->at(i).at(k).magma_level = 2 - world_data->feature_map[x / 16][y / 16].features->feature_init[x % 16][y % 16][feature->feature_idx]->start_depth;
                        break;

                    case df::feature_type::volcano:
                        mlt->at(i).at(k).magma_level = 3;
                        break;

                    default:
                        break;
                    }
                }
                else if (feature->layer != -1 &&
                    feature->min_z != -30000) {
                    auto layer = world_data->underground_regions[feature->layer];

                    layer_bottom[layer->layer_depth] = feature->min_z;
                    layer_top[layer->layer_depth] = feature->max_z;
                    base_z = std::min((int)base_z, (int)feature->min_z);

                    if (layer->type == df::world_underground_region::MagmaSea) {
                        min_z = feature->min_z;  //  The features are individual per region tile
                    }
                }
            }

            //  Compute shifts for layers in the stack.

            if (max_soil_depth == -1) {  //  Not set to zero by the biome
                max_soil_depth = std::max((154 - elevation) / 5, 1);
            }

            soil_erosion = geo_summary->at(world_data->region_map[adjusted.x][adjusted.y].geo_index).soil_size -
                std::min((int)geo_summary->at(world_data->region_map[adjusted.x][adjusted.y].geo_index).soil_size, (int)max_soil_depth);
            int16_t layer_shift[16];
            int16_t cur_shift = elevation + soil_erosion - 1;

            mlt->at(i).at(k).aquifer = false;
            mlt->at(i).at(k).clay = false;
            mlt->at(i).at(k).sand = false;
            mlt->at(i).at(k).flux = false;
            mlt->at(i).at(k).coal = false;

            if (max_soil_depth == 0) {
                mlt->at(i).at(k).soil_depth = 0;
            }
            else {
                mlt->at(i).at(k).soil_depth = geo_summary->at(world_data->region_map[adjusted.x][adjusted.y].geo_index).soil_size - soil_erosion;
            }
            mlt->at(i).at(k).offset = offset;
            mlt->at(i).at(k).elevation = details->elevation[i][k];
            mlt->at(i).at(k).river_present = false;
            mlt->at(i).at(k).river_elevation = 100;

            if (details->rivers_vertical.active[i][k] != 0) {
                mlt->at(i).at(k).river_present = true;
                mlt->at(i).at(k).river_elevation = details->rivers_vertical.elevation[i][k];
            }
            else if (details->rivers_horizontal.active[i][k] != 0) {
                mlt->at(i).at(k).river_present = true;
                mlt->at(i).at(k).river_elevation = details->rivers_horizontal.elevation[i][k];
            }

            if (tile->min_region_soil > mlt->at(i).at(k).soil_depth) {
                tile->min_region_soil = mlt->at(i).at(k).soil_depth;
            }

            if (tile->max_region_soil < mlt->at(i).at(k).soil_depth) {
                tile->max_region_soil = mlt->at(i).at(k).soil_depth;
            }

            end_check_l = static_cast<uint16_t>(world_data->geo_biomes[world_data->region_map[adjusted.x][adjusted.y].geo_index]->layers.size());
            if (end_check_l > 16) end_check_l = 16;

            for (uint16_t l = 0; l < end_check_l; l++) {
                auto layer = world_data->geo_biomes[world_data->region_map[adjusted.x][adjusted.y].geo_index]->layers[l];
                layer_shift[l] = cur_shift;

                if (layer->type == df::geo_layer_type::SOIL ||
                    layer->type == df::geo_layer_type::SOIL_SAND) {
                    int16_t size = layer->top_height - layer->bottom_height - 1;
                    //  Comment copied from prospector.cpp (like the logic)...
                    //  This is to replicate the behavior of a probable bug in the
                    //  map generation code : if a layer is partially eroded, the
                    //  removed levels are in fact transferred to the layer below,
                    //  because unlike the case of removing the whole layer, the code
                    //  does not execute a loop to shift the lower part of the stack up.
                    if (size > soil_erosion) {
                        cur_shift = cur_shift - soil_erosion;
                    }

                    soil_erosion -= std::min((int)soil_erosion, (int)size);
                }
            }

            last_bottom = elevation;
            //  Don't have to set up the end_check as we can reuse the one above.

            for (uint16_t l = 0; l < end_check_l; l++) {
                auto layer = world_data->geo_biomes[world_data->region_map[adjusted.x][adjusted.y].geo_index]->layers[l];
                top_z = last_bottom - 1;
                bottom_z = std::max((int)layer->bottom_height + layer_shift[l], (int)min_z);

                if (l == 15) {
                    bottom_z = min_z;  // stretch layer if needed
                }

                if (top_z >= bottom_z) {
                    last_bottom = bottom_z;

                    mlt->at(i).at(k).minerals[layer->mat_index] = true;

                    end_check_m = static_cast<uint16_t>(world->raws.inorganics[layer->mat_index]->metal_ore.mat_index.size());

                    for (uint16_t m = 0; m < end_check_m; m++) {
                        mlt->at(i).at(k).metals[world->raws.inorganics[layer->mat_index]->metal_ore.mat_index[m]] = true;
                    }

                    if (layer->type == df::geo_layer_type::SOIL ||
                        layer->type == df::geo_layer_type::SOIL_SAND) {
                        if (world->raws.inorganics[layer->mat_index]->flags.is_set(df::inorganic_flags::SOIL_SAND)) {
                            mlt->at(i).at(k).sand = true;
                        }
                    }

                    if (world->raws.inorganics[layer->mat_index]->economic_uses.size() > 0) {
                        mlt->at(i).at(k).economics[layer->mat_index] = true;

                        end_check_m = static_cast<uint16_t>(world->raws.inorganics[layer->mat_index]->economic_uses.size());
                        for (uint16_t m = 0; m < end_check_m; m++) {
                            if (world->raws.inorganics[layer->mat_index]->economic_uses[m] == state->clay_reaction) {
                                mlt->at(i).at(k).clay = true;
                            }

                            else if (world->raws.inorganics[layer->mat_index]->economic_uses[m] == state->flux_reaction) {
                                mlt->at(i).at(k).flux = true;
                            }
                        }

                        for (uint16_t m = 0; m < state->coals.size(); m++) {
                            if (layer->mat_index == state->coals [m]) {
                                mlt->at(i).at(k).coal = true;
                                break;
                            }
                        }
                    }

                    end_check_m = static_cast<uint16_t>(layer->vein_mat.size());

                    for (uint16_t m = 0; m < end_check_m; m++) {
                        mlt->at(i).at(k).minerals[layer->vein_mat[m]] = true;

                        end_check_n = static_cast<uint16_t>(world->raws.inorganics[layer->vein_mat[m]]->metal_ore.mat_index.size());

                        for (uint16_t n = 0; n < end_check_n; n++) {
                            mlt->at(i).at(k).metals[world->raws.inorganics[layer->vein_mat[m]]->metal_ore.mat_index[n]] = true;
                        }

                        if (world->raws.inorganics[layer->vein_mat[m]]->economic_uses.size() > 0) {
                            mlt->at(i).at(k).economics[layer->vein_mat[m]] = true;

                            end_check_n = static_cast<uint16_t>(world->raws.inorganics[layer->vein_mat[m]]->economic_uses.size());
                            for (uint16_t n = 0; n < end_check_n; n++) {
                                if (world->raws.inorganics[layer->vein_mat[m]]->economic_uses[n] == state->clay_reaction) {
                                    mlt->at(i).at(k).clay = true;
                                }

                                else if (world->raws.inorganics[layer->vein_mat[m]]->economic_uses[n] == state->flux_reaction) {
                                    mlt->at(i).at(k).flux = true;
                                }
                            }

                            for (uint16_t n = 0; n < state->coals.size(); n++) {
                                if (layer->vein_mat [m] == state->coals[n]) {
                                    mlt->at(i).at(k).coal = true;
                                    break;
                                }
                            }
                        }
                    }

                    if (bottom_z <= elevation - 3 &&
                        world->raws.inorganics[layer->mat_index]->flags.is_set(df::inorganic_flags::AQUIFER)) {
                        mlt->at(i).at(k).aquifer = true;
                    }
                }
            }
        }
    }

    //  This is messy. DF has some weird logic to leave out river bends with a South and an East connection, as well
    //  as river sources (and presumably sinks) that are to the North or the West of the connecting river.
    //  Experiments indicate these implicit river bends inherit their River Elevation from the lower of the two
    //  "parents", and it's assumed river sources and sinks similarly inherit it from their sole "parent".
    //  Two issues are known:
    //  - Lake and Ocean tiles may be marked as having a river when DF doesn't. However, DF does allow for rivers to
    //    exist in Ocean/Lake tiles, as well as sources/sinks.
    //  - DF generates rivers on/under glaciers, but does not display them (as they're frozen), nor are their names
    //    displayed.
    //
    for (uint8_t i = 1; i < 16; i++) {
        for (uint8_t k = 0; k < 15; k++) {
            if (details->rivers_horizontal.active[i][k] != 0 &&
                details->rivers_vertical.active[i - 1][k + 1] != 0 &&
                !mlt->at(i - 1).at(k).river_present) {  //  Probably never true
                mlt->at(i - 1).at(k).river_present = true;
                mlt->at(i - 1).at(k).river_elevation = mlt->at(i).at(k).river_elevation;

                if (mlt->at(i - 1).at(k).river_elevation > mlt->at(i - 1).at(k + 1).river_elevation) {
                    mlt->at(i - 1).at(k).river_elevation = mlt->at(i - 1).at(k + 1).river_elevation;
                }
            }
        }
    }

    for (uint8_t i = 0; i < 16; i++) {
        for (uint8_t k = 1; k < 16; k++) {
            if (details->rivers_vertical.active[i][k] != 0 &&
                !mlt->at(i).at(k - 1).river_present) {
                mlt->at(i).at(k - 1).river_present = true;
                mlt->at(i).at(k - 1).river_elevation = mlt->at(i).at(k).river_elevation;
            }
        }
    }
            
    for (uint8_t i = 1; i < 16; i++) {
        for (uint8_t k = 0; k < 16; k++) {
            if (details->rivers_horizontal.active[i][k] != 0 &&
                !mlt->at(i - 1).at(k).river_present) {
                mlt->at(i - 1).at(k).river_present = true;
                mlt->at(i - 1).at(k).river_elevation = mlt->at(i).at(k).river_elevation;
            }
        }
    }

    survey_results->at(x).at(y).aquifer_count = 0;
    survey_results->at(x).at(y).clay_count = 0;
    survey_results->at(x).at(y).sand_count = 0;
    survey_results->at(x).at(y).flux_count = 0;
    survey_results->at(x).at(y).coal_count = 0;
    survey_results->at(x).at(y).min_region_soil = 10;
    survey_results->at(x).at(y).max_region_soil = 0;
    survey_results->at(x).at(y).savagery_count[0] = 0;
    survey_results->at(x).at(y).savagery_count[1] = 0;
    survey_results->at(x).at(y).savagery_count[2] = 0;
    survey_results->at(x).at(y).evilness_count[0] = 0;
    survey_results->at(x).at(y).evilness_count[1] = 0;
    survey_results->at(x).at(y).evilness_count[2] = 0;

    for (uint8_t i = 0; i < 16; i++) {
        for (uint8_t k = 0; k < 16; k++) {
            if (mlt->at(i).at(k).aquifer) { survey_results->at(x).at(y).aquifer_count++; }
            if (mlt->at(i).at(k).clay) { survey_results->at(x).at(y).clay_count++; }
            if (mlt->at(i).at(k).sand) { survey_results->at(x).at(y).sand_count++; }
            if (mlt->at(i).at(k).flux) { survey_results->at(x).at(y).flux_count++; }
            if (mlt->at(i).at(k).coal) { survey_results->at(x).at(y).coal_count++; }

            if (mlt->at(i).at(k).soil_depth < survey_results->at(x).at(y).min_region_soil) {
                survey_results->at(x).at(y).min_region_soil = mlt->at(i).at(k).soil_depth;
            }

            if (mlt->at(i).at(k).soil_depth > survey_results->at(x).at(y).max_region_soil) {
                survey_results->at(x).at(y).max_region_soil = mlt->at(i).at(k).soil_depth;
            }

            if (mlt->at(i).at(k).river_present) {
                if (i < 15 &&
                    mlt->at(i + 1).at(k).river_present &&
                    abs (mlt->at(i).at(k).river_elevation - mlt->at(i + 1).at(k).river_elevation) >
                    survey_results->at(x).at(y).max_waterfall) {
                    survey_results->at(x).at(y).max_waterfall =
                        abs(mlt->at(i).at(k).river_elevation - mlt->at(i + 1).at(k).river_elevation);
                }

                if (k < 15 &&
                    mlt->at(i).at(k + 1).river_present &&
                    abs(mlt->at(i).at(k).river_elevation - mlt->at(i).at(k + 1).river_elevation) >
                    survey_results->at(x).at(y).max_waterfall) {
                    survey_results->at(x).at(y).max_waterfall =
                        abs(mlt->at(i).at(k).river_elevation - mlt->at(i).at(k + 1).river_elevation);
                }
            }

            // River size surveyed separately
            // biome_index handled above
            // biome handled below
            // evil weather handled separately
            // reanimating handled separately
            // thralling handled separately

            survey_results->at(x).at(y).savagery_count[mlt->at(i).at(k).savagery_level]++;
            survey_results->at(x).at(y).evilness_count[mlt->at(i).at(k).evilness_level]++;

            for (uint16_t l = 0; l < state->max_inorganic; l++) {
                if (mlt->at(i).at(k).metals[l]) { survey_results->at(x).at(y).metals[l] = true; }
                if (mlt->at(i).at(k).economics[l]) { survey_results->at(x).at(y).economics[l] = true; }
                if (mlt->at(i).at(k).minerals[l]) { survey_results->at(x).at(y).minerals[l] = true; }
            }
        }
    }

    for (uint8_t i = 1; i < 10; i++) {
        if (survey_results->at(x).at(y).biome_index[i] == -1) {
            survey_results->at(x).at(y).biome[i] = -1;
        }
    }

    bool biomes[ENUM_LAST_ITEM(biome_type) + 1];
    for (uint8_t i = 0; i <= ENUM_LAST_ITEM(biome_type); i++) {
        biomes[i] = false;
    }

    for (uint8_t i = 1; i < 10; i++)
    {
        if (survey_results->at(x).at(y).biome[i] != -1) {
            biomes[survey_results->at(x).at(y).biome[i]] = true;
        }
    }
    int count = 0;
    for (uint8_t i = 0; i <= ENUM_LAST_ITEM(biome_type); i++) {
        if (biomes[i]) count++;
    }

    tile->biome_count = count;

    for (uint8_t i = 0; i < 16; i++) {
        tile->north_row[i].aquifer = mlt->at(i).at(0).aquifer;
        tile->south_row[i].aquifer = mlt->at(i).at(15).aquifer;
        tile->west_column[i].aquifer = mlt->at(0).at(i).aquifer;
        tile->east_column[i].aquifer = mlt->at(15).at(i).aquifer;

        tile->north_row[i].clay= mlt->at(i).at(0).clay;
        tile->south_row[i].clay = mlt->at(i).at(15).clay;
        tile->west_column[i].clay = mlt->at(0).at(i).clay;
        tile->east_column[i].clay = mlt->at(15).at(i).clay;

        tile->north_row[i].sand = mlt->at(i).at(0).sand;
        tile->south_row[i].sand = mlt->at(i).at(15).sand;
        tile->west_column[i].sand = mlt->at(0).at(i).sand;
        tile->east_column[i].sand = mlt->at(15).at(i).sand;

        tile->north_row[i].flux = mlt->at(i).at(0).flux;  //  Not used
        tile->south_row[i].flux = mlt->at(i).at(15).flux;
        tile->west_column[i].flux = mlt->at(0).at(i).flux;
        tile->east_column[i].flux = mlt->at(15).at(i).flux;

        tile->north_row[i].coal = mlt->at(i).at(0).coal; //  Not used
        tile->south_row[i].coal = mlt->at(i).at(15).coal;
        tile->west_column[i].coal = mlt->at(0).at(i).coal;
        tile->east_column[i].coal = mlt->at(15).at(i).coal;

        tile->north_row[i].soil_depth = mlt->at(i).at(0).soil_depth;
        tile->south_row[i].soil_depth = mlt->at(i).at(15).soil_depth;
        tile->west_column[i].soil_depth = mlt->at(0).at(i).soil_depth;
        tile->east_column[i].soil_depth = mlt->at(15).at(i).soil_depth;

        tile->north_row[i].offset = mlt->at(i).at(0).offset; //  Not used
        tile->south_row[i].offset = mlt->at(i).at(15).offset;
        tile->west_column[i].offset = mlt->at(0).at(i).offset;
        tile->east_column[i].offset = mlt->at(15).at(i).offset;

        tile->north_row[i].elevation = mlt->at(i).at(0).elevation;
        tile->south_row[i].elevation = mlt->at(i).at(15).elevation;
        tile->west_column[i].elevation = mlt->at(0).at(i).elevation;
        tile->east_column[i].elevation = mlt->at(15).at(i).elevation;

        tile->north_row[i].river_present = mlt->at(i).at(0).river_present; //  Not used
        tile->south_row[i].river_present = mlt->at(i).at(15).river_present;
        tile->west_column[i].river_present = mlt->at(0).at(i).river_present;
        tile->east_column[i].river_present = mlt->at(15).at(i).river_present;

        tile->north_row[i].river_elevation = mlt->at(i).at(0).river_elevation; //  Not used
        tile->south_row[i].river_elevation = mlt->at(i).at(15).river_elevation;
        tile->west_column[i].river_elevation = mlt->at(0).at(i).river_elevation;
        tile->east_column[i].river_elevation = mlt->at(15).at(i).river_elevation;

        tile->north_row[i].adamantine_level = mlt->at(i).at(0).adamantine_level; //  Not used
        tile->south_row[i].adamantine_level = mlt->at(i).at(15).adamantine_level;
        tile->west_column[i].adamantine_level = mlt->at(0).at(i).adamantine_level;
        tile->east_column[i].adamantine_level = mlt->at(15).at(i).adamantine_level;

        tile->north_row[i].magma_level = mlt->at(i).at(0).magma_level; //  Not used
        tile->south_row[i].magma_level = mlt->at(i).at(15).magma_level;
        tile->west_column[i].magma_level = mlt->at(0).at(i).magma_level;
        tile->east_column[i].magma_level = mlt->at(15).at(i).magma_level;

        tile->north_row[i].biome_offset = mlt->at(i).at(0).biome_offset;
        tile->south_row[i].biome_offset = mlt->at(i).at(15).biome_offset;
        tile->west_column[i].biome_offset = mlt->at(0).at(i).biome_offset;
        tile->east_column[i].biome_offset = mlt->at(15).at(i).biome_offset;

        tile->north_row[i].savagery_level = mlt->at(i).at(0).savagery_level;
        tile->south_row[i].savagery_level = mlt->at(i).at(15).savagery_level;
        tile->west_column[i].savagery_level = mlt->at(0).at(i).savagery_level;
        tile->east_column[i].savagery_level = mlt->at(15).at(i).savagery_level;

        tile->north_row[i].evilness_level = mlt->at(i).at(0).evilness_level;
        tile->south_row[i].evilness_level = mlt->at(i).at(15).evilness_level;
        tile->west_column[i].evilness_level = mlt->at(0).at(i).evilness_level;
        tile->east_column[i].evilness_level = mlt->at(15).at(i).evilness_level;

        tile->north_row[i].metals.resize(0);  //  Not used
        tile->south_row[i].metals.resize(0);
        tile->west_column[i].metals.resize(0);
        tile->east_column[i].metals.resize(0);

        tile->north_row[i].economics.resize(0);  //  Not used
        tile->south_row[i].economics.resize(0);
        tile->west_column[i].economics.resize(0);
        tile->east_column[i].economics.resize(0);

        tile->north_row[i].minerals.resize(0);  //  Not used
        tile->south_row[i].minerals.resize(0);
        tile->west_column[i].minerals.resize(0);
        tile->east_column[i].minerals.resize(0);

        tile->north_corner_selection[i] = world_data->region_details[0]->edges.biome_corner[i][0];
        tile->west_corner_selection[i] = world_data->region_details[0]->edges.biome_corner[0][i];
    }

    for (uint8_t i = 0; i < 16; i++) {
        for (uint8_t k = 0; k < 16; k++) {
            tile->region_type[i][k] = world_data->regions[tile->biome_index[mlt->at(i).at(k).biome_offset]]->type;
        }
    }

    tile->surveyed = true;
}

//=================================================================================

df::coord2d embark_assist::survey::apply_offset(uint16_t x, uint16_t y, int8_t offset) {
    df::coord2d result;
    result.x = x;
    result.y = y;

    switch (offset) {
    case 1:
        result.x--;
        result.y++;
        break;

    case 2:
        result.y++;
        break;

    case 3:
        result.x++;
        result.y++;
        break;

    case 4:
        result.x--;
        break;

    case 5:
        break;  // Center. No change

    case 6:
        result.x++;
        break;

    case 7:
        result.x--;
        result.y--;
        break;

    case 8:
        result.y--;
        break;

    case 9:
        result.x++;
        result.y--;
        break;

    default:
        //  Bug. Just act as if it's the center...
        break;
    }

    if (result.x < 0) {
        result.x = 0;
    }
    else if (result.x >= world->worldgen.worldgen_parms.dim_x) {
        result.x = world->worldgen.worldgen_parms.dim_x - 1;
    }

    if (result.y < 0) {
        result.y = 0;
    }
    else if (result.y >= world->worldgen.worldgen_parms.dim_y) {
        result.y = world->worldgen.worldgen_parms.dim_y - 1;
    }

    return result;
}

//=================================================================================

void adjust_coordinates(int16_t *x, int16_t *y, int8_t *i, int8_t *k) {
    if (*i < 0) {
        *x = *x - 1;
        *i = *i + 16;
    }
    else if (*i > 15) {
        *x = *x + 1;
        *i = *i - 16;
    }

    if (*k < 0) {
        *y = *y - 1;
        *k = *k + 16;
    }
    else if (*k > 15) {
        *y = *y + 1;
        *k = *k - 16;
    }
}

//=================================================================================

df::world_region_type embark_assist::survey::region_type_of(embark_assist::defs::world_tile_data *survey_results,
    int16_t x,
    int16_t y,
    int8_t i,
    int8_t k) {

    df::world_data *world_data = world->world_data;
    int16_t effective_x = x;
    int16_t effective_y = y;
    int8_t effective_i = i;
    int8_t effective_k = k;
    adjust_coordinates(&effective_x, &effective_y, &effective_i, &effective_i);

    if (effective_x < 0 ||
        effective_x >= world_data->world_width ||
        effective_y < 0 ||
        effective_y >= world_data->world_height) {
        return df::world_region_type::Lake;  //  Essentially dummy value, yielding to everything. It will
                                             //  be handled properly later anyway.
    }

    return survey_results->at(effective_x).at(effective_y).region_type[effective_i][effective_k];
}

//=================================================================================

uint8_t  embark_assist::survey::translate_corner(embark_assist::defs::world_tile_data *survey_results,
    uint8_t corner_location,
    uint16_t x,
    uint16_t y,
    uint8_t i,
    uint8_t k) {

    df::world_data *world_data = world->world_data;
    df::world_region_type nw_region_type;
    df::world_region_type n_region_type;
    df::world_region_type w_region_type;
    df::world_region_type home_region_type;

    int16_t effective_x = x;
    int16_t effective_y = y;
    int8_t effective_i = i;
    int8_t effective_k = k;

    uint8_t effective_corner;
    bool nw_region_type_active;
    bool n_region_type_active;
    bool w_region_type_active;
    bool home_region_type_active;
    uint8_t nw_region_type_level;
    uint8_t n_region_type_level;
    uint8_t w_region_type_level;
    uint8_t home_region_type_level;

    if (corner_location == 4) {  //  We're the reference. No change.
    }
    else if (corner_location == 5) {  //  Tile to the east is the reference
        effective_i = i + 1;
    }
    else if (corner_location == 7) {  //  Tile to the south is the reference
        effective_k = k + 1;
    }
    else {  //  8, tile to the southeast is the reference.
        effective_i = i + 1;
        effective_k = k + 1;
    }

    adjust_coordinates(&effective_x, &effective_y, &effective_i, &effective_i);

    if (effective_x == world_data->world_width) {
        if (effective_y == world_data->world_height) {  //  Only the SE corner of the SE most tile of the world can reference this.
            return 4;
        }
        else {  //  East side corners of the east edge of the world
            if (corner_location == 5) {
                return 1;
            }
            else {  //  Can only be corner_location == 8
                return 4;
            }
        }
    }
    else if (effective_y == world_data->world_height) {
        if (corner_location == 7) {
            return 4;
        }
        else {  //  Can only be corner_location == 8
            return 3;
        }
    }

    if (effective_x == x && effective_y == y) {
        effective_corner = world_data->region_details[0]->edges.biome_corner[effective_i][effective_k];
    }
    else if (effective_y != y) {
        effective_corner = survey_results->at(effective_x).at(effective_y).north_corner_selection[effective_i];
    }
    else {
        effective_corner = survey_results->at(effective_x).at(effective_y).west_corner_selection[effective_k];
    }

    nw_region_type = embark_assist::survey::region_type_of(survey_results, x, y, effective_i - 1, effective_k - 1);
    n_region_type = embark_assist::survey::region_type_of(survey_results, x, y, effective_i, effective_k - 1);
    w_region_type = embark_assist::survey::region_type_of(survey_results, x, y, effective_i - 1, effective_k);
    home_region_type = embark_assist::survey::region_type_of(survey_results, x, y, effective_i, effective_k);

    if (nw_region_type == df::world_region_type::Lake ||
        nw_region_type == df::world_region_type::Ocean) {
        nw_region_type_level = 0;
    }
    else if (nw_region_type == df::world_region_type::Mountains) {
        nw_region_type_level = 1;
    }
    else {
        nw_region_type_level = 2;
    }

    if (n_region_type == df::world_region_type::Lake ||
        n_region_type == df::world_region_type::Ocean) {
        n_region_type_level = 0;
    }
    else if (n_region_type == df::world_region_type::Mountains) {
        n_region_type_level = 1;
    }
    else {
        n_region_type_level = 2;
    }

    if (w_region_type == df::world_region_type::Lake ||
        w_region_type == df::world_region_type::Ocean) {
        w_region_type_level = 0;
    }
    else if (w_region_type == df::world_region_type::Mountains) {
        w_region_type_level = 1;
    }
    else {
        w_region_type_level = 2;
    }

    if (home_region_type == df::world_region_type::Lake ||
        home_region_type == df::world_region_type::Ocean) {
        home_region_type_level = 0;
    }
    else if (n_region_type == df::world_region_type::Mountains) {
        home_region_type_level = 1;
    }
    else {
        home_region_type_level = 2;
    }

    if (effective_x == 0 && effective_i == 0) {  //  West edge of the world
        if (effective_y == 0 && effective_k == 0) {
            return 4;  //  Only a single reference to this info, the own tile.
        }
        else {
            nw_region_type_level = -1;  //  Knock out the unreachable corners
            w_region_type_level = -1;
        }
    }

    if (effective_y == 0 && effective_k == 0) {  //  North edge of the world
        nw_region_type_level = -1;  //  Knock out the unreachable corners
        n_region_type_level = -1;

        if (corner_location == 4 && effective_corner == 1) { // The logic below would select the wrong alternative.
            effective_corner = 3;
        }
    }

    nw_region_type_active = nw_region_type_level >= n_region_type_level &&
        nw_region_type_level >= w_region_type_level &&
        nw_region_type_level >= home_region_type_level;

    n_region_type_active = n_region_type_level >= nw_region_type_level &&
        n_region_type_level >= w_region_type_level &&
        n_region_type_level >= home_region_type_level;

    w_region_type_active = w_region_type_level >= nw_region_type_level &&
        w_region_type_level >= n_region_type_level &&
        w_region_type_level >= home_region_type_level;

    home_region_type_active = home_region_type_level >= nw_region_type_level &&
        home_region_type_level >= n_region_type_level &&
        home_region_type_level >= w_region_type_level;

    if ((effective_corner == 0 && !nw_region_type_active) ||
        (effective_corner == 1 && !n_region_type_active) ||
        (effective_corner == 2 && !w_region_type_active) ||
        (effective_corner == 3 && !home_region_type_active)) {
        //  The designated corner is suppressed. The precedence list below seems
        //  to match what DF produces except in the case adjusted above.
        if (nw_region_type_active) {
            effective_corner = 0;
        }
        else if (n_region_type_active) {
            effective_corner = 1;
        }
        else if (w_region_type_active) {
            effective_corner = 2;
        }
        else {
            effective_corner = 3;
        }
    }

    switch (effective_corner) {
    case 0:
        return corner_location - 4;
        break;

    case 1:
        return corner_location - 3;
        break;

    case 2:
        return corner_location - 1;
        break;

    case 3:
        return corner_location;
        break;
    }

    return -1;  //  Should never happen

                /*  The logic that's reduced to the code above.
                switch (corner_location) {
                case 0: //  N/A Not to the north or west
                case 1: //  N/A
                case 2: //  N/A
                case 3: //  N/A
                case 6: //  N/A
                return -1;  //  Should never happen
                break;

                case 4: //  Self
                switch (corner) {
                case 0:
                return 0;  //  Northwest
                break;

                case 1:
                return 1;  //  North
                break;

                case 2:
                return 3;  //  West
                break;

                case 3:
                return 4;  //  Self
                }

                case 5: //  East
                switch (corner) {
                case 0:
                return 1;  //  North
                break;

                case 1:
                return 2;  //  Northeast
                break;

                case 2:
                return 4;  //  Self
                break;

                case 3:
                return 5;  //  East
                }
                case 7: //  South
                switch (corner) {
                case 0:
                return 3;  //  West
                break;

                case 1:
                return 4;  //  Self
                break;

                case 2:
                return 6;  //  Southwest
                break;

                case 3:
                return 7;  //  South
                }

                case 8: //  Southeast
                switch (corner) {
                case 0:
                return 4;  //  Self
                break;

                case 1:
                return 5;  //  East
                break;

                case 2:
                return 7;  //  South
                break;

                case 3:
                return 8;  //  Southeast
                }
                }
                */
}

//=================================================================================

uint8_t embark_assist::survey::translate_ns_edge(embark_assist::defs::world_tile_data *survey_results,
    bool own_edge,
    uint16_t x,
    uint16_t y,
    uint8_t i,
    uint8_t k) {

    df::world_data *world_data = world->world_data;
    uint8_t effective_edge;
    df::world_region_type north_region_type;
    df::world_region_type south_region_type;

    if (own_edge) {
        effective_edge = world_data->region_details[0]->edges.biome_x[i][k];
        south_region_type = embark_assist::survey::region_type_of(survey_results, x, y, i, k);
        north_region_type = embark_assist::survey::region_type_of(survey_results, x, y, i, k - 1);
    }
    else {
        effective_edge = world_data->region_details[0]->edges.biome_x[i][k + 1];
        north_region_type = embark_assist::survey::region_type_of(survey_results, x, y, i, k);
        south_region_type = embark_assist::survey::region_type_of(survey_results, x, y, i, k + 1);
    }

    //  Apply rules for Ocean && Lake to yield to everything else,
    //  and Mountain to everything but those.
    //
    if ((north_region_type == df::world_region_type::Lake ||
        north_region_type == df::world_region_type::Ocean) &&
        south_region_type != df::world_region_type::Lake &&
        south_region_type != df::world_region_type::Ocean) {
        effective_edge = 1;
    }

    if ((south_region_type == df::world_region_type::Lake ||
        south_region_type == df::world_region_type::Ocean) &&
        north_region_type != df::world_region_type::Lake &&
        north_region_type != df::world_region_type::Ocean) {
        effective_edge = 0;
    }

    if (north_region_type == df::world_region_type::Mountains &&
        south_region_type != df::world_region_type::Lake &&
        south_region_type != df::world_region_type::Ocean &&
        south_region_type != df::world_region_type::Mountains) {
        effective_edge = 1;
    }

    if (south_region_type == df::world_region_type::Mountains &&
        north_region_type != df::world_region_type::Lake &&
        north_region_type != df::world_region_type::Ocean &&
        north_region_type != df::world_region_type::Mountains) {
        effective_edge = 0;
    }

    if (effective_edge == 0) {
        if (own_edge) {
            return 1;
        }
        else {
            return 4;
        }
    }
    else {
        if (own_edge) {
            return 4;
        }
        else {
            return 7;
        }
    }
}

//=================================================================================

uint8_t embark_assist::survey::translate_ew_edge(embark_assist::defs::world_tile_data *survey_results,
    bool own_edge,
    uint16_t x,
    uint16_t y,
    uint8_t i,
    uint8_t k) {

    df::world_data *world_data = world->world_data;
    uint8_t effective_edge;
    df::world_region_type west_region_type;
    df::world_region_type east_region_type;

    if (own_edge) {
        effective_edge = world_data->region_details[0]->edges.biome_x[i][k];
        east_region_type = embark_assist::survey::region_type_of(survey_results, x, y, i, k);
        west_region_type = embark_assist::survey::region_type_of(survey_results, x, y, i - 1, k);
    }
    else {
        effective_edge = world_data->region_details[0]->edges.biome_x[i + 1][k];
        west_region_type = embark_assist::survey::region_type_of(survey_results, x, y, i, k);
        east_region_type = embark_assist::survey::region_type_of(survey_results, x, y, i + 1, k);
    }

    //  Apply rules for Ocean && Lake to yield to everything else,
    //  and Mountain to everything but those.
    //
    if ((west_region_type == df::world_region_type::Lake ||
        west_region_type == df::world_region_type::Ocean) &&
        east_region_type != df::world_region_type::Lake &&
        east_region_type != df::world_region_type::Ocean) {
        effective_edge = 1;
    }

    if ((east_region_type == df::world_region_type::Lake ||
        east_region_type == df::world_region_type::Ocean) &&
        west_region_type != df::world_region_type::Lake &&
        west_region_type != df::world_region_type::Ocean) {
        effective_edge = 0;
    }

    if (west_region_type == df::world_region_type::Mountains &&
        west_region_type != df::world_region_type::Lake &&
        east_region_type != df::world_region_type::Ocean &&
        east_region_type != df::world_region_type::Mountains) {
        effective_edge = 1;
    }

    if (east_region_type == df::world_region_type::Mountains &&
        east_region_type != df::world_region_type::Lake &&
        west_region_type != df::world_region_type::Ocean &&
        west_region_type != df::world_region_type::Mountains) {
        effective_edge = 0;
    }
    if (effective_edge == 0) {
        if (own_edge) {
            return 3;
        }
        else {
            return 4;
        }
    }
    else {
        if (own_edge) {
            return 4;
        }
        else {
            return 5;
        }
    }
}

//=================================================================================

void embark_assist::survey::survey_region_sites(embark_assist::defs::site_lists *site_list) {
    //            color_ostream_proxy out(Core::getInstance().getConsole());
    auto screen = Gui::getViewscreenByType<df::viewscreen_choose_start_sitest>(0);
    df::world_data *world_data = world->world_data;
    int8_t index = 0;

    site_list->clear();

    for (uint32_t i = 0; i < world_data->region_map[screen->location.region_pos.x][screen->location.region_pos.y].sites.size(); i++) {
        auto site = world_data->region_map[screen->location.region_pos.x][screen->location.region_pos.y].sites[i];
        switch (site->type) {
        case df::world_site_type::PlayerFortress:
        case df::world_site_type::DarkFortress:
        case df::world_site_type::MountainHalls:
        case df::world_site_type::ForestRetreat:
        case df::world_site_type::Town:
        case df::world_site_type::Fortress:
            break;  //  Already visible

        case df::world_site_type::Cave:
            if (!world->worldgen.worldgen_parms.all_caves_visible) {
                site_list->push_back({ (uint8_t)site->rgn_min_x , (uint8_t)site->rgn_min_y, 'c' });  //  Cave
            }
            break;

        case df::world_site_type::Monument:
            if (site->subtype_info->lair_type != -1 ||
                site->subtype_info->is_monument == 0) {  //  Not Tomb, which is visible already
            }
            else if (site->subtype_info->lair_type == -1) {
                site_list->push_back({ (uint8_t)site->rgn_min_x , (uint8_t)site->rgn_min_y, 'V' });  //  Vault
            }
            else {
                site_list->push_back({ (uint8_t)site->rgn_min_x , (uint8_t)site->rgn_min_y, 'M' });  //  Any other Monument type. Pyramid?
            }
            break;

        case df::world_site_type::ImportantLocation:
            site_list->push_back({ (uint8_t)site->rgn_min_x , (uint8_t)site->rgn_min_y, 'i' });  //  Don't really know what that is...
            break;

        case df::world_site_type::LairShrine:
            if (site->subtype_info->lair_type == 0 ||
                site->subtype_info->lair_type == 1 ||
                site->subtype_info->lair_type == 4) {  // Only Rocs seen. Mountain lair?
                site_list->push_back({ (uint8_t)site->rgn_min_x , (uint8_t)site->rgn_min_y, 'l' });  //  Lair
            }
            else if (site->subtype_info->lair_type == 2) {
                site_list->push_back({ (uint8_t)site->rgn_min_x , (uint8_t)site->rgn_min_y, 'L' });  //  Labyrinth
            }
            else if (site->subtype_info->lair_type == 3) {
                site_list->push_back({ (uint8_t)site->rgn_min_x , (uint8_t)site->rgn_min_y, 'S' });  //  Shrine
            }
            else {
                site_list->push_back({ (uint8_t)site->rgn_min_x , (uint8_t)site->rgn_min_y, '?' });  //  Can these exist?
            }
            break;

        case df::world_site_type::Camp:
            site_list->push_back({ (uint8_t)site->rgn_min_x , (uint8_t)site->rgn_min_y, 'C' });  //  Camp
            break;

        default:
            site_list->push_back({ (uint8_t)site->rgn_min_x , (uint8_t)site->rgn_min_y, '!' });  //  Not even in the enum...
            break;
        }
    }
}

//=================================================================================

void embark_assist::survey::survey_embark(embark_assist::defs::mid_level_tiles *mlt,
    embark_assist::defs::world_tile_data *survey_results,
    embark_assist::defs::site_infos *site_info,
    bool use_cache) {

    //color_ostream_proxy out(Core::getInstance().getConsole());
    auto screen = Gui::getViewscreenByType<df::viewscreen_choose_start_sitest>(0);
    int16_t elevation = 0;
    uint16_t x = screen->location.region_pos.x;
    uint16_t y = screen->location.region_pos.y;
    std::vector<bool> metals(state->max_inorganic);
    std::vector<bool> economics(state->max_inorganic);
    std::vector<bool> minerals(state->max_inorganic);
    bool incursion_processing_failed = false;
    df::world_data *world_data = world->world_data;

    if (!use_cache) {  //  For some reason DF scrambles these values on world tile movements (at least in Lua...).
        state->local_min_x = screen->location.embark_pos_min.x;
        state->local_min_y = screen->location.embark_pos_min.y;
        state->local_max_x = screen->location.embark_pos_max.x;
        state->local_max_y = screen->location.embark_pos_max.y;
    }

    state->x = x;
    state->y = y;

    site_info->incursions_processed = true;
    site_info->aquifer = false;
    site_info->aquifer_full = true;
    site_info->min_soil = 10;
    site_info->max_soil = 0;
    site_info->flat = true;
    site_info->max_waterfall = 0;
    site_info->clay = false;
    site_info->sand = false;
    site_info->flux = false;
    site_info->coal = false;
    site_info->blood_rain = false;
    site_info->permanent_syndrome_rain = false;
    site_info->temporary_syndrome_rain = false;
    site_info->reanimating = false;
    site_info->thralling = false;
    site_info->metals.clear();
    site_info->economics.clear();
    site_info->metals.clear();

    for (uint8_t i = state->local_min_x; i <= state->local_max_x; i++) {
        for (uint8_t k = state->local_min_y; k <= state->local_max_y; k++) {
            if (mlt->at(i).at(k).aquifer) {
                site_info->aquifer = true;
            }
            else {
                site_info->aquifer_full = false;
            }

            if (mlt->at(i).at(k).soil_depth < site_info->min_soil) {
                site_info->min_soil = mlt->at(i).at(k).soil_depth;
            }

            if (mlt->at(i).at(k).soil_depth > site_info->max_soil) {
                site_info->max_soil = mlt->at(i).at(k).soil_depth;
            }

            if (i == state->local_min_x && k == state->local_min_y) {
                elevation = mlt->at(i).at(k).elevation;
            }
            else if (elevation != mlt->at(i).at(k).elevation) {
                site_info->flat = false;
            }

            if (mlt->at(i).at(k).river_present) {
                if (i < 15 &&
                    mlt->at(i + 1).at(k).river_present &&
                    abs(mlt->at(i).at(k).river_elevation - mlt->at(i + 1).at(k).river_elevation) >
                    site_info->max_waterfall) {
                    site_info->max_waterfall =
                        abs(mlt->at(i).at(k).river_elevation - mlt->at(i + 1).at(k).river_elevation);
                }

                if (k < 15 &&
                    mlt->at(i).at(k + 1).river_present &&
                    abs(mlt->at(i).at(k).river_elevation - mlt->at(i).at(k + 1).river_elevation) >
                    site_info->max_waterfall) {
                    site_info->max_waterfall =
                        abs(mlt->at(i).at(k).river_elevation - mlt->at(i).at(k + 1).river_elevation);
                }
            }

            if (mlt->at(i).at(k).clay) {
                site_info->clay = true;
            }

            if (mlt->at(i).at(k).sand) {
                site_info->sand = true;
            }

            if (mlt->at(i).at(k).flux) {
                site_info->flux = true;
            }

            if (mlt->at(i).at(k).coal) {
                site_info->coal = true;
            }

            if (survey_results->at(x).at(y).blood_rain[mlt->at(i).at(k).biome_offset]) {
                site_info->blood_rain = true;
            }

            if (survey_results->at(x).at(y).permanent_syndrome_rain[mlt->at(i).at(k).biome_offset]) {
                site_info->permanent_syndrome_rain = true;
            }

            if (survey_results->at(x).at(y).temporary_syndrome_rain[mlt->at(i).at(k).biome_offset]) {
                site_info->temporary_syndrome_rain = true;
            }

            if (survey_results->at(x).at(y).reanimating[mlt->at(i).at(k).biome_offset]) {
                site_info->reanimating = true;
            }

            if (survey_results->at(x).at(y).thralling[mlt->at(i).at(k).biome_offset]) {
                site_info->thralling = true;
            }

            for (uint16_t l = 0; l < state->max_inorganic; l++) {
                metals[l] = metals[l] || mlt->at(i).at(k).metals[l];
                economics[l] = economics[l] || mlt->at(i).at(k).economics[l];
                minerals[l] = minerals[l] || mlt->at(i).at(k).minerals[l];
            }
        }
    }

    for (uint16_t l = 0; l < state->max_inorganic; l++) {
        if (metals[l]) {
            site_info->metals.push_back(l);
        }

        if (economics[l]) {
            site_info->economics.push_back(l);
        }

        if (minerals[l]) {
            site_info->minerals.push_back(l);
        }
    }

    //  Take incursions into account.

    for (int8_t i = state->local_min_x; i <= state->local_max_x; i++) {
        //  NW corner, north row
        if ((i == 0 && state->local_min_y == 0 && x - 1 >= 0 && y - 1 >= 0 && !survey_results->at(x - 1).at (y - 1).surveyed) ||
            (i == 0 && x - 1 >= 0 && !survey_results->at(x - 1).at(y).surveyed) ||
            (state->local_min_y == 0 && y - 1 >= 0 && !survey_results->at(x).at(y - 1).surveyed)) {
            incursion_processing_failed = true;
        }
        else {
            process_embark_incursion_mid_level_tile
            (translate_corner(survey_results,
                4,
                x,
                y,
                i,
                state->local_min_y),
                site_info,
                survey_results,
                mlt,
                i,
                state->local_min_y);
        }

        //  N edge, north row
        if (state->local_min_y == 0 && y - 1 >= 0 && !survey_results->at(x).at(y - 1).surveyed) {
            incursion_processing_failed = true;
        }
        else {
            process_embark_incursion_mid_level_tile
            (translate_ns_edge(survey_results,
                true,
                x,
                y,
                i,
                state->local_min_y),
                site_info,
                survey_results,
                mlt,
                i,
                state->local_min_y);
        }

        //  NE corner, north row
        if ((i == 15 && state->local_min_y == 0 && x + 1 < world_data->world_width && y - 1 >= 0 && !survey_results->at(x + 1).at(y - 1).surveyed) ||
            (i == 15 && x + 1 < world_data->world_width && !survey_results->at(x + 1).at(y).surveyed) ||
            (state->local_min_y == 0 && y - 1 >= 0 && !survey_results->at(x).at(y - 1).surveyed)) {
            incursion_processing_failed = true;
        }
        else {
            process_embark_incursion_mid_level_tile
            (translate_corner(survey_results,
                5,
                x,
                y,
                i,
                state->local_min_y),
                site_info,
                survey_results,
                mlt,
                i,
                state->local_min_y);
         }

        //  SW corner, south row
        if ((i == 0 && state->local_max_y == 15 && x - 1 >= 0 && y + 1 < world_data->world_height && !survey_results->at(x - 1).at(y + 1).surveyed) ||
            (i == 0 && x - 1 >= 0 && !survey_results->at(x - 1).at(y).surveyed) ||
            (state->local_max_y == 15 && y + 1 < world_data->world_height && !survey_results->at(x).at(y + 1).surveyed)) {
            incursion_processing_failed = true;
        }
        else {
            process_embark_incursion_mid_level_tile
            (translate_corner(survey_results,
                7,
                x,
                y,
                i,
                state->local_max_y),
                site_info,
                survey_results,
                mlt,
                i,
                state->local_max_y);
        }

        //  S edge, south row
        if (state->local_max_y == 15 && y + 1 < world_data->world_height && !survey_results->at(x).at(y + 1).surveyed) {
            incursion_processing_failed = true;
        }
        else {
            process_embark_incursion_mid_level_tile
            (translate_ns_edge(survey_results,
                false,
                x,
                y,
                i,
                state->local_max_y),
                site_info,
                survey_results,
                mlt,
                i,
                state->local_max_y);
        }

        //  SE corner south row
        if ((i == 15 && state->local_max_y == 15 && x + 1 < world_data->world_width && y + 1 < world_data->world_height && !survey_results->at(x + 1).at(y + 1).surveyed) ||
            (i == 15 && x + 1 < world_data->world_width && !survey_results->at(x + 1).at(y).surveyed) ||
            (state->local_max_y == 15 && y + 1 < world_data->world_height && !survey_results->at(x).at(y + 1).surveyed)) {
            incursion_processing_failed = true;
        }
        else {
            process_embark_incursion_mid_level_tile
            (translate_corner(survey_results,
                8,
                x,
                y,
                i,
                state->local_max_y),
                site_info,
                survey_results,
                mlt,
                i,
                state->local_max_y);
        }
    }

    for (int8_t k = state->local_min_y; k <= state->local_max_y; k++) {
        // NW corner, west side
        if ((state->local_min_x == 0 && x - 1 >= 0 && !survey_results->at(x - 1).at(y).surveyed)) {
            incursion_processing_failed = true;
        }
        else if (k > state->local_min_y) { //  We've already covered the NW corner of the NW, with its complications.
            process_embark_incursion_mid_level_tile
            (translate_corner(survey_results,
                4,
                x,
                y,
                state->local_min_x,
                k),
                site_info,
                survey_results,
                mlt,
                state->local_min_x,
                k);
        }

        // W edge, west side
        if (state->local_min_x == 0 && x - 1 >= 0 && !survey_results->at(x - 1).at(y).surveyed) {
            incursion_processing_failed = true;
        }
        else {
            process_embark_incursion_mid_level_tile
            (translate_ns_edge(survey_results,
                true,
                x,
                y,
                state->local_min_x,
                k),
                site_info,
                survey_results,
                mlt,
                state->local_min_x,
                k);
        }

        // SW corner, west side
        if (state->local_min_x == 0 && x - 1 >= 0 && !survey_results->at(x - 1).at(y).surveyed) {
            incursion_processing_failed = true;
        }
        else if (k < state->local_max_y) { //  We've already covered the SW corner of the SW tile, with its complicatinons.
            process_embark_incursion_mid_level_tile
            (translate_corner(survey_results,
                7,
                x,
                y,
                state->local_min_x,
                k),
                site_info,
                survey_results,
                mlt,
                state->local_min_x,
                k);
        }

        // NE corner, east side
        if ((state->local_max_x == 15 && x + 1 < world_data->world_width && !survey_results->at(x + 1).at(y).surveyed)) {
            incursion_processing_failed = true;
        }
        else if (k > state->local_min_y) { //  We've already covered the NE tile's NE corner, with its complications.
            process_embark_incursion_mid_level_tile
            (translate_corner(survey_results,
                5,
                x,
                y,
                state->local_max_x,
                k),
                site_info,
                survey_results,
                mlt,
                state->local_max_x,
                k);
        }

        // E edge, east side
        if (state->local_max_x == 15 && x + 1 < world_data->world_width && !survey_results->at(x + 1).at(y).surveyed) {
            incursion_processing_failed = true;
        }
        else {
            process_embark_incursion_mid_level_tile
            (translate_ns_edge(survey_results,
                false,
                x,
                y,
                state->local_max_x,
                k),
                site_info,
                survey_results,
                mlt,
                state->local_max_x,
                k);
        }

        // SE corner, east side
        if (state->local_max_x == 15 && x + 1 < world_data->world_width && !survey_results->at(x + 1).at(y).surveyed) {
            incursion_processing_failed = true;
        }
        else if (k < state->local_max_y) { //  We've already covered the SE tile's SE corner, with its complications.
            process_embark_incursion_mid_level_tile
            (translate_corner(survey_results,
                8,
                x,
                y,
                state->local_max_x,
                k),
                site_info,
                survey_results,
                mlt,
                state->local_max_x,
                k);
        }
    }

    if (incursion_processing_failed) site_info->incursions_processed = false;
}

//=================================================================================

void embark_assist::survey::shutdown() {
    delete state;
}

