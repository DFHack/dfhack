-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.

---@meta

---@class file_compressorst
---@field compressed boolean
---@field f stl-fstream
---@field in_buffer any[]
---@field in_buffersize number
---@field in_buffer_amount_loaded number
---@field in_buffer_position number
---@field out_buffer any[]
---@field out_buffersize number
---@field out_buffer_amount_written integer
df.file_compressorst = nil

---@enum interface_breakdown_types
df.interface_breakdown_types = {
    NONE = 0,
    QUIT = 1,
    STOPSCREEN = 2,
    TOFIRST = 3,
}

---@enum interface_push_types
df.interface_push_types = {
    AS_PARENT = 0,
    AS_CHILD = 1,
    AT_BACK = 2,
    AT_FRONT = 3,
}

---@param events integer
function df.viewscreen.feed(events) end

function df.viewscreen.logic() end

function df.viewscreen.render() end

---@param w integer
---@param h integer
function df.viewscreen.resize(w, h) end

function df.viewscreen.set_port_flags() end

---@class viewscreen
---@field child viewscreen
---@field parent viewscreen
---@field breakdown_level interface_breakdown_types
---@field option_key_pressed integer
---@field widgets widget_container
df.viewscreen = nil

---@class interfacest
---@field original_fps integer
---@field view viewscreen
---@field flag integer
---@field shutdown_interface_tickcount integer
---@field shutdown_interface_for_ms integer
df.interfacest = nil

---@class extentst
---@field x integer
---@field y integer
---@field w integer
---@field h integer
df.extentst = nil

---@param events integer
function df.widget.feed(events) end

function df.widget.logic() end

function df.widget.render() end

function df.widget.arrange() end

---@class widget
---@field parent integer
---@field name string
---@field rect extentst
---@field rel_x integer
---@field rel_y integer
---@field visible boolean
df.widget = nil

---@class widget_menu
-- inherited from widget
---@field parent integer
---@field name string
---@field rect extentst
---@field rel_x integer
---@field rel_y integer
---@field visible boolean
-- end widget
---@field lines stl-map
---@field selection integer
---@field last_displayheight integer
---@field bleached boolean
---@field colors stl-map
df.widget_menu = nil

---@class widget_textbox
-- inherited from widget
---@field parent integer
---@field name string
---@field rect extentst
---@field rel_x integer
---@field rel_y integer
---@field visible boolean
-- end widget
---@field str string
---@field toggle boolean
---@field flags integer
df.widget_textbox = nil

---@class widget_button
-- inherited from widget
---@field parent integer
---@field name string
---@field rect extentst
---@field rel_x integer
---@field rel_y integer
---@field visible boolean
-- end widget
---@field callback integer
df.widget_button = nil

---@class widget_container
-- inherited from widget
---@field parent integer
---@field name string
---@field rect extentst
---@field rel_x integer
---@field rel_y integer
---@field visible boolean
-- end widget
---@field children widget[]
df.widget_container = nil

---@class widget_tabs
-- inherited from widget_container
-- inherited from widget
---@field parent integer
---@field name string
---@field rect extentst
---@field rel_x integer
---@field rel_y integer
---@field visible boolean
-- end widget
---@field children widget[]
-- end widget_container
---@field cur_idx integer
---@field tab_labels string[]
df.widget_tabs = nil

---@class MacroScreenLoad
-- inherited from viewscreen
---@field child viewscreen
---@field parent viewscreen
---@field breakdown_level interface_breakdown_types
---@field option_key_pressed integer
---@field widgets widget_container
-- end viewscreen
---@field menu widget_menu
---@field width integer
---@field height integer
df.MacroScreenLoad = nil

---@class MacroScreenSave
-- inherited from viewscreen
---@field child viewscreen
---@field parent viewscreen
---@field breakdown_level interface_breakdown_types
---@field option_key_pressed integer
---@field widgets widget_container
-- end viewscreen
---@field id widget_textbox
df.MacroScreenSave = nil

---@class world_dat_summary_unk
---@field unnamed_world_dat_summary_unk_1 integer
---@field unnamed_world_dat_summary_unk_2 integer
---@field unk_3 string
---@field timeline string
---@field unnamed_world_dat_summary_unk_5 string
---@field unnamed_world_dat_summary_unk_6 integer
---@field unnamed_world_dat_summary_unk_7 integer
---@field unnamed_world_dat_summary_unk_8 integer
---@field unnamed_world_dat_summary_unk_9 integer
---@field unnamed_world_dat_summary_unk_10 integer
---@field unnamed_world_dat_summary_unk_11 string

---@class world_dat_summary_last_id when loading, DF sets *_next_id to these fields plus 1
---@field unit integer
---@field soul integer
---@field ite integer
---@field entity integer
---@field nemesis integer
---@field artifact integer
---@field building integer
---@field machine integer
---@field hist_figure integer
---@field hist_event integer
---@field hist_event_collection integer
---@field unit_chunk integer
---@field art_image_chunk integer
---@field task integer
---@field squad integer
---@field formation integer
---@field activity integer
---@field interaction_instance integer
---@field written_content integer
---@field identity integer
---@field incident integer
---@field crime integer
---@field vehicle integer
---@field army integer
---@field army_controller integer
---@field army_tracking_info integer
---@field cultural_identity integer
---@field agreement integer
---@field poetic_form integer since v0.42.01
---@field musical_form integer since v0.42.01
---@field dance_form integer since v0.42.01
---@field scale integer since v0.42.01
---@field rhythm integer since v0.42.01
---@field occupation integer since v0.42.01
---@field belief_system integer since v0.44.01
---@field image_set integer since v0.47.01
---@field divination_set integer since v0.47.01

---@class world_dat_summary
---@field name language_name
---@field unk_1 string
---@field unk_2 integer[] same as the one at the top of world_data
---@field last_id world_dat_summary_last_id when loading, DF sets *_next_id to these fields plus 1
---@field unk world_dat_summary_unk
df.world_dat_summary = nil

---@enum viewscreen_adopt_regionst_cur_step
df.viewscreen_adopt_regionst_cur_step = {
    OpeningFile = 0,
    ProcessingRawData = 1,
    AllocatingSpace = 2,
    LoadingItems = 3,
    LoadingBuildings = 4,
    LoadingEntities = 5,
    LoadingCoinInformation = 6,
    LoadingMapData = 7,
    LoadingCivilizedPopulations = 8,
    LoadingHistory = 9,
    LoadingParameters = 10,
    LoadingArtifacts = 11,
    LoadingActiveHistoricalFigures = 12,
    LoadingSquads = 13,
    LoadingFormations = 14,
    LoadingActivities = 15,
    LoadingInteractions = 16,
    LoadingWrittenContent = 17,
    LoadingIdentities = 18,
    LoadingIncidents = 19,
    LoadingCrimes = 20,
    LoadingVehicles = 21,
    LoadingArmies = 22,
    LoadingArmyControllers = 23,
    LoadingTrackingInformation = 24,
    LoadingCulturalIdentities = 25,
    LoadingAgreements = 26,
    LoadingArtForms = 27,
    LoadingOccupations = 28,
    LoadingBeliefSystems = 29,
    LoadingImageSets = 30,
    LoadingDivinationSets = 31,
    ClosingFile = 32,
    RebuildingTemporaryInformation = 33,
    PreparingGame = 34,
    Failed = 35,
}
---@class viewscreen_adopt_regionst
-- inherit viewscreen
---@field compressor file_compressorst
---@field viewscreen_adopt_regionst_cur_step viewscreen_adopt_regionst_cur_step
---@field save_version save_version
---@field cur_save world_dat_summary
---@field glosses matgloss_list
---@field progress integer 0..35
df.viewscreen_adopt_regionst = nil

