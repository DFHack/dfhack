-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.

---@meta

---@class ui_build_item_req
---@field filter job_item_filter
---@field candidates item[]
---@field candidate_selected boolean[]
---@field unk_a0 integer[]
---@field candidate_enabled boolean[]
---@field count_required integer
---@field count_max integer if 0, fixed at required
---@field count_provided integer
df.ui_build_item_req = nil

---@enum build_req_choice_type
df.build_req_choice_type = {
    General = 0,
    Specific = 1,
}

---@return build_req_choice_type
function df.build_req_choicest.getType() end

---@param str string
function df.build_req_choicest.getName(str) end

---@return integer
function df.build_req_choicest.unnamed_method() end

---@param item_id integer
---@return boolean
function df.build_req_choicest.isCandidate(item_id) end

---@return boolean
function df.build_req_choicest.unnamed_method() end

---@return integer
function df.build_req_choicest.getUsedCount() end

---@return integer
function df.build_req_choicest.getNumCandidates() end

---@return boolean
function df.build_req_choicest.unnamed_method() end

---@return boolean
function df.build_req_choicest.unnamed_method() end -- get_unk1

function df.build_req_choicest.unnamed_method() end -- set_unk1

function df.build_req_choicest.unnamed_method() end -- clear_unk1

---@class build_req_choicest
---@field distance integer
---@field unnamed_build_req_choicest_3 boolean
---@field unnamed_build_req_choicest_4 string
df.build_req_choicest = nil

---@class build_req_choice_genst
-- inherited from build_req_choicest
---@field distance integer
---@field unnamed_build_req_choicest_3 boolean
---@field unnamed_build_req_choicest_4 string
-- end build_req_choicest
---@field item_type item_type
---@field item_subtype integer
---@field mat_type integer
---@field mat_index integer
---@field candidates any[]
---@field used_count integer
---@field unk_1 boolean
df.build_req_choice_genst = nil

---@class build_req_choice_specst
-- inherited from build_req_choicest
---@field distance integer
---@field unnamed_build_req_choicest_3 boolean
---@field unnamed_build_req_choicest_4 string
-- end build_req_choicest
---@field candidate item
---@field candidate_id integer
df.build_req_choice_specst = nil

function df.global.buildreq.unnamed_method() end

function df.global.buildreq.unnamed_method() end

function df.global.buildreq.unnamed_method() end

---@class buildreq
---@field requirements ui_build_item_req[]
---@field choices build_req_choicest[]
---@field building_type building_type if -1, in Build menu; otherwise select item
---@field building_subtype integer
---@field custom_type integer
---@field stage integer 0 no materials, 1 place, 2 select item
---@field req_index integer
---@field sel_index integer
---@field is_grouped integer
---@field errors string[]
---@field unk4 string[]
---@field tiles any[]
---@field cur_walk_tag integer
---@field plate_info pressure_plate_info
---@field min_weight_races integer[]
---@field max_weight_races integer[]
---@field unnamed_buildreq_17 integer[]
---@field unnamed_buildreq_18 integer[]
---@field unnamed_buildreq_19 integer[]
---@field unnamed_buildreq_20 integer
---@field unnamed_buildreq_21 integer
---@field friction integer since v0.34.08
---@field use_dump integer since v0.34.08
---@field dump_x_shift integer since v0.34.08
---@field dump_y_shift integer since v0.34.08
---@field speed integer since v0.34.08
---@field pos coord since v0.50.01
---@field direction integer since v0.50.01
---@field unnamed_buildreq_29 integer
---@field selection_pos coord since v0.50.01
---@field selection_area integer since v0.50.01
---@field unnamed_buildreq_32 integer
---@field unnamed_buildreq_33 integer[] since v0.50.01
df.global.buildreq = nil

---@enum interface_category_building
df.interface_category_building = {
    NONE = -1,
    WEAPON = 0,
    ARMOR = 1,
    FURNITURE = 2,
    SIEGE = 3,
    TRAP = 4,
    OTHER = 5,
    METAL = 6,
    SELECT_MEMORIAL_UNIT = 7,
}

---@enum interface_category_construction
df.interface_category_construction = {
    NONE = -1,
    MAIN = 0,
    SIEGEENGINE = 1,
    TRAP = 2,
    WORKSHOP = 3,
    FURNACE = 4,
    CONSTRUCTION = 5,
    MACHINE = 6,
    TRACK = 7,
}

---@param y integer
---@param limx_min integer
---@param limx_max integer
function df.interface_button.print_info(y, limx_min, limx_max) end -- ghost, buried, memorialized

---@param str string
function df.interface_button.text(str) end

function df.interface_button.press() end

---@param selected boolean
function df.interface_button.set_button_color(selected) end

function df.interface_button.set_leave_button() end

---@return integer
function df.interface_button.tile() end

function df.interface_button.set_tile_color() end

---@param box integer
function df.interface_button.prepare_tool_tip(box) end

---@return boolean
function df.interface_button.pressable() end

---@return boolean
function df.interface_button.has_view() end

---@return boolean
function df.interface_button.is_alphabetized() end

---@return string
function df.interface_button.get_objection_string() end

---@return string
function df.interface_button.get_info_string() end

---@class interface_button
---@field hotkey interface_key
---@field leave_button boolean
---@field flag integer since v0.40.23
---@field filter_str string
df.interface_button = nil

---@class interface_button_buildingst
-- inherited from interface_button
---@field hotkey interface_key
---@field leave_button boolean
---@field flag integer since v0.40.23
---@field filter_str string
-- end interface_button
---@field bd building
df.interface_button_buildingst = nil

---@class interface_button_building_category_selectorst
-- inherited from interface_button_buildingst
-- inherited from interface_button
---@field hotkey interface_key
---@field leave_button boolean
---@field flag integer since v0.40.23
---@field filter_str string
-- end interface_button
---@field bd building
-- end interface_button_buildingst
---@field category interface_category_building
---@field prepare_interface integer
df.interface_button_building_category_selectorst = nil

---@class interface_button_building_material_selectorst
-- inherited from interface_button_buildingst
-- inherited from interface_button
---@field hotkey interface_key
---@field leave_button boolean
---@field flag integer since v0.40.23
---@field filter_str string
-- end interface_button
---@field bd building
-- end interface_button_buildingst
---@field material integer
---@field matgloss integer
---@field job_item_flag bitfield
---@field prepare_interface integer
df.interface_button_building_material_selectorst = nil

---@class interface_button_building_new_jobst
-- inherited from interface_button_buildingst
-- inherited from interface_button
---@field hotkey interface_key
---@field leave_button boolean
---@field flag integer since v0.40.23
---@field filter_str string
-- end interface_button
---@field bd building
-- end interface_button_buildingst
---@field jobtype job_type
---@field mstring string
---@field itemtype item_type
---@field subtype integer
---@field material integer
---@field matgloss integer
---@field specflag bitfield
---@field spec_id integer refers to various things, such as histfigs OR races
---@field job_item_flag bitfield
---@field add_building_location boolean
---@field show_help_instead boolean
---@field objection string
---@field info string
df.interface_button_building_new_jobst = nil

---@class interface_button_building_custom_category_selectorst
-- inherited from interface_button_buildingst
-- inherited from interface_button
---@field hotkey interface_key
---@field leave_button boolean
---@field flag integer since v0.40.23
---@field filter_str string
-- end interface_button
---@field bd building
-- end interface_button_buildingst
---@field custom_category_token string
df.interface_button_building_custom_category_selectorst = nil

---@enum construction_category_type
df.construction_category_type = {
    NONE = -1,
    MAIN = 0,
    WORKSHOPS = 1,
    WORKSHOPS_FURNACES = 2,
    WORKSHOPS_CLOTHING = 3,
    WORKSHOPS_FARMING = 4,
    FURNITURE = 5,
    DOORS_HATCHES = 6,
    WALLS_FLOORS = 7,
    MACHINES_FLUIDS = 8,
    CAGES_RESTRAINTS = 9,
    TRAPS = 10,
    MILITARY = 11,
}

---@class bb_buttonst
---@field category construction_category_type
---@field type integer
---@field subtype integer
---@field custom_building_id integer
---@field number integer
---@field grid_height integer
---@field texpos integer
---@field str string
---@field hotkey interface_key
df.bb_buttonst = nil

---@enum construction_interface_page_status_type
df.construction_interface_page_status_type = {
    NONE = -1,
    FULL = 0,
    ICONS_ONLY = 1,
    OFF = 2,
}

---@class construction_interface_pagest
---@field category construction_category_type
---@field bb_button bb_buttonst[]
---@field last_main_sx integer
---@field last_main_ex integer
---@field last_main_sy integer
---@field last_main_ey integer
---@field page_status construction_interface_page_status_type
---@field number_of_columns integer
---@field column_height integer
---@field column_width integer
---@field selected_button bb_buttonst
---@field scrolling boolean
---@field scroll_position integer
df.construction_interface_pagest = nil

---@enum room_flow_shape_type
df.room_flow_shape_type = {
    NONE = -1,
    RECTANGLE = 0,
    WALL_FLOW = 1,
    FLOOR_FLOW = 2,
}

---@enum cannot_expel_reason_type
df.cannot_expel_reason_type = {
    NONE = -1,
    HEREDITARY = 0,
    ELECTED = 1,
    MEET_WORKERS = 2,
    SPOUSE_NOT_PRESENT = 3,
    SPOUSE_HEREDITARY = 4,
    SPOUSE_ELECTED = 5,
    SPOUSE_MEET_WORKERS = 6,
    CHILD_NOT_PRESENT = 7,
    CHILD_HEREDITARY = 8,
    CHILD_ELECTED = 9,
    CHILD_MEET_WORKERS = 10,
}

---@enum mine_mode_type
df.mine_mode_type = {
    NONE = -1,
    ALL = 0,
    AUTOMINE_NON_LAYER_MATERIAL = 1,
    MARK_ECONOMIC_ONLY = 2,
    MARK_GEMS_ONLY = 3,
}

---@enum job_details_option_type
df.job_details_option_type = {
    NONE = -1,
    MATERIAL = 0,
    IMAGE = 1,
    CLOTHING_SIZE = 2,
    IMPROVEMENT_TYPE = 3,
}

---@enum job_details_context_type
df.job_details_context_type = {
    NONE = -1,
    BUILDING_TASK_LIST = 0,
    CREATURES_LIST_TASK = 1,
    TASK_LIST_TASK = 2,
    BUILDING_WORK_ORDER = 3,
    MANAGER_WORK_ORDER = 4,
}

