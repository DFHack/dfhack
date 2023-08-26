import re
import xml.etree.ElementTree as ET
from collections.abc import Iterable
from dataclasses import dataclass
from pathlib import Path

PATH_XML = "./library/xml"
PATH_OUTPUT = "./types/library/"
PATH_LIB_CONFIG = "./types/config.json"
PATH_CONFIG = ".luarc.json"

ALIAS = "---@meta\n\n---@alias bitfield integer\n"
LIB_CONFIG = """{
  "name": "DFHack Lua",
  "words": [
      "dfhack"
  ]
}\n"""

CONFIG = f"""{{
  "workspace.library": ["{PATH_OUTPUT}"],
  "workspace.ignoreDir": ["build"]
}}\n"""

DF_GLOBAL = [
    "activity_next_id",
    "adventure",
    "agreement_next_id",
    "army_controller_next_id",
    "army_next_id",
    "army_tracking_info_next_id",
    "art_image_chunk_next_id",
    "artifact_next_id",
    "belief_system_next_id",
    "building_next_id",
    "buildreq",
    "created_item_count",
    "created_item_matindex",
    "created_item_mattype",
    "created_item_subtype",
    "created_item_type",
    "crime_next_id",
    "cultural_identity_next_id",
    "cur_rain",
    "cur_rain_counter",
    "cur_season",
    "cur_season_tick",
    "cur_snow",
    "cur_snow_counter",
    "cur_year",
    "cur_year_tick",
    "cur_year_tick_advmode",
    "current_weather",
    "cursor",
    "d_init",
    "dance_form_next_id",
    "debug_combat",
    "debug_fastmining",
    "debug_noberserk",
    "debug_nodrink",
    "debug_noeat",
    "debug_nomoods",
    "debug_nopause",
    "debug_nosleep",
    "debug_showambush",
    "debug_turbospeed",
    "debug_wildlife",
    "divination_set_next_id",
    "enabler",
    "entity_next_id",
    "flow_guide_next_id",
    "flows",
    "formation_next_id",
    "game",
    "gamemode",
    "gametype",
    "gps",
    "gview",
    "handleannounce",
    "hist_event_collection_next_id",
    "hist_event_next_id",
    "hist_figure_next_id",
    "identity_next_id",
    "image_set_next_id",
    "incident_next_id",
    "init",
    "interactinvslot",
    "interaction_instance_next_id",
    "interactitem",
    "item_next_id",
    "job_next_id",
    "jobvalue",
    "jobvalue_setter",
    "machine_next_id",
    "map_renderer",
    "min_load_version",
    "movie_version",
    "musical_form_next_id",
    "nemesis_next_id",
    "occupation_next_id",
    "pause_state",
    "plotinfo",
    "poetic_form_next_id",
    "preserveannounce",
    "process_dig",
    "process_jobs",
    "proj_next_id",
    "rhythm_next_id",
    "save_on_exit",
    "scale_next_id",
    "schedule_next_id",
    "selection_rect",
    "soul_next_id",
    "squad_next_id",
    "standing_orders_auto_butcher",
    "standing_orders_auto_collect_webs",
    "standing_orders_auto_fishery",
    "standing_orders_auto_kiln",
    "standing_orders_auto_kitchen",
    "standing_orders_auto_loom",
    "standing_orders_auto_other",
    "standing_orders_auto_slaughter",
    "standing_orders_auto_smelter",
    "standing_orders_auto_tan",
    "standing_orders_dump_bones",
    "standing_orders_dump_corpses",
    "standing_orders_dump_hair",
    "standing_orders_dump_other",
    "standing_orders_dump_shells",
    "standing_orders_dump_skins",
    "standing_orders_dump_skulls",
    "standing_orders_farmer_harvest",
    "standing_orders_forbid_other_dead_items",
    "standing_orders_forbid_other_nohunt",
    "standing_orders_forbid_own_dead",
    "standing_orders_forbid_own_dead_items",
    "standing_orders_forbid_used_ammo",
    "standing_orders_gather_animals",
    "standing_orders_gather_bodies",
    "standing_orders_gather_food",
    "standing_orders_gather_furniture",
    "standing_orders_gather_minerals",
    "standing_orders_gather_refuse",
    "standing_orders_gather_refuse_outside",
    "standing_orders_gather_vermin_remains",
    "standing_orders_gather_wood",
    "standing_orders_job_cancel_announce",
    "standing_orders_mix_food",
    "standing_orders_use_dyed_cloth",
    "standing_orders_zoneonly_drink",
    "standing_orders_zoneonly_fish",
    "task_next_id",
    "texture",
    "timed_events",
    "ui_building_assign_is_marked",
    "ui_building_assign_items",
    "ui_building_assign_type",
    "ui_building_assign_units",
    "ui_building_in_assign",
    "ui_building_in_resize",
    "ui_building_item_cursor",
    "ui_building_resize_radius",
    "ui_lever_target_type",
    "ui_look_cursor",
    "ui_look_list",
    "ui_menu_width",
    "ui_selected_unit",
    "ui_unit_view_mode",
    "ui_workshop_in_add",
    "ui_workshop_job_cursor",
    "unit_chunk_next_id",
    "unit_next_id",
    "updatelightstate",
    "vehicle_next_id",
    "version",
    "weathertimer",
    "window_x",
    "window_y",
    "window_z",
    "world",
    "written_content_next_id",
]

