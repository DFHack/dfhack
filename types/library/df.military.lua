-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.

---@meta

---@alias uniform_indiv_choice bitfield

---@class item_filter_spec
---@field item_type item_type
---@field item_subtype integer
---@field material_class entity_material_category
---@field mattype integer
---@field matindex integer
df.item_filter_spec = nil

---@class squad_uniform_spec
---@field item integer
---@field item_filter item_filter_spec
---@field color integer
---@field assigned any[]
---@field indiv_choice bitfield
df.squad_uniform_spec = nil

---@class squad_ammo_spec
---@field item_filter item_filter_spec
---@field amount integer
---@field flags bitfield
---@field assigned any[]
df.squad_ammo_spec = nil

---@alias squad_use_flags bitfield

---@enum uniform_category
df.uniform_category = {
    body = 0,
    head = 1,
    pants = 2,
    gloves = 3,
    shoes = 4,
    shield = 5,
    weapon = 6,
}

---@alias uniform_flags bitfield

---@enum barrack_preference_category
df.barrack_preference_category = {
    Bed = 0,
    Armorstand = 1,
    Box = 2,
    Cabinet = 3,
}

---@enum squad_event_type
df.squad_event_type = {
    None = -1,
    Unk0 = 0,
    Unk1 = 1,
    Unk2 = 2,
}

---@class squad_position
---@field occupant integer
---@field orders squad_order[]
---@field preferences any[]
---@field uniform any[]
---@field unk_c4 string
---@field flags bitfield
---@field assigned_items any[]
---@field quiver integer
---@field backpack integer
---@field flask integer
---@field unk_1 integer
---@field activities integer[]
---@field events integer[]
---@field unk_2 integer
df.squad_position = nil

---@class squad_schedule_order
---@field order squad_order
---@field min_count integer
---@field positions boolean[]
df.squad_schedule_order = nil

---@class squad_schedule_entry
---@field name string
---@field sleep_mode integer 0 room, 1 barrack will, 2 barrack need
---@field uniform_mode integer 0 uniformed, 1 civ clothes
---@field orders squad_schedule_order[]
---@field order_assignments any[]
df.squad_schedule_entry = nil

---@class squad_ammo
---@field ammunition squad_ammo_spec[]
---@field train_weapon_free any[]
---@field train_weapon_inuse any[]
---@field ammo_items any[]
---@field ammo_units any[]
---@field update bitfield

---@class squad
---@field id integer
---@field name language_name
---@field alias string if not empty, used instead of name
---@field positions squad_position[]
---@field orders squad_order[]
---@field schedule any[]
---@field cur_routine_idx integer
---@field rooms any[]
---@field rack_combat integer[]
---@field rack_training integer[]
---@field uniform_priority integer
---@field activity integer
---@field ammo squad_ammo
---@field carry_food integer
---@field carry_water integer
---@field entity_id integer since v0.40.01
---@field leader_position integer since v0.40.01
---@field leader_assignment integer since v0.40.01
---@field unk_1 integer since v0.44.01
---@field unk_v50_1 integer Appears to be a transient per-squad texture id. Initialised on squad ui click
---@field unk_v50_2 integer Always 1 less than the above field when initialised, and has tied initialisation
---@field symbol integer 0 to 22 inclusive, row-wise. Only used in graphics mode
---@field foreground_r integer
---@field foreground_g integer
---@field foreground_b integer
---@field background_r integer
---@field background_g integer
---@field background_b integer
df.squad = nil

---@enum squad_order_type
df.squad_order_type = {
    MOVE = 0,
    KILL_LIST = 1,
    DEFEND_BURROWS = 2,
    PATROL_ROUTE = 3,
    TRAIN = 4,
    DRIVE_ENTITY_OFF_SITE = 5,
    CAUSE_TROUBLE_FOR_ENTITY = 6,
    KILL_HF = 7,
    DRIVE_ARMIES_FROM_SITE = 8,
    RETRIEVE_ARTIFACT = 9,
    RAID_SITE = 10,
    RESCUE_HF = 11,
}