---@enum stock_pile_pointer_type
df.stock_pile_pointer_type = {
    NONE = -1,
    ANIMAL_EMPTY_CAGES = 0,
    ANIMAL_EMPTY_ANIMAL_TRAPS = 1,
    FOOD_PREPARED_FOOD = 2,
    REFUSE_ROTTEN_RAW_HIDE = 3,
    REFUSE_UNROTTEN_RAW_HIDE = 4,
    WEAPON_USABLE = 5,
    WEAPON_NON_USABLE = 6,
    ARMOR_USABLE = 7,
    ARMOR_NON_USABLE = 8,
}

---@enum stockpile_tools_context_type
df.stockpile_tools_context_type = {
    NONE = -1,
    STOCKPILE = 0,
}

---@enum stockpile_link_context_type
df.stockpile_link_context_type = {
    NONE = -1,
    STOCKPILE = 0,
    WORKSHOP = 1,
    HAULING_STOP = 2,
}

---@enum hauling_stop_conditions_context_type
df.hauling_stop_conditions_context_type = {
    NONE = -1,
    HAULING_MENU = 0,
}

---@enum assign_vehicle_context_type
df.assign_vehicle_context_type = {
    NONE = -1,
    HAULING_MENU = 0,
}

---@enum location_details_context_type
df.location_details_context_type = {
    NONE = -1,
    FROM_ZONE = 0,
    FROM_LOCATION_SELECTOR = 1,
}

---@enum location_selector_context_type
df.location_selector_context_type = {
    NONE = -1,
    ZONE_MEETING_AREA_ASSIGNMENT = 0,
}

---@enum custom_symbol_context_type
df.custom_symbol_context_type = {
    NONE = -1,
    BURROW = 0,
    BURROW_PAINT = 1,
    WORK_DETAIL = 2,
    SQUAD_MENU = 3,
}

---@enum name_creator_context_type
df.name_creator_context_type = {
    NONE = -1,
    EMBARK_FORT_NAME = 0,
    EMBARK_GROUP_NAME = 1,
    IMAGE_CREATOR_NAME = 2,
    LOCATION_NAME = 3,
    SQUAD_NAME = 4,
    INFO_NOBLES_ELEVATING_POSITION_SYMBOL = 5,
}

---@enum image_creator_context_type
df.image_creator_context_type = {
    NONE = -1,
    EMBARK_FORT_SYMBOL = 0,
    JOB_DETAILS_MAIN = 1,
    JOB_DETAILS_IMPROVEMENT = 2,
    DESIGNATION_ENGRAVING = 3,
}

---@enum image_creator_option_type
df.image_creator_option_type = {
    NONE = -1,
    ALLOW_ARTIST_TO_CHOOSE = 0,
    RELATED_TO_HFID = 1,
    RELATED_TO_STID = 2,
    RELATED_TO_ENID = 3,
    RELATED_TO_HEID = 4,
    EXISTING_IMAGE = 5,
    NEW_IMAGE = 6,
    NEW_IMAGE_ELEMENT_CREATURE = 7,
    NEW_IMAGE_ELEMENT_HF = 8,
    NEW_IMAGE_ELEMENT_PLANT = 9,
    NEW_IMAGE_ELEMENT_TREE = 10,
    NEW_IMAGE_ELEMENT_SHAPE = 11,
    NEW_IMAGE_ELEMENT_ITEM = 12,
    NEW_IMAGE_ELEMENT_ARTIFACT = 13,
    NEW_IMAGE_PROPERTY = 14,
    NEW_IMAGE_PROPERTY_ACTOR = 15,
    NEW_IMAGE_PROPERTY_TARGET = 16,
    NEW_IMAGE_DELETE_ELEMENTS = 17,
}

---@enum unit_selector_context_type
df.unit_selector_context_type = {
    NONE = -1,
    ZONE_PEN_ASSIGNMENT = 0,
    ZONE_PIT_ASSIGNMENT = 1,
    ZONE_BEDROOM_ASSIGNMENT = 2,
    ZONE_OFFICE_ASSIGNMENT = 3,
    ZONE_DINING_HALL_ASSIGNMENT = 4,
    ZONE_TOMB_ASSIGNMENT = 5,
    CHAIN_ASSIGNMENT = 6,
    CAGE_ASSIGNMENT = 7,
    WORKER_ASSIGNMENT = 8,
    OCCUPATION_ASSIGNMENT = 9,
    BURROW_ASSIGNMENT = 10,
    SQUAD_KILL_ORDER = 11,
    SQUAD_FILL_POSITION = 12,
}

---@enum squad_selector_context_type
df.squad_selector_context_type = {
    NONE = -1,
    ZONE_BARRACKS_ASSIGNMENT = 0,
    ZONE_ARCHERY_RANGE_ASSIGNMENT = 1,
}

---@enum squad_schedule_context_type
df.squad_schedule_context_type = {
    NONE = -1,
    FROM_SQUAD_MENU = 0,
}

---@enum squad_equipment_context_type
df.squad_equipment_context_type = {
    NONE = -1,
    FROM_SQUAD_MENU = 0,
}

---@enum patrol_routes_context_type
df.patrol_routes_context_type = {
    NONE = -1,
    GIVING_SQUAD_PATROL_ORDER = 0,
}

---@enum burrow_selector_context_type
df.burrow_selector_context_type = {
    NONE = -1,
    GIVING_SQUAD_ORDER = 0,
}

---@enum view_sheet_trait_type
df.view_sheet_trait_type = {
    NONE = -1,
    PHYS_ATT_PLUS = 0,
    PHYS_ATT_MINUS = 1,
    MENT_ATT_PLUS = 2,
    MENT_ATT_MINUS = 3,
    PERSONALITY_FACET_HIGH = 4,
    PERSONALITY_FACET_LOW = 5,
    VALUE_HIGH = 6,
    VALUE_LOW = 7,
}

---@enum view_sheet_unit_knowledge_type
df.view_sheet_unit_knowledge_type = {
    NONE = -1,
    PHILOSOPHY_FLAG = 0,
    PHILOSOPHY_FLAG2 = 1,
    MATHEMATICS_FLAG = 2,
    MATHEMATICS_FLAG2 = 3,
    HISTORY_FLAG = 4,
    ASTRONOMY_FLAG = 5,
    NATURALIST_FLAG = 6,
    CHEMISTRY_FLAG = 7,
    GEOGRAPHY_FLAG = 8,
    MEDICINE_FLAG = 9,
    MEDICINE_FLAG2 = 10,
    MEDICINE_FLAG3 = 11,
    ENGINEERING_FLAG = 12,
    ENGINEERING_FLAG2 = 13,
    POETIC_FORM = 14,
    MUSICAL_FORM = 15,
    DANCE_FORM = 16,
    WRITTEN_CONTENT = 17,
}

---@enum view_sheets_context_type
df.view_sheets_context_type = {
    NONE = -1,
    REGULAR_PLAY = 0,
    PREPARE_CAREFULLY = 1,
}

---@enum view_sheet_type
df.view_sheet_type = {
    NONE = -1,
    UNIT = 0,
    ITEM = 1,
    BUILDING = 2,
    ENGRAVING = 3,
    ENGRAVING_PLANNED = 4,
    UNIT_LIST = 5,
    ITEM_LIST = 6,
}

---@enum unit_list_mode_type
df.unit_list_mode_type = {
    NONE = -1,
    CITIZEN = 0,
    PET = 1,
    OTHER = 2,
    DECEASED = 3,
}

---@enum buildings_mode_type
df.buildings_mode_type = {
    NONE = -1,
    ZONES = 0,
    LOCATIONS = 1,
    STOCKPILES = 2,
    WORKSHOPS = 3,
    FARMPLOTS = 4,
}

---@enum kitchen_pref_category_type
df.kitchen_pref_category_type = {
    NONE = -1,
    PLANTS = 0,
    SEEDS = 1,
    DRINK = 2,
    OTHER = 3,
}

---@enum standing_orders_category_type
df.standing_orders_category_type = {
    NONE = -1,
    AUTOMATED_WORKSHOPS = 0,
    HAULING = 1,
    REFUSE_AND_DUMPING = 2,
    AUTOMATIC_FORBIDDING = 3,
    CHORES = 4,
    OTHER = 5,
}

---@enum stone_use_category_type
df.stone_use_category_type = {
    NONE = -1,
    ECONOMIC = 0,
    OTHER = 1,
}

---@enum labor_mode_type
df.labor_mode_type = {
    NONE = -1,
    WORK_DETAILS = 0,
    STANDING_ORDERS = 1,
    KITCHEN = 2,
    STONE_USE = 3,
}

---@enum artifacts_mode_type
df.artifacts_mode_type = {
    NONE = -1,
    ARTIFACTS = 0,
    SYMBOLS = 1,
    NAMED_OBJECTS = 2,
    WRITTEN_CONTENT = 3,
}

---@enum counterintelligence_mode_type
df.counterintelligence_mode_type = {
    NONE = -1,
    INTERROGATIONS = 0,
    ACTORS = 1,
    ORGANIZATIONS = 2,
    PLOTS = 3,
}

---@enum justice_interface_mode_type
df.justice_interface_mode_type = {
    NONE = -1,
    OPEN_CASES = 0,
    CLOSED_CASES = 1,
    COLD_CASES = 2,
    FORTRESS_GUARD = 3,
    CONVICTS = 4,
    COUNTERINTELLIGENCE = 5,
}

---@enum info_interface_mode_type
df.info_interface_mode_type = {
    NONE = -1,
    CREATURES = 0,
    JOBS = 1,
    BUILDINGS = 2,
    LABOR = 3,
    WORK_ORDERS = 4,
    ADMINISTRATORS = 5,
    ARTIFACTS = 6,
    JUSTICE = 7,
}

---@enum main_menu_option_type
df.main_menu_option_type = {
    NONE = -1,
    RETURN = 0,
    SAVE_AND_QUIT = 1,
    SAVE_AND_CONTINUE = 2,
    SETTINGS = 3,
    SUCCUMB_TO_INVASION = 4,
    ABANDON_FORTRESS = 5,
    RETIRE_FORTRESS = 6,
    QUIT_WITHOUT_SAVING = 7,
    END_GAME = 8,
    SAVE_TO_EXISTING_FOLDER = 9,
    SAVE_TO_NEW_FOLDER_NEW_TIMELINE = 10,
    SAVE_TO_NEW_FOLDER_EXISTING_TIMELINE = 11,
    RETURN_TO_TITLE = 12,
    CONTINUE = 13,
}

---@enum options_context_type
df.options_context_type = {
    NONE = -1,
    MAIN_DWARF = 0,
    MAIN_DWARF_GAME_OVER = 1,
    MAIN_DWARF_HELP = 2,
    MAIN_DWARF_SAVE_AND_EXIT_CHOICES = 3,
    MAIN_DWARF_SAVE_AND_EXIT_CHOICES_ENDED = 4,
    ABORT_FROM_STARTING_GAME = 5,
}