DF = [
    "itemimprovement_coveredst",
    "history_event_hf_preachst",
    "spoils_report",
    "itemdef_weaponst",
    "viewscreen",
    "creation_zone_pwg_alterationst",
    "interaction_effect_contactst",
    "history_event_agreements_voidedst",
    "proj_itemst",
    "ui_sidebar_mode",
    "itemdef_pantsst",
    "historical_figure_relationships",
    "history_event_entity_searched_sitest",
    "building_farmplotst",
    "reaction_flags",
    "embark_feature",
    "item_shieldst",
    "history_event_artifact_storedst",
    "resource_allotment_specifier_clothing_glovesst",
    "entity_occasion_schedule_feature",
    "gait_info",
    "activity_event_encounterst",
    "history_event_collection_theftst",
    "knowledge_scholar_flags_2",
    "written_content_type",
    "poetic_form_action",
    "tuning_type",
    "history_event_hf_razed_buildingst",
    "armor_properties",
    "unit_coin_debt",
    "creature_handler",
    "histfig_entity_link_former_slavest",
    "adventure_movement_release_hold_tilest",
    "item_rockst",
    "block_square_event_grassst",
    "activity_event_conflictst",
    "squad_schedule_order",
    "activity_entry",
    "talk_choice",
    "item",
    "body_layer_status",
    "breath_attack_type",
    "site_type",
    "identity",
    "caste_body_info",
    "room_rent_info",
    "item_chainst",
    "building_shopst",
    "markup_text_boxst",
    "resource_allotment_specifier_clothst",
    "script_var_unitst",
    "hive_flags",
    "history_event_entity_rampaged_in_sitest",
    "interaction_effect_change_weatherst",
    "squad_schedule_context_type",
    "language_name_type",
    "history_event_hist_figure_abductedst",
    "history_event_entity_razed_buildingst",
    "unit_action_data_push_object",
    "interaction_target_locationst",
    "script_step_topicdiscussionst",
    "item_trapcompst",
    "fmod_sound",
    "resource_allotment_specifier_clothing_bodyst",
    "histfig_entity_link_slavest",
    "equipment_update",
    "unit_genes",
    "general_ref_artifact",
    "history_event_war_destroyed_sitest",
    "history_event_artifact_claim_formedst",
    "block_flags",
    "history_event_hfs_formed_intrigue_relationshipst",
    "resource_allotment_specifier_armor_pantsst",
    "intrigue_corruption",
    "entity_tissue_style",
    "creature_interaction_effect_change_personalityst",
    "mental_picture_property_emotionst",
    "building_road_dirtst",
    "job_type",
    "activity_event_skill_demonstrationst",
    "resource_allotment_specifier_toothst",
    "global_table_entry",
    "general_ref_musical_formst",
    "item_critter",
    "coord_rect",
    "item_boulderst",
    "active_script_var_longst",
    "buildings_mode_type",
    "town_labor_type",
    "building_gear_assemblyst",
    "body_size_info",
    "general_ref_building_civzone_assignedst",
    "knowledge_scholar_flags_8",
    "item_figurinest",
    "feature_outdoor_riverst",
    "item_tablest",
    "abstract_building_dungeonst",
    "histfig_site_link_prison_abstract_buildingst",
    "organization_entry_nodest",
    "general_ref_interactionst",
    "widget_button",
    "building_window_glassst",
    "creature_interaction_effect_bp_appearance_modifierst",
    "interaction_effect_change_item_qualityst",
    "feature_init",
    "tile_bitmask",
    "building_grate_wallst",
    "world_site_flags",
    "renderer",
    "performance_event_type",
    "worldgen_parms",
    "block_square_event_mineralst",
    "squad_order_raid_sitest",
    "general_ref_entity_art_image",
    "feature_cavest",
    "resource_allotment_specifier_pearlst",
    "general_ref_unit_infantst",
    "army_controller_sub11",
    "dye_info",
    "item_bagst",
    "item_binst",
    "view_sheet_unit_knowledge_type",
    "large_integer",
    "job_item",
    "army_controller_sub18",
    "item_petst",
    "itemimprovement_bandsst",
    "environment_type",
    "unit_action_data_dodge",
    "resource_allotment_specifier_type",
    "history_event_collection_entity_overthrownst",
    "history_event_war_site_tribute_forcedst",
    "itemimprovement_itemspecificst",
    "building_supportst",
    "flow_guide_trailing_flowst",
    "adv_background_option_type",
    "buildings_other_id",
    "item_barst",
    "history_event_site_retiredst",
    "history_event_topicagreement_madest",
    "history_event_hf_ransomedst",
    "history_event_performancest",
    "world_construction_square_tunnelst",
    "world_underground_region",
    "history_event_war_site_taken_overst",
    "historical_kills",
    "viewscreen_export_regionst",
    "toy_flags",
    "script_varst",
    "manager_order_condition_order",
    "art_image_element",
    "uniform_category",
    "history_event_hist_figure_simple_battle_eventst",
    "ethic_type",
    "feature_init_subterranean_from_layerst",
    "resource_allotment_specifier_leatherst",
    "itemimprovement_threadst",
    "item_globst",
    "general_ref_value_levelst",
    "abstract_building_counting_housest",
    "squad_order_retrieve_artifactst",
    "construction_category_type",
    "unit_attribute",
    "viewscreen_setupdwarfgamest",
    "d_init_flags4",
    "history_event_entity_lawst",
    "mental_picture_propertyst",
    "history_event_add_hf_entity_honorst",
    "breed",
    "histfig_hf_link_pet_ownerst",
    "history_event_masterpiece_created_engravingst",
    "view_sheets_context_type",
    "texture_handlerst",
    "feature_init_deep_surface_portalst",
    "dance_form_group_size",
    "tissue_style_type",
    "barrack_preference_category",
    "itemdef_trapcompst",
    "map_block_column",
    "history_event_body_abusedst",
    "hospital_supplies",
    "world_region",
    "item_weaponrackst",
    "item_kill_info",
    "caste_attack",
    "history_event_add_entity_site_profile_flagst",
    "entity_activity_statistics",
    "schedule_slot",
    "item_ballistaarrowheadst",
    "creature_interaction_effect_unconsciousnessst",
    "agreement_details_data_citizenship",
    "widget",
    "map_renderer",
    "divination_set_roll",
    "curse_attr_change",
    "spatter_common",
    "unit_action_data_move",
    "creature_interaction_effect_bleedingst",
    "block_square_event",
    "history_event_site_diedst",
    "property_ownership",
    "history_event_remove_hf_site_linkst",
    "army_controller_quest",
    "history_event_masterpiece_lostst",
    "actor_entryst",
    "general_ref_dance_formst",
    "dance_form_configuration",
    "standing_orders_category_type",
    "site_realization_building_info_castle_wallst",
    "squad_selector_context_type",
    "art_image_property",
    "army_controller",
    "stockpile_list",
    "army_controller_sub7",
    "matgloss_list",
    "interaction",
    "abstract_building_marketst",
    "history_event_hf_act_on_buildingst",
    "item_boxst",
    "itemimprovement_sewn_imagest",
    "item_bookst",
    "musical_form_purpose",
    "job_item_ref",
    "activity_event_store_objectst",
    "dipscript_info",
    "interrogation_report",
    "resource_allotment_specifier_anvilst",
    "histfig_entity_link_former_mercenaryst",
    "history_event_change_hf_body_statest",
    "entity_position_flags",
    "script_step_discussst",
    "musical_form_passage_length_type",
    "item_plantst",
    "building_instrumentst",
    "ui_hotkey",
    "site_realization_building_info_castle_courtyardst",
    "interface_key",
    "cwo_buildingst",
    "histfig_body_state",
    "mission_campaign_report",
    "body_detail_plan",
    "army_controller_invasion_order",
    "world_raws",
    "interaction_target_corpsest",
    "item_totemst",
    "army_controller_sub16",
    "moving_party",
    "identity_unk_95",
    "unit_parley",
    "interaction_effect_resurrectst",
    "history_event_tradest",
    "squad_order_defend_burrowsst",
    "mental_picture_element_hfst",
    "world_gen_param_charst",
    "whereabouts_type",
    "viewscreen_savegamest",
    "punishment",
    "buildreq",
    "adventurest",
    "mental_picture_element_sitest",
    "history_event_hf_freedst",
    "helm_flags",
    "witness_report",
    "entity_position_raw_flags",
    "item_corpsepiecest",
    "unit_context_block",
    "crime_witness",
    "language_word_table",
    "knowledge_scholar_category_flag",
    "entity_buy_requests",
    "flow_guide_item_cloudst",
    "resource_allotment_specifier_meatst",
    "itemdef_toyst",
    "job",
    "reaction_product",
    "temperaturest",
    "init_input",
    "item_flaskst",
    "incident_data_writing",
    "embark_location",
    "history_event_remove_hf_entity_linkst",
    "history_event_hfs_formed_reputation_relationshipst",
    "activity_info",
    "incident",
    "mental_picture_property_datest",
    "reaction_product_type",
    "world_site",
    "item_traction_benchst",
    "unit_demand",
    "general_ref_unit_milkeest",
    "renderer_2d_base",
    "reaction_reagent_type",
    "history_event_assume_identityst",
    "options_context_type",
    "identity_unk_94",
    "resource_allotment_specifier_hornst",
    "history_event_agreement_formedst",
    "squad_order_kill_hfst",
    "general_ref_contained_in_itemst",
    "itemdef_foodst",
    "building_extents",
    "squad_order_movest",
    "poetic_form_perspective",
    "plot_role_type",
    "workshop_profile",
    "resource_allotment_specifier_clothing_pantsst",
    "agreement_details",
    "item_quernst",
    "itemimprovement_specific_type",
    "body_component_info",
    "history_event_entity_dissolvedst",
    "curses_color",
    "general_ref_building_nest_boxst",
    "history_event_squad_vs_squadst",
    "flow_info",
    "entity_occasion_schedule",
    "unit_instrument_skill",
    "announcement_flags",
    "interaction_effect_raise_ghostst",
    "creature_interaction_effect_impair_functionst",
    "creature_interaction_effect_target",
    "wound_curse_info",
    "encased_horror",
    "interaction_source_disturbancest",
    "language_name_category",
    "general_ref_unit_tradebringerst",
    "history_event_ceremonyst",
    "meeting_diplomat_info",
    "histfig_entity_link_position_claimst",
    "squad_order_type",
    "job_item_filter",
    "history_event_war_peace_acceptedst",
    "dance_form_move_group_type",
    "manager_order",
    "activity_event_playst",
    "init_display_flags",
    "cave_column",
    "building_archerytargetst",
    "army_controller_sub5",
    "history_event_hist_figure_woundedst",
    "creature_interaction_effect_sense_creature_classst",
    "material",
    "general_ref_unit",
    "history_event_masterpiece_created_item_improvementst",
    "general_ref_sitest",
    "building_bars_verticalst",
    "item_chairst",
    "building_furnacest",
    "history_event_gamblest",
    "language_symbol",
    "agreement_details_data_plot_frame_treason",
    "plotinfost",
    "unit_action_data_job2",
    "resource_allotment_specifier_stonest",
    "interaction_source_experimentst",
    "interface_category_building",
    "machine_handler",
    "texblitst",
    "unit_soul",
    "justification",
    "history_event_add_hf_site_linkst",
    "building_road_pavedst",
    "item_powder",
    "history_event_competitionst",
    "main_designation_type",
    "creature_interaction_effect_display_symbolst",
    "itemimprovement_image_setst",
    "item_cabinetst",
    "construction_type",
    "unit_bp_health_flags",
    "feature_magma_core_from_layerst",
    "history_event_topicagreement_concludedst",
    "reaction_description",
    "d_init_flags2",
    "interaction_target_info",
    "reaction_product_item_improvementst",
    "descriptor_color",
    "item_stockpile_ref",
    "insurrection_outcome",
    "art_facet_type",
    "itemimprovement",
    "geo_layer_type",
    "cursed_tomb",
    "building_workshopst",
    "squad_order_trainst",
    "body_part_raw_flags",
    "tile_building_occ",
    "knowledge_scholar_flags_10",
    "history_event_hist_figure_travelst",
    "combat_report_event_type",
    "mental_picture_elementst",
    "history_event_masterpiece_created_arch_constructst",
    "history_event_hf_convictedst",
    "itemimprovement_art_imagest",
    "creation_zone_pwg_alteration_location_deathst",
    "history_event_war_site_new_leaderst",
    "history_event_written_content_composedst",
    "resource_allotment_specifier_tablest",
    "abstract_building_flags",
    "abstract_building_type",
    "activity_entry_type",
    "world_gen_param_flagst",
    "itemdef_armorst",
    "adventurer_attribute_level",
    "unit_appearance",
    "init_window",
    "agreement_details_type",
    "entity_sell_prices",
    "improvement_type",
    "script_step_dipeventst",
    "profession",
    "world_construction_square_bridgest",
    "animal_training_level",
    "general_ref_unit_holderst",
    "tile_liquid_flow",
    "announcement_type",
    "entity_action_type",
    "settings_context_type",
    "creature_interaction_effect_ment_att_changest",
    "appearance_modifier_type",
    "need_type",
    "d_init_flags1",
    "general_ref_coinbatch",
    "architectural_element",
    "front_type",
    "gate_flags",
    "coord_path",
    "arena_context_type",
    "armor_flags",
    "item_ballistapartsst",
    "incident_data_artifact",
    "history_event_masterpiece_created_dye_itemst",
    "activity_event_writest",
    "art_image_property_intransitive_verbst",
    "armor_general_flags",
    "skill_rating",
    "plant_tree_info",
    "stockpile_link_context_type",
    "site_realization_building_info_castle_towerst",
    "job_skill",
    "art_image_element_type",
    "reaction_product_improvement_flags",
    "item_body_component",
    "interaction_effect_cleanst",
    "value_type",
    "art_image_ref",
    "art_image_property_type",
    "interaction_effect_add_syndromest",
    "script_step_eventst",
    "mental_picture_property_shapest",
    "army_controller_sub20",
    "work_detail",
    "artifact_claim_type",
    "history_event_sabotagest",
    "widget_tabs",
    "histfig_hf_link_former_masterst",
    "ui_look_list",
    "artifacts_mode_type",
    "assign_uniform_context_type",
    "tiletype_material",
    "labor_mode_type",
    "beat_type",
    "histfig_entity_link_occupationst",
    "block_square_event_type",
    "named_scale",
    "item_gratest",
    "body_part_status",
    "location_selector_context_type",
    "interaction_flags",
    "body_part_template_contype",
    "body_part_template_flags",
    "build_req_choice_type",
    "building_extents_type",
    "building_flags",
    "building_type",
    "item_gemst",
    "builtin_mats",
    "dance_form_movement_path",
    "meeting_event",
    "creature_interaction_effect_phys_att_changest",
    "cannot_expel_reason_type",
    "adventure_movement_item_interactst",
    "general_ref_activity_eventst",
    "script_step_textviewerst",
    "creature_interaction_effect_blistersst",
    "item_type",
    "building_display_furniturest",
    "cie_add_tag_mask1",
    "activity_event_sparringst",
    "occupation",
    "history_event_collection_persecutionst",
    "talk_choice_type",
    "d_init_flags3",
    "xlsx_sheet_handle",
    "adventure_option_eat_unit_contaminantst",
    "construction_flags",
    "resource_allotment_specifier_armor_glovesst",
    "building_statuest",
    "construction_interface_page_status_type",
    "conversation_menu",
    "army_controller_sub15",
    "item_verminst",
    "knowledge_scholar_flags_1",
    "region_map_entry",
    "spatter",
    "dance_form_move_modifier",
    "unit_poetic_skill",
    "justice_interface_mode_type",
    "feature_init_underworld_from_layerst",
    "unit_chunk",
    "army_controller_sub19",
    "unit_action_data_release_item",
    "occasion_schedule_type",
    "army_controller_sub22",
    "building_coffinst",
    "craft_material_class",
    "histfig_hf_link_deityst",
    "item_clothst",
    "item_liquid",
    "creation_zone_pwg_alteration_type",
    "creature_graphics_role",
    "creature_interaction_effect_flags",
    "vermin_category",
    "unit_selector_context_type",
    "knowledge_scholar_flags_6",
    "creature_interaction_effect_target_mode",
    "army_controller_sub6",
    "interaction_source_creature_actionst",
    "creature_interaction_effect_type",
    "general_ref_building_destinationst",
    "general_ref_unit_patientst",
    "histfig_site_link_home_site_realization_buildingst",
    "creature_interaction_target_flags",
    "bp_appearance_modifier",
    "goal_type",
    "game_mode",
    "crime_type",
    "rhythm",
    "cumulus_type",
    "flow_reuse_pool",
    "d_init_autosave",
    "worldgen_region_type",
    "relationship_event_supplement",
    "creature_interaction_effect_body_mat_interactionst",
    "cmv_version",
    "enabler",
    "site_reputation_info",
    "general_ref",
    "general_ref_entity_popst",
    "d_init_nickname",
    "building_bedst",
    "world_construction_roadst",
    "history_event_reason_info",
    "caste_raw_flags",
    "resource_allotment_specifier_craftsst",
    "squad_order",
    "dance_form_context",
    "site_realization_building",
    "army_controller_sub23",
    "vehicle",
    "item_amuletst",
    "building_animaltrapst",
    "site_realization_building_info_shrinest",
    "vermin",
    "corpse_material_type",
    "language_translation",
    "dance_form_move_type",
    "history_event_collection_site_conqueredst",
    "dance_form_partner_change_type",
    "dance_form_partner_cue_frequency",
    "coin_batch",
    "abstract_building_templest",
    "dance_form_partner_distance",
    "position_creation_reason_type",
    "location_scribe_jobs",
    "renderer_2d",
    "entity_name_type",
    "entity_position",
    "death_condition_type",
    "world_construction_square",
    "unit_item_use",
    "active_script_var_unitst",
    "death_type",
    "announcements",
    "dfhack_knowledge_scholar_flag",
    "dfhack_material_category",
    "feature_volcanost",
    "interaction_target",
    "adventure_movement_optionst",
    "dfhack_room_quality_level",
    "history_event_knowledge_discoveredst",
    "script_step_setvarst",
    "unit_action_data_unk_sub_21",
    "item_corpsest",
    "history_event_artifact_copiedst",
    "door_flags",
    "history_event_item_stolenst",
    "stop_depart_condition",
    "history_event_collection_occasionst",
    "proj_magicst",
    "emotion_type",
    "history_event_hist_figure_simple_actionst",
    "creature_interaction_effect_skill_roll_adjustst",
    "engraving_flags",
    "entity_entity_link_type",
    "resource_allotment_specifier_bedst",
    "unit_misc_trait",
    "itemdef_siegeammost",
    "vermin_flags",
    "unit_musical_skill",
    "entity_event_type",
    "timed_event",
    "army_controller_sub13",
    "interaction_source_secretst",
    "general_ref_entity_itemownerst",
    "general_ref_building_use_target_2st",
    "entity_raw_flags",
    "entity_sell_category",
    "resource_allotment_specifier_armor_bootsst",
    "chord",
    "entity_site_link_type",
    "era_type",
    "color_modifier_raw",
    "resource_allotment_specifier_gemsst",
    "ethic_response",
    "agreement_details_data_plot_assassination",
    "feature_alteration_type",
    "itemimprovement_clothst",
    "temple_deity_type",
    "plant",
    "item_braceletst",
    "feature_init_flags",
    "history_event_spotted_leaving_sitest",
    "feature_type",
    "written_content_style",
    "flow_guide_type",
    "general_ref_item_type",
    "flow_type",
    "fog_type",
    "sphere_type",
    "general_ref_entity_offeredst",
    "histfig_entity_link_criminalst",
    "histfig_hf_link_former_spousest",
    "general_ref_languagest",
    "histfig_hf_link_former_apprenticest",
    "furniture_type",
    "histfig_entity_link_type",
    "dfhack_lua_viewscreen",
    "resource_allotment_specifier_cheesest",
    "item_powder_miscst",
    "entity_population",
    "entity_position_assignment",
    "tissue",
    "building_slabst",
    "graphic_map_portst",
    "building_traction_benchst",
    "embark_item_choice",
    "timed_event_type",
    "adventure_item_interact_choicest",
    "hauling_stop_conditions_context_type",
    "help_context_type",
    "MacroScreenSave",
    "history_event_collection_performancest",
    "mood_type",
    "hillock_house_type",
    "file_compressorst",
    "world_river",
    "item_backpackst",
    "wqc_item_traitst",
    "histfig_hf_link_type",
    "history_event_processionst",
    "history_event_change_hf_moodst",
    "resource_allotment_specifier_clothing_bootsst",
    "script_step_conditionalst",
    "history_event_collection_purgest",
    "histfig_relationship_type",
    "interaction_target_materialst",
    "histfig_site_link_type",
    "general_ref_unit_climberst",
    "historical_entity_type",
    "history_event_collection_type",
    "init_window_flags",
    "viewscreen_initial_prepst",
    "history_event_regionpop_incorporated_into_entityst",
    "history_event_flags",
    "itemdef_ammost",
    "item_animaltrapst",
    "history_event_merchant_flags",
    "history_event_reason",
    "adventure_environment_optionst",
    "histfig_hf_link_fatherst",
    "knowledge_scholar_flags_5",
    "history_event_type",
    "wound_damage_flags1",
    "region_block_event_sphere_fieldst",
    "knowledge_scholar_flags_0",
    "image_creator_context_type",
    "general_ref_unit_beateest",
    "image_creator_option_type",
    "info_interface_mode_type",
    "itemimprovement_writingst",
    "init_display_filter_mode",
    "init_input_flags",
    "building_doorst",
    "block_square_event_designation_priorityst",
    "item_foodst",
    "squad_ammo_spec",
    "inorganic_flags",
    "instrument_flags",
    "plant_flags",
    "history_event_failed_frame_attemptst",
    "interaction_effect_location_hint",
    "histfig_entity_link_squadst",
    "interaction_effect_type",
    "art_image_element_creaturest",
    "item_actual",
    "unit_flags4",
    "plant_growth",
    "interaction_source_usage_hint",
    "interaction_target_location_type",
    "weather_type",
    "building_grate_floorst",
    "item_history_info",
    "general_ref_building_well_tag",
    "activity_event_ponder_topicst",
    "interaction_target_type",
    "battlefield",
    "interface_breakdown_types",
    "interface_category_construction",
    "interface_push_types",
    "interaction_effect_animatest",
    "item_flags",
    "text_info_element_stringst",
    "item_roughst",
    "item_flags2",
    "item_magicness_type",
    "item_matstate",
    "itemdef_instrumentst",
    "body_appearance_modifier",
    "agreement",
    "job_flags",
    "itemdef_flags",
    "history_event_dance_form_createdst",
    "resource_allotment_specifier_armor_bodyst",
    "history_event_simple_battle_subtype",
    "general_ref_building_triggerst",
    "image_set",
    "job_handler",
    "job_details_context_type",
    "unit_inventory_item",
    "lair_type",
    "creature_interaction_effect_reduce_painst",
    "creature_interaction_effect_reduce_feverst",
    "pressure_plate_info",
    "legend_pagest",
    "job_item_flags1",
    "general_ref_unit_geldeest",
    "job_item_flags2",
    "cave_column_rectangle",
    "stockpile_settings",
    "embark_note",
    "history_event_artifact_foundst",
    "job_material_category",
    "job_skill_class",
    "history_event_creature_devouredst",
    "performance_play_orderst",
    "creature_interaction_effect_reduce_swellingst",
    "creature_interaction_effect_drowsinessst",
    "kitchen_exc_type",
    "reaction",
    "general_ref_unit_traineest",
    "building_stockpilest",
    "item_bedst",
    "creature_interaction_effect_feel_emotionst",
    "knowledge_scholar_flags_12",
    "script_step_constructtopiclistst",
    "knowledge_scholar_flags_13",
    "timbre_type",
    "resource_allotment_specifier_shellst",
    "coord2d",
    "items_other_id",
    "knowledge_scholar_flags_7",
    "history_event_hf_attacked_sitest",
    "creature_interaction",
    "site_realization_building_info_trenchesst",
    "assume_identity_mode",
    "history_event_insurrection_endedst",
    "job_details_option_type",
    "creature_raw_flags",
    "squad_order_kill_listst",
    "unit_syndrome",
    "interaction_source_deityst",
    "region_map_entry_flags",
    "history_event_entity_alliance_formedst",
    "block_square_event_item_spatterst",
    "tiletype",
    "creature_interaction_effect_special_attack_interactionst",
    "language_word_flags",
    "squad_schedule_entry",
    "activity_event_harassmentst",
    "layer_type",
    "squad_order_patrol_routest",
    "weapon_attack",
    "stockpile_tools_context_type",
    "location_details_context_type",
    "site_realization_building_info_tree_housest",
    "pattern_type",
    "machine_conn_modes",
    "general_ref_building_cagedst",
    "main_bottom_mode_type",
    "gait_type",
    "site_reputation_report",
    "items_other",
    "histfig_site_link_home_site_realization_sulst",
    "reaction_reagent",
    "creature_interaction_effect_regrow_partsst",
    "agreement_details_data_promise_position",
    "widget_textbox",
    "world_construction_wallst",
    "masterpiece_loss_type",
    "history_event_masterpiece_createdst",
    "interface_button",
    "meeting_event_type",
    "merc_role_type",
    "mental_picture_element_type",
    "musical_form_passage",
    "mental_picture_property_type",
    "biome_type",
    "activity_event_performancest",
    "interface_button_building_custom_category_selectorst",
    "mental_attribute_type",
    "agreement_details_data_position_corruption",
    "item_toyst",
    "viewscreen_worldst",
    "adventure_movement_item_interact_ridest",
    "monument_type",
    "mountain_peak_flags",
    "world_gen_param_seedst",
    "squad_uniform_spec",
    "musical_form_feature",
    "musical_form_melody_frequency",
    "musical_form_melody_style",
    "general_ref_building_use_target_1st",
    "musical_form_passage_type",
    "occupation_sub1",
    "musical_form_pitch_style",
    "general_ref_building_chainst",
    "incident_data_identity",
    "resource_allotment_specifier_weapon_meleest",
    "name_creator_context_type",
    "adventure_movement_climbst",
    "next_global_id",
    "itemimprovement_instrument_piecest",
    "unit_dance_skill",
    "occasion_schedule_feature",
    "counterintelligence_mode_type",
    "occupation_type",
    "history_event_collection_journeyst",
    "organic_mat_category",
    "histfig_entity_link_positionst",
    "building_bookcasest",
    "interaction_source_underground_specialst",
    "orientation_flags",
    "history_event_collection",
    "history_event_entity_breach_feature_layerst",
    "extentst",
    "pants_flags",
    "histfig_site_link_home_site_saved_civzonest",
    "building_bars_floorst",
    "patrol_routes_context_type",
    "musical_form",
    "performance_participant_type",
    "general_ref_building_display_furniturest",
    "personality_facet_type",
    "creature_interaction_effect_reduce_nauseast",
    "physical_attribute_type",
    "pitch_choice_type",
    "plant_material_def",
    "feature_init_pitst",
    "build_req_choice_specst",
    "viewscreen_update_regionst",
    "unit_action_data_climb",
    "art_image_element_itemst",
    "plant_root_tile",
    "plant_tree_tile",
    "plot_strategy_type",
    "poetic_form_additional_feature",
    "resource_allotment_specifier_cabinetst",
    "poetic_form_caesura_position",
    "widget_container",
    "history_event_hist_figure_diedst",
    "poetic_form_feature",
    "building_armorstandst",
    "history_event_topicagreement_rejectedst",
    "poetic_form_mood",
    "specific_ref_type",
    "inorganic_raw",
    "abstract_building_entombed",
    "text_info_elementst",
    "poetic_form_subject",
    "item_splintst",
    "general_ref_unit_foodreceiverst",
    "item_trappartsst",
    "projectile_flags",
    "abstract_building_guildhallst",
    "squad_position",
    "relationship_event",
    "pronoun_type",
    "unit_emotion_memory",
    "item_cheesest",
    "reaction_reagent_flags",
    "report_zoom_type",
    "region_weather_type",
    "region_block_event_type",
    "room_flow_shape_type",
    "save_substage",
    "creature_interaction_effect_oozingst",
    "art_image_property_verb",
    "appearance_modifier_growth_interval",
    "settings_tab_type",
    "hauling_stop",
    "history_event_entity_incorporatedst",
    "creature_interaction_effect_reduce_paralysisst",
    "item_crutchst",
    "shop_type",
    "ocean_wave_maker",
    "site_realization_building_infost",
    "siegeengine_type",
    "world_gen_param_flagarrayst",
    "tower_shape",
    "simple_action_type",
    "history_event_collection_ceremonyst",
    "building_civzonest",
    "site_realization_building_type",
    "site_shop_type",
    "building_boxst",
    "feature_init_cavest",
    "coord",
    "activity_event_socializest",
    "slab_engraving_type",
    "sound_production_type",
    "poetic_form_pattern",
    "squad_equipment_context_type",
    "ui_unit_view_mode",
    "squad_event_type",
    "building_nest_boxst",
    "block_square_event_material_spatterst",
    "general_ref_unit_slaughtereest",
    "stock_pile_pointer_type",
    "stockpile_category",
    "stockpile_group_set",
    "creature_interaction_effect_close_open_woundsst",
    "tiletype_special",
    "resource_allotment_specifier",
    "burrow",
    "history_event_hf_enslavedst",
    "stratus_type",
    "history_event_hist_figure_revivedst",
    "interaction_effect",
    "army_controller_visit",
    "setup_character_info",
    "entity_site_link_flags",
    "cmv_attribute",
    "ghost_type",
    "tile_designation",
    "entity_entity_link",
    "viewscreen_choose_game_typest",
    "tile_dig_designation",
    "tile_liquid_flow_dir",
    "history_event_context",
    "tile_occupancy",
    "material_vec_ref",
    "general_ref_nemesis",
    "histfig_hf_link_companionst",
    "tile_traffic",
    "assign_vehicle_context_type",
    "intrigue",
    "tiletype_shape",
    "graphic",
    "tiletype_shape_basic",
    "init",
    "stone_use_category_type",
    "general_ref_contains_unitst",
    "creature_raw_graphics",
    "history_event_entity_overthrownst",
    "feature_init_outdoor_riverst",
    "histfig_hf_link",
    "world_gen_param_memberst",
    "knowledge_scholar_flags_3",
    "tissue_flags",
    "tool_flags",
    "tool_uses",
    "creation_zone_pwg_alteration_srp_ruinedst",
    "itemdef_shoesst",
    "uniform_indiv_choice",
    "trap_type",
    "histfig_hf_link_prisonerst",
    "trapcomp_flags",
    "tree_house_type",
    "world_dat_summary",
    "unit",
    "ui_advmode_menu",
    "activity_event_participants",
    "undead_flags",
    "creature_interaction_effect_flash_symbolst",
    "uniform_flags",
    "descriptor_pattern",
    "activity_event_researchst",
    "unit_personality",
    "unit_action_type_group",
    "interaction_target_creaturest",
    "item_seedsst",
    "feature_deep_surface_portalst",
    "unit_flags1",
    "mental_picture_property_adjectivest",
    "unit_flags2",
    "history_event_first_contactst",
    "unit_action_data_talk",
    "unit_flags3",
    "army_controller_invasion",
    "unit_health_flags",
    "unit_labor",
    "unit_labor_category",
    "general_ref_projectile",
    "unit_action_data_suck_blood",
    "history_event_failed_intrigue_corruptionst",
    "unit_list_mode_type",
    "unit_path_goal",
    "item_windowst",
    "unit_report_type",
    "unit_station_type",
    "squad_order_drive_entity_off_sitest",
    "itemdef",
    "inclusion_type",
    "historical_entity",
    "poetic_form_subject_target",
    "itemimprovement_pagesst",
    "history_event_hf_confrontedst",
    "general_ref_poetic_formst",
    "general_ref_type",
    "creature_raw",
    "army_controller_villainous_visit",
    "histfig_hf_link_masterst",
    "history_event_tactical_situationst",
    "itemimprovement_spikesst",
    "general_ref_locationst",
    "script_step_simpleactionst",
    "entity_recipe",
    "weapon_flags",
    "witness_report_flags",
    "job_cancel",
    "fire",
    "cri_unitst",
    "work_detail_flags",
    "adventure_movement_hold_itemst",
    "creature_interaction_effect_necrosisst",
    "history_event_collection_warst",
    "creature_interaction_effect_add_simple_flagst",
    "world_construction_type",
    "building_def",
    "world_region_type",
    "world_site_inhabitant",
    "worldgen_range_type",
    "general_ref_unit_suckeest",
    "identity_type",
    "general_ref_subregionst",
    "building_siegeenginest",
    "item_pipe_sectionst",
    "wound_damage_flags2",
    "adventure_environment_pickup_ignite_vegst",
    "lever_target_type",
    "route_stockpile_link",
    "cached_texturest",
    "zoom_commands",
    "army",
    "world_region_details",
    "general_ref_entity_stolenst",
    "world_site_realization",
    "conflict_level",
    "feature_init_magma_core_from_layerst",
    "global",
    "resource_allotment_specifier_armor_helmst",
    "itemdef_glovesst",
    "abstract_building_contents",
    "abstract_building_dark_towerst",
    "abstract_building_hospitalst",
    "MacroScreenLoad",
    "abstract_building_inn_tavernst",
    "itemimprovement_illustrationst",
    "histfig_entity_link",
    "resource_allotment_specifier_powderst",
    "abstract_building_keepst",
    "item_plant_growthst",
    "proj_list_link",
    "abstract_building_libraryst",
    "resource_allotment_specifier_tallowst",
    "building_weaponrackst",
    "item_meatst",
    "abstract_building_tombst",
    "artifact_claim",
    "abstract_building_towerst",
    "abstract_building_underworld_spirest",
    "active_script_varst",
    "activity_event",
    "item_coinst",
    "plant_raw_flags",
    "activity_event_copy_written_contentst",
    "adventure_environment_place_in_it_containerst",
    "site_realization_building_info_hillock_housest",
    "histfig_hf_link_apprenticest",
    "history_event_poetic_form_createdst",
    "histfig_hf_link_spousest",
    "activity_event_fill_service_orderst",
    "ui_build_item_req",
    "building_cagest",
    "activity_event_guardst",
    "language_word_table_index",
    "viewscreen_choose_start_sitest",
    "activity_event_individual_skill_drillst",
    "entity_position_raw",
    "activity_event_play_with_toyst",
    "activity_event_prayerst",
    "item_armorstandst",
    "activity_event_readst",
    "history_event_artifact_possessedst",
    "unit_action_type",
    "item_liquipowder",
    "activity_event_reunionst",
    "general_ref_creaturest",
    "cie_add_tag_mask2",
    "activity_event_teach_topicst",
    "activity_event_training_sessionst",
    "general_ref_historical_eventst",
    "resource_allotment_specifier_flaskst",
    "body_part_raw",
    "adventure_movement_building_interactst",
    "interaction_source_ingestionst",
    "adventure_environment_ingest_materialst",
    "item_skin_tannedst",
    "adventure_environment_pickup_chop_treest",
    "wound_effect_type",
    "unit_item_wrestle",
    "adventure_environment_pickup_make_campfirest",
    "adventure_environment_pickup_vermin_eventst",
    "sub_rhythm",
    "machine_tile_set",
    "item_statuest",
    "history_event_created_buildingst",
    "activity_event_discuss_topicst",
    "adventure_environment_unit_suck_bloodst",
    "hauler_type",
    "adventure_item_interact_fill_from_containerst",
    "mission",
    "adventure_item_interact_fill_with_materialst",
    "adventure_item_interact_give_namest",
    "resource_allotment_specifier_chairst",
    "history_event_modified_buildingst",
    "item_crownst",
    "adventure_item_interact_heat_from_tilest",
    "adventure_item_interact_pull_outst",
    "entity_material_category",
    "adventure_item_interact_strugglest",
    "dance_form_section",
    "histfig_site_link_prison_site_building_profilest",
    "adventure_environment_ingest_from_containerst",
    "nemesis_offload",
    "unit_action_data_parry",
    "history_event_war_attacked_sitest",
    "nemesis_flags",
    "special_mat_table",
    "entity_claim_mask",
    "workshop_type",
    "material_template",
    "adventure_movement_item_interact_guidest",
    "adventure_movement_item_interact_pushst",
    "honors_type",
    "entity_occasion_info",
    "item_sheetst",
    "machine_info",
    "building_tradedepotst",
    "adventure_movement_release_hold_itemst",
    "histfig_hf_link_motherst",
    "army_controller_sub1",
    "mental_picture_property_color_patternst",
    "conversation",
    "history_event_artifact_droppedst",
    "history_event_reclaim_sitest",
    "history_event_change_creature_typest",
    "adventure_option_view_contaminantst",
    "squad_order_cause_trouble_for_entityst",
    "adventure_optionst",
    "ammo_flags",
    "dance_form",
    "site_realization_building_info_market_squarest",
    "agreement_details_data_demonic_binding",
    "agreement_details_data_join_party",
    "agreement_details_data_location",
    "agreement_details_data_parley",
    "agreement_details_data_plot_abduct",
    "general_ref_item",
    "agreement_details_data_plot_conviction",
    "general_ref_unit_reporteest",
    "world_object_data",
    "text_info_element_longst",
    "building_handler",
    "creature_interaction_effect_stop_bleedingst",
    "history_event_collection_battlest",
    "agreement_details_data_plot_steal_artifact",
    "embark_profile",
    "resource_allotment_specifier_cropst",
    "agreement_details_data_residency",
    "agreement_party",
    "adventure_option_eat_item_contaminantst",
    "histfig_site_link_lairst",
    "army_controller_sub14",
    "history_event_war_peace_rejectedst",
    "general_ref_building_holderst",
    "histfig_entity_link_prisonerst",
    "history_event_hf_interrogatedst",
    "dance_form_move_location",
    "syndrome_flags",
    "art_image",
    "mine_mode_type",
    "interaction_source_type",
    "general_ref_unit_interrogateest",
    "world_geo_layer",
    "world_construction_square_roadst",
    "art_image_element_plantst",
    "world_construction_tunnelst",
    "art_image_element_treest",
    "art_image_property_transitive_verbst",
    "entity_uniform",
    "init_display",
    "building_def_furnacest",
    "item_quality",
    "creature_interaction_effect_swellingst",
    "bb_buttonst",
    "belief_system",
    "block_burrow",
    "block_burrow_link",
    "tile_liquid",
    "shoes_flags",
    "squad_use_flags",
    "block_square_event_spoorst",
    "resource_allotment_specifier_bonest",
    "block_square_event_world_constructionst",
    "hash_rngst",
    "unit_health_info",
    "resource_allotment_specifier_metalst",
    "body_part_template",
    "history_event_artifact_givenst",
    "body_template",
    "histfig_hf_link_childst",
    "scribejob",
    "language_name_component",
    "general_ref_is_artifactst",
    "item_slabst",
    "build_req_choice_genst",
    "activity_event_conversationst",
    "build_req_choicest",
    "adventure_movement_movest",
    "squad_order_drive_armies_from_sitest",
    "stockpile_links",
    "building_actual",
    "squad_order_rescue_hfst",
    "building_axle_horizontalst",
    "building_axle_verticalst",
    "creature_interaction_effect_numbnessst",
    "history_event_collection_competitionst",
    "entity_population_unk4",
    "item_weaponst",
    "building_cabinetst",
    "building_windowst",
    "building_chainst",
    "agreement_details_data_plot_sabotage",
    "site_dispute_type",
    "ocean_wave",
    "building_constructionst",
    "world_population_type",
    "item_smallgemst",
    "season",
    "artifact_record",
    "histfig_entity_link_former_occupationst",
    "building_def_item",
    "construction",
    "building_def_workshopst",
    "building_design",
    "building_drawbuffer",
    "building_floodgatest",
    "projectile",
    "agreement_details_data_plot_infiltration_coup",
    "building_hatchst",
    "building_hivest",
    "squad_order_cannot_reason",
    "building_nestst",
    "adventure_movement_hold_tilest",
    "building_wellst",
    "screw_pump_direction",
    "dfhack_viewscreen",
    "interaction_source",
    "history_event_war_field_battlest",
    "building_rollersst",
    "building_windmillst",
    "building_screw_pumpst",
    "building_squad_use",
    "histfig_entity_link_mercenaryst",
    "kitchen_pref_category_type",
    "building_tablest",
    "building",
    "main_interface_settings",
    "reaction_product_item_flags",
    "civzone_type",
    "projectile_type",
    "building_water_wheelst",
    "abstract_building_mead_hallst",
    "itemdef_helmst",
    "item_pantsst",
    "building_weaponst",
    "building_roadst",
    "plant_raw",
    "general_ref_building",
    "building_window_gemst",
    "item_shoesst",
    "popup_message",
    "buildings_other",
    "feature_magma_poolst",
    "mission_report",
    "main_menu_option_type",
    "general_ref_unit_kidnapeest",
    "item_quiverst",
    "caravan_state",
    "caste_clothing_item",
    "world_mountain_peak",
    "caste_raw",
    "item_millstonest",
    "cave_column_link",
    "tactical_situation",
    "construction_interface_pagest",
    "knowledge_scholar_flags_4",
    "histfig_site_link",
    "item_barrelst",
    "creation_zone_pwg_alteration_campst",
    "creation_zone_pwg_alteration_srb_ruinedst",
    "knowledge_scholar_flags_9",
    "misc_trait_type",
    "creature_interaction_effect",
    "item_floodgatest",
    "world_cavein_flags",
    "d_init_tunnel",
    "creature_interaction_effect_body_appearance_modifierst",
    "d_init_embark_confirm",
    "creature_interaction_effect_body_transformationst",
    "interaction_effect_create_itemst",
    "history_hit_item",
    "creature_interaction_effect_can_do_interactionst",
    "machine",
    "viewscreen_new_regionst",
    "interaction_source_attackst",
    "history_event_add_hf_entity_linkst",
    "item_ringst",
    "machine_standardst",
    "region_weather",
    "history_event_entity_expels_hfst",
    "histfig_site_link_seat_of_powerst",
    "creature_interaction_effect_dizzinessst",
    "job_type_class",
    "creature_interaction_effect_erratic_behaviorst",
    "creature_interaction_effect_feverst",
    "creature_interaction_effect_heal_nervesst",
    "gamest",
    "creature_variation",
    "creature_interaction_effect_material_force_adjustst",
    "histfig_hf_link_imprisonerst",
    "unit_complaint",
    "resource_allotment_specifier_weapon_rangedst",
    "creature_interaction_effect_nauseast",
    "world",
    "init_font",
    "meeting_context",
    "ghost_goal",
    "unit_action",
    "feature_underworld_from_layerst",
    "musical_form_passage_component_type",
    "interface_button_building_material_selectorst",
    "save_version",
    "resource_allotment_specifier_woodst",
    "creature_interaction_effect_painst",
    "itemdef_toolst",
    "resource_allotment_specifier_ammost",
    "general_ref_unit_itemownerst",
    "organization_entryst",
    "creature_variation_convert_tag",
    "unit_action_data_block",
    "part_of_speech",
    "resource_allotment_specifier_clothing_helmst",
    "general_ref_unit_sheareest",
    "script_step_diphistoryst",
    "burrow_selector_context_type",
    "creature_interaction_effect_reduce_dizzinessst",
    "creature_interaction_effect_speed_changest",
    "interaction_source_regionst",
    "history_event_hf_learns_secretst",
    "feature",
    "fortress_type",
    "interaction_instance",
    "resource_allotment_specifier_soapst",
    "general_ref_spherest",
    "campfire",
    "item_earringst",
    "job_subtype_surgery",
    "item_branchst",
    "manager_order_status",
    "general_ref_historical_figurest",
    "game_type",
    "feature_subterranean_from_layerst",
    "worldgen_parms_ps",
    "history_event_building_profile_acquiredst",
    "histfig_entity_link_former_squadst",
    "item_hatch_coverst",
    "building_chairst",
    "history_event_replaced_buildingst",
    "historical_figure",
    "feature_alteration",
    "histfig_site_link_hangoutst",
    "world_data",
    "history_event_hf_recruited_unit_type_for_entityst",
    "effect_info",
    "creature_interaction_effect_vomit_bloodst",
    "view_sheet_trait_type",
    "interface_setst",
    "tiletype_variant",
    "creature_interaction_effect_heal_tissuesst",
    "history_event_first_contact_failedst",
    "unit_action_data_recover",
    "witness_report_type",
    "world_gen_param_basest",
    "instrument_piece",
    "history_event_merchantst",
    "unit_action_data_hold_terrain",
    "interfacest",
    "cultural_identity",
    "plant_growth_print",
    "unit_request",
    "d_init",
    "region_block_eventst",
    "dance_form_move",
    "z_level_flags",
    "coord2d_path",
    "history_event_musical_form_createdst",
    "site_realization_building_info_shop_housest",
    "descriptor_shape",
    "map_block",
    "history_event_artifact_recoveredst",
    "musical_form_interval",
    "itemimprovement_rings_hangingst",
    "dipscript_popup",
    "divination_set",
    "divine_treasure",
    "history_event_hist_figure_reach_summitst",
    "history_event_collection_abductionst",
    "resource_allotment_specifier_backpackst",
    "reaction_category",
    "musical_form_melodies",
    "map_viewport",
    "history_event_hf_gains_secret_goalst",
    "plot_entryst",
    "history_event_masterpiece_created_foodst",
    "block_square_event_frozen_liquidst",
    "site_building_item",
    "job_item_vector_id",
    "unit_action_data_unk_sub_22",
    "creature_interaction_effect_cure_infectionsst",
    "entity_event",
    "embark_symbol",
    "item_threadst",
    "engraving",
    "entity_animal_raw",
    "item_bucketst",
    "item_fish_rawst",
    "history_event_artifact_destroyedst",
    "entity_buy_prices",
    "histfig_hf_link_deceased_spousest",
    "item_scepterst",
    "item_drinkst",
    "item_magicness",
    "widget_menu",
    "creature_interaction_effect_cough_bloodst",
    "item_siegeammost",
    "activity_event_worshipst",
    "history_event_change_hf_statest",
    "item_remainsst",
    "web_cluster",
    "theft_method_type",
    "item_fishst",
    "history_event_masterpiece_created_itemst",
    "tile_pagest",
    "incident_data_performance",
    "building_bridgest",
    "gloves_flags",
    "item_ammost",
    "training_knowledge_level",
    "activity_event_make_believest",
    "nemesis_record",
    "world_construction_bridgest",
    "world_site_type",
    "feature_init_magma_poolst",
    "entity_raw",
    "dance_form_partner_intent",
    "historical_figure_info",
    "unit_action_data_release_terrain",
    "unit_action_data_hold_item",
    "view_sheet_type",
    "history_event_site_surrenderedst",
    "report",
    "entity_sell_requests",
    "mental_picture_element_regionst",
    "instrument_register",
    "entity_site_link",
    "item_coffinst",
    "general_ref_abstract_buildingst",
    "building_offering_placest",
    "interaction_effect_hidest",
    "entity_unk_v47_1",
    "proj_unitst",
    "interface_button_buildingst",
    "viewscreen_dwarfmodest",
    "feature_deep_special_tubest",
    "job_list_link",
    "material_common",
    "unit_action_data_stand_up",
    "feature_alteration_new_pop_maxst",
    "mental_picture_property_timest",
    "units_other",
    "graphic_viewportst",
    "histfig_site_link_occupationst",
    "unit_action_data_jump",
    "adventure_environment_place_in_bld_containerst",
    "feature_alteration_new_lava_fill_zst",
    "feature_init_deep_special_tubest",
    "musical_form_sub4",
    "general_ref_entity",
    "local_population",
    "custom_symbol_context_type",
    "history_event_entity_actionst",
    "report_init",
    "item_constructed",
    "meeting_topic",
    "musicsoundst",
    "mod_headerst",
    "history_event_entity_equipment_purchasest",
    "history_event_circumstance_info",
    "world_region_feature",
    "feature_pitst",
    "creature_interaction_effect_remove_simple_flagst",
    "matter_state",
    "unit_ghost_info",
    "item_filter_spec",
    "world_history",
    "init_media_flags",
    "histfig_flags",
    "script_step_invasionst",
    "item_orthopedic_castst",
    "flow_guide",
    "general_ref_written_contentst",
    "item_crafted",
    "job_art_specification",
    "viewscreen_titlest",
    "world_construction_square_wallst",
    "party_info",
    "adventure_movement_attack_creaturest",
    "tissue_template",
    "item_catapultpartsst",
    "machine_type",
    "unit_action_data_lie_down",
    "world_geo_biome",
    "item_armorst",
    "temple_deity_data",
    "process_unit_aux",
    "army_controller_sub21",
    "history_event_hf_does_interactionst",
    "general_ref_building_triggertargetst",
    "incident_hfid",
    "manager_order_template",
    "resource_allotment_data",
    "history_event_created_sitest",
    "invasion_info",
    "difficultyst",
    "general_ref_contains_itemst",
    "item_doorst",
    "language_name",
    "entity_position_responsibility",
    "history_event",
    "item_toolst",
    "histfig_site_link_home_site_abstract_buildingst",
    "general_ref_feature_layerst",
    "vague_relationship_type",
    "adventure_item_interact_readst",
    "resource_allotment_specifier_bagst",
    "reaction_product_itemst",
    "scale_type",
    "history_event_hist_figure_reunionst",
    "viewscreen_game_cleanerst",
    "viewscreen_legendsst",
    "unit_action_data_unk_sub_23",
    "item_woodst",
    "item_blocksst",
    "knowledge_scholar_flags_11",
    "manager_order_condition_item",
    "general_ref_knowledge_scholar_flagst",
    "resource_allotment_specifier_quiverst",
    "musical_form_style",
    "item_liquid_miscst",
    "language_word",
    "history_event_entity_fled_sitest",
    "entity_occasion",
    "history_event_change_hf_jobst",
    "general_ref_unit_cageest",
    "resource_allotment_specifier_threadst",
    "script_stepst",
    "histfig_entity_link_memberst",
    "history_event_sneak_into_sitest",
    "agreement_details_data_plot_induce_war",
    "script_var_longst",
    "history_event_agreement_concludedst",
    "viewscreen_adopt_regionst",
    "mental_picture",
    "history_event_hf_act_on_artifactst",
    "unit_action_data_unk_sub_20",
    "startup_charactersheet_petst",
    "history_event_diplomat_lostst",
    "army_flags",
    "schedule_info",
    "glowing_barrier",
    "tissue_style_raw",
    "history_event_insurrection_startedst",
    "hauling_route",
    "general_ref_unit_workerst",
    "world_population_ref",
    "general_ref_mapsquare",
    "crime",
    "history_event_created_world_constructionst",
    "furnace_type",
    "building_trapst",
    "histfig_entity_link_former_memberst",
    "histfig_entity_link_former_positionst",
    "histfig_entity_link_former_prisonerst",
    "building_wagonst",
    "creature_interaction_effect_paralysisst",
    "mental_picture_property_actionst",
    "history_event_war_plundered_sitest",
    "creature_interaction_effect_bruisingst",
    "world_gen_param_valuest",
    "mental_picture_property_toolst",
    "history_event_collection_processionst",
    "art_image_chunk",
    "interaction_effect_material_emissionst",
    "power_info",
    "histfig_hf_link_loverst",
    "activity_event_ranged_practicest",
    "resource_allotment_specifier_boxst",
    "unit_action_data_job",
    "creature_interaction_effect_display_namest",
    "unit_thought_type",
    "syndrome",
    "history_era",
    "history_event_add_hf_hf_linkst",
    "general_ref_unit_riderst",
    "activity_event_type",
    "site_realization_crossroads",
    "poetic_form_part",
    "history_event_entity_persecutedst",
    "history_event_artifact_hiddenst",
    "history_event_artifact_lostst",
    "history_event_artifact_transformedst",
    "item_eggst",
    "job_cancel_reason",
    "squad",
    "world_population",
    "specific_ref",
    "history_event_collection_beast_attackst",
    "history_event_collection_duelst",
    "history_event_collection_insurrectionst",
    "embark_finder_option",
    "abstract_building",
    "building_users",
    "musical_form_instruments",
    "loadgame_save_info",
    "history_event_collection_raidst",
    "mental_picture_property_positionst",
    "init_media",
    "unit_relationship_type",
    "history_event_entity_createdst",
    "histfig_entity_link_enemyst",
    "world_landmass",
    "world_site_unk130",
    "creature_interaction_effect_cure_infectionst",
    "scale",
    "deep_vein_hollow",
    "item_glovesst",
    "item_helmst",
    "history_event_artifact_createdst",
    "unit_action_data_unsteady",
    "units_other_id",
    "feature_init_volcanost",
    "history_event_hf_destroyed_sitest",
    "rhythm_pattern",
    "strain_type",
    "meeting_variable",
    "history_event_hf_relationship_deniedst",
    "history_event_hist_figure_new_petst",
    "item_anvilst",
    "unit_preference",
    "unit_action_data_attack",
    "entity_uniform_item",
    "body_part_layer_flags",
    "palette_pagest",
    "item_instrumentst",
    "item_gobletst",
    "material_flags",
    "history_event_remove_hf_hf_linkst",
    "reaction_reagent_itemst",
    "interaction_effect_propel_unitst",
    "resource_allotment_specifier_extractst",
    "world_construction",
    "viewscreen_new_arenast",
    "training_assignment",
    "body_part_layer_raw",
    "resource_allotment_specifier_skinst",
    "unit_skill",
    "interaction_effect_summon_unitst",
    "xlsx_file_handle",
    "interface_button_building_category_selectorst",
    "interface_button_building_new_jobst",
    "mandate",
    "job_item_flags3",
    "art_image_element_shapest",
    "itemdef_shieldst",
    "history_event_create_entity_positionst",
    "activity_event_combat_trainingst",
    "history_event_site_disputest",
    "written_content",
    "item_cagest",
    "unit_wound",
    "reputation_type",
    "general_ref_is_nemesisst",
    "poetic_form",
    "viewscreen_loadgamest",
    "plot_agreement",
]