---@enum squad_order_cannot_reason
df.squad_order_cannot_reason = {
    not_following_order = 0,
    activity_cancelled = 1,
    no_barracks = 2,
    improper_barracks = 3,
    no_activity = 4,
    cannot_individually_drill = 5,
    does_not_exist = 6,
    no_archery_target = 7,
    improper_building = 8,
    unreachable_location = 9,
    invalid_location = 10,
    no_reachable_valid_target = 11,
    no_burrow = 12,
    not_in_squad = 13,
    no_patrol_route = 14,
    no_reachable_point_on_route = 15,
    invalid_order = 16,
    no_temple = 17,
    no_library = 18,
    no_item = 19,
    cannot_leave_site = 20,
}

---@return squad_order
function df.squad_order.clone() end

---@param file file_compressorst
function df.squad_order.write_file(file) end

---@param file file_compressorst
---@param loadversion save_version
function df.squad_order.read_file(file, loadversion) end

---@return squad_order_type
function df.squad_order.getType() end

---@return boolean
function df.squad_order.isPatrol() end

---@param x integer
---@param y integer
---@param z integer
function df.squad_order.offsetPosition(x, y, z) end

---@param arg_0 integer
---@param arg_1 integer
---@param soldier unit
function df.squad_order.process(arg_0, arg_1, soldier) end

---@param soldier unit
---@return squad_order_cannot_reason
function df.squad_order.reasonCannot(soldier) end

---@param soldier unit
---@return boolean
function df.squad_order.decUniformLock(soldier) end

---@return boolean
function df.squad_order.isFulfilled() end -- true if all killed, since v0.34.11

---@return integer
function df.squad_order.getTargetUnits() end

---@param soldier unit
---@return integer
function df.squad_order.getUniformType(soldier) end

---@param arg_0 string
function df.squad_order.getDescription(arg_0) end

---@param arg_0 integer
---@return boolean
function df.squad_order.isInactive(arg_0) end -- always false

---@return boolean
function df.squad_order.isCombat() end -- not train

---@param other squad_order
---@return boolean
function df.squad_order.isEqual(other) end

---@class squad_order
---@field unk_v40_1 integer since v0.40.01
---@field unk_v40_2 integer since v0.40.01
---@field year integer since v0.40.01
---@field year_tick integer since v0.40.01
---@field unk_v40_3 integer since v0.40.01
---@field unk_1 integer
df.squad_order = nil

---@class squad_order_movest
-- inherited from squad_order
---@field unk_v40_1 integer since v0.40.01
---@field unk_v40_2 integer since v0.40.01
---@field year integer since v0.40.01
---@field year_tick integer since v0.40.01
---@field unk_v40_3 integer since v0.40.01
---@field unk_1 integer
-- end squad_order
---@field pos coord
---@field point_id integer
df.squad_order_movest = nil

---@class squad_order_kill_listst
-- inherited from squad_order
---@field unk_v40_1 integer since v0.40.01
---@field unk_v40_2 integer since v0.40.01
---@field year integer since v0.40.01
---@field year_tick integer since v0.40.01
---@field unk_v40_3 integer since v0.40.01
---@field unk_1 integer
-- end squad_order
---@field units any[]
---@field histfigs any[]
---@field title string
df.squad_order_kill_listst = nil

---@class squad_order_defend_burrowsst
-- inherited from squad_order
---@field unk_v40_1 integer since v0.40.01
---@field unk_v40_2 integer since v0.40.01
---@field year integer since v0.40.01
---@field year_tick integer since v0.40.01
---@field unk_v40_3 integer since v0.40.01
---@field unk_1 integer
-- end squad_order
---@field burrows integer[]
df.squad_order_defend_burrowsst = nil

---@class squad_order_patrol_routest
-- inherited from squad_order
---@field unk_v40_1 integer since v0.40.01
---@field unk_v40_2 integer since v0.40.01
---@field year integer since v0.40.01
---@field year_tick integer since v0.40.01
---@field unk_v40_3 integer since v0.40.01
---@field unk_1 integer
-- end squad_order
---@field route_id integer
df.squad_order_patrol_routest = nil