---@enum help_context_type
df.help_context_type = {
    NONE = -1,
    WORLD_GEN_MESSAGE = 0,
    EMBARK_TUTORIAL_CHICE = 1,
    EMBARK_MESSAGE = 2,
    START_TUTORIAL_CAMERA_CONTROLS = 3,
    START_TUTORIAL_MINING = 4,
    START_TUTORIAL_STOCKPILES = 5,
    START_TUTORIAL_CHOPPING = 6,
    START_TUTORIAL_WORKSHOPS_AND_TASKS = 7,
    START_TUTORIAL_SHEETS = 8,
    START_TUTORIAL_ALERTS = 9,
    START_TUTORIAL_PREPARING_FOR_CARAVAN = 10,
    DONE_WITH_FIRST_STEPS_MESSAGE = 11,
    POPUP_ZONES = 12,
    POPUP_BURROWS = 13,
    POPUP_HAULING = 14,
    POPUP_STOCKS = 15,
    POPUP_WORK_DETAILS = 16,
    POPUP_NOBLES = 17,
    POPUP_JUSTICE = 18,
    POPUP_SQUADS = 19,
    POPUP_WORLD = 20,
    POPUP_WORK_ORDERS = 21,
    REVISIT_CAMERA_CONTROLS = 22,
    REVISIT_MINING = 23,
    REVISIT_STOCKPILES = 24,
    REVISIT_CHOPPING = 25,
    REVISIT_WORKSHOPS_AND_TASKS = 26,
    REVISIT_SHEETS = 27,
    REVISIT_ALERTS = 28,
    REVISIT_PREPARING_FOR_CARAVAN = 29,
    GUIDE_SURVIVAL = 30,
    GUIDE_PLANTING = 31,
    GUIDE_OTHER_FOOD_SOURCES = 32,
    GUIDE_BINS_BAGS_AND_BARRELS = 33,
    GUIDE_TRADE = 34,
    GUIDE_OFFICES = 35,
    GUIDE_ORE_AND_SMELTING = 36,
    GUIDE_TRAPS_AND_LEVERS = 37,
    GUIDE_WELLS = 38,
    GUIDE_HANDLING_LIGHT_AQUIFERS = 39,
    GUIDE_CLOTHING = 40,
    GUIDE_MEETING_AREAS_AND_LOCATIONS = 41,
    GUIDE_MILITARY = 42,
    GUIDE_CHANNELS_AND_RAMPS = 43,
    GUIDE_REFUSE = 44,
    GUIDE_DEEPER = 45,
    GUIDE_HAPPINESS = 46,
    GUIDE_GOALS = 47,
}

---@enum settings_tab_type
df.settings_tab_type = {
    NONE = -1,
    VIDEO = 0,
    AUDIO = 1,
    GAME = 2,
    KEYBINDINGS = 3,
    DIFFICULTY = 4,
}

---@enum settings_context_type
df.settings_context_type = {
    NONE = -1,
    OUTSIDE_PLAY = 0,
    FORT_MODE = 1,
}

---@enum arena_context_type
df.arena_context_type = {
    NONE = -1,
    CREATURE = 0,
    SKILLS = 1,
    EQUIPMENT = 2,
    CONDITIONS = 3,
}

---@enum assign_uniform_context_type
df.assign_uniform_context_type = {
    NONE = -1,
    CREATE_SQUAD_FROM_SQUAD_MENU = 0,
    FROM_SQUAD_EQUIPMENT_MENU = 1,
}

---@enum main_bottom_mode_type
df.main_bottom_mode_type = {
    NONE = -1,
    BUILDING = 0,
    BUILDING_PLACEMENT = 1,
    BUILDING_PICK_MATERIALS = 2,
    ZONE = 3,
    ZONE_PAINT = 4,
    STOCKPILE = 5,
    STOCKPILE_PAINT = 6,
    BURROW = 7,
    BURROW_PAINT = 8,
    HAULING = 9,
    ARENA_UNIT = 10,
    ARENA_TREE = 11,
    ARENA_WATER_PAINT = 12,
    ARENA_MAGMA_PAINT = 13,
    ARENA_SNOW_PAINT = 14,
    ARENA_MUD_PAINT = 15,
    ARENA_REMOVE_PAINT = 16,
}

---@enum main_designation_type
df.main_designation_type = {
    NONE = -1,
    DIG_DIG = 0,
    DIG_REMOVE_STAIRS_RAMPS = 1,
    DIG_STAIR_UP = 2,
    DIG_STAIR_UPDOWN = 3,
    DIG_STAIR_DOWN = 4,
    DIG_RAMP = 5,
    DIG_CHANNEL = 6,
    CHOP = 7,
    GATHER = 8,
    SMOOTH = 9,
    TRACK = 10,
    ENGRAVE = 11,
    FORTIFY = 12,
    REMOVE_CONSTRUCTION = 13,
    CLAIM = 14,
    UNCLAIM = 15,
    MELT = 16,
    NO_MELT = 17,
    DUMP = 18,
    NO_DUMP = 19,
    HIDE = 20,
    NO_HIDE = 21,
    TOGGLE_ENGRAVING = 22,
    DIG_FROM_MARKER = 23,
    DIG_TO_MARKER = 24,
    CHOP_FROM_MARKER = 25,
    CHOP_TO_MARKER = 26,
    GATHER_FROM_MARKER = 27,
    GATHER_TO_MARKER = 28,
    SMOOTH_FROM_MARKER = 29,
    SMOOTH_TO_MARKER = 30,
    DESIGNATE_TRAFFIC_HIGH = 31,
    DESIGNATE_TRAFFIC_NORMAL = 32,
    DESIGNATE_TRAFFIC_LOW = 33,
    DESIGNATE_TRAFFIC_RESTRICTED = 34,
    ERASE = 35,
}

---@class gamest_mod_manager
---@field mod_header mod_headerst[]
---@field subscribed_file_id integer
---@field doing_mod_upload boolean
---@field mod_upload_header mod_headerst[]
---@field mod_upload_completed boolean
---@field uploading_mod_index integer
---@field CreateItemResult integer
---@field SubmitItemUpdateResult integer

---@class gamest_command_line
---@field original string
---@field arg_vect string[]
---@field gen_id number
---@field world_seed number
---@field use_seed boolean
---@field world_param string
---@field use_param integer

---@class gamest_minimap
---@field minimap any[]
---@field update integer
---@field mustmake integer
---@field printed_z integer
---@field buffer_symbol any[]
---@field buffer_f any[]
---@field buffer_b any[]
---@field buffer_br any[]
---@field texpos integer

---@class gamest_main_interface_arena_tree
---@field open boolean
---@field age integer in years
---@field editing_age boolean
---@field unnamed_gamest_main_interface_arena_tree_4 integer
---@field age_str string string representation of age field
---@field editing_filter boolean
---@field filter string
---@field tree_types_filtered any[]
---@field tree_types_all any[]
---@field unnamed_gamest_main_interface_arena_tree_10 integer
---@field unnamed_gamest_main_interface_arena_tree_11 integer
---@field unnamed_gamest_main_interface_arena_tree_12 integer
---@field unnamed_gamest_main_interface_arena_tree_13 integer

---@class gamest_main_interface_arena_unit
---@field open boolean
---@field context arena_context_type
---@field race integer
---@field caste integer
---@field team integer
---@field interaction integer
---@field tame boolean
---@field editing_filter boolean
---@field filter string
---@field races_filtered integer[]
---@field castes_filtered integer[]
---@field races_all integer[]
---@field castes_all integer[]
---@field unnamed_gamest_main_interface_arena_unit_14 integer
---@field unnamed_gamest_main_interface_arena_unit_15 integer
---@field unnamed_gamest_main_interface_arena_unit_16 integer
---@field unnamed_gamest_main_interface_arena_unit_17 integer
---@field skills any[]
---@field skill_levels integer[]
---@field unnamed_gamest_main_interface_arena_unit_20 integer
---@field unnamed_gamest_main_interface_arena_unit_21 integer
---@field unnamed_gamest_main_interface_arena_unit_22 integer
---@field unnamed_gamest_main_interface_arena_unit_23 integer
---@field unnamed_gamest_main_interface_arena_unit_24 integer
---@field unnamed_gamest_main_interface_arena_unit_25 integer
---@field unnamed_gamest_main_interface_arena_unit_26 integer
---@field unnamed_gamest_main_interface_arena_unit_27 integer[]
---@field unnamed_gamest_main_interface_arena_unit_28 integer
---@field equipment_item_type integer[]
---@field equipment_item_subtype integer[]
---@field equipment_mat_type integer[]
---@field equipment_mat_index integer[]
---@field equipment_quantity integer[]
---@field interactions interaction_effect[]
---@field unnamed_gamest_main_interface_arena_unit_35 integer
---@field unnamed_gamest_main_interface_arena_unit_36 integer

---@class gamest_main_interface_help
---@field open boolean
---@field flag integer
---@field context_flag integer
---@field context help_context_type
---@field header string
---@field text markup_text_boxst[] tutorials
---@field floor_dug integer

---@class gamest_main_interface_options_saver saverst
---@field unnamed_gamest_main_interface_options_saver_1 integer
---@field unnamed_gamest_main_interface_options_saver_2 integer[]
---@field unnamed_gamest_main_interface_options_saver_3 integer[]
---@field unnamed_gamest_main_interface_options_saver_4 any[]
---@field unnamed_gamest_main_interface_options_saver_5 integer
---@field unnamed_gamest_main_interface_options_saver_6 integer
---@field unnamed_gamest_main_interface_options_saver_7 integer

---@class gamest_main_interface_options
---@field open boolean
---@field context options_context_type
---@field header string
---@field text string[]
---@field fort_retirement_confirm boolean
---@field fort_abandon_confirm boolean
---@field fort_quit_without_saving_confirm boolean
---@field option main_menu_option_type[]
---@field option_index integer[]
---@field entering_manual_folder boolean
---@field entering_manual_str string
---@field confirm_manual_overwrite boolean
---@field entering_timeline boolean
---@field entering_timeline_str string
---@field do_manual_save boolean
---@field manual_save_timer integer
---@field overwrite_save_folder string[]
---@field ended_game boolean
---@field doing_help boolean
---@field doing_help_box markup_text_boxst
---@field guide_context integer[]
---@field scroll_position_guide integer
---@field scrolling_guide boolean
---@field popup_context integer[]
---@field scroll_position_popup integer
---@field scrolling_popup boolean
---@field filecomp file_compressorst
---@field saver gamest_main_interface_options_saver saverst

---@class gamest_main_interface_hotkey
---@field open boolean
---@field scroll_position integer
---@field scrolling boolean
---@field entering_index integer
---@field entering_name boolean

