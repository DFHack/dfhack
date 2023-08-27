-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.

---@meta

---@enum weather_type
df.weather_type = {
    None = 0,
    Rain = 1,
    Snow = 2,
}

---@enum next_global_id
df.next_global_id = {
    unit = 0,
    soul = 1,
    item = 2,
    civ = 3,
    nem = 4,
    artifact = 5,
    job = 6,
    schedule = 7,
    proj = 8,
    building = 9,
    machine = 10,
    flow_guide = 11,
    histfig = 12,
    histevent = 13,
    histeventcol = 14,
    unitchunk = 15,
    imagechunk = 16,
    task = 17,
    squad = 18,
    formation = 19,
    activity = 20,
    interaction_instance = 21,
    written_content = 22,
    identity = 23,
    incident = 24,
    crime = 25,
    vehicle = 26,
    army = 27,
    army_controller = 28,
    army_tracking_info = 29,
    cultural_identity = 30,
    agreement = 31,
    poetic_form = 32,
    musical_form = 33,
    dance_form = 34,
    scale = 35,
    rhythm = 36,
    occupation = 37,
    belief_system = 38,
    image_set = 39,
    divination_set = 40,
}

---@class global_table_entry
---@field name string
---@field address integer
---@field size integer
df.global_table_entry = nil

---@type global_table_entry[]
df.global.global_table = nil

---@type integer
df.global.cursor = nil

---@type integer
df.global.selection_rect = nil

---@enum game_mode
df.game_mode = {
    DWARF = 0,
    ADVENTURE = 1,
    num = 2,
    NONE = 3,
}

---@enum game_type
df.game_type = {
    DWARF_MAIN = 0,
    ADVENTURE_MAIN = 1,
    VIEW_LEGENDS = 2,
    DWARF_RECLAIM = 3,
    DWARF_ARENA = 4,
    ADVENTURE_ARENA = 5,
    ADVENTURE_DUNGEON = 6,
    DWARF_TUTORIAL = 7,
    DWARF_UNRETIRE = 8,
    ADVENTURE_WORLD_DEBUG = 9,
    num = 10,
    NONE = 11,
}

---@type game_mode
df.global.gamemode = nil

---@type game_type
df.global.gametype = nil

---@type integer[]
df.global.ui_menu_width = nil

---@type any[]
df.global.created_item_type = nil

---@type any[]
df.global.created_item_subtype = nil

---@type any[]
df.global.created_item_mattype = nil

---@type integer[]
df.global.created_item_matindex = nil

---@type integer[]
df.global.created_item_count = nil

---@type map_renderer
df.global.map_renderer = nil

---@type d_init
df.global.d_init = nil

---@type flow_info[]
df.global.flows = nil

---@type enabler
df.global.enabler = nil

---@type graphic
df.global.gps = nil

---@type interfacest
df.global.gview = nil

---@type init
df.global.init = nil

---@type texture_handlerst
df.global.texture = nil

---@type timed_event[]
df.global.timed_events = nil

---@type plotinfost
df.global.plotinfo = nil

---@type adventurest
df.global.adventure = nil

---@type buildreq
df.global.buildreq = nil

---@type integer[]
df.global.ui_building_assign_type = nil

---@type boolean[]
df.global.ui_building_assign_is_marked = nil

---@type unit[]
df.global.ui_building_assign_units = nil

---@type item[]
df.global.ui_building_assign_items = nil

---@type ui_look_list
df.global.ui_look_list = nil

---@type gamest
df.global.game = nil

---@type world
df.global.world = nil

---@type save_version
df.global.version = nil

---@type save_version
df.global.min_load_version = nil

---@type cmv_version
df.global.movie_version = nil

---@type integer
df.global.activity_next_id = nil

---@type integer
df.global.agreement_next_id = nil

---@type integer
df.global.army_controller_next_id = nil

---@type integer
df.global.army_next_id = nil

---@type integer
df.global.army_tracking_info_next_id = nil

---@type integer
df.global.art_image_chunk_next_id = nil