---@class squad_order_trainst
-- inherited from squad_order
---@field unk_v40_1 integer since v0.40.01
---@field unk_v40_2 integer since v0.40.01
---@field year integer since v0.40.01
---@field year_tick integer since v0.40.01
---@field unk_v40_3 integer since v0.40.01
---@field unk_1 integer
-- end squad_order
df.squad_order_trainst = nil

---@class squad_order_drive_entity_off_sitest
-- inherited from squad_order
---@field unk_v40_1 integer since v0.40.01
---@field unk_v40_2 integer since v0.40.01
---@field year integer since v0.40.01
---@field year_tick integer since v0.40.01
---@field unk_v40_3 integer since v0.40.01
---@field unk_1 integer
-- end squad_order
---@field unk_2 integer
---@field unk_3 integer
---@field unk_4 string
df.squad_order_drive_entity_off_sitest = nil

---@class squad_order_cause_trouble_for_entityst
-- inherited from squad_order
---@field unk_v40_1 integer since v0.40.01
---@field unk_v40_2 integer since v0.40.01
---@field year integer since v0.40.01
---@field year_tick integer since v0.40.01
---@field unk_v40_3 integer since v0.40.01
---@field unk_1 integer
-- end squad_order
---@field entity_id integer
---@field override_name string
df.squad_order_cause_trouble_for_entityst = nil

---@class squad_order_kill_hfst
-- inherited from squad_order
---@field unk_v40_1 integer since v0.40.01
---@field unk_v40_2 integer since v0.40.01
---@field year integer since v0.40.01
---@field year_tick integer since v0.40.01
---@field unk_v40_3 integer since v0.40.01
---@field unk_1 integer
-- end squad_order
---@field histfig_id integer
---@field title string
df.squad_order_kill_hfst = nil

---@class squad_order_drive_armies_from_sitest
-- inherited from squad_order
---@field unk_v40_1 integer since v0.40.01
---@field unk_v40_2 integer since v0.40.01
---@field year integer since v0.40.01
---@field year_tick integer since v0.40.01
---@field unk_v40_3 integer since v0.40.01
---@field unk_1 integer
-- end squad_order
---@field unk_2 integer
---@field unk_3 integer
---@field unk_4 string
df.squad_order_drive_armies_from_sitest = nil

---@class squad_order_retrieve_artifactst
-- inherited from squad_order
---@field unk_v40_1 integer since v0.40.01
---@field unk_v40_2 integer since v0.40.01
---@field year integer since v0.40.01
---@field year_tick integer since v0.40.01
---@field unk_v40_3 integer since v0.40.01
---@field unk_1 integer
-- end squad_order
---@field artifact_id integer
---@field unk_2 coord
df.squad_order_retrieve_artifactst = nil

---@class squad_order_raid_sitest
-- inherited from squad_order
---@field unk_v40_1 integer since v0.40.01
---@field unk_v40_2 integer since v0.40.01
---@field year integer since v0.40.01
---@field year_tick integer since v0.40.01
---@field unk_v40_3 integer since v0.40.01
---@field unk_1 integer
-- end squad_order
---@field unk_2 integer
---@field unk_3 coord
df.squad_order_raid_sitest = nil

---@class squad_order_rescue_hfst
-- inherited from squad_order
---@field unk_v40_1 integer since v0.40.01
---@field unk_v40_2 integer since v0.40.01
---@field year integer since v0.40.01
---@field year_tick integer since v0.40.01
---@field unk_v40_3 integer since v0.40.01
---@field unk_1 integer
-- end squad_order
---@field unk_2 integer
---@field unk_3 coord
df.squad_order_rescue_hfst = nil