---@class gamest_main_interface_create_work_order
---@field open boolean
---@field forced_bld_id integer
---@field jminfo_master manager_order_template[]
---@field building cwo_buildingst[]
---@field scroll_position_building integer
---@field scrolling_building boolean
---@field selected_building_index integer
---@field scroll_position_job integer
---@field scrolling_job boolean
---@field job_filter string
---@field entering_job_filter boolean

---@class gamest_main_interface_assign_uniform
---@field open boolean
---@field context assign_uniform_context_type
---@field scroll_position integer
---@field scrolling boolean
---@field cand_uniform entity_uniform[]

---@class gamest_main_interface_squad_supplies
---@field open boolean
---@field squad_id integer

---@class gamest_main_interface_create_squad
---@field open boolean
---@field scroll_position integer
---@field scrolling boolean
---@field cand_new_squad_appoint_epp entity_position_assignment[]
---@field cand_new_squad_appoint_epp_ep entity_position[]
---@field cand_new_squad_existing_epp entity_position_assignment[]
---@field cand_new_squad_existing_epp_ep entity_position[]
---@field cand_new_squad_new_epp_from_ep entity_position[]
---@field new_squad_appoint_epp entity_position_assignment
---@field new_squad_appoint_epp_ep entity_position
---@field new_squad_existing_epp entity_position_assignment
---@field new_squad_existing_epp_ep entity_position
---@field new_squad_new_epp_from_ep entity_position

---@class gamest_main_interface_squads
---@field open boolean
---@field scroll_position integer
---@field scrolling boolean
---@field squad_id integer[]
---@field squad_selected boolean[]
---@field viewing_squad_index integer
---@field squad_hfid_selected integer[]
---@field entering_squad_nickname boolean
---@field squad_nickname_str string
---@field giving_move_order boolean
---@field giving_kill_order boolean
---@field kill_unid integer[]
---@field kill_doing_rectangle boolean
---@field giving_patrol_order boolean
---@field giving_burrow_order boolean
---@field editing_squad_schedule_id integer
---@field editing_squad_schedule_routine_index integer
---@field editing_squad_schedule_month integer
---@field editing_squad_schedule_whole_squad_selected boolean
---@field editing_squad_schedule_pos_selected integer[]
---@field editing_squad_schedule_min_follow integer
---@field scroll_position_orderp integer
---@field scrolling_orderp boolean
---@field cell_nickname_str string
---@field entering_cell_nickname boolean

---@class gamest_main_interface_info_justice
---@field current_mode justice_interface_mode_type
---@field cage_chain_needed integer
---@field cage_chain_count integer
---@field open_case crime[]
---@field scroll_position_open_cases integer
---@field scrolling_open_cases boolean
---@field selected_open_case integer
---@field scroll_position_selected_open_case integer
---@field scrolling_selected_open_case boolean
---@field closed_case crime[]
---@field scroll_position_closed_cases integer
---@field scrolling_closed_cases boolean
---@field selected_closed_case integer
---@field scroll_position_selected_closed_case integer
---@field scrolling_selected_closed_case boolean
---@field cold_case crime[]
---@field scroll_position_cold_cases integer
---@field scrolling_cold_cases boolean
---@field selected_cold_case integer
---@field scroll_position_selected_cold_case integer
---@field scrolling_selected_cold_case boolean
---@field cri_fortress_guard cri_unitst[]
---@field scroll_position_fortress_guard integer
---@field scrolling_fortress_guard boolean
---@field sorting_guard_nameprof boolean
---@field sorting_guard_nameprof_is_ascending boolean
---@field sorting_guard_nameprof_doing_name boolean
---@field sorting_guard_nameprof_doing_prof boolean
---@field cri_convict cri_unitst[]
---@field scroll_position_convicts integer
---@field scrolling_convicts boolean
---@field selected_convict integer
---@field convict_crime crime[]
---@field convict_lawaction integer lawactionst
---@field sorting_convict_nameprof boolean
---@field sorting_convict_nameprof_is_ascending boolean
---@field sorting_convict_nameprof_doing_name boolean
---@field sorting_convict_nameprof_doing_prof boolean
---@field scroll_position_selected_convict integer
---@field scrolling_selected_convict boolean
---@field convicting boolean
---@field conviction_list unit[]
---@field scroll_position_conviction integer
---@field scrolling_conviction boolean
---@field interrogating boolean
---@field interrogation_list unit[]
---@field interrogation_list_flag integer[]
---@field scroll_position_interrogation integer
---@field scrolling_interrogation boolean
---@field interrogation_report_box string[]
---@field interrogation_report_box_width integer
---@field interrogation_report any[] interrogation_reportst
---@field viewing_interrogation_report integer interrogation_reportst
---@field scroll_position_interrogation_list integer
---@field scrolling_interrogation_list boolean
---@field scroll_position_interrogation_report integer
---@field scrolling_interrogation_report boolean
---@field base_actor_entry actor_entryst[]
---@field base_organization_entry organization_entryst[]
---@field base_plot_entry plot_entryst[]
---@field counterintelligence_mode counterintelligence_mode_type
---@field counterintelligence_selected integer
---@field counterintelligence_filter_str string
---@field entering_counterintelligence_filter boolean
---@field selected_counterintelligence_oen integer organization_entry_nodest
---@field scroll_position_counterintelligence integer
---@field scrolling_counterintelligence boolean
---@field value_actor_entry actor_entryst[]
---@field value_organization_entry organization_entryst[]
---@field value_plot_entry plot_entryst[]
---@field actor_entry actor_entryst[]
---@field organization_entry organization_entryst[]
---@field plot_entry plot_entryst[]

---@class gamest_main_interface_info_artifacts
---@field mode artifacts_mode_type
---@field list any[]
---@field scroll_position integer[]
---@field scrolling boolean[]

---@class gamest_main_interface_info_administrators
---@field noblelist any[]
---@field spec_prof entity_position_assignment[]
---@field spec_hfid integer[]
---@field spec_enid integer[]
---@field scroll_position_noblelist integer
---@field scrolling_noblelist boolean
---@field desc_hover_text string[]
---@field last_hover_width integer
---@field last_hover_entity_id integer
---@field last_hover_ep_id integer
---@field choosing_candidate boolean
---@field candidate_noblelist_ind integer
---@field candidate any[]
---@field scroll_position_candidate integer
---@field scrolling_candidate boolean
---@field assigning_symbol boolean
---@field symbol_noblelist_ind integer
---@field cand_symbol item[]
---@field cand_symbol_new_ind integer[]
---@field cand_symbol_is_symbol_of_ind integer[]
---@field cand_symbol_value integer[]
---@field scroll_position_symbol integer
---@field scrolling_symbol boolean
---@field handling_symbol_closure_ind integer

---@class gamest_main_interface_info_work_orders_conditions
---@field open boolean
---@field wq manager_order
---@field item_condition_satisfied boolean[]
---@field order_condition_satisfied boolean[]
---@field scroll_position_conditions integer
---@field scrolling_conditions boolean
---@field suggested_item_condition any[] workquota_item_conditionst
---@field scroll_position_suggested integer
---@field scrolling_suggested boolean
---@field filter string
---@field compare_master string[]
---@field change_type integer
---@field change_wqc integer workquota_item_conditions
---@field scroll_position_change integer
---@field scrolling_change integer
---@field item_type_master integer[]
---@field item_subtype_master integer[]
---@field item_type_on boolean[]
---@field item_material_master integer[]
---@field item_matgloss_master integer[]
---@field item_matstate_master integer[]
---@field item_material_on boolean[]
---@field item_trait_master wqc_item_traitst[]
---@field selecting_order_condition boolean
---@field condition_wq manager_order[]
---@field scroll_position_condition_wq integer
---@field scrolling_condition_wq boolean
---@field entering_logic_number boolean
---@field logic_number_str string
---@field entering_logic_wqc integer workquota_item_conditionst

---@class gamest_main_interface_info_work_orders
---@field scroll_position_work_orders integer
---@field scrolling_work_orders boolean
---@field conditions gamest_main_interface_info_work_orders_conditions
---@field entering_number boolean
---@field number_str string
---@field entering_wq manager_order
---@field b_entering_number boolean
---@field b_number_str string
---@field b_entering_wq manager_order

---@class gamest_main_interface_info_labor_stone_use
---@field current_category stone_use_category_type
---@field stone_mg_index any[]
---@field stone_restriction_p any[]
---@field stone_item_use_str string[]
---@field scroll_position integer[]
---@field scrolling boolean[]

---@class gamest_main_interface_info_labor_kitchen
---@field current_category kitchen_pref_category_type
---@field known_type any[]
---@field known_subtype any[]
---@field known_mat any[]
---@field known_matg any[]
---@field known_num any[]
---@field known_rest any[]
---@field known_canrest any[]
---@field known_name any[]
---@field scroll_position integer[]
---@field scrolling boolean[]

---@class gamest_main_interface_info_labor_standing_orders
---@field current_category standing_orders_category_type
---@field unit unit[]
---@field labor_list integer[]
---@field scroll_position_labor_list integer
---@field scrolling_labor_list boolean
---@field scroll_position_units integer
---@field scrolling_units boolean

---@class gamest_main_interface_info_labor_work_details
---@field selected_work_detail_index integer
---@field scroll_position_work_details integer
---@field scrolling_work_details boolean
---@field assignable_unit unit[]
---@field scroll_position_assignable_unit integer
---@field scrolling_assignable_unit boolean
---@field entering_custom_detail_name boolean
---@field editing_work_detail integer work_detailst
---@field labor_list integer[]
---@field scroll_position_labor_list integer
---@field scrolling_labor_list boolean
---@field skill_used integer[]
---@field skill_num integer

---@class gamest_main_interface_info_labor
---@field mode labor_mode_type
---@field work_details gamest_main_interface_info_labor_work_details
---@field standing_orders gamest_main_interface_info_labor_standing_orders
---@field kitchen gamest_main_interface_info_labor_kitchen
---@field stone_use gamest_main_interface_info_labor_stone_use

---@class gamest_main_interface_info_buildings
---@field mode buildings_mode_type
---@field list any[]
---@field scrolling_position integer[]
---@field scrolling boolean[]

---@class gamest_main_interface_info_jobs
---@field cri_job cri_unitst[]
---@field scrolling_cri_job boolean
---@field scroll_position_cri_job integer