---@enum embark_finder_option
df.embark_finder_option = {
    DimensionX = 0,
    DimensionY = 1,
    Savagery = 2,
    Spirit = 3,
    Elevation = 4,
    Temperature = 5,
    Rain = 6,
    Drainage = 7,
    FluxStone = 8,
    AquiferLight = 9,
    AquiferHeavy = 10,
    River = 11,
    UndergroundRiver = 12,
    UndergroundPool = 13,
    MagmaPool = 14,
    MagmaPipe = 15,
    Chasm = 16,
    BottomlessPit = 17,
    OtherFeatures = 18,
    Soil = 19,
    Clay = 20,
    Sand = 21,
}

---@class embark_location starter_infost?
---@field region_pos coord2d
---@field reclaim_site integer
---@field reclaim_idx integer
---@field embark_pos_min coord2d
---@field embark_pos_max coord2d
df.embark_location = nil

---@enum viewscreen_choose_start_sitest_find_results
df.viewscreen_choose_start_sitest_find_results = {
    None = -1,
    Searching = 0,
    NoResult = 1,
    Partial = 2,
    Suitable = 3,
}
---@enum viewscreen_choose_start_sitest_page
df.viewscreen_choose_start_sitest_page = {
    Biome = 0,
    Neighbors = 1,
    Civilization = 2,
    Elevation = 3,
    Cliffs = 4,
    Reclaim = 5,
    Reclaim2 = 6,
    Find = 7,
    Notes = 8,
}
---@class viewscreen_choose_start_sitest
-- inherit viewscreen
---@field viewscreen_choose_start_sitest_page viewscreen_choose_start_sitest_page
---@field location embark_location
---@field animating_quick_start_timer integer
---@field setting_up_map_timer integer
---@field region_cent_x integer
---@field region_cent_y integer
---@field mouse_scrolling_map boolean
---@field mouse_anchor_mx integer
---@field mouse_anchor_my integer
---@field mouse_anchor_pmx integer
---@field mouse_anchor_pmy integer
---@field neighbor_hover_ax integer
---@field neighbor_hover_ay integer
---@field neighbor_hover_mm_sx integer
---@field neighbor_hover_mm_sy integer
---@field neighbor_hover_mm_ex integer
---@field neighbor_hover_mm_ey integer
---@field def_candidate historical_entity[]
---@field def_candidate_nearst world_site[]
---@field def_candidate_mindist integer[]
---@field def_candidate_state integer[]
---@field zoomed_in boolean
---@field zoom_cent_x integer
---@field zoom_cent_y integer
---@field show_cliffs boolean
---@field show_elevation boolean
---@field choosing_civilization boolean
---@field scroll_position_civ integer
---@field scrolling_civ boolean
---@field choosing_reclaim boolean
---@field scroll_position_reclaim integer
---@field scrolling_reclaim boolean
---@field choosing_embark boolean
---@field embark_dx integer
---@field embark_dy integer
---@field warn_mm_startx integer
---@field warn_mm_endx integer
---@field warn_mm_starty integer
---@field warn_mm_endy integer
---@field doing_site_finder boolean
---@field scroll_position_param integer
---@field scrolling_param boolean
---@field biome_idx integer
---@field biome_highlighted boolean
---@field in_embark_light_aquifer boolean
---@field in_embark_heavy_aquifer boolean
---@field in_embark_salt boolean
---@field in_embark_large boolean
---@field in_embark_narrow boolean
---@field in_embark_only_warning boolean
---@field in_embark_civ_dying boolean
---@field selected_reclaim integer
---@field selected_civ integer
---@field start_civ historical_entity[]
---@field start_civ_nem_num integer[]
---@field start_civ_entpop_num integer[]
---@field start_civ_site_num integer[]
---@field reclaim_detail_box string[] since v0.40.01
---@field reclaim_detail_he history_event since v0.40.01
---@field reclaim_detail_she history_event since v0.40.01
---@field reclaim_detail_box_last_processing_dimx integer since v0.40.01
---@field find_cur_best_value integer
---@field find_block_x integer
---@field find_block_y integer
---@field find_block_dx integer to world width / 16
---@field find_block_dy integer to world height / 16
---@field find_select integer
---@field find_param integer[]
---@field find_missed_param boolean[]
---@field find_missed_metal_ore integer[]
---@field find_param_list integer[]
---@field find_metal_ore integer[]
---@field skip_metal_ore integer[]
---@field viewscreen_choose_start_sitest_find_results viewscreen_choose_start_sitest_find_results
---@field find_ax integer
---@field find_ay integer
---@field find_mm_sx integer
---@field find_mm_ex integer
---@field find_mm_sy integer
---@field find_mm_ey integer
---@field note_index integer[]
---@field text_box string[]
---@field notes_entering_text boolean
---@field notes_list_select integer
---@field notes_cur_sym integer
---@field notes_sym_select_1 integer
---@field notes_sym_select_2 integer
---@field notes_sym_select_3 integer
---@field notes_selected_note integer
df.viewscreen_choose_start_sitest = nil

---@enum mission_type
df.mission_type = {
    Raid = 2,
    RecoverArtifact = 17,
    RescuePerson = 18,
    RequestWorkers = 19,
}
---@class mission
---@field army_controller integer
---@field entity integer
---@field target_site integer
---@field unk_2 integer
---@field target_x integer
---@field target_y integer
---@field unk_3 integer
---@field unk_4 integer
---@field unk_5 integer
---@field unk_6 integer
---@field unk_7 integer
---@field unk_8 integer
---@field unk_9 integer
---@field unk_10 integer
---@field year integer
---@field year_tick integer
---@field unk_12 integer
---@field army_controller2 integer
---@field histfig integer
---@field unk_14 integer
---@field unk_16 integer
---@field unk_17 integer
---@field unk_18 integer
---@field unk_19 integer
---@field unk_20 integer
---@field unk_21 integer
---@field unk_15 integer
---@field unk_22 integer
---@field squads integer[]
---@field messengers integer[]
---@field unk_23 integer
---@field unk_24 integer
---@field details raid | recovery | rescue | request
---@field mission_type mission_type
---@field unk_25 integer
df.mission = nil

