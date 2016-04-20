/*
  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

// You can always find the latest version of this plugin in Github
// https://github.com/ragundo/exportmaps


#ifndef DFHACK_H
#define DFHACK_H

#include <cstdint>
#include <string>
#include <fstream>
#include <iostream>

#define DFHACK_WANT_MISCUTILS
#include <google/protobuf/stubs/common.h>   // For int16_t etc

#include <Core.h>
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <VersionInfo.h>

#include <modules/Gui.h>
#include <modules/Translation.h>
#include <modules/Units.h>


#include <df/biome_type.h>
#include <df/building_design.h>
#include <df/building_actual.h>
#include <df/building_animaltrapst.h>
#include <df/building_archerytargetst.h>
#include <df/building_armorstandst.h>
#include <df/building_axle_horizontalst.h>
#include <df/building_axle_verticalst.h>
#include <df/building_bars_floorst.h>
#include <df/building_bars_verticalst.h>
#include <df/building_bedst.h>
#include <df/building_boxst.h>
#include <df/building_bridgest.h>
#include <df/building_cabinetst.h>
#include <df/building_cagest.h>
#include <df/building_chainst.h>
#include <df/building_chairst.h>
#include <df/building_civzonest.h>
#include <df/building_coffinst.h>
#include <df/building_constructionst.h>
#include <df/building_def_furnacest.h>
#include <df/building_def.h>
#include <df/building_def_item.h>
#include <df/building_def_workshopst.h>
#include <df/building_design.h>
#include <df/building_doorst.h>
#include <df/building_drawbuffer.h>
#include <df/building_extents.h>
#include <df/building_farmplotst.h>
#include <df/building_flags.h>
#include <df/building_floodgatest.h>
#include <df/building_furnacest.h>
#include <df/building_gear_assemblyst.h>
#include <df/building_grate_floorst.h>
#include <df/building_grate_wallst.h>
#include <df/building_hatchst.h>
#include <df/building_hivest.h>
#include <df/building_nest_boxst.h>
#include <df/building_nestst.h>
#include <df/building_road_dirtst.h>
#include <df/building_road_pavedst.h>
#include <df/building_roadst.h>
#include <df/building_rollersst.h>
#include <df/building_screw_pumpst.h>
#include <df/building_shopst.h>
#include <df/building_siegeenginest.h>
#include <df/building_slabst.h>
#include <df/building_squad_use.h>
#include <df/building_statuest.h>
#include <df/building_stockpilest.h>
#include <df/building_supportst.h>
#include <df/building_tablest.h>
#include <df/building_traction_benchst.h>
#include <df/building_tradedepotst.h>
#include <df/building_trapst.h>
#include <df/building_type.h>
#include <df/building_users.h>
#include <df/building_wagonst.h>
#include <df/building_water_wheelst.h>
#include <df/building_weaponrackst.h>
#include <df/building_weaponst.h>
#include <df/building_wellst.h>
#include <df/building_windmillst.h>
#include <df/building_window_gemst.h>
#include <df/building_window_glassst.h>
#include <df/building_windowst.h>
#include <df/building_workshopst.h>

#include <df/campfire.h>
#include <df/construction.h>
#include <df/coord2d.h>
#include <df/coord.h>
#include <df/creature_raw.h>

#include <df/deep_vein_hollow.h>

#include <df/engraving.h>
#include <df/entity_position.h>
#include <df/entity_position_assignment.h>
#include <df/entity_entity_link.h>
#include <df/entity_site_link.h>

#include <df/feature.h>
#include <df/feature_alteration.h>
#include <df/feature_alteration_new_lava_fill_zst.h>
#include <df/feature_alteration_new_pop_maxst.h>
#include <df/feature_cavest.h>
#include <df/feature_deep_special_tubest.h>
#include <df/feature_deep_surface_portalst.h>
#include <df/feature_init.h>
#include <df/feature_init_cavest.h>
#include <df/feature_magma_core_from_layerst.h>
#include <df/feature_magma_poolst.h>
#include <df/feature_outdoor_riverst.h>
#include <df/feature_pitst.h>
#include <df/feature_subterranean_from_layerst.h>
#include <df/feature_type.h>
#include <df/feature_underworld_from_layerst.h>
#include <df/feature_volcanost.h>
#include <df/feature_init_deep_special_tubest.h>
#include <df/feature_init_deep_surface_portalst.h>
#include <df/feature_init_magma_core_from_layerst.h>
#include <df/feature_init_magma_poolst.h>
#include <df/feature_init_outdoor_riverst.h>
#include <df/feature_init_pitst.h>
#include <df/feature_init_subterranean_from_layerst.h>
#include <df/feature_init_underworld_from_layerst.h>
#include <df/feature_init_volcanost.h>

#include <df/general_ref.h>
#include <df/geo_layer_type.h>
#include <df/glowing_barrier.h>

#include <df/historical_entity.h>
#include <df/history_event.h>
#include <df/history_event_collection.h>

#include <df/inclusion_type.h>
#include <df/item_actual.h>
#include <df/item_ammost.h>
#include <df/item_amuletst.h>
#include <df/item_animaltrapst.h>
#include <df/item_anvilst.h>
#include <df/item_armorstandst.h>
#include <df/item_armorst.h>
#include <df/item_backpackst.h>
#include <df/item_ballistaarrowheadst.h>
#include <df/item_ballistapartsst.h>
#include <df/item_barrelst.h>
#include <df/item_barst.h>
#include <df/item_bedst.h>
#include <df/item_binst.h>
#include <df/item_blocksst.h>
#include <df/item_body_component.h>
#include <df/item_bookst.h>
#include <df/item_boulderst.h>
#include <df/item_boxst.h>
#include <df/item_braceletst.h>
#include <df/item_bucketst.h>
#include <df/item_cabinetst.h>
#include <df/item_cagest.h>
#include <df/item_catapultpartsst.h>
#include <df/item_chainst.h>
#include <df/item_chairst.h>
#include <df/item_cheesest.h>
#include <df/item_clothst.h>
#include <df/item_coffinst.h>
#include <df/item_coinst.h>
#include <df/item_constructed.h>
#include <df/item_corpsepiecest.h>
#include <df/item_corpsest.h>
#include <df/item_crafted.h>
#include <df/item_critter.h>
#include <df/item_crownst.h>
#include <df/item_crutchst.h>
#include <df/itemdef_ammost.h>
#include <df/itemdef_armorst.h>
#include <df/itemdef_foodst.h>
#include <df/itemdef_glovesst.h>
#include <df/itemdef.h>
#include <df/itemdef_helmst.h>
#include <df/itemdef_instrumentst.h>
#include <df/itemdef_pantsst.h>
#include <df/itemdef_shieldst.h>
#include <df/itemdef_shoesst.h>
#include <df/itemdef_siegeammost.h>
#include <df/itemdef_toolst.h>
#include <df/itemdef_toyst.h>
#include <df/itemdef_trapcompst.h>
#include <df/itemdef_weaponst.h>
#include <df/item_doorst.h>
#include <df/item_drinkst.h>
#include <df/item_earringst.h>
#include <df/item_eggst.h>
#include <df/item_figurinest.h>
#include <df/item_filter_spec.h>
#include <df/item_fish_rawst.h>
#include <df/item_fishst.h>
#include <df/item_flags2.h>
#include <df/item_flags.h>
#include <df/item_flaskst.h>
#include <df/item_floodgatest.h>
#include <df/item_foodst.h>
#include <df/item_gemst.h>
#include <df/item_globst.h>
#include <df/item_glovesst.h>
#include <df/item_gobletst.h>
#include <df/item_gratest.h>
#include <df/item.h>
#include <df/item_hatch_coverst.h>
#include <df/item_helmst.h>
#include <df/item_history_info.h>
#include <df/itemimprovement_art_imagest.h>
#include <df/itemimprovement_bandsst.h>
#include <df/itemimprovement_clothst.h>
#include <df/itemimprovement_coveredst.h>
#include <df/itemimprovement.h>
#include <df/itemimprovement_illustrationst.h>
#include <df/itemimprovement_itemspecificst.h>
#include <df/itemimprovement_pagesst.h>
#include <df/itemimprovement_rings_hangingst.h>
#include <df/itemimprovement_sewn_imagest.h>
#include <df/itemimprovement_spikesst.h>
#include <df/itemimprovement_threadst.h>
#include <df/item_instrumentst.h>
#include <df/item_kill_info.h>
#include <df/item_liquid.h>
#include <df/item_liquid_miscst.h>
#include <df/item_liquipowder.h>
#include <df/item_magicness.h>
#include <df/item_magicness_type.h>
#include <df/item_matstate.h>
#include <df/item_meatst.h>
#include <df/item_millstonest.h>
#include <df/item_orthopedic_castst.h>
#include <df/item_pantsst.h>
#include <df/item_petst.h>
#include <df/item_pipe_sectionst.h>
#include <df/item_plant_growthst.h>
#include <df/item_plantst.h>
#include <df/item_powder.h>
#include <df/item_powder_miscst.h>
#include <df/item_quality.h>
#include <df/item_quernst.h>
#include <df/item_quiverst.h>
#include <df/item_remainsst.h>
#include <df/item_ringst.h>
#include <df/item_rockst.h>
#include <df/item_roughst.h>
#include <df/item_scepterst.h>
#include <df/item_seedsst.h>
#include <df/item_shieldst.h>
#include <df/item_shoesst.h>
#include <df/item_siegeammost.h>
#include <df/item_skin_tannedst.h>
#include <df/item_slabst.h>
#include <df/item_smallgemst.h>
#include <df/items_other_id.h>
#include <df/item_splintst.h>
#include <df/item_statuest.h>
#include <df/item_stockpile_ref.h>
#include <df/item_tablest.h>
#include <df/item_threadst.h>
#include <df/item_toolst.h>
#include <df/item_totemst.h>
#include <df/item_toyst.h>
#include <df/item_traction_benchst.h>
#include <df/item_trapcompst.h>
#include <df/item_trappartsst.h>
#include <df/item_type.h>
#include <df/item_verminst.h>
#include <df/item_weaponrackst.h>
#include <df/item_weaponst.h>
#include <df/item_windowst.h>
#include <df/item_woodst.h>

#include <df/job.h>
#include <df/job_item.h>
#include <df/job_item_ref.h>
#include <df/job_list_link.h>

#include <df/language_name.h>
#include <df/local_population.h>

#include <df/map_block.h>

#include <df/region_map_entry.h>

#include <df/site_realization_building.h>
#include <df/site_realization_crossroads.h>
#include <df/specific_ref.h>

#include <df/ui.h>
#include <df/unit.h>
#include <df/unit_item_wrestle.h>
#include <df/unit_labor.h>
#include <df/unit_skill.h>
#include <df/unit_soul.h>

#include <df/vermin.h>

#include <df/viewscreen.h>
#include <df/viewscreen_adventure_logst.h>
#include <df/viewscreen_announcelistst.h>
#include <df/viewscreen_barterst.h>
#include <df/viewscreen_buildinglistst.h>
#include <df/viewscreen_buildingst.h>
#include <df/viewscreen_choose_start_sitest.h>
#include <df/viewscreen_civlistst.h>
#include <df/viewscreen_conversationst.h>
#include <df/viewscreen_createquotast.h>
#include <df/viewscreen_customize_unitst.h>
#include <df/viewscreen_dungeonmodest.h>
#include <df/viewscreen_dungeon_monsterstatusst.h>
#include <df/viewscreen_dungeon_wrestlest.h>
#include <df/viewscreen_dwarfmodest.h>
#include <df/viewscreen_entityst.h>
#include <df/viewscreen_export_graphical_mapst.h>
#include <df/viewscreen_export_regionst.h>
#include <df/viewscreen_game_cleanerst.h>
#include <df/viewscreen_itemst.h>
#include <df/viewscreen_joblistst.h>
#include <df/viewscreen_jobmanagementst.h>
#include <df/viewscreen_jobst.h>
#include <df/viewscreen_justicest.h>
#include <df/viewscreen_kitchenprefst.h>
#include <df/viewscreen_layer.h>
#include <df/viewscreen_layer_arena_creaturest.h>
#include <df/viewscreen_layer_assigntradest.h>
#include <df/viewscreen_layer_choose_language_namest.h>
#include <df/viewscreen_layer_currencyst.h>
#include <df/viewscreen_layer_export_play_mapst.h>
#include <df/viewscreen_layer_militaryst.h>
#include <df/viewscreen_layer_musicsoundst.h>
#include <df/viewscreen_layer_noblelistst.h>
#include <df/viewscreen_layer_overall_healthst.h>
#include <df/viewscreen_layer_reactionst.h>
#include <df/viewscreen_layer_squad_schedulest.h>
#include <df/viewscreen_layer_stockpilest.h>
#include <df/viewscreen_layer_stone_restrictionst.h>
#include <df/viewscreen_layer_unit_actionst.h>
#include <df/viewscreen_layer_unit_healthst.h>
#include <df/viewscreen_layer_unit_relationshipst.h>
#include <df/viewscreen_layer_workshop_profilest.h>
#include <df/viewscreen_layer_world_gen_param_presetst.h>
#include <df/viewscreen_layer_world_gen_paramst.h>
#include <df/viewscreen_legendsst.h>
#include <df/viewscreen_loadgamest.h>
#include <df/viewscreen_meetingst.h>
#include <df/viewscreen_movieplayerst.h>
#include <df/viewscreen_new_regionst.h>
#include <df/viewscreen_noblest.h>
#include <df/viewscreen_optionst.h>
#include <df/viewscreen_overallstatusst.h>
#include <df/viewscreen_petst.h>
#include <df/viewscreen_pricest.h>
#include <df/viewscreen_reportlistst.h>
#include <df/viewscreen_requestagreementst.h>
#include <df/viewscreen_savegamest.h>
#include <df/viewscreen_selectitemst.h>
#include <df/viewscreen_setupadventurest.h>
#include <df/viewscreen_setupdwarfgamest.h>
#include <df/viewscreen_storesst.h>
#include <df/viewscreen_textviewerst.h>
#include <df/viewscreen_titlest.h>
#include <df/viewscreen_topicmeeting_fill_land_holder_positionsst.h>
#include <df/viewscreen_topicmeetingst.h>
#include <df/viewscreen_topicmeeting_takerequestsst.h>
#include <df/viewscreen_tradeagreementst.h>
#include <df/viewscreen_tradegoodsst.h>
#include <df/viewscreen_tradelistst.h>
#include <df/viewscreen_treasurelistst.h>
#include <df/viewscreen_unitlist_page.h>
#include <df/viewscreen_unitlistst.h>
#include <df/viewscreen_unitst.h>
#include <df/viewscreen_update_regionst.h>
#include <df/viewscreen_wagesst.h>

#include <df/world.h>
#include <df/world_construction.h>
#include <df/world_construction_bridgest.h>
#include <df/world_construction_roadst.h>
#include <df/world_construction_tunnelst.h>
#include <df/world_construction_wallst.h>
#include <df/world_construction_square.h>
#include <df/world_construction_square_bridgest.h>
#include <df/world_construction_square_roadst.h>
#include <df/world_construction_square_tunnelst.h>
#include <df/world_construction_square_wallst.h>
#include <df/world_data.h>
#include <df/world_geo_biome.h>
#include <df/world_geo_layer.h>
#include <df/world_population.h>
#include <df/world_population_type.h>
#include <df/world_population_ref.h>
#include <df/world_region.h>
#include <df/world_region_details.h>
#include <df/world_region_feature.h>
#include <df/world_river.h>
#include <df/world_site.h>
#include <df/world_site_inhabitant.h>
#include <df/world_site_realization.h>
#include <df/world_underground_region.h>
#include <df/world_unk_20.h>

#endif