########################################
#          Symbols processing          #
########################################

class_map: dict[str, str] = {}


def parse_xml(file: Path) -> str:
    tree = ET.parse(file)
    root = tree.getroot()
    result: str = "-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.\n\n---@meta\n\n"
    for child in root:
        for tag in parse_tag(child):
            result += tag + "\n"
    return result


def parse_tag(el: ET.Element) -> Iterable[str]:
    match el.tag:
        case "enum-type":
            yield enum(el)
        case "struct-type":
            for s in struct(el):
                yield s
        case "class-type":
            for s in struct(el):
                yield s
        case "global-object":
            yield global_object(el)
        case "bitfield-type":
            yield bitfield(el)
        case "df-other-vectors-type":
            yield df_other_vectors_type(el)
        case "df-linked-list-type":
            yield df_linked_list_type(el)
        case _:
            print(f"-- SKIPPED TAG {el.tag}")
            yield f"-- SKIPPED TAG {el.tag}"


def line(text: str, intend: int = 0) -> str:
    prefix = "".join(" " * intend)
    return f"{prefix}{text}\n"


def field(name: str, type: str, comment: str = "", intend: int = 0) -> str:
    return line("---@field " + name + " " + type + comment)


def base_type(type: str) -> str:
    match type:
        case "int8_t" | "uint8_t" | "int16_t" | "uint16_t" | "int32_t" | "uint32_t" | "int64_t" | "uint64_t" | "size_t":
            return "integer"
        case "stl-string" | "static-string" | "ptr-string":
            return "string"
        case "s-float" | "d-float" | "long" | "ulong":
            return "number"
        case "bool":
            return "boolean"
        case "pointer" | "padding":
            return "integer"  # or what?
        case "stl-bit-vector":
            return "boolean[]"
        case _:
            return type