---@type integer
df.global.artifact_next_id = nil

---@type integer
df.global.belief_system_next_id = nil

---@type integer
df.global.building_next_id = nil

---@type integer
df.global.crime_next_id = nil

---@type integer
df.global.cultural_identity_next_id = nil

---@type integer
df.global.dance_form_next_id = nil

---@type integer
df.global.divination_set_next_id = nil

---@type integer
df.global.entity_next_id = nil

---@type integer
df.global.flow_guide_next_id = nil

---@type integer
df.global.formation_next_id = nil

---@type integer
df.global.hist_event_collection_next_id = nil

---@type integer
df.global.hist_event_next_id = nil

---@type integer
df.global.hist_figure_next_id = nil

---@type integer
df.global.identity_next_id = nil

---@type integer
df.global.image_set_next_id = nil

---@type integer
df.global.incident_next_id = nil

---@type integer
df.global.interaction_instance_next_id = nil

---@type integer
df.global.item_next_id = nil

---@type integer
df.global.job_next_id = nil

---@type integer
df.global.machine_next_id = nil

---@type integer
df.global.musical_form_next_id = nil

---@type integer
df.global.nemesis_next_id = nil

---@type integer
df.global.occupation_next_id = nil

---@type integer
df.global.poetic_form_next_id = nil

---@type integer
df.global.proj_next_id = nil

---@type integer
df.global.rhythm_next_id = nil

---@type integer
df.global.scale_next_id = nil

---@type integer
df.global.schedule_next_id = nil

---@type integer
df.global.soul_next_id = nil

---@type integer
df.global.squad_next_id = nil

---@type integer
df.global.task_next_id = nil

---@type integer
df.global.unit_chunk_next_id = nil

---@type integer
df.global.unit_next_id = nil

---@type integer
df.global.vehicle_next_id = nil

---@type integer
df.global.written_content_next_id = nil

---@type integer
df.global.cur_year = nil

---@type integer
df.global.cur_year_tick = nil

---@type integer
df.global.cur_year_tick_advmode = nil

---@type season
df.global.cur_season = nil

---@type integer
df.global.cur_season_tick = nil

---@type any[]
df.global.current_weather = nil

---@type boolean
df.global.pause_state = nil

---@type boolean Requests dig designations to be processed next frame.
df.global.process_dig = nil

---@type boolean Requests building jobs to be processed next frame.
df.global.process_jobs = nil

---@type boolean
df.global.ui_building_in_assign = nil

---@type boolean
df.global.ui_building_in_resize = nil

---@type integer
df.global.ui_building_resize_radius = nil

---@type integer
df.global.ui_building_item_cursor = nil

---@type integer
df.global.ui_look_cursor = nil

---@type integer
df.global.ui_selected_unit = nil

---@type ui_unit_view_mode
df.global.ui_unit_view_mode = nil

---@type boolean
df.global.ui_workshop_in_add = nil

---@type integer
df.global.ui_workshop_job_cursor = nil

---@enum lever_target_type
df.lever_target_type = {
    NONE = -1,
    BarsVertical = 66,
    BarsFloor = 70,
    SpearsSpikes = 83,
    TrackStop = 84,
    GearAssembly = 97,
    Bridge = 98,
    Chain = 99,
    Door = 100,
    EncrustGems = 101,
    Floodgate = 102,
    GrateFloor = 103,
    Hatch = 104,
    Cage = 106,
    LeverMechanism = 108, -- use in lever
    Support = 115,
    TargetMechanism = 116, -- use in target
    GrateWall = 119,
}

---@type lever_target_type
df.global.ui_lever_target_type = nil

---@type integer
df.global.window_x = nil

---@type integer
df.global.window_y = nil

---@type integer
df.global.window_z = nil

---@type boolean Prevents the game from being paused
df.global.debug_nopause = nil

---@type boolean Same as ARTIFACTS:NO
df.global.debug_nomoods = nil

---@type boolean Functionality unknown, combat-related
df.global.debug_combat = nil