---@class viewscreen_dwarfmodest
-- inherited from viewscreen
---@field child viewscreen
---@field parent viewscreen
---@field breakdown_level interface_breakdown_types
---@field option_key_pressed integer
---@field widgets widget_container
-- end viewscreen
---@field shown_site_name integer
---@field jeweler_mat_count integer
---@field jeweler_cutgem integer[]
---@field jeweler_encrust integer[]
---@field unit_labors_sidemenu any[]
---@field unit_labors_sidemenu_uplevel any[]
---@field unit_labors_sidemenu_uplevel_idx integer
---@field sideSubmenu integer
---@field keyRepeat integer
---@field trained_animals unit[]
---@field trained_animal_idx integer
---@field number_assigned_hunt integer
---@field number_assigned_war integer
df.viewscreen_dwarfmodest = nil

---@class viewscreen_export_regionst_units_progress
---@field save_file_id integer[]
---@field save_file_member_idx integer[]
---@field units unit[]
---@field current_chunk unit_chunk
---@field current_save_file_id integer
---@field offloaded_units integer

---@enum viewscreen_export_regionst_state
df.viewscreen_export_regionst_state = {
    Initializing = 0,
    PreliminaryCleaning = 1,
    OffloadingUnits = 2,
    OffloadingArtImages = 3,
    OffloadingFile = 4,
    CharacterizingRawData = 5,
    SortingWorldInformation = 6,
    AllocatingSpace = 7,
    SavingItems = 8,
    SavingBuildings = 9,
    SavingEntities = 10,
    SavingCoinInformation = 11,
    SavingMapData = 12,
    SavingCivilizedPopulations = 13,
    SavingHistory = 14,
    SavingParameters = 15,
    SavingArtifacts = 16,
    SavingActiveHistoricalFigures = 17,
    SavingSquads = 18,
    SavingFormations = 19,
    SavingActivities = 20,
    SavingInteractions = 21,
    SavingWrittenContent = 22,
    SavingIdentities = 23,
    SavingIncidents = 24,
    SavingCrimes = 25,
    SavingVehicles = 26,
    SavingArmies = 27,
    SavingArmyControllers = 28,
    SavingTrackingInfo = 29,
    SavingCulturalIdentities = 30,
    SavingAgreements = 31,
    SavingArtForms = 32,
    SavingOccupations = 33,
    SavingBeliefSystems = 34,
    SavingImageSets = 35,
    SavingDivinationSets = 36,
    ClosingFile = 37,
    SynchronizingFolders = 38,
}
---@class viewscreen_export_regionst
-- inherit viewscreen
---@field play_now boolean
---@field viewscreen_export_regionst_state viewscreen_export_regionst_state
---@field progress integer 0..40
---@field units_progress viewscreen_export_regionst_units_progress
---@field compressor file_compressorst
---@field folder_name string
---@field timeline_name string
df.viewscreen_export_regionst = nil

---@enum viewscreen_game_cleanerst_state
df.viewscreen_game_cleanerst_state = {
    CleaningGameObjects = 0,
    CleaningStrandedObjects = 1,
    CleaningPlayObjects = 2,
}
---@class viewscreen_game_cleanerst
-- inherit viewscreen
---@field viewscreen_game_cleanerst_state viewscreen_game_cleanerst_state
df.viewscreen_game_cleanerst = nil

---@class viewscreen_initial_prepst
-- inherited from viewscreen
---@field child viewscreen
---@field parent viewscreen
---@field breakdown_level interface_breakdown_types
---@field option_key_pressed integer
---@field widgets widget_container
-- end viewscreen
---@field render_count integer
---@field logic_step integer
---@field unk_90 stl-future
df.viewscreen_initial_prepst = nil

---@param num string
function df.world_gen_param_basest.get_text(num) end

---@return boolean
function df.world_gen_param_basest.has_string_entry() end

---@return boolean
function df.world_gen_param_basest.nullifiable() end

---@return boolean
function df.world_gen_param_basest.togglealble() end

---@return boolean
function df.world_gen_param_basest.has_max_min() end

---@return boolean
function df.world_gen_param_basest.has_increase_decrease() end

---@return integer
function df.world_gen_param_basest.get_min() end

---@return integer
function df.world_gen_param_basest.get_max() end

---@param value_str string
function df.world_gen_param_basest.set_value(value_str) end

function df.world_gen_param_basest.nullify() end

function df.world_gen_param_basest.toggle() end

function df.world_gen_param_basest.decrease() end

function df.world_gen_param_basest.increase() end

---@class world_gen_param_basest
---@field text string
df.world_gen_param_basest = nil

---@class world_gen_param_seedst
-- inherited from world_gen_param_basest
---@field text string
-- end world_gen_param_basest
---@field val_ptr string
df.world_gen_param_seedst = nil

---@class world_gen_param_valuest
-- inherited from world_gen_param_basest
---@field text string
-- end world_gen_param_basest
---@field null_text string
---@field can_be_nullified boolean
---@field value_text string[]
df.world_gen_param_valuest = nil

---@class world_gen_param_charst
-- inherited from world_gen_param_valuest
-- inherited from world_gen_param_basest
---@field text string
-- end world_gen_param_basest
---@field null_text string
---@field can_be_nullified boolean
---@field value_text string[]
-- end world_gen_param_valuest
---@field val_ptr integer
---@field min integer
---@field max integer
---@field null_value integer
---@field value_val integer[]
---@field can_toggle boolean
df.world_gen_param_charst = nil

---@class world_gen_param_memberst
-- inherited from world_gen_param_valuest
-- inherited from world_gen_param_basest
---@field text string
-- end world_gen_param_basest
---@field null_text string
---@field can_be_nullified boolean
---@field value_text string[]
-- end world_gen_param_valuest
---@field val_ptr integer
---@field min integer
---@field max integer
---@field null_value integer
---@field value_val integer[]
---@field does_have_min_max boolean
df.world_gen_param_memberst = nil

---@class world_gen_param_flagst
-- inherited from world_gen_param_valuest
-- inherited from world_gen_param_basest
---@field text string
-- end world_gen_param_basest
---@field null_text string
---@field can_be_nullified boolean
---@field value_text string[]
-- end world_gen_param_valuest
---@field val_ptr integer
---@field bit integer
---@field value_val integer[]
df.world_gen_param_flagst = nil

---@class world_gen_param_flagarrayst
-- inherited from world_gen_param_valuest
-- inherited from world_gen_param_basest
---@field text string
-- end world_gen_param_basest
---@field null_text string
---@field can_be_nullified boolean
---@field value_text string[]
-- end world_gen_param_valuest
---@field val_ptr integer
---@field flag integer
---@field value_val integer[]
df.world_gen_param_flagarrayst = nil

---@enum legend_pagest_mode
df.legend_pagest_mode = {
    NONE = -1,
    MAIN = 0,
    HFS = 1,
    SITES = 2,
    ARTIFACTS = 3,
    BOOKS = 4,
    SUBREGIONS = 5,
    ENTITIES = 6,
    ART = 7,
    ABS = 8,
    ERA = 9,
    HEC = 10,
    MAPS = 11,
    FEATURE_LAYERS = 12,
    POPULATIONS = 13,
}
---@class legend_pagest
---@field header string
---@field legend_pagest_mode legend_pagest_mode
---@field index integer
---@field text_box markup_text_boxst
---@field scroll_position_list integer
---@field scrolling_list boolean
---@field lptr integer
---@field scroll_position_text integer
---@field scrolling_text boolean
---@field filter_str string
---@field entering_filter boolean
df.legend_pagest = nil