def fetch_name(el: ET.Element, options: list[str], default: str = "WTF_UNKNOWN_NAME") -> str:
    for item in options:
        if item in el.attrib:
            return el.attrib[item]
    return default


def fetch_type(el: ET.Element, prefix: str) -> str:
    type = (
        el.attrib["type-name"]
        if "type-name" in el.attrib
        else el.attrib["pointer-type"]
        if "pointer-type" in el.attrib
        else el.tag
    )

    match el.tag:
        case "df-flagarray":
            return (el.attrib["index-enum"] if "index-enum" in el.attrib else "any") + "[]"
        case "pointer":
            return "any[]" if "is-array" in el.attrib else base_type(type)
        case "stl-vector" | "static-array":
            if "type-name" in el.attrib:
                return base_type(el.attrib["type-name"]) + "[]"
            elif "pointer-type" in el.attrib:
                return base_type(el.attrib["pointer-type"]) + "[]"
            else:
                return "any[]"
        case "bitfield":
            return "bitfield"
        case "compound":
            if "type-name" in el.attrib:
                return el.attrib["type-name"]
            if "pointer-type" in el.attrib:
                return el.attrib["pointer-type"]
            if "is-union" in el.attrib and el.attrib["is-union"] == "true":
                t: list[str] = []
                for child in el:
                    t.append(base_type(child.attrib["type-name"] if "type-name" in child.attrib else child.tag))
                return " | ".join(t) if t.__len__() > 0 else base_type(type)
            else:
                return prefix + "_" + fetch_name(el, ["name", "type-name"], "unknown")
        case _:
            return base_type(type)