---@class gamest_main_interface_info_creatures
---@field current_mode unit_list_mode_type
---@field cri_unit any[]
---@field scrolling_cri_unit boolean[]
---@field scroll_position_cri_unit integer[]
---@field sorting_cit_nameprof boolean
---@field sorting_cit_nameprof_is_ascending boolean
---@field sorting_cit_nameprof_doing_name boolean
---@field sorting_cit_nameprof_doing_prof boolean
---@field sorting_cit_job boolean
---@field sorting_cit_job_is_ascending boolean
---@field sorting_cit_stress boolean
---@field sorting_cit_stress_is_ascending boolean
---@field adding_trainer boolean
---@field trainer_animal_target unit
---@field trainer unit[]
---@field trainer_option integer[]
---@field scrolling_trainer boolean
---@field scroll_position_trainer integer
---@field showing_overall_training boolean
---@field atk_index integer[]
---@field scrolling_overall_training boolean
---@field scroll_position_overall_training integer
---@field assign_work_animal boolean
---@field assign_work_animal_animal unit
---@field work_animal_recipient unit[]
---@field scrolling_work_animal boolean
---@field scroll_position_work_animal integer
---@field showing_activity_details boolean
---@field activity_details_text markup_text_boxst
---@field scrolling_activity_details boolean
---@field scroll_position_activity_details integer

---@class gamest_main_interface_info
---@field open boolean
---@field current_mode info_interface_mode_type
---@field creatures gamest_main_interface_info_creatures
---@field jobs gamest_main_interface_info_jobs
---@field buildings gamest_main_interface_info_buildings
---@field labor gamest_main_interface_info_labor
---@field work_orders gamest_main_interface_info_work_orders
---@field administrators gamest_main_interface_info_administrators
---@field artifacts gamest_main_interface_info_artifacts
---@field justice gamest_main_interface_info_justice

---@class gamest_main_interface_view_sheets
---@field open boolean
---@field context view_sheets_context_type
---@field active_sheet view_sheet_type
---@field active_id integer
---@field viewing_unid integer[]
---@field viewing_itid integer[]
---@field viewing_bldid integer
---@field viewing_x integer
---@field viewing_y integer
---@field viewing_z integer
---@field scroll_position integer
---@field scrolling boolean
---@field tab view_sheet_type[]
---@field tab_id integer[]
---@field active_sub_tab integer
---@field trait view_sheet_trait_type[]
---@field trait_id integer[]
---@field trait_magnitude integer[]
---@field trait_num integer
---@field last_tick_update number
---@field reqroom integer[] demands
---@field curroom integer[] demands
---@field labor_skill_ind integer[]
---@field labor_skill_val integer[]
---@field labor_skill_w_rust integer[]
---@field labor_skill_num integer
---@field combat_skill_ind integer[]
---@field combat_skill_val integer[]
---@field combat_skill_w_rust integer[]
---@field combat_skill_num integer
---@field other_skill_ind integer[]
---@field other_skill_val integer[]
---@field other_skill_w_rust integer[]
---@field other_skill_num integer
---@field ent_vect historical_entity[]
---@field ep_vect entity_position[]
---@field ep_vect_spouse boolean[]
---@field unmet_need_type integer[]
---@field unmet_need_spec_id integer[]
---@field unmet_need_se integer[]
---@field unmet_need_num integer
---@field raw_thought_str string[]
---@field thought_box integer[] color_text_boxst
---@field thought_box_width integer
---@field scroll_position_inventory integer
---@field scrolling_inventory boolean
---@field scroll_position_relations integer
---@field scrolling_relations boolean
---@field rel_name string[]
---@field relation integer[]
---@field relation_f integer[]
---@field rel_unid integer[]
---@field rel_hf historical_figure[]
---@field rel_rphv any[] relationship_profile_hf_visualst
---@field rel_rphh any[] relationship_profile_hf_historicalst
---@field rel_value integer[]
---@field unit_overview_customizing boolean
---@field unit_overview_entering_nickname boolean
---@field unit_overview_entering_profession_nickname boolean
---@field unit_overview_entering_str string
---@field unit_overview_expelling boolean
---@field unit_overview_expel_cannot_expel_reason cannot_expel_reason_type
---@field unit_overview_expel_selected_dest_stid integer
---@field unit_overview_expel_dest_stid integer[]
---@field unit_overview_expel_total_unid integer[]
---@field scroll_position_unit_overview_expel integer
---@field scrolling_unit_overview_expel boolean
---@field guest_text string[]
---@field scroll_position_groups integer
---@field scrolling_groups boolean
---@field unit_group_enid integer[]
---@field unit_group_hfel integer[]
---@field unit_group_epid integer[]
---@field unit_group_eppid integer[]
---@field unit_group_ep_is_spouse boolean[]
---@field unit_group_rep integer[]
---@field unit_group_rep_level integer[]
---@field scroll_position_thoughts integer
---@field scrolling_thoughts boolean
---@field thoughts_active_tab integer
---@field thoughts_raw_memory_str string[]
---@field thoughts_memory_box integer[] color_text_boxst
---@field thoughts_memory_box_width integer
---@field scroll_position_personality integer
---@field scrolling_personality boolean
---@field personality_active_tab integer
---@field personality_raw_str string[]
---@field personality_box integer[] color_text_boxst
---@field personality_width integer
---@field unit_labor_active_tab integer
---@field scroll_position_unit_labor integer
---@field scrolling_unit_labor boolean
---@field unit_workshop_id integer[]
---@field unit_labor_assigned_animal_unid integer[]
---@field unit_labor_assignable_animal_unid integer[]
---@field scroll_position_unit_skill integer
---@field scrolling_unit_skill boolean
---@field scroll_position_skill_description integer
---@field scrolling_skill_description boolean
---@field unit_skill_active_tab integer
---@field unit_skill job_skill[]
---@field unit_skill_val integer[]
---@field unit_skill_val_w_rust integer[]
---@field unit_knowledge_type view_sheet_unit_knowledge_type[]
---@field unit_knowledge_id integer[]
---@field unit_knowledge_bits integer[]
---@field skill_description_raw_str string[]
---@field skill_description_box integer[] color_text_boxst
---@field skill_description_width integer
---@field scroll_position_unit_room integer
---@field scrolling_unit_room integer
---@field unit_room_civzone_id integer[]
---@field unit_room_curval integer[]
---@field unit_military_active_tab integer
---@field scroll_position_unit_military_assigned integer
---@field scrolling_unit_military_assigned boolean
---@field scroll_position_unit_military_kills integer
---@field scrolling_unit_military_kills boolean
---@field kill_description_raw_str string[]
---@field kill_description_box integer[] color_text_boxst
---@field kill_description_width integer
---@field unit_health_active_tab integer
---@field scroll_position_unit_health integer
---@field scrolling_unit_health boolean
---@field unit_health_raw_str string[]
---@field unit_health_box integer[] color_text_boxst
---@field unit_health_width integer
---@field raw_current_thought string
---@field current_thought string[]
---@field current_thought_width integer
---@field scroll_position_item integer
---@field scrolling_item boolean
---@field scroll_position_building_job integer
---@field scrolling_building_job boolean
---@field building_job_filter_str string
---@field entering_building_job_filter boolean
---@field scroll_position_cage_occupants integer
---@field scrolling_cage_occupants boolean
---@field scroll_position_displayed_items integer
---@field scrolling_displayed_items boolean
---@field linking_lever boolean
---@field need_accessible_mechanism_warning boolean
---@field linking_lever_bld_id integer
---@field linking_lever_mech_lever_id integer
---@field linking_lever_mech_target_id integer
---@field show_linked_buildings boolean
---@field scroll_position_linked_buildings integer
---@field scrolling_linked_buildings boolean
---@field building_entering_nickname boolean
---@field building_entering_str string
---@field work_order_id integer[]
---@field scroll_position_work_orders integer
---@field scrolling_work_orders boolean
---@field gen_work_order_num_str string
---@field entering_gen_work_order_num boolean
---@field entering_wq_number boolean
---@field wq_number_str string
---@field entering_wq_id integer
---@field engraving_title string
---@field raw_description string
---@field description string[]
---@field description_width integer
---@field scroll_position_description integer
---@field scrolling_description boolean
---@field scroll_position_item_contents integer
---@field scrolling_item_contents boolean
---@field item_use string[]
---@field item_use_reaction_index integer[]

---@class gamest_main_interface_custom_stockpile
---@field open boolean
---@field scroll_position_main integer
---@field scroll_position_sub integer
---@field scroll_position_spec integer
---@field scrolling_main boolean
---@field scrolling_sub boolean
---@field scrolling_spec boolean
---@field spec_filter string
---@field entering_spec_filter boolean
---@field abd building_stockpilest
---@field sp stockpile_settings
---@field cur_main_mode stockpile_list
---@field cur_main_mode_flag integer
---@field cur_sub_mode stockpile_list
---@field main_mode stockpile_list[]
---@field main_mode_flag integer[]
---@field sub_mode stockpile_list[]
---@field sub_mode_ptr_type stock_pile_pointer_type[]
---@field sub_mode_ptr integer[]
---@field spec_item any[]
---@field cur_spec_item_sz integer
---@field counted_cur_spec_item_sz integer

---@class gamest_main_interface_stockpile_tools
---@field open boolean
---@field context stockpile_tools_context_type
---@field bld_id integer
---@field entering_barrels boolean
---@field entering_bins boolean
---@field entering_wheelbarrows boolean
---@field number_str string

---@class gamest_main_interface_stockpile_link
---@field open boolean
---@field context stockpile_link_context_type
---@field bld_id integer
---@field hr_id integer
---@field hs_id integer
---@field scroll_position_link_list integer
---@field scrolling_link_list boolean
---@field adding_new_link boolean
---@field adding_new_link_type integer

---@class gamest_main_interface_stockpile
---@field doing_rectangle boolean
---@field box_on_left boolean
---@field erasing boolean
---@field repainting boolean
---@field cur_bld building_stockpilest

---@class gamest_main_interface_assign_vehicle
---@field open boolean
---@field context assign_vehicle_context_type
---@field i_vehicle vehicle[]
---@field route_id integer
---@field scroll_position integer
---@field scrolling boolean

---@class gamest_main_interface_hauling_stop_conditions
---@field open boolean
---@field context hauling_stop_conditions_context_type
---@field route_id integer
---@field stop_id integer
---@field scroll_position integer
---@field scrolling boolean

---@class gamest_main_interface_location_details
---@field open boolean
---@field context location_details_context_type
---@field selected_ab abstract_building
---@field open_area_dx integer
---@field open_area_dy integer
---@field wc_count integer
---@field loc_occupation integer[] occupationst
---@field loc_ent historical_entity[]
---@field loc_position entity_position[]
---@field loc_epp entity_position_assignment[]
---@field scroll_position_occupation integer
---@field scrolling_occupation boolean
---@field desired_number_str string
---@field entering_desired_number integer

---@class gamest_main_interface_location_selector
---@field open boolean
---@field context location_selector_context_type
---@field valid_ab abstract_building[]
---@field scroll_position_location integer
---@field scrolling_location boolean
---@field current_hover_index integer
---@field choosing_temple_religious_practice boolean
---@field valid_religious_practice integer[]
---@field scroll_position_deity integer
---@field scrolling_deity boolean
---@field choosing_craft_guild boolean
---@field valid_religious_practice_id integer[]
---@field valid_craft_guild_type profession[]
---@field scroll_position_guild integer
---@field scrolling_guild boolean