---@class viewscreen_legendsst
-- inherited from viewscreen
---@field child viewscreen
---@field parent viewscreen
---@field breakdown_level interface_breakdown_types
---@field option_key_pressed integer
---@field widgets widget_container
-- end viewscreen
---@field unhid_sum integer
---@field init_stage integer
---@field init_cur_era integer
---@field init_cur_era_num integer
---@field init_cur_era_denom integer
---@field init_sub_stage integer
---@field histfigs integer[]
---@field sites integer[]
---@field artifacts integer[]
---@field codices integer[]
---@field regions integer[]
---@field layers integer[]
---@field entities integer[]
---@field structure_sites integer[]
---@field structures_indices integer[]
---@field entity_population integer[]
---@field main_choice integer[]
---@field era_choice_index integer[]
---@field era_choice_num integer[]
---@field era_choice_denom integer[]
---@field hec_id integer[]
---@field showing_all_era_collections integer
---@field region_snapshot any[]
---@field region_view_x integer
---@field region_view_y integer
---@field region_view_mode integer
---@field civ_site_view integer
---@field region_view_snapshot_index integer
---@field histfigs_filtered integer[] index into histfigs
---@field sites_filtered integer[] index into sites
---@field artifacts_filtered integer[] index into artifacts
---@field codices_filtered integer[] index into codices
---@field regions_filtered integer[] index into regions
---@field layers_filtered integer[] index into layers
---@field entity_populations_filtered integer[]
---@field entities_filtered integer[] index into entities
---@field structures_filtered integer[] index into structures
---@field total_codices integer
---@field total_artifacts integer
---@field page legend_pagest[]
---@field active_page_index integer
---@field page_scroll integer
---@field unk_338 stl-future
---@field unk_348 integer
df.viewscreen_legendsst = nil

---@class loadgame_save_info
---@field next_ids integer[]
---@field game_type game_type only 0 (fort) 1 (adv) 3(reclaim) are valid
---@field fort_name string
---@field world_name string
---@field year integer
---@field unnamed_loadgame_save_info_6 integer
---@field folder_name string
---@field unnamed_loadgame_save_info_8 string
---@field unnamed_loadgame_save_info_9 string
---@field unnamed_loadgame_save_info_10 integer
---@field unnamed_loadgame_save_info_11 integer
---@field unnamed_loadgame_save_info_12 integer
---@field unnamed_loadgame_save_info_13 integer
---@field unnamed_loadgame_save_info_14 integer
---@field unnamed_loadgame_save_info_15 string
df.loadgame_save_info = nil

---@class matgloss_list
---@field unk_0 any[]
---@field generated_inorganics any[]
---@field generated_plants any[]
---@field generated_items any[]
---@field generated_creatures any[]
---@field generated_entities any[]
---@field generated_reactions any[]
---@field generated_interactions any[]
---@field generated_languages any[]
---@field inorganics string[]
---@field plants string[]
---@field bodies string[]
---@field bodyglosses string[]
---@field creatures string[]
---@field items string[]
---@field buildings string[]
---@field entities string[]
---@field words string[]
---@field symbols string[]
---@field translations string[]
---@field colors string[]
---@field shapes string[]
---@field patterns string[]
---@field reactions string[]
---@field material_templates string[]
---@field tissue_templates string[]
---@field body_detail_plans string[]
---@field creature_variations string[]
---@field interactions string[]
---@field text_sets string[]
---@field musics string[]
---@field sounds string[]
---@field mod_ids string[]
---@field mod_versions integer[]
---@field mod_compatible_versions integer[]
---@field mod_folder_paths string[]
---@field mod_names string[]
---@field mod_display_versions string[]
df.matgloss_list = nil

---@enum viewscreen_loadgamest_cur_step After the on-screen text shown while loading.
df.viewscreen_loadgamest_cur_step = {
    OpeningFile = 0,
    ProcessingRawData = 1,
    AllocatingSpace = 2,
    LoadingItems = 3,
    LoadingUnits = 4,
    LoadingJobs = 5,
    LoadingSchedules = 6,
    LoadingProjectiles = 7,
    LoadingBuildings = 8,
    LoadingMachines = 9,
    LoadingFlowGuides = 10,
    LoadingEffects = 11,
    LoadingEntities = 12,
    LoadingLocalAnimalPopulations = 13,
    LoadingEvents = 14,
    LoadingMandates = 15,
    LoadingWorkQuotas = 16,
    LoadingWorldEvents = 17,
    LoadingCoinInformation = 18,
    LoadingSquads = 19,
    LoadingFormations = 20,
    LoadingActivities = 21,
    LoadingInteractions = 22,
    LoadingWrittenContent = 23,
    LoadingIdentities = 24,
    LoadingIncidents = 25,
    LoadingCrimes = 26,
    LoadingVehicles = 27,
    LoadingArmies = 28,
    LoadingArmyControllers = 29,
    LoadingTrackingInformation = 30,
    LoadingCulturalIdentities = 31,
    LoadingAgreements = 32,
    LoadingArtForms = 33,
    LoadingOccupations = 34,
    LoadingBeliefSystems = 35,
    LoadingImageSets = 36,
    LoadingDivinationSets = 37,
    LoadingAnnouncements = 38,
    LoadingFortressInformation = 39,
    LoadingWorldInformation = 40,
    LoadingArtifacts = 41,
    LoadingActiveHistoricalFigures = 42,
    LoadingAdventure = 43,
    LoadingGeneralInformation = 44,
    ClosingFile = 45,
    RebuildingTemporaryInformation = 46,
    RebuildingMoreTemporaryInformation = 47,
    PreparingGameScreen = 48,
    HandlingCompatibilityIssues = 49,
    Finishing = 50,
}
---@class viewscreen_loadgamest
-- inherit viewscreen
---@field viewscreen_loadgamest_cur_step viewscreen_loadgamest_cur_step After the on-screen text shown while loading.
---@field progress integer since v0.40.01
---@field compressor file_compressorst
---@field glosses matgloss_list
---@field loading integer
---@field save_version integer
---@field cur_save loadgame_save_info
df.viewscreen_loadgamest = nil