def fetch_comment(el: ET.Element) -> str:
    s: list[str] = []
    if "comment" in el.attrib:
        s.append(el.attrib["comment"])
    if "since" in el.attrib:
        s.append("since " + el.attrib["since"])
    out = ", ".join(s)
    return " " + out if out != "" else ""


def fetch_union_name(el: ET.Element) -> str:
    out: list[str] = []
    for i, child in enumerate(el, start=1):
        if "name" in child.attrib:
            out.append(child.attrib["name"])
        else:
            out.append(f"unk_{i}")
    return "_or_".join(out)


def enum(el: ET.Element, parent: str = "", prefix: str = "df.") -> str:
    name = fetch_name(el, ["name", "type-name"])
    if parent != "":
        name = f"{parent}_{name}"
    if name in DF_GLOBAL:
        prefix = "df.global."
    comment = fetch_comment(el)
    s: str = f"---@enum {name}{comment}\n"
    s += f"{prefix}{name} = {{\n"
    shift = 0
    for i, child in enumerate(el, start=0):
        if child.tag != "enum-item":
            shift -= 1
            continue
        if "value" in child.attrib:
            shift = int(child.attrib["value"]) - i
        comment = " -- " + child.attrib["comment"] if "comment" in child.attrib else ""
        if "name" in child.attrib:
            s += line(child.attrib["name"] + f" = {i + shift},{comment}", 4)
        else:
            s += line(f"unk_{i} = {i + shift},", 4)
    s += "}\n"
    return s