---@enum army_controller_type
df.army_controller_type = {
    t0 = 0,
    t1 = 1,
    InvasionOrder = 2,
    t3 = 3,
    Invasion = 4, -- Seen on army_controller entries of armies referenced by InvasionOrder controllers (which do not refer to the InvasionOrder entries). These Invasion entries are not in the 'all' vector
    t5 = 5,
    t6 = 6,
    t7 = 7,
    t8 = 8,
    t9 = 9,
    t10 = 10,
    t11 = 11,
    Visit = 12, -- Used both for regular visitors to a fortress and exiled characters sent to 'visit' other sites
    t13 = 13,
    t14 = 14,
    t15 = 15,
    t16 = 16,
    Quest = 17, -- Used to indicate artifact quests
    t18 = 18,
    t19 = 19,
    t20 = 20,
    t21 = 21,
    t22 = 22,
    t23 = 23,
    VillainousVisit = 24,
}
---@class army_controller
---@field id integer all army.controllers seen and reached via InvasionOrder controllers' armies have been of type = Invasion and absent from the 'all' vector
---@field entity_id integer
---@field site_id integer Invasion/Order: site to invade. Visit/Quest/VillainousVisit: site to 'visit'
---@field unk_1 integer
---@field pos_x integer Look like the unit is map_block, i.e. 3 * 16 * world tile. Position of target, which is the starting point for defeated invasions
---@field pos_y integer
---@field unk_18 integer Seen one case of 1990 for VillainVisiting
---@field unk_1c integer same value for the same visitor
---@field unk_20 integer[]
---@field year integer
---@field year_tick integer
---@field unk_34 integer id of other army controller (Invasion) from same entity seen here
---@field unk_38 integer copy of the id seen here, as well as a t7 for a t5 controller
---@field master_hf integer InvasionOrder: Civ/sitegov master. Invasion: leader of the attack, can be in army nemesis vector
---@field general_hf integer InvasionOrder:leader of the attack. Invasion: subordinate squad leader(?) in army nemesis vector. Can be same as master
---@field unk_44_1 integer
---@field unk_44_2 integer
---@field visitor_nemesis_id integer Set for VillainousVisit
---@field unk_44_4 integer 3, 6 seen for Villain
---@field unk_44_5 integer[]
---@field unk_50 integer
---@field unk_54 integer[]
---@field unk_44_11v integer[] since v0.44.11
---@field unk_v50_b0 integer[] since v0.50.01
---@field mission_report mission_report
---@field data t1 | InvasionOrder | Invasion | t5 | t6 | t7 | t11 | Visit | t13 | t14 | t15 | t16 | Quest | t18 | t19 | t20 | t21 | t22 | t23 | VillainousVisit
---@field army_controller_type army_controller_type
df.army_controller = nil

---@class army_controller_sub1
---@field unk_1 integer
---@field unk_2 integer
---@field unk_3 integer
df.army_controller_sub1 = nil

---@class army_controller_invasion_order
---@field unk_1 integer
---@field unk_2 integer
---@field unk_3 integer
---@field unk_4 integer
---@field unk_4a any[]
---@field unk_5 integer[]
---@field unk_6 integer[]
---@field unk_7 integer
---@field unk_8 integer
---@field unk_9 integer
---@field unk_10 integer[] since v0.44.06
df.army_controller_invasion_order = nil

---@class army_controller_invasion
---@field unk_1 integer
---@field unk_2 bitfield
df.army_controller_invasion = nil

---@class army_controller_sub5
---@field pos_x integer in map_block coordinates. Same as those of the main struct seen
---@field pos_y integer
---@field unk_1 integer 0 seen
---@field year integer
---@field year_tick integer
df.army_controller_sub5 = nil

---@class army_controller_sub6
---@field unk_1 integer
---@field unk_2 integer
---@field unk_3 integer
---@field unk_4 integer
---@field unk_5 integer
---@field unk_6 integer
---@field unk_7 integer
---@field unk_8 integer
---@field unk_9 integer
df.army_controller_sub6 = nil

---@class army_controller_sub7
---@field unk_1 integer
---@field unk_2 integer
---@field unk_3 any[]
---@field unk_4 integer 0 seen
---@field pos_x integer map_block coordinates. Same as those of the main struct seen
---@field pos_y integer
---@field unk_5 integer 0 seen
---@field unk_6 integer
---@field unk_7 integer
---@field unk_8 integer
df.army_controller_sub7 = nil

---@class army_controller_sub11
---@field unk_1 integer
---@field unk_2 integer
---@field unk_3 any[]
df.army_controller_sub11 = nil