---@class worldgen_parms
---@field title string
---@field seed string since v0.34.01
---@field history_seed string since v0.34.01
---@field name_seed string since v0.34.01
---@field creature_seed string since v0.34.01
---@field dim_x integer
---@field dim_y integer
---@field custom_name string
---@field has_seed boolean
---@field has_history_seed boolean
---@field has_name_seed boolean
---@field has_creature_seed boolean
---@field embark_points integer
---@field peak_number_min integer
---@field partial_ocean_edge_min integer
---@field complete_ocean_edge_min integer
---@field volcano_min integer
---@field region_counts any[]
---@field river_mins integer[]
---@field subregion_max integer
---@field cavern_layer_count integer
---@field cavern_layer_openness_min integer
---@field cavern_layer_openness_max integer
---@field cavern_layer_passage_density_min integer
---@field cavern_layer_passage_density_max integer
---@field cavern_layer_water_min integer
---@field cavern_layer_water_max integer
---@field have_bottom_layer_1 boolean
---@field have_bottom_layer_2 boolean
---@field levels_above_ground integer
---@field levels_above_layer_1 integer
---@field levels_above_layer_2 integer
---@field levels_above_layer_3 integer
---@field levels_above_layer_4 integer
---@field levels_above_layer_5 integer
---@field levels_at_bottom integer
---@field cave_min_size integer
---@field cave_max_size integer
---@field mountain_cave_min integer
---@field non_mountain_cave_min integer
---@field total_civ_number integer
---@field rain_ranges_1 integer
---@field rain_ranges_0 integer
---@field rain_ranges_2 integer
---@field drainage_ranges_1 integer
---@field drainage_ranges_0 integer
---@field drainage_ranges_2 integer
---@field savagery_ranges_1 integer
---@field savagery_ranges_0 integer
---@field savagery_ranges_2 integer
---@field volcanism_ranges_1 integer
---@field volcanism_ranges_0 integer
---@field volcanism_ranges_2 integer
---@field ranges any[]
---@field beast_end_year integer
---@field end_year integer
---@field beast_end_year_percent integer
---@field total_civ_population integer
---@field site_cap integer
---@field elevation_ranges_1 integer
---@field elevation_ranges_0 integer
---@field elevation_ranges_2 integer
---@field mineral_scarcity integer
---@field megabeast_cap integer
---@field semimegabeast_cap integer
---@field titan_number integer
---@field titan_attack_trigger integer[]
---@field demon_number integer
---@field night_troll_number integer
---@field bogeyman_number integer
---@field nightmare_number integer since v0.47.01
---@field vampire_number integer
---@field werebeast_number integer
---@field werebeast_attack_trigger integer[] since v0.47.01
---@field secret_number integer
---@field regional_interaction_number integer
---@field disturbance_interaction_number integer
---@field evil_cloud_number integer
---@field evil_rain_number integer
---@field generate_divine_materials integer since v0.40.01
---@field allow_divination integer since v0.47.01
---@field allow_demonic_experiments integer since v0.47.01
---@field allow_necromancer_experiments integer since v0.47.01
---@field allow_necromancer_lieutenants integer since v0.47.01
---@field allow_necromancer_ghouls integer since v0.47.01
---@field allow_necromancer_summons integer since v0.47.01
---@field good_sq_counts_0 integer
---@field evil_sq_counts_0 integer
---@field good_sq_counts_1 integer
---@field evil_sq_counts_1 integer
---@field good_sq_counts_2 integer
---@field evil_sq_counts_2 integer
---@field elevation_frequency integer[]
---@field rain_frequency integer[]
---@field drainage_frequency integer[]
---@field savagery_frequency integer[]
---@field temperature_frequency integer[]
---@field volcanism_frequency integer[]
---@field ps worldgen_parms_ps
---@field reveal_all_history integer
---@field cull_historical_figures integer
---@field erosion_cycle_count integer
---@field periodically_erode_extremes integer
---@field orographic_precipitation integer
---@field playable_civilization_required integer
---@field all_caves_visible integer
---@field show_embark_tunnel integer
---@field pole integer
---@field unk_1 boolean
df.worldgen_parms = nil

---@class worldgen_parms_ps
---@field width integer
---@field height integer
---@field data any[]
df.worldgen_parms_ps = nil

---@class viewscreen_new_regionst
-- inherited from viewscreen
---@field child viewscreen
---@field parent viewscreen
---@field breakdown_level interface_breakdown_types
---@field option_key_pressed integer
---@field widgets widget_container
-- end viewscreen
---@field worldgen_presets worldgen_parms[]
---@field doing_params integer
---@field param_list_open boolean
---@field scroll_position_param_list integer
---@field scrolling_param_list boolean
---@field scroll_position_params integer
---@field scrolling_params boolean
---@field entering_param_name boolean
---@field sel_param integer
---@field enter_seed string
---@field editing_seed boolean
---@field editing_name boolean
---@field editing_title boolean
---@field confirm_delete boolean
---@field confirm_new_dims boolean
---@field new_dimx integer
---@field new_dimy integer
---@field confirming_abort_save integer
---@field confirming_start_save integer
---@field params_need_save integer
---@field entering_value_str boolean
---@field entering_value_index integer
---@field value_str string
---@field member world_gen_param_basest[]
---@field last_saved_tc number
---@field last_loaded_tc number
---@field doing_simple_params integer
---@field simple_sel integer
---@field simple_world_size integer
---@field simple_history integer
---@field simple_civ_num integer
---@field simple_site_cap integer
---@field simple_beast integer
---@field simple_savagery integer
---@field simple_minerals integer
---@field abort_world_gen_dialogue integer
---@field reject_dialogue integer
---@field reject_dialogue_type integer
---@field text_box string[]
---@field mouse_scrolling_map boolean
---@field mouse_anchor_mx integer
---@field mouse_anchor_my integer
---@field mouse_anchor_pmx integer
---@field mouse_anchor_pmy integer
---@field raw_load boolean
---@field stage_count integer
---@field raw_load_stage integer
---@field doing_mods boolean
---@field scroll_position_available_mods integer
---@field scrolling_available_mods boolean
---@field scroll_position_selected_mods integer
---@field scrolling_selected_mods boolean
---@field base_available_id string[]
---@field base_available_numeric_version integer[]
---@field base_available_earliest_compat_numeric_version integer[]
---@field base_available_src_dir string[]
---@field base_available_name string[]
---@field base_available_displayed_version string[]
---@field base_available_mod_header mod_headerst[]
---@field object_load_order_id string[]
---@field object_load_order_numeric_version integer[]
---@field object_load_order_earliest_compat_numeric_version integer[]
---@field object_load_order_src_dir string[]
---@field object_load_order_name string[]
---@field object_load_order_displayed_version string[]
---@field object_load_order_mod_header mod_headerst[]
---@field available_id string[]
---@field available_numeric_version integer[]
---@field available_earliest_compat_numeric_version integer[]
---@field available_src_dir string[]
---@field available_name string[]
---@field available_displayed_version string[]
---@field available_mod_header mod_headerst[]
---@field hover_mod_description string[]
---@field last_hover_mod_id string
---@field last_hover_mod_version integer
---@field last_hover_width integer
df.viewscreen_new_regionst = nil

---@class nemesis_offload
---@field nemesis_save_file_id integer[]
---@field nemesis_member_idx integer[]
---@field units unit[]
---@field cur_unit_chunk unit_chunk
---@field cur_unit_chunk_num integer
---@field units_offloaded integer
df.nemesis_offload = nil