def struct(el: ET.Element, name_prefix: str = "", prefix: str = "df.") -> list[str]:
    parent_name = fetch_name(el, ["name", "type-name"])
    if name_prefix != "":
        parent_name = f"{name_prefix}_{parent_name}"
    if parent_name in DF_GLOBAL:
        prefix = "df.global."

    comment = fetch_comment(el)
    s = f"---@class {parent_name}{comment}\n"
    if "inherits-from" in el.attrib:
        s += f"-- inherit {el.attrib['inherits-from']}\n"
    out: list[str] = []

    for index, child in enumerate(el, start=1):
        type = fetch_type(child, parent_name)
        if (
            child.tag == "custom-methods"
            # or child.tag == "virtual-methods"
            or type == "code-helper"
            or type == "comment"
        ):
            continue
        name = fetch_name(child, ["name"], f"unnamed_{parent_name}_{index}")
        if child.tag == "virtual-methods":
            for g in child:
                out.append(vmethod(g, f"{prefix}{parent_name}"))
            continue
        if child.tag == "enum":
            if child.__len__() > 0:
                out.append(enum(child, parent=parent_name))
                type = (
                    parent_name + "_" + child.attrib["name"]
                    if "name" in child.attrib
                    else f"{parent_name}_unknown_enum_{index}"
                )
            else:
                type = (
                    child.attrib["type-name"]
                    if "type-name" in child.attrib
                    else child.attrib["name"]
                    if "name" in child.attrib
                    else f"{parent_name}_unknown_enum_{index}"
                )
        if child.tag == "compound" and child.__len__() > 0:
            if "is-union" in child.attrib:
                continue  # HANDLE UNIONS? skip for now
            else:
                for g in struct(child, parent_name):
                    out.append(g)

        s += field(name, type, fetch_comment(child))

    class_map[parent_name] = s

    if parent_name in DF_GLOBAL:
        prefix = "df.global."

    if parent_name in DF_GLOBAL or parent_name in DF:
        s += f"{prefix}{parent_name} = nil\n"

    out.append(s)
    return out