---@class gamest_main_interface_burrow_selector
---@field open boolean
---@field context burrow_selector_context_type
---@field burrow_id integer[]
---@field selected boolean[]
---@field scroll_position integer
---@field scrolling integer

---@class gamest_main_interface_squad_selector
---@field open boolean
---@field context squad_selector_context_type
---@field squad_id integer[]
---@field bld_id integer
---@field scroll_position integer
---@field scrolling integer

---@class gamest_main_interface_squad_schedule
---@field open boolean
---@field context squad_schedule_context_type
---@field scroll_position integer
---@field scrolling boolean
---@field scroll_position_month integer
---@field scrolling_month boolean
---@field routine_page integer
---@field squad_id integer[]
---@field viewing_months_squad_id integer
---@field last_tick_update number
---@field editing_routines boolean
---@field scroll_position_edit_routine integer
---@field scrolling_edit_routine boolean
---@field routine_name_str string
---@field entering_routine_name boolean
---@field entering_routine_name_id integer
---@field deleting_routine_id integer
---@field copying_routine_id integer
---@field copying_squad_id integer
---@field copying_squad_month integer

---@class gamest_main_interface_squad_equipment
---@field open boolean
---@field context squad_equipment_context_type
---@field scroll_position integer
---@field scrolling boolean
---@field squad_id integer[]
---@field squad_pos integer[]
---@field last_tick_update number
---@field customizing_equipment boolean
---@field customizing_squad_id integer
---@field customizing_squad_pos integer
---@field customizing_squad_uniform_nickname string
---@field customizing_squad_entering_uniform_nickname boolean
---@field scroll_position_cs integer
---@field scrolling_cs boolean
---@field scroll_position_cssub integer
---@field scrolling_cssub boolean
---@field cs_cat integer[] EntityUniformItemCategory
---@field cs_it_spec_item_id integer[]
---@field cs_it_type integer[]
---@field cs_it_subtype integer[]
---@field cs_civ_mat integer[] EntityMaterial
---@field cs_spec_mat integer[]
---@field cs_spec_matg integer[]
---@field cs_color_pattern_index integer[] ColoredPattern
---@field cs_icp_flag integer[]
---@field cs_assigned_item_number integer[]
---@field cs_assigned_item_id integer[]
---@field cs_uniform_flag integer
---@field cs_adding_new_entry_category integer EntityUniformItemCategory
---@field cs_add_list_type integer[]
---@field cs_add_list_subtype integer[]
---@field cs_add_list_flag integer[]
---@field cs_add_list_is_foreign boolean[]
---@field cs_setting_material boolean
---@field cs_setting_list_ind integer
---@field cs_setting_material_ent integer[] EntityMaterial
---@field cs_setting_material_mat integer[]
---@field cs_setting_material_matg integer[]
---@field cs_setting_color_pattern boolean
---@field cs_setting_color_pattern_index integer[] ColoredPattern
---@field cs_setting_color_pattern_is_dye boolean[]
---@field cs_adding_specific_item boolean
---@field cs_add_spec_id integer[]

---@class gamest_main_interface_patrol_routes
---@field open boolean
---@field context patrol_routes_context_type
---@field scroll_position integer
---@field scrolling boolean
---@field adding_new_route boolean
---@field new_route_name string
---@field entering_new_route_name boolean
---@field new_point coord_path
---@field route_line coord_path[] patrol_route_linest
---@field add_is_edit_of_route_id integer
---@field changed_points_on_edit boolean

---@class gamest_main_interface_custom_symbol
---@field open boolean
---@field context custom_symbol_context_type
---@field burrow_id integer
---@field work_detail_id integer
---@field squad_id integer
---@field scroll_position integer
---@field scrolling boolean
---@field doing_background_color boolean
---@field swatch_r any[]
---@field swatch_g any[]
---@field swatch_b any[]

---@class gamest_main_interface_announcement_alert
---@field open boolean
---@field viewing_alert report
---@field viewing_alert_button boolean
---@field zoom_line_is_start boolean[]
---@field zoom_line_ann report[]
---@field zoom_line_unit unit[]
---@field zoom_line_unit_uac integer[]
---@field alert_text string[]
---@field alert_width integer
---@field alert_list_size integer
---@field scroll_position_alert integer
---@field scrolling_alert boolean
---@field viewing_unit unit
---@field viewing_unit_uac integer
---@field uac_zoom_line_is_start boolean[]
---@field uac_zoom_line_ann report[]
---@field uac_text string[]
---@field uac_width integer
---@field uac_list_size integer
---@field scroll_position_uac integer
---@field scrolling_uac boolean

---@class gamest_main_interface_unit_selector
---@field open boolean
---@field context unit_selector_context_type
---@field unid integer[]
---@field itemid integer[]
---@field selected integer[]
---@field already integer[]
---@field bld_id integer
---@field skill_used integer[]
---@field skill_num integer
---@field loc_occupation integer occupationst
---@field loc_ent historical_entity
---@field loc_position entity_position
---@field loc_epp entity_position_assignment
---@field burrow_id integer
---@field squad_id integer
---@field squad_position integer
---@field scroll_position integer
---@field scrolling boolean

---@class gamest_main_interface_image_creator_ics
---@field jb job
---@field wq manager_order
---@field location_detail integer
---@field image_ent historical_entity
---@field art_image art_image
---@field adv_art_specifier integer
---@field hf historical_figure
---@field exit_flag integer
---@field flag integer

---@class gamest_main_interface_image_creator
---@field open boolean
---@field context image_creator_context_type
---@field header string
---@field current_option image_creator_option_type
---@field scrolling_list boolean
---@field scroll_position_list integer
---@field doing_filter boolean
---@field filter string
---@field entering_number boolean
---@field number_str string
---@field st_master integer[] site ptr, native name, translated name
---@field ent_master integer[] entity ptr, native name, translated name
---@field plant_master integer[] plant id, name
---@field tree_master integer[] plant id, name
---@field shape_master integer[] shape id, shape adj, name
---@field item_master integer[] item type, item subtype, name
---@field artifact_master integer[] artifact ptr, native name, translated name
---@field hf_master integer[] histfig ptr, native name, translated name
---@field property_master integer[] art property type, bool transitive, name
---@field hf historical_figure[]
---@field st world_site[]
---@field ent historical_entity[]
---@field hist_event history_event[]
---@field art_image art_image[]
---@field new_image art_image
---@field new_image_race integer[]
---@field new_image_caste integer[]
---@field new_image_hf historical_figure[]
---@field new_image_plant integer[]
---@field new_image_tree integer[]
---@field new_image_shape integer[]
---@field new_image_shape_adj integer[]
---@field new_image_item integer[]
---@field new_image_item_subtype integer[]
---@field new_image_artifact artifact_record[]
---@field new_image_property integer[]
---@field new_image_property_transitive boolean[]
---@field new_image_property_actor_target integer[]
---@field new_image_active_property integer
---@field new_image_active_property_transitive boolean
---@field new_image_active_property_actor_ind integer
---@field new_image_active_property_target_ind integer
---@field art_box string[]
---@field scrolling_art_box boolean
---@field scroll_position_art_box integer
---@field last_art_box_width integer
---@field selected_box string[]
---@field last_selected_box_width integer
---@field last_selected_index integer
---@field back_out_warn boolean
---@field image_back_out_warn boolean
---@field must_do_image_back_out_warn boolean
---@field ics gamest_main_interface_image_creator_ics

---@class gamest_main_interface_name_creator
---@field open boolean
---@field context name_creator_context_type
---@field namer historical_entity
---@field name language_name
---@field name_type language_name_type
---@field cur_name_place integer
---@field cur_word_place integer
---@field word_sel language_word_table
---@field word_index integer[]
---@field word_index_asp integer[]
---@field scroll_position_word integer
---@field scrolling_word boolean
---@field entering_first_name boolean
---@field entering_cull_str boolean
---@field cull_str string
---@field adv_naming_pet_actev activity_event_conversationst
---@field named_unit unit

---@class gamest_main_interface_assign_display_item
---@field open boolean
---@field display_bld building_display_furniturest
---@field new_display_itid integer[]
---@field type_list integer[]
---@field filtered_type_list integer[]
---@field current_type item_type
---@field scroll_position_type integer
---@field scroll_position_item integer
---@field scrolling_type boolean
---@field scrolling_item boolean
---@field item_filter string
---@field entering_item_filter boolean
---@field storeamount integer[]
---@field badamount integer[]
---@field unnamed_gamest_main_interface_assign_display_item_15 any[]
---@field unnamed_gamest_main_interface_assign_display_item_16 integer[]
---@field unnamed_gamest_main_interface_assign_display_item_17 integer[]
---@field unnamed_gamest_main_interface_assign_display_item_18 integer[]
---@field unnamed_gamest_main_interface_assign_display_item_19 integer[]
---@field unnamed_gamest_main_interface_assign_display_item_20 integer[]
---@field unnamed_gamest_main_interface_assign_display_item_21 boolean[]
---@field i_height integer
---@field current_type_i_list item[]
---@field current_type_a_subtype integer[]
---@field current_type_a_subcat1 integer[]
---@field current_type_a_subcat2 integer[]
---@field current_type_a_amount integer[]
---@field current_type_a_expanded boolean[]
---@field current_type_a_on boolean[]

---@class gamest_main_interface_stocks
---@field open boolean
---@field type_list integer[]
---@field filtered_type_list integer[]
---@field current_type item_type
---@field scroll_position_type integer
---@field scroll_position_item integer
---@field scrolling_type boolean
---@field scrolling_item boolean
---@field item_filter string
---@field entering_item_filter boolean
---@field storeamount integer[]
---@field badamount integer[]
---@field unnamed_gamest_main_interface_stocks_13 any[]
---@field unnamed_gamest_main_interface_stocks_14 integer[]
---@field unnamed_gamest_main_interface_stocks_15 integer[]
---@field unnamed_gamest_main_interface_stocks_16 integer[]
---@field unnamed_gamest_main_interface_stocks_17 integer[]
---@field unnamed_gamest_main_interface_stocks_18 integer[]
---@field unnamed_gamest_main_interface_stocks_19 boolean[]
---@field i_height integer
---@field current_type_i_list item[]
---@field current_type_a_subtype integer[]
---@field current_type_a_subcat1 integer[]
---@field current_type_a_subcat2 integer[]
---@field current_type_a_amount integer[]
---@field current_type_a_expanded boolean[]
---@field current_type_a_on boolean[]
---@field current_type_a_flag integer[]