---@enum viewscreen_savegamest_cur_step
df.viewscreen_savegamest_cur_step = {
    Initializing = 0,
    CheckingDirectoryStructure = 1,
    PreliminaryCleaning = 2,
    OffloadingUnits = 3,
    OffloadingArt = 4,
    OpeningFile = 5,
    CharacterizingRawData = 6,
    AllocatingSpace = 7,
    SavingItems = 8,
    SavingUnits = 9,
    SavingJobs = 10,
    SavingSchedules = 11,
    SavingProjectiles = 12,
    SavingBuildings = 13,
    SavingMachines = 14,
    SavingFlowGuides = 15,
    SavingEffects = 16,
    SavingEntities = 17,
    SavingLocalAnimalPopulations = 18,
    SavingEvents = 19,
    SavingMandates = 20,
    SavingWorkQuotas = 21,
    SavingWorldEvents = 22,
    SavingCoinInformation = 23,
    SavingSquads = 24,
    SavingFormations = 25,
    SavingActivities = 26,
    SavingInteractions = 27,
    SavingWrittenContent = 28,
    SavingIdentities = 29,
    SavingIncidents = 30,
    SavingCrimes = 31,
    SavingVehicles = 32,
    SavingArmies = 33,
    SavingArmyControllers = 34,
    SavingTrackingInformation = 35,
    SavingCulturalIdentities = 36,
    SavingAgreement = 37,
    SavingArtForms = 38,
    SavingOccupations = 39,
    SavingBeliefSystems = 40,
    SavingImageSets = 41,
    SavingDivinationSets = 42,
    SavingAnnouncements = 43,
    SavingFortressInformation = 44,
    SavingWorldInformation = 45,
    SavingArtifacts = 46,
    SavingActiveHistoricalFigures = 47,
    SavingAdventureData = 48,
    SavingGeneralInformation = 49,
    ClosingFile = 50,
    Finishing = 51,
}
---@class viewscreen_savegamest
-- inherit viewscreen
---@field unk_1 string
---@field viewscreen_savegamest_cur_step viewscreen_savegamest_cur_step
---@field progress integer since v0.40.01
---@field offload nemesis_offload
---@field compressor file_compressorst
---@field unnamed_viewscreen_savegamest_6 string
---@field unnamed_viewscreen_savegamest_7 string
---@field unnamed_viewscreen_savegamest_8 string
---@field unnamed_viewscreen_savegamest_9 integer
df.viewscreen_savegamest = nil

---@enum adventurer_attribute_level
df.adventurer_attribute_level = {
    VeryLow = 0,
    Low = 1,
    BelowAverage = 2,
    Average = 3,
    AboveAverage = 4,
    High = 5,
    Superior = 6,
}

---@class startup_charactersheet_petst
---@field name language_name
---@field race integer
---@field caste integer
---@field type integer
df.startup_charactersheet_petst = nil

---@enum adv_background_option_type
df.adv_background_option_type = {
    NONE = -1,
    SQUAD_EPPID = 0,
    REGULAR_UNIT = 1,
}

---@enum setup_character_info_sub_mode
df.setup_character_info_sub_mode = {
    NONE = -1,
    RACE = 0,
    SUBRACE = 1,
    NEMESIS = 2,
    ENTITY = 3,
    DOING_SUB = 4,
    SUB_SKILLS = 5,
    SUB_APPEARANCE = 6,
    SUB_PERSONALITY = 7,
    SUB_BACKGROUND = 8,
    SUB_EQUIPMENT = 9,
    SUB_MOUNTS_AND_PETS = 10,
    FINAL_CONFIRMATION = 11,
}
---@enum setup_character_info_difficulty
df.setup_character_info_difficulty = {
    Peasant = 0,
    Hero = 1,
    Demigod = 2,
}
---@class setup_character_info startup_charactersheetst
---@field name language_name
---@field race integer
---@field caste integer
---@field skilllevel any[]
---@field quick_entity_id integer
---@field entity_population_id integer
---@field breed_id integer
---@field cultural_identity_id integer
---@field nemesis_index integer
---@field start_mil_type integer
---@field start_civ_type integer
---@field skill_picks_left integer
---@field phys_att_range_val any[]
---@field ment_att_range_val any[]
---@field setup_character_info_difficulty setup_character_info_difficulty
---@field start_site_id integer since v0.47.01
---@field background_start_squad_epp_id integer since v0.47.01
---@field background_unit profession since v0.47.01
---@field background_skill_bonus integer[] since v0.47.01
---@field worship_hfid integer
---@field worship_enid integer since v0.40.01
---@field worship_strength integer since v0.40.01
---@field pform unit_appearance
---@field birth_year integer
---@field birth_season_count integer
---@field age_death_year integer
---@field age_death_season_count integer
---@field pers unit_personality
---@field is_from_wilderpop_or_feature boolean
---@field flag integer
---@field setup_character_info_sub_mode setup_character_info_sub_mode
---@field visited_mode boolean[]
---@field selecting_atts boolean
---@field selected_att integer
---@field att_points integer
---@field posskill any[]
---@field selected_sk integer
---@field ip integer
---@field entering_name boolean
---@field old_name string
---@field background_text string[]
---@field goodsite world_site[]
---@field active_column integer
---@field background_option adv_background_option_type[]
---@field background_option_squad_epp_id any[]
---@field background_option_unit integer[] type should be profession?
---@field religious_practice_option integer[]
---@field religious_practice_id integer[]
---@field pos_caste integer[]
---@field st_selector integer
---@field bo_selector integer
---@field rp_selector integer
---@field background_desc string[]
---@field appearance_text string[]
---@field appearance_offscreen_randomized boolean
---@field appearance_was_fully_randomized boolean
---@field pers_scroll_y integer
---@field personal_values_text string[]
---@field personality_text string[]
---@field civ_values_text string[]
---@field doing_specific_personality boolean
---@field selected_specific_pers_item integer
---@field min_pers integer[]
---@field max_pers integer[]
---@field civ_value_level integer[]
---@field eqpet_points integer
---@field s_item item_actual[]
---@field selected_i integer
---@field etl embark_item_choice
---@field itype integer
---@field istype integer
---@field imat integer
---@field imatg integer
---@field item_desc string[]
---@field selected_pet_l integer
---@field selected_pet_r integer
---@field pet_side integer
---@field pet startup_charactersheet_petst[]
df.setup_character_info = nil

---@class embark_item_choice
---@field list any[]
---@field race integer[]
---@field caste integer[]
---@field profession any[]
df.embark_item_choice = nil

---@class embark_profile
---@field name string
---@field skill_type integer[]
---@field skill_dwarf_idx integer[]
---@field skill_level integer[]
---@field reclaim_dwarf_idx integer[]
---@field reclaim_prof1 profession[]
---@field reclaim_prof2 profession[]
---@field item_type integer[]
---@field item_subtype integer[]
---@field mat_type integer[]
---@field mat_index integer[]
---@field item_count integer[]
---@field pet_race integer[]
---@field pet_caste integer[]
---@field pet_profession profession[]
---@field pet_count integer[]
df.embark_profile = nil