def global_object(el: ET.Element) -> str:
    comment = fetch_comment(el)
    name = fetch_name(el, ["name", "type-name"])
    if el.__len__() > 0:
        el = el[0]
    type = fetch_type(el, "")
    return f"---@type {type}{comment}\ndf.global.{name} = nil\n"


def bitfield(el: ET.Element) -> str:
    comment = fetch_comment(el)
    name = fetch_name(el, ["name", "type-name"])
    return f"---@alias {name} bitfield{comment}\n"


def df_linked_list_type(el: ET.Element) -> str:
    comment = fetch_comment(el)
    name = fetch_name(el, ["name", "type-name"])
    return f"---@alias {name} {el.attrib['item-type']}[]{comment}\n"


def df_other_vectors_type(el: ET.Element, parent_name: str = "", prefix: str = "df.") -> str:
    comment = fetch_comment(el)
    el_name = fetch_name(el, ["name", "type-name"])
    s = f"---@class {el_name}{comment}\n"
    for index, child in enumerate(el, start=0):
        name = fetch_name(child, ["name"], f"unnamed_{index}")
        if child.tag != "stl-vector":
            s += "UNKNOWN VECTOR FIELD"
        else:
            s += field(name, fetch_type(child, parent_name), fetch_comment(child))
    return s


def vmethod(el: ET.Element, glob: str) -> str:
    comment = fetch_comment(el)
    name = fetch_name(el, ["name"], "unnamed_method")
    ret = el.attrib["ret-type"] if "ret-type" in el.attrib else ""
    args: list[tuple[str, str]] = []
    for i, child in enumerate(el):
        if child.tag == "comment":
            continue
        if child.tag == "ret-type":
            ret = fetch_type(child[0], "")
        else:
            args.append(
                (
                    child.attrib["name"] if "name" in child.attrib and child.attrib["name"] != "local" else f"arg_{i}",
                    fetch_type(child, ""),
                )
            )

    s = ""
    params: list[str] = []
    for a in args:
        s += f"---@param {a[0]} {a[1]}\n"
        params.append(a[0])
    if ret != "":
        s += f"---@return {base_type(ret)}\n"

    s += f"function {glob}.{name}({', '.join(params)}) end{' --' + comment if comment != '' else ''}\n"

    return s


def shim_ineritance(data: str) -> str:
    for line in data.split("\n"):
        if line.startswith("-- inherit "):
            name = line.replace("-- inherit ", "").replace("\n", "")
            data = data.replace(
                line,
                f"-- inherited from {name}\n" + shim_ineritance(class_map[name].split("\n", 1)[-1]) + "-- end " + name,
            )
    return data


def resolve_inheritance(file: Path) -> None:
    to_replace: dict[str, str] = {}
    data: str = ""
    with file.open("r", encoding="utf-8") as src:
        data = src.read()
        for m in data.split("\n\n"):
            if m.startswith("---@class"):
                to_replace[m] = shim_ineritance(m)

    if to_replace.__len__() > 0:
        with file.open("r", encoding="utf-8") as src:
            data = src.read()
        with file.open("w", encoding="utf-8") as dest:
            for k in to_replace:
                data = to_replace[k].join(data.split(k))
            dest.write(data)