---@class gamest_main_interface_petitions
---@field open boolean
---@field have_responsible_person boolean
---@field agreement_id integer[]
---@field selected_agreement_id integer
---@field scroll_position integer
---@field scrolling boolean

---@class gamest_main_interface_diplomacy
---@field open boolean
---@field mm meeting_event
---@field actor unit
---@field target unit
---@field actor_unid integer
---@field target_unid integer
---@field flag integer
---@field text markup_text_boxst
---@field selecting_land_holder_position boolean
---@field taking_requests boolean
---@field land_holder_parent_civ historical_entity
---@field land_holder_child_civ historical_entity
---@field land_holder_pos_id integer[]
---@field land_holder_assigned_hfid integer[]
---@field land_holder_avail_hfid integer[]
---@field scroll_position_land_holder_pos integer
---@field scrolling_land_holder_pos boolean
---@field scroll_position_land_holder_hf integer
---@field scrolling_land_holder_hf boolean
---@field land_holder_selected_pos integer
---@field taking_requests_tablist integer[]
---@field scroll_position_taking_requests_tab integer
---@field scrolling_taking_requests_tab boolean
---@field scroll_position_taking_requests_tab_item integer
---@field scrolling_taking_requests_tab_item boolean
---@field taking_requests_selected_tab integer
---@field scroll_position_text integer
---@field scrolling_text boolean
---@field dipev dipscript_popup
---@field parley meeting_diplomat_info
---@field environment meeting_context

---@class gamest_main_interface_trade
---@field open boolean
---@field choosing_merchant boolean
---@field merlist caravan_state[]
---@field scroll_position_merlist integer
---@field scrolling_merlist boolean
---@field title string
---@field talker string
---@field fortname string
---@field place string
---@field st world_site
---@field bld building_tradedepotst
---@field mer caravan_state
---@field civ historical_entity
---@field stillunloading integer
---@field havetalker integer
---@field merchant_trader unit
---@field fortress_trader unit
---@field good any[]
---@field goodflag any[]
---@field good_amount any[]
---@field i_height any[]
---@field master_type_a_type any[]
---@field master_type_a_subtype any[]
---@field master_type_a_expanded any[]
---@field current_type_a_type any[]
---@field current_type_a_subtype any[]
---@field current_type_a_expanded any[]
---@field current_type_a_on any[]
---@field current_type_a_flag any[]
---@field scroll_position_item any[]
---@field scrolling_item any[]
---@field item_filter any[]
---@field entering_item_filter any[]
---@field talkline integer trade reply
---@field buildlists integer
---@field handle_appraisal integer
---@field counter_offer boolean
---@field counter_offer_item item[]
---@field scroll_position_counter_offer integer
---@field scrolling_counter_offer boolean
---@field entering_amount integer
---@field amount_str string
---@field big_announce string[]
---@field scroll_position_big_announce integer
---@field scrolling_big_announce boolean

---@class gamest_main_interface_assign_trade
---@field open boolean
---@field trade_depot_bld building_tradedepotst
---@field type_list integer[]
---@field filtered_type_list integer[]
---@field current_type item_type
---@field scroll_position_type integer
---@field scroll_position_item integer
---@field scrolling_type boolean
---@field scrolling_item boolean
---@field item_filter string
---@field entering_item_filter boolean
---@field storeamount integer[]
---@field badamount integer[]
---@field unk_a8 any[]
---@field unk_c0 integer[]
---@field unk_d8 integer[]
---@field unk_f0 integer[]
---@field unk_108 integer[]
---@field unk_120 integer[]
---@field unk_138 boolean[]
---@field i_height integer
---@field current_type_tgi any[]
---@field current_type_a_subtype integer[]
---@field current_type_a_subcat1 integer[]
---@field current_type_a_subcat2 integer[]
---@field current_type_a_amount integer[]
---@field current_type_a_expanded boolean[]
---@field current_type_a_on boolean[]
---@field current_type_a_flag integer[]
---@field sort_by_distance boolean
---@field pending_on_top boolean
---@field exclude_prohib boolean

---@class gamest_main_interface_buildjob
---@field display_furniture_bld building_display_furniturest since v0.44.01
---@field display_furniture_selected_item integer since v0.44.01

---@class gamest_main_interface_job_details since v0.42.06
---@field open boolean
---@field context job_details_context_type
---@field jb job
---@field wq manager_order
---@field current_option job_details_option_type
---@field current_option_index integer
---@field option job_details_option_type[]
---@field option_index integer[]
---@field scroll_position_option integer
---@field scrolling_option boolean
---@field search coord
---@field bld building
---@field material integer[]
---@field matgloss integer[]
---@field material_count integer[]
---@field material_master integer[]
---@field matgloss_master integer[]
---@field material_count_master integer[]
---@field scroll_position_material integer
---@field scrolling_material boolean
---@field material_filter string
---@field material_doing_filter boolean
---@field clothing_size_race_index integer[] race id
---@field clothing_size_race_index_master integer[] race id
---@field scroll_position_race integer
---@field scrolling_race boolean
---@field clothing_size_race_filter string
---@field clothing_size_race_doing_filter boolean
---@field improvement_type improvement_type[]
---@field scroll_position_improvement integer
---@field scrolling_improvement boolean

---@class gamest_main_interface_location_list
---@field valid_ab abstract_building[] since v0.42.01
---@field selected_ab integer since v0.42.01
---@field valid_religious_practice temple_deity_type[]
---@field valid_religious_practice_id temple_deity_data[]
---@field selected_religious_practice integer
---@field choosing_location_type boolean
---@field choosing_temple_religious_practice boolean
---@field choosing_craft_guild boolean
---@field valid_craft_guild_type profession[] since v0.47.01
---@field selected_craft_guild integer

---@class gamest_main_interface_hospital since v0.44.11
---@field cur_scroll integer
---@field bed_count integer
---@field table_count integer
---@field traction_bench_count integer
---@field box_count integer

---@class gamest_main_interface_view
---@field inv unit_inventory_item[]
---@field contam spatter[]
---@field guest_text any[] since v0.42.01
---@field uniform_selection boolean since v0.34.08
---@field selected_uniform integer since v0.34.08
---@field selected_squad integer since v0.34.08
---@field squad_list_sq squad[] since v0.34.08
---@field squad_list_ep entity_position[] since v0.34.08
---@field squad_list_epp entity_position_assignment[] since v0.34.08
---@field squad_list_has_subord_pos boolean[] since v0.34.08
---@field squad_list_add_index integer[] since v0.34.08
---@field create_ep entity_position
---@field create_epp entity_position_assignment
---@field create_sub_ep entity_position
---@field can_remove_from_squad boolean
---@field stuck_commander boolean
---@field have_calced_info boolean
---@field naming_squad boolean
---@field name_squad squad
---@field expel_total_list unit[]
---@field expel_outskirt_list world_site[] since v0.44.11
---@field expel_outskirt_list_selected integer since v0.44.11
---@field expel_selecting_destination integer since v0.44.11
---@field expel_cannot_expel_reason cannot_expel_reason_type since v0.44.11

---@class gamest_main_interface_burrow
---@field painting_burrow burrow
---@field doing_rectangle boolean
---@field erasing boolean
---@field scroll_position integer
---@field scrolling boolean
---@field entering_name boolean
---@field entering_name_index integer

---@class gamest_main_interface_civzone
---@field remove boolean
---@field flow_shape room_flow_shape_type
---@field doing_rectangle boolean
---@field doing_multizone boolean
---@field last_doing_multizone boolean
---@field box_on_left boolean
---@field erasing boolean
---@field adding_new_type integer
---@field cur_bld building_civzonest
---@field list building_civzonest[]
---@field zone_just_created building_civzonest[]
---@field furniture_rejected_in_use integer
---@field furniture_rejected_not_enclosed integer
---@field repainting integer

---@class gamest_main_interface_construction
---@field button interface_button[]
---@field press_button interface_button[]
---@field category interface_category_construction
---@field selected integer
---@field page construction_interface_pagest[]
---@field max_height integer
---@field total_width integer
---@field must_update_buttons boolean
---@field bb_placement_type integer
---@field bb_placement_subtype integer
---@field bb_placement_custom_building_id integer
---@field item_filter string
---@field entering_item_filter boolean
---@field scrolling_item boolean
---@field scroll_position_item integer

---@class gamest_main_interface_building
---@field button interface_button[]
---@field press_button interface_button[]
---@field filtered_button interface_button[]
---@field selected integer
---@field category interface_category_building
---@field material integer
---@field matgloss integer
---@field job_item_flag bitfield
---@field current_custom_category_token string since v0.42.01
---@field current_tool_tip string[] since v0.42.01

---@class gamest_main_interface_designation since v0.40.20
---@field marker_only boolean
---@field show_priorities boolean set to one if using +/-
---@field priority integer *1000
---@field mine_mode mine_mode_type
---@field show_advanced_options boolean
---@field entering_traffic_high_str boolean
---@field entering_traffic_normal_str boolean
---@field entering_traffic_low_str boolean
---@field entering_traffic_restricted_str boolean
---@field traffic_high_str string
---@field traffic_normal_str string
---@field traffic_low_str string
---@field traffic_restricted_str string
---@field sliding_traffic_high boolean
---@field sliding_traffic_normal boolean
---@field sliding_traffic_low boolean
---@field sliding_traffic_restricted boolean