---@class embark_symbol_unk_v43_sub9
---@field unk_s1 integer
---@field unk_s2 integer
---@field unk_s3 integer
---@field unk_s4 integer
---@field unk_s5 integer
---@field unk_s6 integer
---@field unk_s7 integer

---@class embark_symbol
---@field unk_v43_1 any[]
---@field unk_v43_2 any[]
---@field unk_v43_3 integer
---@field unk_v43_4 language_name
---@field unk_v43_sub9 embark_symbol_unk_v43_sub9
---@field unk_v43_10 integer[] uninitialized?
df.embark_symbol = nil

---@class viewscreen_setupdwarfgamest
-- inherited from viewscreen
---@field child viewscreen
---@field parent viewscreen
---@field breakdown_level interface_breakdown_types
---@field option_key_pressed integer
---@field widgets widget_container
-- end viewscreen
---@field title string
---@field dwarf_info setup_character_info[]
---@field embark_skills any[]
---@field reclaim_professions profession[]
---@field preparing_map_timer integer
---@field preparing_map_timer_quick_start boolean
---@field difficulty difficultyst
---@field doing_custom_settings boolean
---@field scroll_position_params integer
---@field scrolling_params boolean
---@field entering_value_str boolean
---@field entering_value_index integer
---@field value_str string
---@field member world_gen_param_basest[]
---@field mode integer
---@field selected_u integer
---@field scroll integer
---@field selected_i integer
---@field current_skill_tab integer
---@field scrolling_skill_list boolean
---@field selected_sk integer
---@field selected_pet integer
---@field side_u integer
---@field side_i integer
---@field y integer
---@field initial_selection integer
---@field embark_confirmation boolean
---@field scrolling_pet_list boolean
---@field chosen_pet_selected integer
---@field scrolling_chosen_pet_list boolean
---@field embark_profile_type integer[]
---@field embark_profile embark_profile[]
---@field scroll_position_initial_selection integer
---@field scrolling_initial_selection boolean
---@field objection string[]
---@field viewing_objections integer
---@field scroll_position_objections integer
---@field scrolling_objections boolean
---@field saving_profile integer
---@field profile_name string
---@field saving_profile_warning integer
---@field etl embark_item_choice
---@field s_item any[]
---@field item_expander_on boolean[]
---@field scroll_position_item integer
---@field current_category entity_sell_category
---@field scroll_position_category integer
---@field scroll_position_category_item integer
---@field scrolling_item boolean
---@field scrolling_category boolean
---@field scrolling_category_item boolean
---@field item_filter string
---@field entering_item_filter boolean
---@field availpetrace_num integer[]
---@field chosen_pet_index integer[]
---@field chosen_pet_num integer[]
---@field fort_name language_name
---@field group_name language_name
---@field update_header boolean
---@field start_symbol art_image
---@field si embark_location
---@field s_unit unit[]
---@field wagon_num integer
---@field points_remaining integer
---@field add_item_type item_type
---@field add_item_subtype integer
---@field add_mattype integer
---@field add_matindex integer
---@field adding_item integer
df.viewscreen_setupdwarfgamest = nil

---@class viewscreen_choose_game_typest
-- inherited from viewscreen
---@field child viewscreen
---@field parent viewscreen
---@field breakdown_level interface_breakdown_types
---@field option_key_pressed integer
---@field widgets widget_container
-- end viewscreen
---@field gametypes integer[]
df.viewscreen_choose_game_typest = nil

---@class viewscreen_titlest
-- inherited from viewscreen
---@field child viewscreen
---@field parent viewscreen
---@field breakdown_level interface_breakdown_types
---@field option_key_pressed integer
---@field widgets widget_container
-- end viewscreen
---@field str_histories string
---@field unnamed_viewscreen_titlest_2 string
---@field clean_first boolean
---@field mode integer
---@field selected integer
---@field selected_r integer
---@field game_start_proceed integer
---@field menu_line_id any[]
---@field gametype integer[]
---@field gametype_str string[]
---@field region_choice world_dat_summary[]
---@field scroll_position_region_choice integer
---@field scrolling_region_choice boolean
---@field savegame_header loadgame_save_info[]
---@field savegame_header_world loadgame_save_info[]
---@field scroll_position_world_choice integer
---@field scrolling_world_choice boolean
---@field savegame_header_game loadgame_save_info[]
---@field scroll_position_game_choice integer
---@field scrolling_game_choice boolean
---@field arena_choice string[]
---@field dungeon_choice string[]
---@field tutorial_choice string[]
---@field str_copyright string
---@field str_version string
---@field src_dir string
---@field stage_count integer
---@field game_start_arena boolean
---@field load_arena_stage integer
---@field game_start_tutorial boolean
---@field load_tutorial_stage integer
---@field game_start_dungeon boolean
---@field load_dungeon_stage integer
---@field managing_mods boolean
---@field mod mod_headerst[]
---@field scroll_position_mods integer
---@field scrolling_mods boolean
---@field hover_mod_description string[]
---@field last_hover_mod_id string
---@field last_hover_mod_version integer
---@field last_hover_width integer
---@field uploading_mods boolean
---@field scroll_position_upload_mods integer
---@field scrolling_upload_mods boolean
---@field hover_upload_mod_description string[]
---@field last_hover_upload_mod_id string
---@field last_hover_upload_mod_version integer
---@field last_hover_upload_width integer
---@field deleting_region boolean
---@field deleting_savegame_game boolean
---@field deleting_savegame_world boolean
---@field deleting_savegame_header integer
---@field deleting_region_header integer
---@field credit_line string[]
---@field credit_line_type integer[]
---@field scroll_position_about integer
---@field scrolling_about boolean
df.viewscreen_titlest = nil

---@class viewscreen_update_regionst
-- inherited from viewscreen
---@field child viewscreen
---@field parent viewscreen
---@field breakdown_level interface_breakdown_types
---@field option_key_pressed integer
---@field widgets widget_container
-- end viewscreen
---@field year integer
---@field year_tick integer
df.viewscreen_update_regionst = nil

---@class viewscreen_worldst_rumor_rpd_indicator_data rpd_indicator_datast
---@field unnamed_viewscreen_worldst_rumor_rpd_indicator_data_1 any[]
---@field unnamed_viewscreen_worldst_rumor_rpd_indicator_data_2 any[]
---@field unnamed_viewscreen_worldst_rumor_rpd_indicator_data_3 integer[]
---@field unnamed_viewscreen_worldst_rumor_rpd_indicator_data_4 integer[]
---@field unnamed_viewscreen_worldst_rumor_rpd_indicator_data_5 integer[]
---@field unnamed_viewscreen_worldst_rumor_rpd_indicator_data_6 integer[]
---@field unnamed_viewscreen_worldst_rumor_rpd_indicator_data_7 integer
---@field unnamed_viewscreen_worldst_rumor_rpd_indicator_data_8 any[]
---@field unnamed_viewscreen_worldst_rumor_rpd_indicator_data_9 integer[]
---@field unnamed_viewscreen_worldst_rumor_rpd_indicator_data_10 integer[]
---@field unnamed_viewscreen_worldst_rumor_rpd_indicator_data_11 integer[]
---@field unnamed_viewscreen_worldst_rumor_rpd_indicator_data_12 integer[]
---@field unnamed_viewscreen_worldst_rumor_rpd_indicator_data_13 integer