def symbols_processing() -> None:
    print("Processing symbols...")
    if not Path(PATH_OUTPUT).is_dir():
        Path(PATH_OUTPUT).mkdir(parents=True, exist_ok=True)
        with Path(PATH_LIB_CONFIG).open("w", encoding="utf-8") as dest:
            print(LIB_CONFIG, file=dest)
        with Path(PATH_CONFIG).open("w", encoding="utf-8") as dest:
            print(CONFIG, file=dest)

    with Path(f"{PATH_OUTPUT}aliases.lua").open("w", encoding="utf-8") as dest:
        print(ALIAS, file=dest)

    for file in sorted(Path(PATH_XML).glob("*.xml")):
        print(f"Symbols -> {file.name}")
        with Path(f"{PATH_OUTPUT}{file.name.replace('.xml', '.lua')}").open("w", encoding="utf-8") as dest:
            print(parse_xml(Path(file)), file=dest)

    for file in sorted(Path(PATH_OUTPUT).glob("*.lua")):
        resolve_inheritance(file)


########################################
#        Signatures processing         #
########################################


PATH_LUAAPI = "./library/LuaApi.cpp"
PATH_LIBRARY = "./library/"
PATH_DFHACK_OUTPUT = "./types/library/dfhack.lua"

PATTERN_WRAPM = r"WRAPM\((.+), (.+)\)"
PATTERN_CWRAP = r"CWRAP\((.+), (.+)\)"
PATTERN_WRAPN = r"WRAPN\((.+), (.+)\)"
PATTERN_WRAP = r"WRAP\((.+)\),"
PATTERN_MODULE_ARRAY = r"dfhack_[\w+_]*module\[\](.|\n)*?NULL,\s{0,1}NULL\s{0,1}\}\n\}"
PATTERN_LFUNC_ARRAY = r"dfhack_[\w+_]*funcs\[\](.|\n)*?NULL,\s{0,1}NULL\s{0,1}\}\n\}"
PATTERN_LFUNC_ITEM = r"\{\s\"(\w+)\",\s(\w+)\s\}"
PATTERN_SIGNATURE = r"^([\w::<>]+).+?\((.*)?\)"
PATTERN_SIGNATURE_SEARCH = r"^.+[\s\*]+(DFHack::){0,1}module::function[\s]{0,1}\([\n]{0,1}(.|,\n)*?\n{0,1}\)[\s\n\w]+\{"


@dataclass
class Arg:
    name: str
    type: str
    unknown: bool = False


@dataclass
class Ret:
    type: str
    unknown: bool = False


@dataclass
class Signature:
    ret: Ret
    args: list[Arg]


@dataclass
class Entry:
    module: str
    fn: str
    type: str
    sig: str | None
    decoded_sig: Signature | None


def parse_luaapi() -> Iterable[Entry]:
    total = 0
    found = 0
    decoded = 0
    with Path(PATH_LUAAPI).open("r", encoding="utf-8") as file:
        data = file.read()
        arrays = [PATTERN_MODULE_ARRAY, PATTERN_LFUNC_ARRAY]
        wrappers = [WRAPM, WRAPN, CWRAP, LFUN]
        for array_pattern in arrays:
            for array in re.finditer(array_pattern, data):
                for wrapper in wrappers:
                    for item in wrapper(array.group(0)):
                        total += 1
                        if item.sig:
                            found += 1
                            item.decoded_sig = decode_signature(item.sig)
                            if item.decoded_sig:
                                decoded += 1
                            yield item
                        else:
                            print("Unable to find signature -> module:", item.module, "function:", item.fn)
    print(f"Signatures -> total: {total}, found: {found}, decoded {decoded}")


def module_name(array: str) -> str:
    return (
        array.split(" ")[0]
        .replace("dfhack", "")
        .replace("module[]", "")
        .replace("funcs[]", "")
        .replace("_", "")
        .capitalize()
    )


def WRAP(array: str) -> Iterable[Entry]:
    module = module_name(array)
    if not module:
        module = ""
    for match in re.finditer(PATTERN_WRAP, array):
        item = Entry(module, match.group(1).split(",")[0], "WRAP", None, None)
        item.sig = find_signature(item)
        yield item


def LFUN(array: str) -> Iterable[Entry]:
    module = module_name(array)
    for match in re.finditer(PATTERN_LFUNC_ITEM, array):
        item = Entry(module, match.group(1), "LFUNC", None, None)
        item.sig = find_signature(item)
        yield item


def CWRAP(array: str) -> Iterable[Entry]:
    module = module_name(array)
    for match in re.finditer(PATTERN_CWRAP, array):
        item = Entry(module, match.group(1), "CWRAP", None, None)
        item.sig = find_signature(item)
        yield item


def WRAPM(array: str) -> Iterable[Entry]:
    for match in re.finditer(PATTERN_WRAPM, array):
        if any(c in match.group(1) or c in match.group(2) for c in ["{", "}"]):
            print("SKIP", match.group(0))
            continue
        item = Entry(match.group(1), match.group(2), "WRAPM", None, None)
        item.sig = find_signature(item)
        yield item


def WRAPN(array: str) -> Iterable[Entry]:
    module = module_name(array)
    for match in re.finditer(PATTERN_WRAPN, array):
        if any(c in match.group(1) or c in match.group(2) for c in ["{", "}"]):
            print("SKIP", match.group(0))
            continue
        item = Entry(module, match.group(1), "WRAPN", None, None)
        item.sig = find_signature(item)
        yield item


def find_signature(item: Entry) -> str | None:
    for entry in Path(PATH_LIBRARY).rglob("*.cpp"):
        with entry.open("r", encoding="utf-8") as file:
            data = file.read()
            regex: set[str] = set()
            if item.module != "":
                regex.add(PATTERN_SIGNATURE_SEARCH.replace("module", item.module).replace("function", item.fn))
            regex.add(PATTERN_SIGNATURE_SEARCH.replace("module::", "").replace("function", item.fn))
            for r in regex:
                for match in re.finditer(r, data, re.MULTILINE):
                    sig = match.group(0).replace("\n", "").replace("{", "").replace("DFHACK_EXPORT ", "").strip()
                    if sig.startswith("if (") or sig.startswith("<<") or sig.find("&&") > 0 or sig.find("->") > 0:
                        continue
                    return sig
    return None


def decode_type(cxx: str) -> str:
    match cxx:
        case "int" | "int8_t" | "uint8_t" | "int16_t" | "uint16_t" | "int32_t" | "uint32_t" | "int64_t" | "uint64_t" | "size_t":
            return "integer"
        case "float" | "long" | "ulong" | "double":
            return "number"
        case "bool":
            return "boolean"
        case "string" | "std::string" | "char":
            return "string"
        case "void":
            return "nil"
        case _:
            if cxx.startswith("std::vector<") or cxx.startswith("vector<"):
                return decode_type(cxx.replace("std::vector<", "").replace("vector<", "")[:-1]).strip() + "[]"
            if cxx.startswith("df::"):
                return cxx[4:]
            if cxx.startswith("std::unique_ptr<"):
                return decode_type(cxx.replace("std::unique_ptr<", "")[:-1]).strip()
            return cxx


def decode_signature(sig: str) -> Signature | None:
    for k in ["const ", "*", "&", "static ", "inline "]:
        sig = sig.replace(k, "")
    match = re.search(PATTERN_SIGNATURE, sig, re.MULTILINE)
    if match:
        type_ret = match.group(1)
        decoded_type_ret = decode_type(type_ret)
        args_pairs = match.group(2).split(", ")
        args: list[Arg] = []
        if args_pairs.__len__() > 0 and match.group(2).__len__() > 0:
            for arg_pair in args_pairs:
                arg_name = arg_pair.split(" ")[-1]
                arg_type = arg_pair.replace(" " + arg_name, "").strip()
                decoded_type_arg = decode_type(arg_type)
                if decoded_type_arg == "lua_State":
                    continue
                arg = Arg(
                    arg_name,
                    decoded_type_arg.replace("::", "__"),
                    (decoded_type_arg == arg_type and arg_type != "string")
                    or any(c.isupper() for c in decoded_type_arg),
                )
                args.append(arg)
        if decoded_type_ret:
            return Signature(
                Ret(
                    decoded_type_ret.replace("::", "__").replace("enums__biome_type__", ""),
                    decoded_type_ret == type_ret and type_ret != "string",
                ),
                args,
            )
    return None


def print_entry(entry: Entry, prefix: str = "dfhack.") -> str:
    s = f"-- CXX SIGNATURE -> {entry.sig}\n"
    known_args = ""
    ret = ""
    if entry.decoded_sig:
        for arg in entry.decoded_sig.args:
            known_args += f"---@param {arg.name} {arg.type}{' -- unknown' if arg.unknown else ''}\n"
        ret = f"---@return {entry.decoded_sig.ret.type}{' -- unknown' if entry.decoded_sig.ret.unknown else ''}\n"
        s += known_args + ret
        s += f"function {prefix}{entry.module.lower()}{'.' if entry.module != '' else ''}{entry.fn}({', '.join([x.name for x in entry.decoded_sig.args])}) end\n"
    return s


def signatures_processing() -> None:
    print("Processing signatures...")
    with Path(PATH_DFHACK_OUTPUT).open("w", encoding="utf-8") as dest:
        print("-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.\n\n---@meta\n\n", file=dest)
        unknown_types: set[str] = set()
        for entry in parse_luaapi():
            if entry.decoded_sig:
                if entry.decoded_sig.ret.unknown:
                    unknown_types.add(f"---@alias {entry.decoded_sig.ret.type.replace('[]', '')} unknown\n")
                for arg in entry.decoded_sig.args:
                    if arg.unknown:
                        unknown_types.add(f"---@alias {arg.type.replace('[]', '')} unknown\n")
            print(print_entry(entry), file=dest)
        print(f"\n-- Unknown types\n{''.join(unknown_types)}", file=dest)


if __name__ == "__main__":
    symbols_processing()
    signatures_processing()