---@class gamest_main_interface
---@field designation gamest_main_interface_designation since v0.40.20
---@field building gamest_main_interface_building
---@field construction gamest_main_interface_construction
---@field civzone gamest_main_interface_civzone
---@field burrow gamest_main_interface_burrow
---@field view gamest_main_interface_view
---@field hospital gamest_main_interface_hospital since v0.44.11
---@field location_list gamest_main_interface_location_list
---@field job_details gamest_main_interface_job_details since v0.42.06
---@field buildjob gamest_main_interface_buildjob
---@field assign_trade gamest_main_interface_assign_trade
---@field trade gamest_main_interface_trade
---@field diplomacy gamest_main_interface_diplomacy
---@field petitions gamest_main_interface_petitions
---@field stocks gamest_main_interface_stocks
---@field assign_display_item gamest_main_interface_assign_display_item
---@field name_creator gamest_main_interface_name_creator
---@field image_creator gamest_main_interface_image_creator
---@field unit_selector gamest_main_interface_unit_selector
---@field announcement_alert gamest_main_interface_announcement_alert
---@field custom_symbol gamest_main_interface_custom_symbol
---@field patrol_routes gamest_main_interface_patrol_routes
---@field squad_equipment gamest_main_interface_squad_equipment
---@field squad_schedule gamest_main_interface_squad_schedule
---@field squad_selector gamest_main_interface_squad_selector
---@field burrow_selector gamest_main_interface_burrow_selector
---@field location_selector gamest_main_interface_location_selector
---@field location_details gamest_main_interface_location_details
---@field hauling_stop_conditions gamest_main_interface_hauling_stop_conditions
---@field assign_vehicle gamest_main_interface_assign_vehicle
---@field stockpile gamest_main_interface_stockpile
---@field stockpile_link gamest_main_interface_stockpile_link
---@field stockpile_tools gamest_main_interface_stockpile_tools
---@field custom_stockpile gamest_main_interface_custom_stockpile
---@field view_sheets gamest_main_interface_view_sheets
---@field info gamest_main_interface_info
---@field squads gamest_main_interface_squads
---@field create_squad gamest_main_interface_create_squad
---@field squad_supplies gamest_main_interface_squad_supplies
---@field assign_uniform gamest_main_interface_assign_uniform
---@field create_work_order gamest_main_interface_create_work_order
---@field hotkey gamest_main_interface_hotkey
---@field options gamest_main_interface_options
---@field help gamest_main_interface_help
---@field settings main_interface_settings
---@field arena_unit gamest_main_interface_arena_unit
---@field arena_tree gamest_main_interface_arena_tree
---@field viewunit_list integer[]
---@field exporting_local integer
---@field mouse_zone integer
---@field skill_ind integer[]
---@field pract_type integer[]
---@field pract_ind integer[]
---@field skill_combat boolean
---@field skill_labor boolean
---@field skill_misc boolean
---@field barracks_selected_squad_ind integer
---@field barracks_squad squad[]
---@field barracks_squad_flag integer[]
---@field entering_building_name boolean
---@field assigning_position boolean
---@field ap_squad squad
---@field ap_sel integer
---@field assigning_position_squad boolean
---@field ap_squad_list squad[]
---@field ap_squad_sel integer
---@field pref_occupation any[] occupationst
---@field selected_pref_occupation integer
---@field main_designation_selected main_designation_type
---@field main_designation_doing_rectangles boolean
---@field bottom_mode_selected main_bottom_mode_type
---@field hover_instructions_on boolean
---@field hover_instructions_last_hover_tick integer
---@field current_hover integer
---@field current_hover_id1 integer union with current_hover_building_type
---@field current_hover_id2 integer union with current_hover_building_subtype
---@field current_hover_id3 integer union with current_hover_building_custom_id
---@field current_hover_key interface_key
---@field current_hover_alert popup_message
---@field current_hover_replace_minimap boolean
---@field current_hover_left_x integer
---@field current_hover_bot_y integer
---@field hover_instruction any[]
---@field last_displayed_hover_inst integer
---@field last_displayed_hover_id1 integer
---@field last_displayed_hover_id2 integer
---@field last_displayed_hover_id3 integer
---@field hover_announcement_alert popup_message
---@field hover_announcement_alert_text string[]
---@field hover_announcement_alert_color integer[]
---@field hover_announcement_alert_bright integer[]
---@field hover_announcement_alert_width integer
---@field hover_announcement_alert_button_text string[]
---@field hover_announcement_alert_button_color integer[]
---@field hover_announcement_alert_button_bright integer[]
---@field hover_announcement_alert_button_width integer
---@field last_hover_click_update integer
---@field last_hover_m coord
---@field recenter_indicator_m coord
---@field mouse_scrolling_map boolean
---@field mouse_anchor_mx integer
---@field mouse_anchor_my integer
---@field mouse_anchor_pmx integer
---@field mouse_anchor_pmy integer
---@field track_path coord_path
---@field keyboard_track_path coord_path
---@field last_track_s coord
---@field last_track_g coord
---@field keyboard_last_track_s coord
---@field keyboard_last_track_g coord

---@class gamest
---@field main_interface gamest_main_interface
---@field minimap gamest_minimap
---@field command_line gamest_command_line
---@field mod_manager gamest_mod_manager
---@field hash_rng hash_rngst
---@field play_rng hash_rngst
---@field start_tick_count integer
---@field autosave_cycle integer
---@field want_to_quit_to_title boolean
---@field flash_11_by_3 any[]
---@field flash_7_by_3 any[]
---@field flash_4_by_3 any[]
---@field external_flag integer
df.gamest = nil

---@class main_interface_settings
---@field open boolean
---@field context settings_context_type
---@field tab settings_tab_type[]
---@field current_mode settings_tab_type
---@field tabs_widget widget_tabs
---@field scroll_position_params integer
---@field scrolling_params boolean
---@field entering_value_str boolean
---@field entering_value_index integer
---@field value_str string
---@field member world_gen_param_basest[]
---@field fullscreen_resolution_open boolean
---@field permitted_fullscreen_w integer[]
---@field permitted_fullscreen_h integer[]
---@field scroll_position_permitted_fullscreen integer
---@field scrolling_permitted_fullscreen boolean
---@field keybinding_category integer[]
---@field keybinding_selected_category integer
---@field keybinding_scroll_position_cat integer
---@field keybinding_scrolling_cat boolean
---@field keybinding_name any[]
---@field keybinding_key any[]
---@field keybinding_binding any[]
---@field keybinding_binding_name any[]
---@field keybinding_flag any[]
---@field keybinding_scroll_position_key integer
---@field keybinding_scrolling_key boolean
---@field keybinding_registering_index integer
---@field keybinding_registering_adding_new boolean
---@field macro_list string[]
---@field difficulty difficultyst
---@field doing_custom_settings boolean
df.main_interface_settings = nil

---@class hash_rngst
---@field splitmix64_state integer
---@field unnamed_hash_rngst_2 integer
---@field unnamed_hash_rngst_3 integer
df.hash_rngst = nil

---@class difficultyst
---@field difficulty_enemies integer 0=off, 1=normal, 2=hard, 3=custom
---@field difficulty_economy integer 0=normal, 1=hard, 2=custom
---@field enemy_pop_trigger integer[]
---@field enemy_prod_trigger integer[]
---@field enemy_trade_trigger integer[]
---@field megabeast_interval integer
---@field forgotten_sens integer
---@field forgotten_irritate_min integer
---@field forgotten_wealth_div integer
---@field wild_sens integer
---@field wild_irritate_min integer
---@field wild_irritate_decay integer
---@field werebeast_interval integer
---@field vampire_fraction integer
---@field invasion_cap_regular integer[]
---@field invasion_cap_monsters integer[]
---@field min_raids_before_siege integer
---@field min_raids_between_sieges integer
---@field siege_frequency integer
---@field cavern_dweller_scale integer
---@field cavern_dweller_max_attackers integer
---@field tree_fell_count_savage integer
---@field tree_fell_count integer
---@field flags bitfield
---@field economy_pop_trigger integer[]
---@field economy_prod_trigger integer[]
---@field economy_trade_trigger integer[]
---@field land_holder_pop_trigger integer[]
---@field land_holder_prod_trigger integer[]
---@field land_holder_trade_trigger integer[]
---@field temple_value integer
---@field temple_complex_value integer
---@field priesthood_unit_count integer
---@field high_priesthood_unit_count integer
---@field guildhall_vaue integer
---@field grand_guildhall_value integer
---@field guild_unit_count integer
---@field grand_guild_unit_count integer
---@field mandate_period integer
---@field demand_period integer
df.difficultyst = nil

---@class markup_text_boxst
---@field unk1 any[]
---@field unk_v50_2 any[]
---@field unk_v50_3 integer
---@field unk_v50_4 integer
---@field unk_v50_5 integer
---@field unk_v50_6 integer
df.markup_text_boxst = nil

---@class wqc_item_traitst
---@field flg integer
---@field flgn integer
---@field reaction_class string
---@field reaction_product_class string
---@field metal_ore integer
---@field contains_reaction_index integer
---@field contains_reagent_index integer
---@field tool_use integer
---@field display_string string
---@field on boolean
df.wqc_item_traitst = nil

---@class cwo_buildingst
---@field type integer
---@field subtype integer
---@field custom_id integer
---@field jminfo manager_order_template[]
---@field name string
df.cwo_buildingst = nil

---@class cri_unitst
---@field un unit
---@field it item
---@field jb job
---@field profession_list_order1 integer
---@field profession_list_order2 integer
---@field stress integer
---@field flag integer
---@field sort_name string
---@field job_sort_name string
---@field owner_un unit
df.cri_unitst = nil

---@class actor_entryst
---@field hf historical_figure
---@field iden integer identityst
---@field name_ptr language_name
---@field list_name string
---@field simple_list_name string
---@field p_list_name string
---@field main_text_box string[]
---@field visual_hfid integer
---@field historical_hfid integer
---@field identity_id integer
---@field alias_identity_id integer[]
---@field principle_org integer organization_entryst
---@field associated_org any[] organization_entryst
---@field associated_plot plot_entryst[]
---@field flag integer
df.actor_entryst = nil

---@class organization_entry_nodest
---@field actor_entry actor_entryst
---@field master organization_entry_nodest
---@field sort_id integer
---@field tier integer
---@field x integer
---@field descendant_sum integer
---@field label string
---@field fcol integer
---@field bcol integer
---@field br integer
---@field name string
---@field status string
df.organization_entry_nodest = nil

---@class organization_entryst
---@field node organization_entry_nodest[]
---@field list_name string
---@field simple_list_name string
---@field p_list_name string
---@field main_text_box string[]
---@field principle_actor_entry actor_entryst
---@field flag integer
df.organization_entryst = nil

---@class plot_entryst
---@field list_name string
---@field simple_list_name string
---@field p_list_name string
---@field agreement integer agreementst
---@field master_hfid integer
---@field organization_name string
df.plot_entryst = nil

---@class mod_headerst
---@field zip_filename string
---@field unzipped_folder string
---@field id string
---@field numeric_version integer
---@field displayed_version string
---@field earliest_compatible_numeric_version integer
---@field earliest_compatible_displayed_version string
---@field author string
---@field name string
---@field description string
---@field dependencies string[]
---@field dependency_type integer[] 0 exact, 1 before, 2 after
---@field conflicts string[]
---@field flags bitfield
---@field src_dir string
---@field steam_file_id integer
---@field steam_title string
---@field steam_description string
---@field steam_tag string[]
---@field steam_key_tag string[]
---@field steam_value_tag string[]
---@field steam_metadata string
---@field steam_changelog string
---@field steamapi_1 string
---@field steamapi_2 boolean
---@field steamapi_3 integer
df.mod_headerst = nil

---@class ui_look_list
---@field items any[]
df.global.ui_look_list = nil

---@enum ui_unit_view_mode_value
df.ui_unit_view_mode_value = {
    General = 0,
    Inventory = 1,
    Preferences = 2,
    Wounds = 3,
    PrefLabor = 4,
    PrefDogs = 5,
    PrefOccupation = 6,
}
---@class ui_unit_view_mode
---@field ui_unit_view_mode_value ui_unit_view_mode_value
df.global.ui_unit_view_mode = nil


