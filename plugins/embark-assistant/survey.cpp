#include <math.h>
#include <vector>
#include <cstring>

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
#include "df/creature_interaction_effect_type.h"
#include "df/entity_raw.h"
#include "df/feature_init.h"
#include "df/feature_type.h"
#include "df/historical_entity.h"
#include "df/historical_entity_type.h"
#include "df/inorganic_flags.h"
#include "df/inorganic_raw.h"
#include "df/interaction.h"
#include "df/interaction_effect.h"
#include "df/interaction_effect_type.h"
#include "df/interaction_instance.h"
#include "df/interaction_source.h"
#include "df/interaction_source_type.h"
#include "df/interaction_target.h"
#include "df/interaction_target_materialst.h"
#include "df/material_common.h"
#include "df/reaction.h"
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

        static states *state = nullptr;

        //=======================================================================================

        embark_assist::defs::river_sizes river_size_of(uint8_t width) {
            if (width < 7) {
                return embark_assist::defs::river_sizes::Stream;
            }
            else if (width < 13) {
                return embark_assist::defs::river_sizes::Minor;
            }
            else if (width < 24) {
                return embark_assist::defs::river_sizes::Medium;
            }
            else {
                return embark_assist::defs::river_sizes::Major;
            }
        }

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
                                for (const auto &syndrome : world->raws.inorganics[material->mat_index]->material.syndrome.syndrome) {
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
                divisor = ((lat * 57) / steps + 0.4);
            }
            else if (world->world_data->world_height == 33) {
                divisor = ((lat * 61) / steps + 0.1);
            }
            else if (world->world_data->world_height == 65) {
                divisor = ((lat * 63) / steps);
            }
            else if (world->world_data->world_height == 129 ||
                world->world_data->world_height == 257) {
                divisor = ((lat * 64) / steps);
            }
            else {
                return max_temperature; // Not any standard world height. No formula available
            }

            return max_temperature - ceil(divisor * 3 / 4);
        }

        //=================================================================================

        void process_embark_incursion(embark_assist::defs::site_infos *site_info,
            embark_assist::defs::world_tile_data *survey_results,
            embark_assist::defs::mid_level_tile_incursion_base *mlt,  // Note this is a single tile, as opposed to most usages of this variable name.
            int16_t elevation,
            uint16_t x,
            uint16_t y) {
            site_info->aquifer |= mlt->aquifer;

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
                        &survey_results->at(fetch_x).at(fetch_y).south_row[fetch_i],
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
                        &survey_results->at(fetch_x).at(fetch_y).north_row[fetch_i],
                        mlt->at(i).at(k).elevation,
                        fetch_x,
                        fetch_y);
                }
            }
            else {
                if (fetch_i < 0) {
                    process_embark_incursion(site_info,
                        survey_results,
                        &survey_results->at(fetch_x).at(fetch_y).east_column[fetch_k],
                        mlt->at(i).at(k).elevation,
                        fetch_x,
                        fetch_y);
                }
                else if (fetch_i > 15) {
                    process_embark_incursion(site_info,
                        survey_results,
                        &survey_results->at(fetch_x).at(fetch_y).west_column[fetch_k],
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

embark_assist::defs::tree_levels tree_level_of(df::world_region_type region_type, int16_t vegetation) {
    if (region_type == df::world_region_type::Glacier ||
        region_type == df::world_region_type::Lake ||
        region_type == df::world_region_type::Mountains ||
        region_type == df::world_region_type::Ocean) {
        return embark_assist::defs::tree_levels::None;
    }
    else {
        if (vegetation == 0) {
            return embark_assist::defs::tree_levels::None;
        }
        else if (vegetation <= 9) {
            return embark_assist::defs::tree_levels::Very_Scarce;
        }
        else if (vegetation <= 32) {
            return embark_assist::defs::tree_levels::Scarce;
        }
        else if (vegetation <= 65) {
            return embark_assist::defs::tree_levels::Woodland;
        }
        else {
            return embark_assist::defs::tree_levels::Heavily_Forested;
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
            results.survey_completed = false;
            results.neighboring_clay = false;
            results.neighboring_sand = false;
            for (uint8_t l = 0; l <= ENUM_LAST_ITEM(biome_type); l++) {
                results.neighboring_biomes[l] = false;
            }

            for (uint8_t l = 0; l <= ENUM_LAST_ITEM(world_region_type); l++) {
                results.neighboring_region_types[l] = false;
            }

            for (uint8_t l = 0; l < 2; l++) {
                results.neighboring_savagery[l] = false;
                results.neighboring_evilness[l] = false;
            }

            results.aquifer = embark_assist::defs::Clear_Aquifer_Bits;
            results.clay_count = 0;
            results.sand_count = 0;
            results.flux_count = 0;
            results.coal_count = 0;
            results.min_region_soil = 10;
            results.max_region_soil = 0;
            results.max_waterfall = 0;
            results.min_tree_level = embark_assist::defs::tree_levels::Heavily_Forested;
            results.max_tree_level = embark_assist::defs::tree_levels::None;
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
                    results.biome[l] = DFHack::Maps::getBiomeTypeWithRef(adjusted.x, adjusted.y, k);
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

                    if (geo_summary->at(geo_index).aquifer_absent) {
                        results.aquifer |= embark_assist::defs::None_Aquifer_Bit;
                    }
                    else if (world_data->region_map[adjusted.x][adjusted.y].drainage % 20 == 7) {
                        results.aquifer |= embark_assist::defs::Heavy_Aquifer_Bit;
                    }
                    else {
                        results.aquifer |= embark_assist::defs::Light_Aquifer_Bit;
                    }

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

                    embark_assist::defs::tree_levels tree_level = tree_level_of(world_data->regions[results.biome_index[l]]->type,
                        world_data->region_map[adjusted.x][adjusted.y].vegetation);

                    if (tree_level < results.min_tree_level) results.min_tree_level = tree_level;
                    if (tree_level > results.max_tree_level) results.max_tree_level = tree_level;
                }
                else {
                    results.biome_index[l] = -1;
                    results.biome[l] = -1;
                    results.max_temperature[l] = -30000;
                    results.min_temperature[l] = -30000;
                }
            }

            bool biomes[ENUM_LAST_ITEM(biome_type) + 1];
            for (uint8_t l = 0; l <= ENUM_LAST_ITEM(biome_type); l++) {
                biomes[l] = false;
            }

            for (uint8_t l = 1; l < 10; l++)
            {
                if (results.biome[l] != -1) {
                    biomes[results.biome[l]] = true;
                }
            }
            int count = 0;
            for (uint8_t l = 0; l <= ENUM_LAST_ITEM(biome_type); l++) {
                if (biomes[l]) count++;
            }

            results.biome_count = count;

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

    embark_assist::survey::survey_evil_weather(survey_results);
}

//=================================================================================

void inline copy_incursion_values(embark_assist::defs::mid_level_tile_incursion_base &target,
    const embark_assist::defs::mid_level_tile_incursion_base &source) {
    target.aquifer = source.aquifer;
    target.clay = source.clay;
    target.sand = source.sand;
    target.soil_depth = source.soil_depth;
    target.elevation = source.elevation;
    target.biome_offset = source.biome_offset;
    target.trees = source.trees;
    target.savagery_level = source.savagery_level;
    target.evilness_level = source.evilness_level;
}

//=================================================================================

void reset_mlt_inorganics(embark_assist::defs::mid_level_tiles &mlts) {
    for (uint8_t i = 0; i < 16; i++) {
        for (uint8_t k = 0; k < 16; k++) {
            embark_assist::defs::mid_level_tile &mlt = mlts[i][k];
            // std::memset is much faster than std::fill and also faster than direct assignment - also std::fill might be compiled to std::memset but is not guaranteed to happen
            // have a look here for why: https://travisdowns.github.io/blog/2020/01/20/zero.html
            std::memset(&mlt.metals[0], false, mlt.metals.size() * sizeof(mlt.metals[0]));
            std::memset(&mlt.economics[0], false, mlt.economics.size() * sizeof(mlt.economics[0]));
            std::memset(&mlt.minerals[0], false, mlt.minerals.size() * sizeof(mlt.minerals[0]));
        }
    }
}

//=================================================================================

void embark_assist::survey::survey_mid_level_tile(embark_assist::defs::geo_data *geo_summary,
    embark_assist::defs::world_tile_data *survey_results,
    embark_assist::defs::mid_level_tiles *mlt) {
//              color_ostream_proxy out(Core::getInstance().getConsole());
    auto screen = Gui::getViewscreenByType<df::viewscreen_choose_start_sitest>(0);
    int16_t x = screen->location.region_pos.x;
    int16_t y = screen->location.region_pos.y;
    embark_assist::defs::region_tile_datum &tile = survey_results->at(x).at(y);
    int8_t max_soil_depth;
    int8_t offset;
    int16_t elevation;
    int16_t last_bottom;
    int16_t top_z;
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
    bool aquifer;

    for (uint16_t i = 0; i < state->max_inorganic; i++) {
        tile.metals[i] = 0;
        tile.economics[i] = 0;
        tile.minerals[i] = 0;
    }

    reset_mlt_inorganics(*mlt);

    for (uint8_t i = 1; i < 10; i++) tile.biome_index[i] = -1;

    for (uint8_t i = 0; i < 16; i++) {
        for (uint8_t k = 0; k < 16; k++) {
            embark_assist::defs::mid_level_tile &mid_level_tile = mlt->at(i).at(k);
            max_soil_depth = -1;

            offset = details->biome[i][k];
            adjusted = apply_offset(x, y, offset);

            if (adjusted.x != x || adjusted.y != y)
            {
                mid_level_tile.biome_offset = offset;
            }
            else
            {
                mid_level_tile.biome_offset = 5;
            }
            const df::region_map_entry &region_map_entry = world_data->region_map[adjusted.x][adjusted.y];

            tile.biome_index[mid_level_tile.biome_offset] = region_map_entry.region_id;

            mid_level_tile.savagery_level = region_map_entry.savagery / 33;
            if (mid_level_tile.savagery_level == 3) {
                mid_level_tile.savagery_level = 2;
            }
            mid_level_tile.evilness_level = region_map_entry.evilness / 33;
            if (mid_level_tile.evilness_level == 3) {
                mid_level_tile.evilness_level = 2;
            }

            elevation = details->elevation[i][k];

            const df::world_geo_biome* world_geo_biome = world_data->geo_biomes[region_map_entry.geo_index];
            // Special biome adjustments
            if (!region_map_entry.flags.is_set(region_map_entry_flags::is_lake)) {
                if (region_map_entry.elevation >= 150) {  //  Mountain
                    max_soil_depth = 0;
                }
                else if (region_map_entry.elevation < 100) {  // Ocean
                    if (elevation == 99) {
                        elevation = 98;
                    }

                    if ((world_geo_biome->unk1 == 4 ||
                        world_geo_biome->unk1 == 5) &&
                        details->unk12e8 < 500) {
                        max_soil_depth = 0;
                    }
                }
            }

            features = details->features[i][k];
            mid_level_tile.adamantine_level = -1;
            mid_level_tile.magma_level = -1;

            end_check_l = static_cast<uint16_t>(features.size());
            for (size_t l = 0; l < end_check_l; l++) {
                auto feature = features[l];

                if (feature->feature_idx != -1) {
                    auto feature_init = world_data->feature_map[x / 16][y / 16].features->feature_init[x % 16][y % 16][feature->feature_idx];
                    switch (feature_init->getType())
                    {
                    case df::feature_type::deep_special_tube:
                        mid_level_tile.adamantine_level = feature_init->start_depth;
                        break;

                    case df::feature_type::magma_pool:
                        mid_level_tile.magma_level = 2 - feature_init->start_depth;
                        break;

                    case df::feature_type::volcano:
                        mid_level_tile.magma_level = 3;
                        break;

                    default:
                        break;
                    }
                }
                else if (feature->layer != -1 &&
                    feature->min_z != -30000) {
                    auto layer = world_data->underground_regions[feature->layer];

                    if (layer->type == df::world_underground_region::MagmaSea) {
                        min_z = feature->min_z;  //  The features are individual per region tile
                    }
                }
            }

            //  Compute shifts for layers in the stack.

            if (max_soil_depth == -1) {  //  Not set to zero by the biome
                max_soil_depth = std::max((154 - elevation) / 5, 1);
            }

            auto geo_datum = geo_summary->at(region_map_entry.geo_index);
            soil_erosion = geo_datum.soil_size -
                std::min((int)geo_datum.soil_size, (int)max_soil_depth);
            int16_t layer_shift[16];
            int16_t cur_shift = elevation + soil_erosion - 1;

            aquifer = false;
            mid_level_tile.aquifer = embark_assist::defs::Clear_Aquifer_Bits;
            mid_level_tile.clay = false;
            mid_level_tile.sand = false;
            mid_level_tile.flux = false;
            mid_level_tile.coal = false;

            if (max_soil_depth == 0) {
                mid_level_tile.soil_depth = 0;
            }
            else {
                mid_level_tile.soil_depth = geo_datum.soil_size - soil_erosion;
            }
            mid_level_tile.offset = offset;
            mid_level_tile.elevation = details->elevation[i][k];
            mid_level_tile.river_size = embark_assist::defs::river_sizes::None;
            mid_level_tile.river_elevation = 100;

            if (details->rivers_vertical.active[i][k] != 0) {
                mid_level_tile.river_size = river_size_of (details->rivers_vertical.x_max[i][k] - details->rivers_vertical.x_min[i][k] + 1);
                mid_level_tile.river_elevation = details->rivers_vertical.elevation[i][k];
            }
            else if (details->rivers_horizontal.active[i][k] != 0) {
                mid_level_tile.river_size = river_size_of (details->rivers_horizontal.y_max[i][k] - details->rivers_horizontal.y_min[i][k] + 1);
                mid_level_tile.river_elevation = details->rivers_horizontal.elevation[i][k];
            }

            if (mid_level_tile.river_size != embark_assist::defs::river_sizes::None &&
                world_tile->flags.is_set(df::region_map_entry_flags::is_brook)) {
                mid_level_tile.river_size = embark_assist::defs::river_sizes::Brook;
            }

            if (tile.min_region_soil > mid_level_tile.soil_depth) {
                tile.min_region_soil = mid_level_tile.soil_depth;
            }

            if (tile.max_region_soil < mid_level_tile.soil_depth) {
                tile.max_region_soil = mid_level_tile.soil_depth;
            }

            end_check_l = static_cast<uint16_t>(world_geo_biome->layers.size());
            if (end_check_l > 16) end_check_l = 16;

            for (uint16_t l = 0; l < end_check_l; l++) {
                auto layer = world_geo_biome->layers[l];
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
                auto layer = world_geo_biome->layers[l];
                top_z = last_bottom - 1;
                bottom_z = std::max((int)layer->bottom_height + layer_shift[l], (int)min_z);

                if (l == 15) {
                    bottom_z = min_z;  // stretch layer if needed
                }

                if (top_z >= bottom_z) {
                    last_bottom = bottom_z;

                    mid_level_tile.minerals[layer->mat_index] = true;

                    const df::inorganic_raw* inorganic_layer = world->raws.inorganics[layer->mat_index];
                    end_check_m = static_cast<uint16_t>(inorganic_layer->metal_ore.mat_index.size());

                    for (uint16_t m = 0; m < end_check_m; m++) {
                        mid_level_tile.metals[inorganic_layer->metal_ore.mat_index[m]] = true;
                    }

                    if (layer->type == df::geo_layer_type::SOIL ||
                        layer->type == df::geo_layer_type::SOIL_SAND) {
                        if (inorganic_layer->flags.is_set(df::inorganic_flags::SOIL_SAND)) {
                            mid_level_tile.sand = true;
                        }
                    }

                    if (inorganic_layer->economic_uses.size() > 0) {
                        mid_level_tile.economics[layer->mat_index] = true;

                        end_check_m = static_cast<uint16_t>(inorganic_layer->economic_uses.size());
                        for (uint16_t m = 0; m < end_check_m; m++) {
                            if (inorganic_layer->economic_uses[m] == state->clay_reaction) {
                                mid_level_tile.clay = true;
                            }
                            else if (inorganic_layer->economic_uses[m] == state->flux_reaction) {
                                mid_level_tile.flux = true;
                            }
                        }

                        for (uint16_t m = 0; m < state->coals.size(); m++) {
                            if (layer->mat_index == state->coals[m]) {
                                mid_level_tile.coal = true;
                                break;
                            }
                        }
                    }

                    end_check_m = static_cast<uint16_t>(layer->vein_mat.size());

                    for (uint16_t m = 0; m < end_check_m; m++) {
                        const int vein_mat_index = layer->vein_mat[m];
                        mid_level_tile.minerals[vein_mat_index] = true;

                        const df::inorganic_raw* inorganic_vein = world->raws.inorganics[vein_mat_index];
                        end_check_n = static_cast<uint16_t>(inorganic_vein->metal_ore.mat_index.size());

                        for (uint16_t n = 0; n < end_check_n; n++) {
                            mid_level_tile.metals[inorganic_vein->metal_ore.mat_index[n]] = true;
                        }

                        if (inorganic_vein->economic_uses.size() > 0) {
                            mid_level_tile.economics[vein_mat_index] = true;

                            end_check_n = static_cast<uint16_t>(inorganic_vein->economic_uses.size());
                            for (uint16_t n = 0; n < end_check_n; n++) {
                                if (inorganic_vein->economic_uses[n] == state->clay_reaction) {
                                    mid_level_tile.clay = true;
                                }

                                else if (inorganic_vein->economic_uses[n] == state->flux_reaction) {
                                    mid_level_tile.flux = true;
                                }
                            }

                            for (uint16_t n = 0; n < state->coals.size(); n++) {
                                if (vein_mat_index == state->coals[n]) {
                                    mid_level_tile.coal = true;
                                    break;
                                }
                            }
                        }
                    }

                    if (bottom_z <= elevation - 3 &&
                        inorganic_layer->flags.is_set(df::inorganic_flags::AQUIFER)) {
                        aquifer = true;
                    }
                }
            }
            if (!aquifer) {
                mid_level_tile.aquifer = embark_assist::defs::None_Aquifer_Bit;
            }
            else if (region_map_entry.drainage % 20 == 7) {
                mid_level_tile.aquifer = embark_assist::defs::Heavy_Aquifer_Bit;
            }
            else {
                mid_level_tile.aquifer = embark_assist::defs::Light_Aquifer_Bit;
            }

            mid_level_tile.trees = tree_level_of(world_data->regions[region_map_entry.region_id]->type,
                region_map_entry.vegetation);
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
            auto western_neighbour = mlt->at(i - 1).at(k);
            if (details->rivers_horizontal.active[i][k] != 0 &&
                details->rivers_vertical.active[i - 1][k + 1] != 0 &&
                western_neighbour.river_size == embark_assist::defs::river_sizes::None) {  //  Probably never true
                western_neighbour.river_size = mlt->at(i).at(k).river_size;
                western_neighbour.river_elevation = mlt->at(i).at(k).river_elevation;

                if (western_neighbour.river_elevation > mlt->at(i - 1).at(k + 1).river_elevation) {
                    western_neighbour.river_elevation = mlt->at(i - 1).at(k + 1).river_elevation;
                }
            }
        }
    }

    for (uint8_t i = 0; i < 16; i++) {
        for (uint8_t k = 1; k < 16; k++) {
            auto northern_neighbour = mlt->at(i).at(k - 1);
            if (details->rivers_vertical.active[i][k] != 0 &&
                northern_neighbour.river_size == embark_assist::defs::river_sizes::None) {
                northern_neighbour.river_size = mlt->at(i).at(k).river_size;
                northern_neighbour.river_elevation = mlt->at(i).at(k).river_elevation;
            }
        }
    }

    for (uint8_t i = 1; i < 16; i++) {
        for (uint8_t k = 0; k < 16; k++) {
            auto western_neighbour = mlt->at(i - 1).at(k);
            if (details->rivers_horizontal.active[i][k] != 0 &&
                western_neighbour.river_size == embark_assist::defs::river_sizes::None) {
                western_neighbour.river_size = mlt->at(i).at(k).river_size;
                western_neighbour.river_elevation = mlt->at(i).at(k).river_elevation;
            }
        }
    }

    tile.aquifer = embark_assist::defs::Clear_Aquifer_Bits;
    tile.clay_count = 0;
    tile.sand_count = 0;
    tile.flux_count = 0;
    tile.coal_count = 0;
    tile.min_region_soil = 10;
    tile.max_region_soil = 0;
    tile.savagery_count[0] = 0;
    tile.savagery_count[1] = 0;
    tile.savagery_count[2] = 0;
    tile.evilness_count[0] = 0;
    tile.evilness_count[1] = 0;
    tile.evilness_count[2] = 0;

    for (uint8_t i = 0; i < 16; i++) {
        for (uint8_t k = 0; k < 16; k++) {
            const embark_assist::defs::mid_level_tile &mid_level_tile = mlt->at(i).at(k);
            tile.aquifer |= mid_level_tile.aquifer;
            if (mid_level_tile.clay) { tile.clay_count++; }
            if (mid_level_tile.sand) { tile.sand_count++; }
            if (mid_level_tile.flux) { tile.flux_count++; }
            if (mid_level_tile.coal) { tile.coal_count++; }

            if (mid_level_tile.soil_depth < tile.min_region_soil) {
                tile.min_region_soil = mid_level_tile.soil_depth;
            }

            if (mid_level_tile.soil_depth > tile.max_region_soil) {
                tile.max_region_soil = mid_level_tile.soil_depth;
            }

            if (mid_level_tile.river_size != embark_assist::defs::river_sizes::None) {
                if (tile.min_river_size == embark_assist::defs::river_sizes::None ||
                    mid_level_tile.river_size < tile.min_river_size) {
                    tile.min_river_size = mid_level_tile.river_size;
                }

                if (tile.max_river_size < mid_level_tile.river_size) {
                    tile.max_river_size = mid_level_tile.river_size;
                }

                if (i < 15) {
                    const embark_assist::defs::mid_level_tile &eastern_neighbour = mlt->at(i + 1).at(k);
                    if (eastern_neighbour.river_size != embark_assist::defs::river_sizes::None &&
                        abs(mid_level_tile.river_elevation - eastern_neighbour.river_elevation) > tile.max_waterfall) {
                        tile.max_waterfall = abs(mid_level_tile.river_elevation - eastern_neighbour.river_elevation);
                    }
                }

                if (k < 15) {
                    const embark_assist::defs::mid_level_tile &southern_neighbour = mlt->at(i).at(k + 1);
                    if (southern_neighbour.river_size != embark_assist::defs::river_sizes::None &&
                        abs(mid_level_tile.river_elevation - southern_neighbour.river_elevation) > tile.max_waterfall) {
                        tile.max_waterfall = abs(mid_level_tile.river_elevation - southern_neighbour.river_elevation);
                    }
                }
            }

            // biome_index handled above
            // biome handled below
            // evil weather handled separately
            // reanimating handled separately
            // thralling handled separately

            if (tile.min_tree_level > mid_level_tile.trees) tile.min_tree_level = mid_level_tile.trees;
            if (tile.max_tree_level < mid_level_tile.trees) tile.max_tree_level = mid_level_tile.trees;

            tile.savagery_count[mid_level_tile.savagery_level]++;
            tile.evilness_count[mid_level_tile.evilness_level]++;

            for (uint16_t l = 0; l < state->max_inorganic; l++) {
                if (mid_level_tile.metals[l]) { tile.metals[l] = true; }
                if (mid_level_tile.economics[l]) { tile.economics[l] = true; }
                if (mid_level_tile.minerals[l]) { tile.minerals[l] = true; }
            }
        }
    }

    for (uint8_t i = 1; i < 10; i++) {
        if (tile.biome_index[i] == -1) {
            tile.biome[i] = -1;
        }
    }

    bool biomes[ENUM_LAST_ITEM(biome_type) + 1];
    for (uint8_t i = 0; i <= ENUM_LAST_ITEM(biome_type); i++) {
        biomes[i] = false;
    }

    for (uint8_t i = 1; i < 10; i++)
    {
        if (tile.biome[i] != -1) {
            biomes[tile.biome[i]] = true;
        }
    }
    int count = 0;
    for (uint8_t i = 0; i <= ENUM_LAST_ITEM(biome_type); i++) {
        if (biomes[i]) count++;
    }

    tile.biome_count = count;

    for (uint8_t i = 0; i < 16; i++) {
        copy_incursion_values(tile.north_row[i], mlt->at(i).at(0));
        copy_incursion_values(tile.south_row[i], mlt->at(i).at(15));
        copy_incursion_values(tile.west_column[i], mlt->at(0).at(i));
        copy_incursion_values(tile.east_column[i], mlt->at(15).at(i));

        tile.north_corner_selection[i] = world_data->region_details[0]->edges.biome_corner[i][0];
        tile.west_corner_selection[i] = world_data->region_details[0]->edges.biome_corner[0][i];
        tile.north_row_biome_x[i] = world_data->region_details[0]->edges.biome_x[i][0];
        tile.west_column_biome_y[i] = world_data->region_details[0]->edges.biome_y[0][i];
    }

    for (uint8_t i = 0; i < 16; i++) {
        for (uint8_t k = 0; k < 16; k++) {
            tile.region_type[i][k] = world_data->regions[tile.biome_index[mlt->at(i).at(k).biome_offset]]->type;
        }
    }

    //  Focus has to be at the world tile to get neighbor info
    //
    if (!tile.surveyed) {
        for (uint16_t i = 0; i < world->entities.all.size(); i++) {
            if (world->entities.all[i]->flags.bits.neighbor) {
                if (world->entities.all[i]->type == df::historical_entity_type::SiteGovernment) {
                    tile.necro_neighbors++;
                }
                else
                {
                    tile.neighbors.push_back(world->entities.all[i]->entity_raw->index);
                }
            }
        }
    }

    tile.surveyed = true;
}

//=================================================================================

df::coord2d embark_assist::survey::apply_offset(uint16_t x, uint16_t y, int8_t offset) {
    df::coord2d result;
    result.x = x;
    result.y = y;

    switch (offset) {
    case embark_assist::defs::offset_directions::Southwest:
        result.x--;
        result.y++;
        break;

    case embark_assist::defs::offset_directions::South:
        result.y++;
        break;

    case embark_assist::defs::offset_directions::Southeast:
        result.x++;
        result.y++;
        break;

    case embark_assist::defs::offset_directions::West:
        result.x--;
        break;

    case embark_assist::defs::offset_directions::Center:
        break;  // Center. No change

    case embark_assist::defs::offset_directions::East:
        result.x++;
        break;

    case embark_assist::defs::offset_directions::Northwest:
        result.x--;
        result.y--;
        break;

    case embark_assist::defs::offset_directions::North:
        result.y--;
        break;

    case embark_assist::defs::offset_directions::Northeast:
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
    adjust_coordinates(&effective_x, &effective_y, &effective_i, &effective_k);

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
    int8_t nw_region_type_level;
    int8_t n_region_type_level;
    int8_t w_region_type_level;
    int8_t home_region_type_level;

    if (corner_location == embark_assist::defs::directions::Center) {  //  We're the reference. No change.
    }
    else if (corner_location == embark_assist::defs::directions::East) {  //  Tile to the east is the reference
        effective_i = i + 1;
    }
    else if (corner_location == embark_assist::defs::directions::South) {  //  Tile to the south is the reference
        effective_k = k + 1;
    }
    else {  //  8, tile to the southeast is the reference.
        effective_i = i + 1;
        effective_k = k + 1;
    }

    adjust_coordinates(&effective_x, &effective_y, &effective_i, &effective_k);

    if (effective_x == world_data->world_width) {
        if (effective_y == world_data->world_height) {  //  Only the SE corner of the SE most tile of the world can reference this.
            return embark_assist::defs::directions::Center;
        }
        else {  //  East side corners of the east edge of the world
            if (effective_y == 0 && effective_k == 0) {  //  North edge of the world. Only one alternative exists.
                return embark_assist::defs::directions::Center;
            }

            nw_region_type = embark_assist::survey::region_type_of(survey_results, effective_x, effective_y, effective_i - 1, effective_k - 1);
            w_region_type = embark_assist::survey::region_type_of(survey_results, effective_x, effective_y, effective_i - 1, effective_k);

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

            if (nw_region_type_level < w_region_type_level) {
                return embark_assist::defs::directions::Center;
            }
            else if (nw_region_type_level > w_region_type_level) {
                return embark_assist::defs::directions::North;
            }

            //  Neither tile will automatically yield to the other
            if (corner_location == embark_assist::defs::directions::East) {
                return embark_assist::defs::directions::North;
            }
            else {  //  Can only be corner_location == embark_assist::defs::SE
                return embark_assist::defs::directions::Center;
            }
        }
    }
    else if (effective_y == world_data->world_height) {
        nw_region_type = embark_assist::survey::region_type_of(survey_results, effective_x, effective_y, effective_i - 1, effective_k - 1);
        n_region_type = embark_assist::survey::region_type_of(survey_results, effective_x, effective_y, effective_i, effective_k - 1);

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

        if (nw_region_type_level < n_region_type_level) {
            return embark_assist::defs::directions::Center;
        }
        else if (nw_region_type_level > n_region_type_level) {
            return embark_assist::defs::directions::East;
        }

        //  Neither tile will automatically yield to the other
        if (corner_location == embark_assist::defs::directions::South) {
            return embark_assist::defs::directions::Center;
        }
        else {  //  Can only be corner_location == embark_assist::defs::directions::Southeast
            return embark_assist::defs::directions::East;
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

    nw_region_type = embark_assist::survey::region_type_of(survey_results, effective_x, effective_y, effective_i - 1, effective_k - 1);
    n_region_type = embark_assist::survey::region_type_of(survey_results, effective_x, effective_y, effective_i, effective_k - 1);
    w_region_type = embark_assist::survey::region_type_of(survey_results, effective_x, effective_y, effective_i - 1, effective_k);
    home_region_type = embark_assist::survey::region_type_of(survey_results, effective_x, effective_y, effective_i, effective_k);

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
            return embark_assist::defs::directions::Center;  //  Only a single reference to this info, the own tile.
        }
        else {
            nw_region_type_level = -1;  //  Knock out the unreachable corners
            w_region_type_level = -1;
        }
    }

    if (effective_y == 0 && effective_k == 0) {  //  North edge of the world
        nw_region_type_level = -1;  //  Knock out the unreachable corners
        n_region_type_level = -1;

        if (corner_location == embark_assist::defs::directions::Center && effective_corner == embark_assist::defs::directions::North) { // The logic below would select the wrong alternative.
            effective_corner = embark_assist::defs::directions::West;
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
        if (y == 0 && k == 0) return embark_assist::defs::directions::Center; //  There's nothing to the north, so we fall back on our own tile.

        effective_edge = world_data->region_details[0]->edges.biome_x[i][k];
        south_region_type = embark_assist::survey::region_type_of(survey_results, x, y, i, k);
        north_region_type = embark_assist::survey::region_type_of(survey_results, x, y, i, k - 1);
    }
    else {
        if (k < 15) {  //  We're still within the same world tile
            effective_edge = world_data->region_details[0]->edges.biome_x[i][k + 1];
        }
        else {
            //  Getting the data from the world tile to the south
            if (y + 1 == world_data->world_height) {
                return embark_assist::defs::directions::Center;  //  There's nothing to the south, so we fall back on our own tile.
            }

            effective_edge = survey_results->at(x).at(y + 1).north_row_biome_x[i];
        }

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
            return embark_assist::defs::directions::North;
        }
        else {
            return embark_assist::defs::directions::Center;
        }
    }
    else {
        if (own_edge) {
            return embark_assist::defs::directions::Center;
        }
        else {
            return embark_assist::defs::directions::South;
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
        if (x == 0 && i == 0) return embark_assist::defs::directions::Center;  //  There's nothing to the west, so we fall back on our own tile.
        effective_edge = world_data->region_details[0]->edges.biome_y[i][k];
        east_region_type = embark_assist::survey::region_type_of(survey_results, x, y, i, k);
        west_region_type = embark_assist::survey::region_type_of(survey_results, x, y, i - 1, k);
    }
    else {
        if (i < 15) {  //  We're still within the same world tile
            effective_edge = world_data->region_details[0]->edges.biome_y[i + 1][k];
        }
        else {  //  Getting the data from the world tile to the east
            if (x + 1 == world_data->world_width) {
                return embark_assist::defs::directions::Center;  //  There's nothing to the east, so we fall back on our own tile.
            }

            effective_edge = survey_results->at(x + 1).at(y).west_column_biome_y[k];
        }
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
            return embark_assist::defs::directions::West;
        }
        else {
            return embark_assist::defs::directions::Center;
        }
    }
    else {
        if (own_edge) {
            return embark_assist::defs::directions::Center;
        }
        else {
            return embark_assist::defs::directions::East;
        }
    }
}

//=================================================================================

void embark_assist::survey::survey_region_sites(embark_assist::defs::site_lists *site_list) {
    //            color_ostream_proxy out(Core::getInstance().getConsole());
    auto screen = Gui::getViewscreenByType<df::viewscreen_choose_start_sitest>(0);
    df::world_data *world_data = world->world_data;

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
            switch (site->subtype_info->monument_type) {
            case df::monument_type::NONE:
            case df::monument_type::TOMB:
                break;  //  NONE shouldn't appear, and Tombs are visible already

            case df::monument_type::VAULT:
                site_list->push_back({ (uint8_t)site->rgn_min_x , (uint8_t)site->rgn_min_y, 'V' });
                break;

            default:
                site_list->push_back({ (uint8_t)site->rgn_min_x , (uint8_t)site->rgn_min_y, 'M' });  //  Any other Monument type. Pyramid?
                break;
            }
            break;

        case df::world_site_type::ImportantLocation:
            site_list->push_back({ (uint8_t)site->rgn_min_x , (uint8_t)site->rgn_min_y, 'i' });  //  Don't really know what that is...
            break;

        case df::world_site_type::LairShrine:
            switch (site->subtype_info->lair_type) {
            case df::lair_type::NONE:
                break;

            case df::lair_type::SIMPLE_MOUND:
                site_list->push_back({ (uint8_t)site->rgn_min_x , (uint8_t)site->rgn_min_y, 'm' });
                break;

            case df::lair_type::SIMPLE_BURROW:
                site_list->push_back({ (uint8_t)site->rgn_min_x , (uint8_t)site->rgn_min_y, 'b' });
                break;
            case df::lair_type::WILDERNESS_LOCATION:
                site_list->push_back({ (uint8_t)site->rgn_min_x , (uint8_t)site->rgn_min_y, 'l' });
                break;

            case df::lair_type::LABYRINTH:
                site_list->push_back({ (uint8_t)site->rgn_min_x , (uint8_t)site->rgn_min_y, 'L' });
                break;

            case df::lair_type::SHRINE:
                site_list->push_back({ (uint8_t)site->rgn_min_x , (uint8_t)site->rgn_min_y, 'S' });
                break;

            default:
                site_list->push_back({ (uint8_t)site->rgn_min_x , (uint8_t)site->rgn_min_y, '?' });  //  Can these exist?
                break;
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

    if (!use_cache) {  //  DF scrambles these values on world tile movements, while embark-tools stabilizes the movement, but its changes to the value are done after we've read them.
        state->local_min_x = screen->location.embark_pos_min.x;
        state->local_min_y = screen->location.embark_pos_min.y;
        state->local_max_x = screen->location.embark_pos_max.x;
        state->local_max_y = screen->location.embark_pos_max.y;
    }

    state->x = x;
    state->y = y;

    site_info->incursions_processed = true;
    site_info->aquifer = embark_assist::defs::Clear_Aquifer_Bits;
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
    site_info->neighbors.clear();

    for (uint8_t i = state->local_min_x; i <= state->local_max_x; i++) {
        for (uint8_t k = state->local_min_y; k <= state->local_max_y; k++) {
            site_info->aquifer |= mlt->at(i).at(k).aquifer;

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

            if (mlt->at(i).at(k).river_size != embark_assist::defs::river_sizes::None) {
                if (i < 15 &&
                    mlt->at(i + 1).at(k).river_size != embark_assist::defs::river_sizes::None &&
                    abs(mlt->at(i).at(k).river_elevation - mlt->at(i + 1).at(k).river_elevation) >
                    site_info->max_waterfall) {
                    site_info->max_waterfall =
                        abs(mlt->at(i).at(k).river_elevation - mlt->at(i + 1).at(k).river_elevation);
                }

                if (k < 15 &&
                    mlt->at(i).at(k + 1).river_size != embark_assist::defs::river_sizes::None &&
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
                embark_assist::defs::directions::Center,
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
                embark_assist::defs::directions::East,
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
                embark_assist::defs::directions::South,
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
                embark_assist::defs::directions::Southeast,
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
                embark_assist::defs::directions::Center,
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
            (translate_ew_edge(survey_results,
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
                embark_assist::defs::directions::South,
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
                embark_assist::defs::directions::East,
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
            (translate_ew_edge(survey_results,
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
                embark_assist::defs::directions::Southeast,
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

    for (uint16_t i = 0; i < survey_results->at(x).at(y).neighbors.size(); i++) {
        site_info->neighbors.push_back(survey_results->at(x).at(y).neighbors[i]);
    }

    site_info->necro_neighbors = survey_results->at(x).at(y).necro_neighbors;
}

//=================================================================================

void embark_assist::survey::shutdown() {
    delete state;
    state = nullptr;
}
