#include <vector>

#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>

#include <modules/Gui.h>
#include "modules/Materials.h"

#include "DataDefs.h"
#include "df/coord2d.h"
#include "df/creature_interaction_effect.h"
#include "df/creature_interaction_effect_display_symbolst.h"
#include "df/creature_interaction_effect_type.h"
#include "df/feature_init.h"
#include "df/inorganic_flags.h"
#include "df/inorganic_raw.h"
#include "df/interaction.h"
#include "df/interaction_instance.h"
#include "df/interaction_source.h"
#include "df/interaction_source_regionst.h"
#include "df/interaction_source_type.h"
#include "df/interaction_target.h"
#include "df/interaction_target_corpsest.h"
#include "df/interaction_target_materialst.h"
#include "df/material_common.h"
#include "df/reaction.h"
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
#include "df/world_river.h"
#include "df/world_site.h"
#include "df/world_site_type.h"
#include "df/world_underground_region.h"

#include "biome_type.h"
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
            auto reactions = world->raws.reactions;
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
//            color_ostream_proxy out(Core::getInstance().getConsole());
            df::world_data *world_data = world->world_data;

            for (uint16_t i = 0; i < world->interaction_instances.all.size(); i++) {
                auto interaction = world->raws.interactions[world->interaction_instances.all[i]->interaction_id];
                uint16_t region_index = world->interaction_instances.all[i]->region_index;
                bool thralling = false;
                bool reanimating = false;

                if (interaction->sources.size() &&
                    interaction->sources[0]->getType() == df::interaction_source_type::REGION) {
                    for (uint16_t k = 0; k < interaction->targets.size(); k++) {
                        if (interaction->targets[k]->getType() == 0) { //  Returns wrong type. Should be df::interaction_target_type::CORPSE
                            reanimating = true;
                        }
                        else if (interaction->targets[k]->getType() == 2) {//  Returns wrong type.. Should be df::interaction_target_type::MATERIAL
                            df::interaction_target_materialst* material = virtual_cast<df::interaction_target_materialst>(interaction->targets[k]);
                            if (material && DFHack::MaterialInfo(material->mat_type, material->mat_index).isInorganic()) {
                                for (uint16_t l = 0; l < world->raws.inorganics[material->mat_index]->material.syndrome.size(); l++) {
                                    for (uint16_t m = 0; m < world->raws.inorganics[material->mat_index]->material.syndrome[l]->ce.size(); m++) {
                                            if (world->raws.inorganics[material->mat_index]->material.syndrome[l]->ce[m]->getType() == df::creature_interaction_effect_type::FLASH_TILE) {
                                            //  Using this as a proxy. There seems to be a group of 4 effects for thralls:
                                            //  display symbol, flash symbol, phys att change and one more.
                                            thralling = true;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                for (uint16_t k = 0; k < world_data->regions[region_index]->region_coords.size(); k++) {
                    survey_results->at(world_data->regions[region_index]->region_coords[k].x).at(world_data->regions[region_index]->region_coords[k].y).evil_weather[5] = true;
                    survey_results->at(world_data->regions[region_index]->region_coords[k].x).at(world_data->regions[region_index]->region_coords[k].y).reanimating[5] = reanimating;
                    survey_results->at(world_data->regions[region_index]->region_coords[k].x).at(world_data->regions[region_index]->region_coords[k].y).thralling[5] = thralling;
                }
            }

            for (uint16_t i = 0; i < world->worldgen.worldgen_parms.dim_x; i++) {
                for (uint16_t k = 0; k < world->worldgen.worldgen_parms.dim_y; k++) {
                    survey_results->at(i).at(k).evil_weather_possible = false;
                    survey_results->at(i).at(k).reanimating_possible = false;
                    survey_results->at(i).at(k).thralling_possible = false;
                    survey_results->at(i).at(k).evil_weather_full = true;
                    survey_results->at(i).at(k).reanimating_full = true;
                    survey_results->at(i).at(k).thralling_full = true;

                    for (uint8_t l = 1; l < 10; l++) {
                        if (survey_results->at(i).at(k).biome_index[l] != -1) {
                            df::coord2d adjusted = apply_offset(i, k, l);
                            survey_results->at(i).at(k).evil_weather[l] = survey_results->at(adjusted.x).at(adjusted.y).evil_weather[5];
                            survey_results->at(i).at(k).reanimating[l] = survey_results->at(adjusted.x).at(adjusted.y).reanimating[5];
                            survey_results->at(i).at(k).thralling[l] = survey_results->at(adjusted.x).at(adjusted.y).thralling[5];

                            if (survey_results->at(i).at(k).evil_weather[l]) {
                                survey_results->at(i).at(k).evil_weather_possible = true;
                            }
                            else {
                                survey_results->at(i).at(k).evil_weather_full = false;
                            }

                            if (survey_results->at(i).at(k).reanimating[l]) {
                                survey_results->at(i).at(k).reanimating_possible = true;
                            }
                            else {
                                survey_results->at(i).at(k).reanimating_full = false;
                            }

                            if (survey_results->at(i).at(k).thralling[l]) {
                                survey_results->at(i).at(k).thralling_possible = true;
                            }
                            else {
                                survey_results->at(i).at(k).thralling_full = false;
                            }
                        }
                    }
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
    //            color_ostream_proxy out(Core::getInstance().getConsole());

    embark_assist::survey::geo_survey(geo_summary);
    for (uint16_t i = 0; i < world->worldgen.worldgen_parms.dim_x; i++) {
        for (uint16_t k = 0; k < world->worldgen.worldgen_parms.dim_y; k++) {
            df::coord2d adjusted;
            df::world_data *world_data = world->world_data;
            uint16_t geo_index;
            uint16_t sav_ev;
            uint8_t offset_count = 0;
            survey_results->at(i).at(k).surveyed = false;
            survey_results->at(i).at(k).aquifer_count = 0;
            survey_results->at(i).at(k).clay_count = 0;
            survey_results->at(i).at(k).sand_count = 0;
            survey_results->at(i).at(k).flux_count = 0;
            survey_results->at(i).at(k).min_region_soil = 10;
            survey_results->at(i).at(k).max_region_soil = 0;
            survey_results->at(i).at(k).waterfall = false;
            survey_results->at(i).at(k).savagery_count[0] = 0;
            survey_results->at(i).at(k).savagery_count[1] = 0;
            survey_results->at(i).at(k).savagery_count[2] = 0;
            survey_results->at(i).at(k).evilness_count[0] = 0;
            survey_results->at(i).at(k).evilness_count[1] = 0;
            survey_results->at(i).at(k).evilness_count[2] = 0;
            survey_results->at(i).at(k).metals.resize(state->max_inorganic);
            survey_results->at(i).at(k).economics.resize(state->max_inorganic);
            survey_results->at(i).at(k).minerals.resize(state->max_inorganic);
            //  Evil weather and rivers are handled in later operations. Should probably be merged into one.

            for (uint8_t l = 1; l < 10; l++)
            {
                adjusted = apply_offset(i, k, l);
                if (adjusted.x != i || adjusted.y != k || l == 5) {
                    offset_count++;

                    survey_results->at(i).at(k).biome_index[l] = world_data->region_map[adjusted.x][adjusted.y].region_id;
                    survey_results->at(i).at(k).biome[l] = get_biome_type(adjusted.x, adjusted.y, k);
                    geo_index = world_data->region_map[adjusted.x][adjusted.y].geo_index;

                    if (!geo_summary->at(geo_index).aquifer_absent) survey_results->at(i).at(k).aquifer_count++;
                    if (!geo_summary->at(geo_index).clay_absent) survey_results->at(i).at(k).clay_count++;
                    if (!geo_summary->at(geo_index).sand_absent) survey_results->at(i).at(k).sand_count++;
                    if (!geo_summary->at(geo_index).flux_absent) survey_results->at(i).at(k).flux_count++;

                    if (geo_summary->at(geo_index).soil_size < survey_results->at(i).at(k).min_region_soil)
                        survey_results->at(i).at(k).min_region_soil = geo_summary->at(geo_index).soil_size;

                    if (geo_summary->at(geo_index).soil_size > survey_results->at(i).at(k).max_region_soil)
                        survey_results->at(i).at(k).max_region_soil = geo_summary->at(geo_index).soil_size;

                    sav_ev = world_data->region_map[adjusted.x][adjusted.y].savagery / 33;
                    if (sav_ev == 3) sav_ev = 2;
                    survey_results->at(i).at(k).savagery_count[sav_ev]++;

                    sav_ev = world_data->region_map[adjusted.x][adjusted.y].evilness / 33;
                    if (sav_ev == 3) sav_ev = 2;
                    survey_results->at(i).at(k).evilness_count[sav_ev]++;

                    for (uint16_t m = 0; m < state->max_inorganic; m++) {
                        if (geo_summary->at(geo_index).possible_metals[m]) survey_results->at(i).at(k).metals[m] = true;
                        if (geo_summary->at(geo_index).possible_economics[m]) survey_results->at(i).at(k).economics[m] = true;
                        if (geo_summary->at(geo_index).possible_minerals[m]) survey_results->at(i).at(k).minerals[m] = true;
                    }
                }
                else {
                    survey_results->at(i).at(k).biome_index[l] = -1;
                    survey_results->at(i).at(k).biome[l] = -1;
                }
            }

            survey_results->at(i).at(k).biome_count = 0;
            for (uint8_t l = 1; l < 10; l++) {
                if (survey_results->at(i).at(k).biome[l] != -1) survey_results->at(i).at(k).biome_count++;
            }

            if (survey_results->at(i).at(k).aquifer_count == offset_count) survey_results->at(i).at(k).aquifer_count = 256;
            if (survey_results->at(i).at(k).clay_count == offset_count) survey_results->at(i).at(k).clay_count = 256;
            if (survey_results->at(i).at(k).sand_count == offset_count) survey_results->at(i).at(k).sand_count = 256;
            if (survey_results->at(i).at(k).flux_count == offset_count) survey_results->at(i).at(k).flux_count = 256;
            for (uint8_t l = 0; l < 3; l++) {
                if (survey_results->at(i).at(k).savagery_count[l] == offset_count) survey_results->at(i).at(k).savagery_count[l] = 256;
                if (survey_results->at(i).at(k).evilness_count[l] == offset_count) survey_results->at(i).at(k).evilness_count[l] = 256;
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
    //            color_ostream_proxy out(Core::getInstance().getConsole());
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
            };

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

            end_check_l = static_cast<uint16_t>(features.size());
            for (size_t l = 0; l < end_check_l; l++) {
                auto feature = features[l];

                if (feature->layer != -1 &&
                    feature->min_z != -30000) {
                    auto layer = world_data->underground_regions[feature->layer];

                    layer_bottom[layer->layer_depth] = feature->min_z;
                    layer_top[layer->layer_depth] = feature->max_z;
                    base_z = std::min((int)base_z, (int)feature->min_z);

                    if (layer->type == df::world_underground_region::MagmaSea) {
                        min_z = feature->min_z;  //  The features are individual per region tile
                        break;
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

            if (details->rivers_vertical.active[i][k] == 1) {
                mlt->at(i).at(k).river_present = true;
                mlt->at(i).at(k).river_elevation = details->rivers_vertical.elevation[i][k];
            }
            else if (details->rivers_horizontal.active[i][k] == 1) {
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
                    //  Comment copied from prospector.cpp(like the logic)...
                    //  This is to replicate the behavior of a probable bug in the
                    //   map generation code : if a layer is partially eroded, the
                    //   removed levels are in fact transferred to the layer below,
                    //  because unlike the case of removing the whole layer, the code
                    // does not execute a loop to shift the lower part of the stack up.
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

    survey_results->at(x).at(y).aquifer_count = 0;
    survey_results->at(x).at(y).clay_count = 0;
    survey_results->at(x).at(y).sand_count = 0;
    survey_results->at(x).at(y).flux_count = 0;
    survey_results->at(x).at(y).min_region_soil = 10;
    survey_results->at(x).at(y).max_region_soil = 0;
    survey_results->at(x).at(y).savagery_count[0] = 0;
    survey_results->at(x).at(y).savagery_count[1] = 0;
    survey_results->at(x).at(y).savagery_count[2] = 0;
    survey_results->at(x).at(y).evilness_count[0] = 0;
    survey_results->at(x).at(y).evilness_count[1] = 0;
    survey_results->at(x).at(y).evilness_count[2] = 0;

    bool river_elevation_found = false;
    int16_t river_elevation;

    for (uint8_t i = 0; i < 16; i++) {
        for (uint8_t k = 0; k < 16; k++) {
            if (mlt->at(i).at(k).aquifer) { survey_results->at(x).at(y).aquifer_count++; }
            if (mlt->at(i).at(k).clay) { survey_results->at(x).at(y).clay_count++; }
            if (mlt->at(i).at(k).sand) { survey_results->at(x).at(y).sand_count++; }
            if (mlt->at(i).at(k).flux) { survey_results->at(x).at(y).flux_count++; }

            if (mlt->at(i).at(k).soil_depth < survey_results->at(x).at(y).min_region_soil) {
                survey_results->at(x).at(y).min_region_soil = mlt->at(i).at(k).soil_depth;
            }

            if (mlt->at(i).at(k).soil_depth > survey_results->at(x).at(y).max_region_soil) {
                survey_results->at(x).at(y).max_region_soil = mlt->at(i).at(k).soil_depth;
            }

            if (mlt->at(i).at(k).river_present) {
                if (river_elevation_found) {
                    if (mlt->at(i).at(k).river_elevation != river_elevation)
                    {
                        survey_results->at(x).at(y).waterfall = true;
                    }
                }
                else {
                    river_elevation_found = true;
                    river_elevation = mlt->at(i).at(k).river_elevation;
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
    embark_assist::defs::site_infos *site_info,
    bool use_cache) {

    //            color_ostream_proxy out(Core::getInstance().getConsole());
    auto screen = Gui::getViewscreenByType<df::viewscreen_choose_start_sitest>(0);
    int16_t elevation;
    uint16_t x = screen->location.region_pos.x;
    uint16_t y = screen->location.region_pos.y;
    bool river_found = false;
    int16_t river_elevation;
    std::vector<bool> metals(state->max_inorganic);
    std::vector<bool> economics(state->max_inorganic);
    std::vector<bool> minerals(state->max_inorganic);

    if (!use_cache) {  //  For some reason DF scrambles these values on world tile movements (at least in Lua...).
        state->local_min_x = screen->location.embark_pos_min.x;
        state->local_min_y = screen->location.embark_pos_min.y;
        state->local_max_x = screen->location.embark_pos_max.x;
        state->local_max_y = screen->location.embark_pos_max.y;
    }

    state->x = x;
    state->y = y;

    site_info->aquifer = false;
    site_info->aquifer_full = true;
    site_info->min_soil = 10;
    site_info->max_soil = 0;
    site_info->flat = true;
    site_info->waterfall = false;
    site_info->clay = false;
    site_info->sand = false;
    site_info->flux = false;
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
                if (river_found) {
                    if (river_elevation != mlt->at(i).at(k).river_elevation) {
                        site_info->waterfall = true;
                    }
                }
                else {
                    river_elevation = mlt->at(i).at(k).river_elevation;
                    river_found = true;
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
}

//=================================================================================

void embark_assist::survey::shutdown() {
    delete state;
}