---@type boolean Functionality unknown, wildlife-related
df.global.debug_wildlife = nil

---@type boolean Disables thirst on everything
df.global.debug_nodrink = nil

---@type boolean Disables hunger on everything
df.global.debug_noeat = nil

---@type boolean Disables drowsiness on everything
df.global.debug_nosleep = nil

---@type boolean Makes hidden ambushers visible on-screen and in the units list (but not to your citizens)
df.global.debug_showambush = nil

---@type boolean All dwarves mine as fast as a Legendary Miner
df.global.debug_fastmining = nil

---@type boolean Insanity can only result in Crazed or Melancholy, never Berserk
df.global.debug_noberserk = nil

---@type boolean All units move and work at maximum speed
df.global.debug_turbospeed = nil

---@type boolean Ending the game saves its state back to world.dat or world.sav
df.global.save_on_exit = nil

---@type integer
df.global.standing_orders_gather_minerals = nil

---@type integer
df.global.standing_orders_gather_wood = nil

---@type integer
df.global.standing_orders_gather_food = nil

---@type integer
df.global.standing_orders_gather_bodies = nil

---@type integer
df.global.standing_orders_gather_animals = nil

---@type integer
df.global.standing_orders_gather_furniture = nil

---@type integer
df.global.standing_orders_farmer_harvest = nil

---@type integer
df.global.standing_orders_job_cancel_announce = nil

---@type integer
df.global.standing_orders_mix_food = nil

---@type integer
df.global.standing_orders_gather_refuse = nil

---@type integer
df.global.standing_orders_gather_refuse_outside = nil

---@type integer
df.global.standing_orders_gather_vermin_remains = nil

---@type integer
df.global.standing_orders_dump_corpses = nil

---@type integer
df.global.standing_orders_dump_skulls = nil

---@type integer
df.global.standing_orders_dump_skins = nil

---@type integer
df.global.standing_orders_dump_bones = nil

---@type integer
df.global.standing_orders_dump_hair = nil

---@type integer
df.global.standing_orders_dump_shells = nil

---@type integer
df.global.standing_orders_dump_other = nil

---@type integer
df.global.standing_orders_forbid_used_ammo = nil

---@type integer
df.global.standing_orders_forbid_other_dead_items = nil

---@type integer
df.global.standing_orders_forbid_own_dead = nil

---@type integer
df.global.standing_orders_forbid_other_nohunt = nil

---@type integer
df.global.standing_orders_forbid_own_dead_items = nil

---@type integer
df.global.standing_orders_auto_loom = nil

---@type integer
df.global.standing_orders_auto_collect_webs = nil

---@type integer
df.global.standing_orders_auto_slaughter = nil

---@type integer
df.global.standing_orders_auto_butcher = nil

---@type integer
df.global.standing_orders_auto_tan = nil

---@type integer
df.global.standing_orders_auto_fishery = nil

---@type integer
df.global.standing_orders_auto_kitchen = nil

---@type integer
df.global.standing_orders_auto_kiln = nil

---@type integer
df.global.standing_orders_auto_smelter = nil

---@type integer
df.global.standing_orders_auto_other = nil

---@type integer
df.global.standing_orders_use_dyed_cloth = nil

---@type integer
df.global.standing_orders_zoneonly_drink = nil

---@type integer
df.global.standing_orders_zoneonly_fish = nil

---@type integer
df.global.cur_snow_counter = nil

---@type integer
df.global.cur_rain_counter = nil

---@type integer
df.global.weathertimer = nil

---@type coord[]
df.global.cur_snow = nil

---@type coord[]
df.global.cur_rain = nil

---@type integer[]
df.global.jobvalue = nil

---@type unit[]
df.global.jobvalue_setter = nil

---@type item
df.global.interactitem = nil

---@type unit_inventory_item
df.global.interactinvslot = nil

---@type boolean
df.global.handleannounce = nil

---@type boolean
df.global.preserveannounce = nil

---@type boolean
df.global.updatelightstate = nil