---@class viewscreen_worldst_rumor_rpd region_print_datast
---@field unnamed_viewscreen_worldst_rumor_rpd_1 integer
---@field unnamed_viewscreen_worldst_rumor_rpd_2 integer
---@field unnamed_viewscreen_worldst_rumor_rpd_3 integer
---@field unnamed_viewscreen_worldst_rumor_rpd_4 integer
---@field unnamed_viewscreen_worldst_rumor_rpd_5 integer
---@field unnamed_viewscreen_worldst_rumor_rpd_6 integer
---@field unnamed_viewscreen_worldst_rumor_rpd_7 integer
---@field unnamed_viewscreen_worldst_rumor_rpd_8 integer
---@field unnamed_viewscreen_worldst_rumor_rpd_9 integer[]
---@field unnamed_viewscreen_worldst_rumor_rpd_10 integer
---@field unnamed_viewscreen_worldst_rumor_rpd_11 integer[]
---@field unnamed_viewscreen_worldst_rumor_rpd_12 integer
---@field unnamed_viewscreen_worldst_rumor_rpd_13 integer
---@field unnamed_viewscreen_worldst_rumor_rpd_14 integer
---@field unnamed_viewscreen_worldst_rumor_rpd_15 integer
---@field unnamed_viewscreen_worldst_rumor_rpd_16 integer[]
---@field unnamed_viewscreen_worldst_rumor_rpd_17 integer
---@field unnamed_viewscreen_worldst_rumor_rpd_18 integer
---@field unnamed_viewscreen_worldst_rumor_rpd_19 integer

---@class viewscreen_worldst
-- inherited from viewscreen
---@field child viewscreen
---@field parent viewscreen
---@field breakdown_level interface_breakdown_types
---@field option_key_pressed integer
---@field widgets widget_container
-- end viewscreen
---@field region_cent_x integer
---@field region_cent_y integer
---@field mouse_scrolling_map boolean
---@field mouse_anchor_mx integer
---@field mouse_anchor_my integer
---@field mouse_anchor_pmx integer
---@field mouse_anchor_pmy integer
---@field view_mode integer
---@field military_goals_hf historical_figure
---@field meet_workers_hf historical_figure
---@field focus_ax integer
---@field focus_ay integer
---@field focus_site world_site
---@field focus_site_artifact artifact_record[]
---@field focus_site_prisoner historical_figure[]
---@field focus_site_messenger_candidate boolean
---@field focus_site_requestable_worker nemesis_record[]
---@field civlist historical_entity[]
---@field last_hover_ent historical_entity
---@field relnem nemesis_record[]
---@field relnem_precedence integer[]
---@field relag any[] civ_agreementst
---@field relag_pending integer[]
---@field scroll_position_civlist integer
---@field scrolling_civlist boolean
---@field army_controller army_controller[]
---@field last_hover_ac army_controller
---@field selected_ac integer
---@field scrolling_ac boolean
---@field scroll_position_ac integer
---@field squad squad[]
---@field squad_flag integer[]
---@field messenger_epp entity_position_assignment[]
---@field messenger_ent historical_entity[]
---@field messenger_flag integer[]
---@field scroll_position_squad integer
---@field scrolling_squad boolean
---@field scroll_position_messenger integer
---@field scrolling_messenger boolean
---@field request_nem nemesis_record[]
---@field scroll_position_request_nem integer
---@field scrolling_request_nem boolean
---@field rumor_master any[]
---@field rumor_rpd viewscreen_worldst_rumor_rpd region_print_datast
---@field rumor_rpd_indicator_data viewscreen_worldst_rumor_rpd_indicator_data rpd_indicator_datast
---@field last_hover_rumor_x integer
---@field last_hover_rumor_y integer
---@field focused_on_last_hover_rumor boolean
---@field rumor_text string[]
---@field scroll_position_rumor integer
---@field scrolling_rumor boolean
---@field mission_report_index integer[]
---@field tribute_report_index integer[]
---@field croll_position_report integer
---@field scrolling_report boolean
---@field active_mission_report integer mission_reportst
---@field mission_cursor_x integer
---@field mission_cursor_y integer
---@field mission_path_data_index integer
---@field mission_path_data_path_index integer
---@field mission_heid_data_index integer
---@field mission_heid_data_heid_index integer
---@field mission_text_box string[]
---@field mission_text_box_color integer[]
---@field mission_timer_year integer
---@field mission_timer_season_count integer
---@field mission_timer_season_count_inc integer
---@field report_paused boolean
---@field mission_fade_in_timer integer
---@field mission_fade_start_ind integer
---@field scroll_position_mission integer
---@field scrolling_mission boolean
---@field active_tribute_report integer tribute_reportst
---@field scroll_position_tribute integer
---@field scrolling_tribute boolean
---@field hf historical_figure[]
---@field scroll_position_citizens integer
---@field scrolling_citizens boolean
---@field last_hover_hf historical_figure
---@field artifact artifact_record[]
---@field artifact_arl any[] artifact_rumor_locationst
---@field scroll_position_artifacts integer
---@field scrolling_artifacts boolean
---@field last_hover_artifact artifact_record
---@field artifact_description string[]
---@field artifact_eac integer entity_artifact_claimst
---@field artifact_rpa_holder historical_figure
---@field artifact_fac_holder historical_figure
df.viewscreen_worldst = nil

---@class viewscreen_new_arenast
-- inherited from viewscreen
---@field child viewscreen
---@field parent viewscreen
---@field breakdown_level interface_breakdown_types
---@field option_key_pressed integer
---@field widgets widget_container
-- end viewscreen
---@field unk_88 integer
---@field progress integer
---@field cur_step integer
---@field unk_94 integer
---@field unk_98 integer
---@field unk_9c integer
---@field unk_a0 integer
---@field unk_a4 integer
---@field unk_a8 integer
---@field unk_ac integer
---@field unk_b0 string[]
---@field unk_c8 integer[]
---@field unk_e0 integer[]
---@field unk_f8 string[]
---@field unk_110 string[]
---@field unk_128 string[]
---@field unk_mods mod_headerst[]
---@field unk_158 string[]
---@field unk_170 integer[]
---@field unk_188 integer[]
---@field unk_1a0 string[]
---@field unk_1b8 string[]
---@field unk_1d0 string[]
---@field unk_mods2 mod_headerst[]
---@field unk_200 string[]
---@field unk_218 integer[]
---@field unk_230 integer[]
---@field unk_248 string[]
---@field unk_260 string[]
---@field unk_278 any[]
---@field unk_290 any[]
---@field unk_2a8 string[]
---@field unk_2c0 string
---@field unk_2e0 integer
---@field unk_2e4 integer
df.viewscreen_new_arenast = nil


