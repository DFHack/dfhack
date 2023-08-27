-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.

---@meta

---@alias job_material_category bitfield

---@alias dfhack_material_category bitfield

---@alias job_list_link job[]

---@alias job_flags bitfield

---@enum job_subtype_surgery
df.job_subtype_surgery = {
    Surgery = 0,
    StopBleeding = 1,
    RepairCompoundFracture = 2,
    RemoveRottenTissue = 3,
}

---@class job
---@field id integer
---@field list_link job_list_link
---@field posting_index integer index into world.job_postings, since v0.40.20
---@field job_type job_type
---@field job_subtype integer
---@field pos coord
---@field completion_timer integer -1 every time unit.counters.job_counter is below 0
---@field unk4 integer garbage, but serialized
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field unk5 integer
---@field item_type item_type for Bait Trap jobs
---@field item_subtype integer when StoreInStockpile this is a unit_labor
---@field item_category bitfield
---@field unnamed_job_17 hist_figure_id | race | improvement
---@field material_category bitfield
---@field reaction_name string
---@field expire_timer integer for stockpiling, +1 per 50 ticks if no worker; del when 20
---@field recheck_cntdn integer for process_jobs
---@field wait_timer integer for units to leave build sites; to recheck stockpiles
---@field unk11 integer since v0.34.08
---@field items job_item_ref[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field job_items job_item[]
---@field guide_path coord_path since v0.34.08
---@field cur_path_index integer since v0.34.08
---@field unk_v4020_2 coord since v0.40.20
---@field art_spec job_art_specification since v0.43.01
---@field order_id integer since v0.43.01
df.job = nil

---@enum job_item_ref_role
df.job_item_ref_role = {
    Other = 0, -- eat, drink, pickup equipment
    Reagent = 1,
    Hauled = 2,
    LinkToTarget = 3, -- used when linking a lever to a building, not sure if swapped
    LinkToTrigger = 4,
    unk_5 = 5,
    TargetContainer = 6,
    QueuedContainer = 7, -- queued to be put in a container
    PushHaulVehicle = 8, -- wheelbarrow
}
---@class job_item_ref
---@field item item
---@field job_item_ref_role job_item_ref_role
---@field is_fetching integer 0 immediately once taken to be brought
---@field job_item_idx integer
df.job_item_ref = nil

---@alias job_item_flags1 bitfield

---@alias job_item_flags2 bitfield

---@alias job_item_flags3 bitfield

---@class job_item
---@field item_type item_type
---@field item_subtype integer
---@field mat_type integer
---@field mat_index integer
---@field flags1 bitfield
---@field quantity integer
---@field vector_id job_item_vector_id
---@field flags2 bitfield
---@field flags3 bitfield
---@field flags4 integer
---@field flags5 integer
---@field metal_ore integer
---@field reaction_class string
---@field has_material_reaction_product string
---@field min_dimension integer pure guess by context
---@field reagent_index integer
---@field contains integer[] used with custom reactions
---@field reaction_id integer
---@field has_tool_use tool_uses
---@field unk_v43_1 integer since v0.43.01
---@field unk_v43_2 integer since v0.43.01
---@field unk_v43_3 integer since v0.43.01
---@field unk_v43_4 integer since v0.43.01
df.job_item = nil

---@class job_item_filter
---@field item_type item_type
---@field item_subtype integer
---@field mat_type integer
---@field mat_index integer
---@field flags1 bitfield
---@field item_vector integer
---@field use_mat_index boolean
---@field flags2 bitfield
---@field use_flags2 boolean
---@field flags3 bitfield
---@field use_flags3 boolean
---@field flags4 integer
---@field use_flags4 boolean
---@field flags5 integer
---@field use_flags5 boolean
---@field reaction_class string
---@field has_material_reaction_product string
---@field metal_ore integer
---@field use_metal_ore boolean
---@field use_reaction_class boolean
---@field use_reaction_product boolean
---@field min_dimension integer
---@field reaction_id integer
---@field contains integer[]
---@field use_contains boolean
---@field has_tool_use tool_uses
---@field has_melee_skill job_skill since v0.34.01
---@field unk_v40_1 integer noticed in v0.40.24
---@field pos coord
---@field unit unit
---@field job job
---@field building building
---@field unk_74 integer
---@field unk_v4305_1 integer since v0.43.05
---@field burrows integer[]
---@field use_burrows boolean
---@field take_from integer since v0.34.08
df.job_item_filter = nil

---@alias manager_order_status bitfield

---@enum job_art_specification_type
df.job_art_specification_type = {
    None = -1,
    HistoricalFigure = 0,
    Site = 1,
    Entity = 2,
    ArtImage = 3,
}
---@class job_art_specification
---@field job_art_specification_type job_art_specification_type
---@field id integer
---@field subid integer
df.job_art_specification = nil

---@enum manager_order_frequency
df.manager_order_frequency = {
    OneTime = 0,
    Daily = 1,
    Monthly = 2,
    Seasonally = 3,
    Yearly = 4,
}
---@class manager_order
---@field id integer
---@field job_type job_type
---@field item_type item_type
---@field item_subtype integer
---@field reaction_name string
---@field mat_type integer
---@field mat_index integer
---@field item_category bitfield
---@field hist_figure_id integer
---@field material_category bitfield
---@field art_spec job_art_specification
---@field amount_left integer
---@field amount_total integer
---@field status bitfield
---@field manager_order_frequency manager_order_frequency
---@field finished_year integer
---@field finished_year_tick integer
---@field workshop_id integer
---@field max_workshops integer 0 is unlimited
---@field item_conditions manager_order_condition_item[]
---@field order_conditions manager_order_condition_order[]
---@field items integer
df.manager_order = nil

---@enum manager_order_condition_item_compare_type
df.manager_order_condition_item_compare_type = {
    AtLeast = 0,
    AtMost = 1,
    GreaterThan = 2,
    LessThan = 3,
    Exactly = 4,
    Not = 5,
}
---@class manager_order_condition_item
---@field manager_order_condition_item_compare_type manager_order_condition_item_compare_type
---@field compare_val integer
---@field item_type item_type
---@field item_subtype integer
---@field mat_type integer
---@field mat_index integer
---@field flags1 bitfield
---@field flags2 bitfield
---@field flags3 bitfield
---@field flags4 integer
---@field flags5 integer
---@field reaction_class string
---@field has_material_reaction_product string
---@field inorganic_bearing integer
---@field min_dimension integer
---@field contains integer[]
---@field reaction_id integer
---@field has_tool_use tool_uses
df.manager_order_condition_item = nil

---@enum manager_order_condition_order_condition
df.manager_order_condition_order_condition = {
    Activated = 0,
    Completed = 1,
}
---@class manager_order_condition_order
---@field order_id integer
---@field manager_order_condition_order_condition manager_order_condition_order_condition
---@field unk_1 integer
df.manager_order_condition_order = nil

---@class manager_order_template jminfost
---@field job_type job_type
---@field reaction_name string
---@field item_type item_type
---@field item_subtype integer
---@field mat_type integer
---@field mat_index integer
---@field item_category bitfield specflag
---@field hist_figure_id integer
---@field material_category bitfield
---@field match_value integer
---@field name string
---@field compare_str string
---@field on boolean
df.manager_order_template = nil

---@class mandate_punishment
---@field hammerstrikes integer
---@field prison_time integer
---@field give_beating integer

---@enum mandate_mode
df.mandate_mode = {
    Export = 0,
    Make = 1,
    Guild = 2,
}
---@class mandate
---@field unit unit
---@field mandate_mode mandate_mode
---@field item_type item_type
---@field item_subtype integer
---@field mat_type integer
---@field mat_index integer
---@field amount_total integer
---@field amount_remaining integer
---@field timeout_counter integer counts once per 10 frames
---@field timeout_limit integer once counter passes limit, mandate ends
---@field punishment mandate_punishment
---@field punish_multiple integer
---@field unk4 integer
df.mandate = nil

---@class training_assignment
---@field animal_id integer
---@field trainer_id integer
---@field flags bitfield
df.training_assignment = nil

---@enum unit_demand_place
df.unit_demand_place = {
    Office = 0,
    Bedroom = 1,
    DiningRoom = 2,
    Tomb = 3,
}
---@class unit_demand
---@field unk_0 integer
---@field unit_demand_place unit_demand_place
---@field item_type item_type
---@field item_subtype integer
---@field mat_type integer
---@field mat_index integer
---@field timeout_counter integer counts once per 10 frames
---@field timeout_limit integer once counter passes limit, mandate ends
df.unit_demand = nil

---@enum job_cancel_reason
df.job_cancel_reason = {
    CANNOT_REACH_SITE = 0,
    INTERRUPTED = 1,
    MOVED = 2,
    NEED_EMPTY_BUCKET = 3,
    NEED_EMPTY_TRAP = 4,
    NEED_EMPTY_BAG = 5,
    NEED_EMPTY_CAGE = 6,
    INCAPABLE_OF_CARRYING = 7,
    TOO_INJURED = 8,
    EXHAUSTED = 9,
    ANIMAL_INACCESSIBLE = 10,
    ITEM_INACCESSIBLE = 11,
    PATIENT_INACCESSIBLE = 12,
    INFANT_INACCESSIBLE = 13,
    NO_PARTNER = 14,
    NOTHING_IN_CAGE = 15,
    NOTHING_TO_CAGE = 16,
    NOTHING_TO_CATCH = 17,
    NO_PATIENT = 18,
    PATIENT_NOT_RESTING = 19,
    NO_INFANT = 20,
    ALREADY_LEADING_CREATURE = 21,
    NO_FOOD_AVAILABLE = 22,
    NEEDS_SPECIFIC_ITEM = 23,
    NO_ITEM = 24,
    NO_AMMUNITION = 25,
    NO_WEAPON = 26,
    WRONG_AMMUNITION = 27,
    AMMUNITION_INACCESSIBLE = 28,
    ITEM_BLOCKING_SITE = 29,
    ANIMAL_NOT_RESTRAINED = 30,
    NO_CREATURE = 31,
    NO_BUILDING = 32,
    INAPPROPRIATE_BUILDING = 33,
    NO_DESIGNATED_AREA = 34,
    NO_FLOOR_SPACE = 35,
    NO_PARTY = 36,
    WRONG_JUSTICE_STATE = 37,
    NOTHING_IN_BUILDING = 38,
    RELIEVED = 39,
    WATER_IS_FROZEN = 40,
    TOO_INSANE = 41,
    TAKEN_BY_MOOD = 42,
    WENT_INSANE = 43,
    THROWING_TANTRUM = 44,
    COULD_NOT_FIND_PATH = 45,
    PATH_BLOCKED = 46,
    SEEKING_ARTIFACT = 47,
    HANDLING_DANGEROUS_CREATURE = 48,
    GOING_TO_BED = 49,
    SEEKING_INFANT = 50,
    DANGEROUS_TERRAIN = 51,
    JOB_ITEM_LOST = 52,
    GETTING_FOOD = 53,
    GETTING_WATER = 54,
    HUNTING_VERMIN_FOR_FOOD = 55,
    TARGET_INACCESSIBLE = 56,
    NO_TARGET = 57,
    NO_MECHANISM_FOR_TARGET = 58,
    NO_TARGET_BUILDING = 59,
    NO_MECHANISM_FOR_TRIGGER = 60,
    NO_TRIGGER = 61,
    NO_AVAILABLE_TRACTION_BENCH = 62,
    ATTACKING_BUILDING = 63,
    LOST_PICK = 64,
    INVALID_OFFICER = 65,
    FAREWELL = 66,
    REMOVED_FROM_GUARD = 67,
    EQUIPMENT_MISMATCH = 68,
    UNCONSCIOUS = 69,
    WEBBED = 70,
    PARALYZED = 71,
    CAGED = 72,
    GETTING_DRINK = 73,
    USING_WELL = 74,
    LOST_AXE = 75,
    RESTING_INJURY = 76,
    UNSCHEDULED = 77,
    FORBIDDEN_AREA = 78,
    DROFOFF_INACCESSIBLE = 79,
    BUILDING_INACCESSIBLE = 80,
    AREA_INACCESSIBLE = 81,
    WATER_SOURCE_VANISHED = 82,
    NO_WATER_SOURCE = 83,
    NO_BUCKET_AT_WELL = 84,
    BUCKET_NOT_EMPTY = 85,
    WELL_DRY = 86,
    BUILDING_SITE_SUBMERGED = 87,
    NEED_SAND_COLLECTION_ZONE = 88,
    SAND_VANISHED = 89,
    AREA_BECAME_INAPPROPRIATE = 90,
    WATER_SOURCE_CONTAMINATED = 91,
    CREATURE_OCCUPYING_SITE = 92,
    NEED_OFFICE = 93,
    NOT_RESPONSIBLE_FOR_TRADE = 94,
    INAPPROPRIATE_DIG_SQUARE = 95,
    TARGET_TOO_INJURED = 96,
    GETTING_MARRIED = 97,
    NEED_SPLINT = 98,
    NEED_THREAD = 99,
    NEED_CLOTH = 100,
    NEED_CRUTCH = 101,
    BAD_SCRIPT_1 = 102,
    BAD_SCRIPT_2 = 103,
    BAD_SCRIPT_3 = 104,
    NEED_CAST_POWDER_BAG = 105,
    NO_WEAPON_2 = 106,
    NO_APPROPRIATE_AMMUNITION = 107,
    CLAY_VANISHED = 108,
    NEED_CLAY_COLLECTION_ZONE = 109,
    NO_COLONY = 110,
    NOT_APPOINTED = 111,
    NO_WEAPON_FOR_EXECUTION = 112,
    NO_LONGER_REQUESTED = 113,
    MORTALLY_AFRAID = 114,
    EMOTIONAL_SHOCK = 115,
    HORRIFIED = 116,
    GRIEVING = 117,
    TERRIFIED = 118,
    IN_CUSTODY = 119,
    TOO_DEPRESSED = 120,
    OBLIVIOUS = 121,
    CATATONIC = 122,
    TOO_SAD = 123,
    IN_AGONY = 124,
    ANGUISHED = 125,
    DESPAIRING = 126,
    DISMAYED = 127,
    DISTRESSED = 128,
    FRIGHTENED = 129,
    MISERABLE = 130,
    MORTIFIED = 131,
    SHAKEN = 132,
    IN_EXISTENTIAL_CRISIS = 133,
    NEEDS_SPECIFIC_ITEM_2 = 134,
}

---@class job_cancel
---@field job_cancel_reason job_cancel_reason
---@field item_type item_type
---@field unnamed_job_cancel_3 integer
---@field unnamed_job_cancel_4 integer
---@field unnamed_job_cancel_5 integer
---@field unnamed_job_cancel_6 integer
---@field unnamed_job_cancel_7 integer
---@field unnamed_job_cancel_8 integer
---@field unnamed_job_cancel_9 bitfield
---@field unnamed_job_cancel_10 integer
---@field unnamed_job_cancel_11 integer
---@field unnamed_job_cancel_12 integer
---@field unnamed_job_cancel_13 string
---@field unnamed_job_cancel_14 string
---@field unnamed_job_cancel_15 integer
---@field unnamed_job_cancel_16 integer
---@field unnamed_job_cancel_17 integer[]
---@field unnamed_job_cancel_18 integer
---@field tool_uses tool_uses
---@field unnamed_job_cancel_20 integer
---@field unnamed_job_cancel_21 integer
---@field unnamed_job_cancel_22 integer
df.job_cancel = nil