---@class army_controller_visit
---@field unk_1 integer
---@field unk_2 integer
---@field unk_3 integer 2 seen on exiled character
---@field unk_4 any[]
---@field unk_5 integer
---@field unk_6 integer
---@field abstract_building integer Monster slayers have -1
---@field purpose history_event_reason
df.army_controller_visit = nil

---@class army_controller_sub13
---@field unk_1 integer
---@field unk_2 integer
---@field unk_3 integer
---@field unk_4 any[]
df.army_controller_sub13 = nil

---@class army_controller_sub14
---@field unk_1 integer
---@field unk_2 integer
---@field unk_3 integer
---@field unk_4 any[]
---@field unk_5 integer since v0.44.11
df.army_controller_sub14 = nil

---@class army_controller_sub15
---@field unk_1 integer
---@field unk_2 integer
---@field unk_3 any[]
---@field unk_4 integer
---@field unk_5 integer
---@field unk_6 integer
---@field unk_7 integer
---@field unk_8 integer
---@field unk_9 integer
---@field unk_10 integer
---@field unk_11 integer
---@field unk_12 integer
---@field unk_13 integer
df.army_controller_sub15 = nil

---@class army_controller_sub16
---@field unk_1 integer
df.army_controller_sub16 = nil

---@class army_controller_quest
---@field artifact_id integer
---@field unk_1 integer
---@field unk_2 integer
---@field unk_3 integer
df.army_controller_quest = nil

---@class army_controller_sub18
---@field unk_1 integer
---@field unk_2 integer
df.army_controller_sub18 = nil

---@class army_controller_sub19
---@field unk_1 integer[]
---@field unk_2 integer
---@field unk_3 integer
---@field unk_4 integer
df.army_controller_sub19 = nil

---@class army_controller_sub20
---@field unk_1 integer
---@field unk_2 integer
---@field unk_3 integer
df.army_controller_sub20 = nil

---@class army_controller_sub21
---@field unk_1 integer
---@field unk_2 integer
df.army_controller_sub21 = nil

---@class army_controller_sub22
---@field unk_1 integer
---@field unk_2 integer
df.army_controller_sub22 = nil

---@class army_controller_sub23
---@field unk_1 integer
---@field unk_2 integer
---@field unk_3 integer
---@field unk_4 integer
df.army_controller_sub23 = nil

---@class army_controller_villainous_visit
---@field site_id integer
---@field entity_id integer
---@field abstract_building integer -1 before arrival
---@field purpose history_event_reason none before arrival
df.army_controller_villainous_visit = nil

---@enum army_flags
df.army_flags = {
    player = 0,
}

---@class army
---@field id integer
---@field pos coord
---@field last_pos coord
---@field unk_10 integer 1, 2, 5, 10, 15, 20, 21 seen
---@field unk_14 integer When set, large value like army or army_controller id, but no match found
---@field unk_18 integer
---@field members any[]
---@field squads world_site_inhabitant[]
---@field unk_3c integer
---@field unk_1 integer since v0.44.01
---@field unk_2 integer 16 only value seen, since v0.47.03
---@field controller_id integer
---@field controller army_controller
---@field flags army_flags[]
---@field block_path_x integer[] path in map_block coordinates. Seems to be the near term
---@field block_path_y integer[]
---@field path_x integer[] path in world coordinates. Seems to be the extension beyond those laid out in block_path_x/y
---@field path_y integer[]
---@field unk_90 integer
---@field unk_94 integer Number counting down. In examined save starts at 80 for id 38 counting down to 0 at 113, obviously with missing numbers somewhere
---@field unk_98 integer
---@field min_smell_trigger integer
---@field max_odor_level integer 1000 if undead are present
---@field max_low_light_vision integer
---@field sense_creature_classes string[]
---@field creature_class string[] Usually 'GENERAL_POISON' and 'MAMMAL'. Seen something else for undead
---@field item_type item_type
---@field item_subtype integer
---@field mat_type integer
---@field mat_index integer
---@field unk_4407_1 item[] since v0.44.07
df.army = nil


