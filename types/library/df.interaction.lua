-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.

---@meta

---@enum interaction_flags
df.interaction_flags = {
    GENERATED = 0,
    EXPERIMENT_ONLY = 1,
}

---@class interaction
---@field name string
---@field id integer
---@field str string[] interaction raws
---@field flags interaction_flags[]
---@field sources interaction_source[] I_SOURCE
---@field targets interaction_target[] I_TARGET
---@field effects interaction_effect[] I_EFFECT
---@field source_hfid integer
---@field source_enid integer since v0.42.01
df.interaction = nil

---@enum interaction_effect_type
df.interaction_effect_type = {
    ANIMATE = 0,
    ADD_SYNDROME = 1,
    RESURRECT = 2,
    CLEAN = 3,
    CONTACT = 4,
    MATERIAL_EMISSION = 5,
    HIDE = 6,
    PROPEL_UNIT = 7,
    SUMMON_UNIT = 8,
    CHANGE_WEATHER = 9,
    RAISE_GHOST = 10,
    CREATE_ITEM = 11,
    CHANGE_ITEM_QUALITY = 12,
}

---@enum interaction_effect_location_hint
df.interaction_effect_location_hint = {
    IN_WATER = 0,
    IN_MAGMA = 1,
    NO_WATER = 2,
    NO_MAGMA = 3,
    NO_THICK_FOG = 4,
    OUTSIDE = 5,
}

---@return interaction_effect_type
function df.interaction_effect.getType() end

---@param file file_compressorst
function df.interaction_effect.write_file(file) end

---@param file file_compressorst
---@param loadversion save_version
function df.interaction_effect.read_file(file, loadversion) end

---@param target unit
---@param arg_1 integer
---@param arg_2 boolean
function df.interaction_effect.activateOnUnit(target, arg_1, arg_2) end

---@param target item
function df.interaction_effect.activateOnItem(target) end

---@param arg_0 integer
---@param arg_1 integer
---@param arg_2 integer
---@param arg_3 integer
---@param arg_4 integer
function df.interaction_effect.parseRaws(arg_0, arg_1, arg_2, arg_3, arg_4) end

---@param arg_0 integer
function df.interaction_effect.finalize(arg_0) end

function df.interaction_effect.applySyndromes() end

---@param arg_0 integer
---@param arg_1 integer
---@param arg_2 boolean
function df.interaction_effect.unnamed_method(arg_0, arg_1, arg_2) end

---@param arg_0 syndrome
---@return boolean
function df.interaction_effect.hasSyndrome(arg_0) end

---@class interaction_effect
---@field index integer index of the effect within the parent interaction.effects
---@field targets string[]
---@field targets_index integer[] for each target used in this effect, list the index of that target within the parent interaction.targets
---@field intermittent integer IE_INTERMITTENT, 0 = weekly
---@field locations interaction_effect_location_hint[] IE_LOCATION
---@field flags bitfield
---@field interaction_id integer
---@field arena_name string IE_ARENA_NAME
df.interaction_effect = nil

---@class interaction_effect_animatest
-- inherited from interaction_effect
---@field index integer index of the effect within the parent interaction.effects
---@field targets string[]
---@field targets_index integer[] for each target used in this effect, list the index of that target within the parent interaction.targets
---@field intermittent integer IE_INTERMITTENT, 0 = weekly
---@field locations interaction_effect_location_hint[] IE_LOCATION
---@field flags bitfield
---@field interaction_id integer
---@field arena_name string IE_ARENA_NAME
-- end interaction_effect
---@field unk_1 integer
---@field syndrome syndrome[]
df.interaction_effect_animatest = nil

---@class interaction_effect_add_syndromest
-- inherited from interaction_effect
---@field index integer index of the effect within the parent interaction.effects
---@field targets string[]
---@field targets_index integer[] for each target used in this effect, list the index of that target within the parent interaction.targets
---@field intermittent integer IE_INTERMITTENT, 0 = weekly
---@field locations interaction_effect_location_hint[] IE_LOCATION
---@field flags bitfield
---@field interaction_id integer
---@field arena_name string IE_ARENA_NAME
-- end interaction_effect
---@field unk_1 integer
---@field syndrome syndrome[]
df.interaction_effect_add_syndromest = nil

---@class interaction_effect_resurrectst
-- inherited from interaction_effect
---@field index integer index of the effect within the parent interaction.effects
---@field targets string[]
---@field targets_index integer[] for each target used in this effect, list the index of that target within the parent interaction.targets
---@field intermittent integer IE_INTERMITTENT, 0 = weekly
---@field locations interaction_effect_location_hint[] IE_LOCATION
---@field flags bitfield
---@field interaction_id integer
---@field arena_name string IE_ARENA_NAME
-- end interaction_effect
---@field unk_1 integer
---@field syndrome syndrome[]
df.interaction_effect_resurrectst = nil

---@class interaction_effect_cleanst
-- inherited from interaction_effect
---@field index integer index of the effect within the parent interaction.effects
---@field targets string[]
---@field targets_index integer[] for each target used in this effect, list the index of that target within the parent interaction.targets
---@field intermittent integer IE_INTERMITTENT, 0 = weekly
---@field locations interaction_effect_location_hint[] IE_LOCATION
---@field flags bitfield
---@field interaction_id integer
---@field arena_name string IE_ARENA_NAME
-- end interaction_effect
---@field grime_level integer IE_GRIME_LEVEL
---@field syndrome_tag bitfield IE_SYNDROME_TAG
---@field unk_1 integer
df.interaction_effect_cleanst = nil

---@class interaction_effect_contactst
-- inherited from interaction_effect
---@field index integer index of the effect within the parent interaction.effects
---@field targets string[]
---@field targets_index integer[] for each target used in this effect, list the index of that target within the parent interaction.targets
---@field intermittent integer IE_INTERMITTENT, 0 = weekly
---@field locations interaction_effect_location_hint[] IE_LOCATION
---@field flags bitfield
---@field interaction_id integer
---@field arena_name string IE_ARENA_NAME
-- end interaction_effect
---@field unk_1 integer
df.interaction_effect_contactst = nil

---@class interaction_effect_material_emissionst
-- inherited from interaction_effect
---@field index integer index of the effect within the parent interaction.effects
---@field targets string[]
---@field targets_index integer[] for each target used in this effect, list the index of that target within the parent interaction.targets
---@field intermittent integer IE_INTERMITTENT, 0 = weekly
---@field locations interaction_effect_location_hint[] IE_LOCATION
---@field flags bitfield
---@field interaction_id integer
---@field arena_name string IE_ARENA_NAME
-- end interaction_effect
---@field unk_1 integer
df.interaction_effect_material_emissionst = nil

---@class interaction_effect_hidest
-- inherited from interaction_effect
---@field index integer index of the effect within the parent interaction.effects
---@field targets string[]
---@field targets_index integer[] for each target used in this effect, list the index of that target within the parent interaction.targets
---@field intermittent integer IE_INTERMITTENT, 0 = weekly
---@field locations interaction_effect_location_hint[] IE_LOCATION
---@field flags bitfield
---@field interaction_id integer
---@field arena_name string IE_ARENA_NAME
-- end interaction_effect
---@field unk_1 integer
df.interaction_effect_hidest = nil

---@class interaction_effect_change_item_qualityst
-- inherited from interaction_effect
---@field index integer index of the effect within the parent interaction.effects
---@field targets string[]
---@field targets_index integer[] for each target used in this effect, list the index of that target within the parent interaction.targets
---@field intermittent integer IE_INTERMITTENT, 0 = weekly
---@field locations interaction_effect_location_hint[] IE_LOCATION
---@field flags bitfield
---@field interaction_id integer
---@field arena_name string IE_ARENA_NAME
-- end interaction_effect
---@field quality_added integer IE_CHANGE_QUALITY
---@field quality_set integer IE_SET_QUALITY
df.interaction_effect_change_item_qualityst = nil

---@class interaction_effect_change_weatherst
-- inherited from interaction_effect
---@field index integer index of the effect within the parent interaction.effects
---@field targets string[]
---@field targets_index integer[] for each target used in this effect, list the index of that target within the parent interaction.targets
---@field intermittent integer IE_INTERMITTENT, 0 = weekly
---@field locations interaction_effect_location_hint[] IE_LOCATION
---@field flags bitfield
---@field interaction_id integer
---@field arena_name string IE_ARENA_NAME
-- end interaction_effect
---@field unk_1 integer
---@field unk_2 integer
df.interaction_effect_change_weatherst = nil

---@class interaction_effect_raise_ghostst
-- inherited from interaction_effect
---@field index integer index of the effect within the parent interaction.effects
---@field targets string[]
---@field targets_index integer[] for each target used in this effect, list the index of that target within the parent interaction.targets
---@field intermittent integer IE_INTERMITTENT, 0 = weekly
---@field locations interaction_effect_location_hint[] IE_LOCATION
---@field flags bitfield
---@field interaction_id integer
---@field arena_name string IE_ARENA_NAME
-- end interaction_effect
---@field unk_1 integer
---@field syndrome syndrome[] assumed based on vmethod reference
df.interaction_effect_raise_ghostst = nil

---@class interaction_effect_create_itemst
-- inherited from interaction_effect
---@field index integer index of the effect within the parent interaction.effects
---@field targets string[]
---@field targets_index integer[] for each target used in this effect, list the index of that target within the parent interaction.targets
---@field intermittent integer IE_INTERMITTENT, 0 = weekly
---@field locations interaction_effect_location_hint[] IE_LOCATION
---@field flags bitfield
---@field interaction_id integer
---@field arena_name string IE_ARENA_NAME
-- end interaction_effect
---@field item_type item_type IE_ITEM
---@field item_subtype integer IE_ITEM
---@field mat_type integer IE_ITEM
---@field mat_index integer IE_ITEM
---@field probability integer IE_ITEM
---@field quantity integer IE_ITEM
---@field quality_min integer IE_ITEM_QUALITY
---@field quality_max integer IE_ITEM_QUALITY
---@field create_artifact integer IE_ITEM_QUALITY:ARTIFACT
---@field unk_1 string these five are probably item1:item2:mat1:mat2:mat3
---@field unk_2 string
---@field unk_3 string
---@field unk_4 string
---@field unk_5 string
df.interaction_effect_create_itemst = nil

---@class interaction_effect_propel_unitst
-- inherited from interaction_effect
---@field index integer index of the effect within the parent interaction.effects
---@field targets string[]
---@field targets_index integer[] for each target used in this effect, list the index of that target within the parent interaction.targets
---@field intermittent integer IE_INTERMITTENT, 0 = weekly
---@field locations interaction_effect_location_hint[] IE_LOCATION
---@field flags bitfield
---@field interaction_id integer
---@field arena_name string IE_ARENA_NAME
-- end interaction_effect
---@field unk_1 integer
---@field propel_force integer IE_PROPEL_FORCE
df.interaction_effect_propel_unitst = nil

---@class interaction_effect_summon_unitst
-- inherited from interaction_effect
---@field index integer index of the effect within the parent interaction.effects
---@field targets string[]
---@field targets_index integer[] for each target used in this effect, list the index of that target within the parent interaction.targets
---@field intermittent integer IE_INTERMITTENT, 0 = weekly
---@field locations interaction_effect_location_hint[] IE_LOCATION
---@field flags bitfield
---@field interaction_id integer
---@field arena_name string IE_ARENA_NAME
-- end interaction_effect
---@field make_pet integer IE_MAKE_PET_IF_POSSIBLE
---@field race_str string CREATURE
---@field caste_str string CREATURE
---@field unk_1 integer[] seen 4 bytes allocated
---@field unk_2 integer[] seen 2 bytes allocate
---@field required_creature_flags integer[] contains indexes of flags in creature_raw_flags, IE_CREATURE_FLAG
---@field forbidden_creature_flags integer[] contains indexes of flags in creature_raw_flags, IE_FORBIDDEN_CREATURE_FLAG
---@field required_caste_flags integer[] contains indexes of flags in caste_raw_flags, IE_CREATURE_CASTE_FLAG
---@field forbidden_caste_flags integer[] contains indexes of flags in caste_raw_flags, IE_FORBIDDEN_CREATURE_CASTE_FLAG
---@field unk_3 integer
---@field unk_4 integer
---@field time_range_min integer IE_TIME_RANGE
---@field time_range_max integer IE_TIME_RANGE
df.interaction_effect_summon_unitst = nil

---@enum interaction_source_type
df.interaction_source_type = {
    REGION = 0,
    SECRET = 1,
    DISTURBANCE = 2,
    DEITY = 3,
    ATTACK = 4,
    INGESTION = 5,
    CREATURE_ACTION = 6,
    UNDERGROUND_SPECIAL = 7,
    EXPERIMENT = 8,
}

---@return interaction_source_type
function df.interaction_source.getType() end

---@param file file_compressorst
function df.interaction_source.write_file(file) end

---@param file file_compressorst
---@param loadversion save_version
function df.interaction_source.read_file(file, loadversion) end

---@param arg_0 integer
---@param arg_1 integer
---@param arg_2 integer
---@param arg_3 integer
---@param arg_4 integer
function df.interaction_source.parseRaws(arg_0, arg_1, arg_2, arg_3, arg_4) end

---@param arg_0 integer
---@param arg_1 integer
function df.interaction_source.unnamed_method(arg_0, arg_1) end

---@param arg_0 integer
---@return boolean
function df.interaction_source.unnamed_method(arg_0) end

---@param arg_0 integer
---@return boolean
function df.interaction_source.unnamed_method(arg_0) end

---@class interaction_source
---@field id integer
---@field frequency integer IS_FREQUENCY
---@field name string IS_NAME
---@field hist_string_1 string IS_HIST_STRING_1
---@field hist_string_2 string IS_HIST_STRING_2
---@field trigger_string_second string IS_TRIGGER_STRING_SECOND
---@field trigger_string_third string IS_TRIGGER_STRING_THIRD
---@field trigger_string string IS_TRIGGER_STRING
df.interaction_source = nil

---@class interaction_source_regionst
-- inherited from interaction_source
---@field id integer
---@field frequency integer IS_FREQUENCY
---@field name string IS_NAME
---@field hist_string_1 string IS_HIST_STRING_1
---@field hist_string_2 string IS_HIST_STRING_2
---@field trigger_string_second string IS_TRIGGER_STRING_SECOND
---@field trigger_string_third string IS_TRIGGER_STRING_THIRD
---@field trigger_string string IS_TRIGGER_STRING
-- end interaction_source
---@field region_flags bitfield
---@field regions integer[]
df.interaction_source_regionst = nil

---@class interaction_source_secretst
-- inherited from interaction_source
---@field id integer
---@field frequency integer IS_FREQUENCY
---@field name string IS_NAME
---@field hist_string_1 string IS_HIST_STRING_1
---@field hist_string_2 string IS_HIST_STRING_2
---@field trigger_string_second string IS_TRIGGER_STRING_SECOND
---@field trigger_string_third string IS_TRIGGER_STRING_THIRD
---@field trigger_string string IS_TRIGGER_STRING
-- end interaction_source
---@field learn_flags bitfield
---@field spheres any[]
---@field goals goal_type[]
---@field book_title_filename string
---@field book_name_filename string
---@field unk_1 integer
---@field unk_2 integer
df.interaction_source_secretst = nil

---@class interaction_source_disturbancest
-- inherited from interaction_source
---@field id integer
---@field frequency integer IS_FREQUENCY
---@field name string IS_NAME
---@field hist_string_1 string IS_HIST_STRING_1
---@field hist_string_2 string IS_HIST_STRING_2
---@field trigger_string_second string IS_TRIGGER_STRING_SECOND
---@field trigger_string_third string IS_TRIGGER_STRING_THIRD
---@field trigger_string string IS_TRIGGER_STRING
-- end interaction_source
---@field unk_1 integer
df.interaction_source_disturbancest = nil

---@enum interaction_source_usage_hint
df.interaction_source_usage_hint = {
    MAJOR_CURSE = 0,
    GREETING = 1,
    CLEAN_SELF = 2,
    CLEAN_FRIEND = 3,
    ATTACK = 4,
    FLEEING = 5,
    NEGATIVE_SOCIAL_RESPONSE = 6,
    TORMENT = 7,
    DEFEND = 8,
    MEDIUM_CURSE = 9,
    MINOR_CURSE = 10,
    MEDIUM_BLESSING = 11,
    MINOR_BLESSING = 12,
}

---@class interaction_source_deityst
-- inherited from interaction_source
---@field id integer
---@field frequency integer IS_FREQUENCY
---@field name string IS_NAME
---@field hist_string_1 string IS_HIST_STRING_1
---@field hist_string_2 string IS_HIST_STRING_2
---@field trigger_string_second string IS_TRIGGER_STRING_SECOND
---@field trigger_string_third string IS_TRIGGER_STRING_THIRD
---@field trigger_string string IS_TRIGGER_STRING
-- end interaction_source
---@field unk_1 integer
---@field usage_hint interaction_source_usage_hint[] IS_USAGE_HINT
df.interaction_source_deityst = nil

---@class interaction_source_attackst
-- inherited from interaction_source
---@field id integer
---@field frequency integer IS_FREQUENCY
---@field name string IS_NAME
---@field hist_string_1 string IS_HIST_STRING_1
---@field hist_string_2 string IS_HIST_STRING_2
---@field trigger_string_second string IS_TRIGGER_STRING_SECOND
---@field trigger_string_third string IS_TRIGGER_STRING_THIRD
---@field trigger_string string IS_TRIGGER_STRING
-- end interaction_source
---@field unk_1 integer
df.interaction_source_attackst = nil

---@class interaction_source_ingestionst
-- inherited from interaction_source
---@field id integer
---@field frequency integer IS_FREQUENCY
---@field name string IS_NAME
---@field hist_string_1 string IS_HIST_STRING_1
---@field hist_string_2 string IS_HIST_STRING_2
---@field trigger_string_second string IS_TRIGGER_STRING_SECOND
---@field trigger_string_third string IS_TRIGGER_STRING_THIRD
---@field trigger_string string IS_TRIGGER_STRING
-- end interaction_source
---@field unk_1 integer
df.interaction_source_ingestionst = nil

---@class interaction_source_creature_actionst
-- inherited from interaction_source
---@field id integer
---@field frequency integer IS_FREQUENCY
---@field name string IS_NAME
---@field hist_string_1 string IS_HIST_STRING_1
---@field hist_string_2 string IS_HIST_STRING_2
---@field trigger_string_second string IS_TRIGGER_STRING_SECOND
---@field trigger_string_third string IS_TRIGGER_STRING_THIRD
---@field trigger_string string IS_TRIGGER_STRING
-- end interaction_source
---@field unk_1 integer
df.interaction_source_creature_actionst = nil

---@class interaction_source_underground_specialst
-- inherited from interaction_source
---@field id integer
---@field frequency integer IS_FREQUENCY
---@field name string IS_NAME
---@field hist_string_1 string IS_HIST_STRING_1
---@field hist_string_2 string IS_HIST_STRING_2
---@field trigger_string_second string IS_TRIGGER_STRING_SECOND
---@field trigger_string_third string IS_TRIGGER_STRING_THIRD
---@field trigger_string string IS_TRIGGER_STRING
-- end interaction_source
df.interaction_source_underground_specialst = nil

---@class interaction_source_experimentst
-- inherited from interaction_source
---@field id integer
---@field frequency integer IS_FREQUENCY
---@field name string IS_NAME
---@field hist_string_1 string IS_HIST_STRING_1
---@field hist_string_2 string IS_HIST_STRING_2
---@field trigger_string_second string IS_TRIGGER_STRING_SECOND
---@field trigger_string_third string IS_TRIGGER_STRING_THIRD
---@field trigger_string string IS_TRIGGER_STRING
-- end interaction_source
---@field unk_1 integer
df.interaction_source_experimentst = nil

---@enum interaction_target_type
df.interaction_target_type = {
    CORPSE = 0,
    CREATURE = 1,
    MATERIAL = 2,
    LOCATION = 3,
}

---@enum interaction_target_location_type
df.interaction_target_location_type = {
    CONTEXT_NONE = -1,
    CONTEXT_REGION = 0,
    CONTEXT_CREATURE = 1,
    CONTEXT_ITEM = 2,
    CONTEXT_BP = 3,
    CONTEXT_LOCATION = 4,
    CONTEXT_CREATURE_OR_LOCATION = 5,
    RANDOM_NEARBY_LOCATION = 6,
}

---@return interaction_target_type
function df.interaction_target.getType() end

---@param file file_compressorst
function df.interaction_target.write_file(file) end

---@param file file_compressorst
---@param loadversion save_version
function df.interaction_target.read_file(file, loadversion) end

---@param arg_0 integer
---@param arg_1 integer
---@param arg_2 integer
---@param arg_3 integer
---@param arg_4 integer
function df.interaction_target.parseRaws(arg_0, arg_1, arg_2, arg_3, arg_4) end

---@param arg_0 integer
function df.interaction_target.unnamed_method(arg_0) end

---@param arg_0 integer
---@param arg_1 integer
---@return boolean
function df.interaction_target.unnamed_method(arg_0, arg_1) end

---@param arg_0 integer
---@param arg_1 integer
---@return boolean
function df.interaction_target.unnamed_method(arg_0, arg_1) end

---@param arg_0 historical_figure
---@param arg_1 interaction
---@return boolean
function df.interaction_target.unnamed_method(arg_0, arg_1) end

---@param arg_0 integer
---@param arg_1 integer
---@return boolean
function df.interaction_target.unnamed_method(arg_0, arg_1) end

---@class interaction_target
---@field index integer
---@field name string
---@field manual_input string IT_MANUAL_INPUT
---@field location interaction_target_location_type IT_LOCATION
---@field reference_name string IT_LOCATION:RANDOM_NEARBY_LOCATION
---@field reference_distance integer IT_LOCATION:RANDOM_NEARBY_LOCATION
df.interaction_target = nil

---@class interaction_target_info
---@field affected_creature_str any[]
---@field affected_creature integer[] IT_AFFECTED_CREATURE
---@field affected_class string[] IT_AFFECTED_CLASS
---@field immune_creature_str any[]
---@field immune_creature integer[] IT_IMMUNE_CREATURE
---@field immune_class string[] IT_IMMUNE_CLASS
---@field forbidden_syndrome_class string[]
---@field requires_1 integer IT_REQUIRES
---@field requires_2 integer IT_REQUIRES
---@field forbidden_1 integer IT_FORBIDDEN
---@field forbidden_2 integer IT_FORBIDDEN
---@field restrictions bitfield
df.interaction_target_info = nil

---@class interaction_target_corpsest
-- inherited from interaction_target
---@field index integer
---@field name string
---@field manual_input string IT_MANUAL_INPUT
---@field location interaction_target_location_type IT_LOCATION
---@field reference_name string IT_LOCATION:RANDOM_NEARBY_LOCATION
---@field reference_distance integer IT_LOCATION:RANDOM_NEARBY_LOCATION
-- end interaction_target
---@field target_info interaction_target_info
df.interaction_target_corpsest = nil

---@class interaction_target_creaturest
-- inherited from interaction_target
---@field index integer
---@field name string
---@field manual_input string IT_MANUAL_INPUT
---@field location interaction_target_location_type IT_LOCATION
---@field reference_name string IT_LOCATION:RANDOM_NEARBY_LOCATION
---@field reference_distance integer IT_LOCATION:RANDOM_NEARBY_LOCATION
-- end interaction_target
---@field target_info interaction_target_info
df.interaction_target_creaturest = nil

---@enum breath_attack_type
df.breath_attack_type = {
    TRAILING_DUST_FLOW = 0,
    TRAILING_VAPOR_FLOW = 1,
    TRAILING_GAS_FLOW = 2,
    SOLID_GLOB = 3,
    LIQUID_GLOB = 4,
    UNDIRECTED_GAS = 5,
    UNDIRECTED_VAPOR = 6,
    UNDIRECTED_DUST = 7,
    WEB_SPRAY = 8,
    DRAGONFIRE = 9,
    FIREJET = 10,
    FIREBALL = 11,
    WEATHER_CREEPING_GAS = 12,
    WEATHER_CREEPING_VAPOR = 13,
    WEATHER_CREEPING_DUST = 14,
    WEATHER_FALLING_MATERIAL = 15,
    SPATTER_POWDER = 16,
    SPATTER_LIQUID = 17,
    UNDIRECTED_ITEM_CLOUD = 18,
    TRAILING_ITEM_FLOW = 19,
    SHARP_ROCK = 20,
    OTHER = 21,
}

---@class interaction_target_materialst
-- inherited from interaction_target
---@field index integer
---@field name string
---@field manual_input string IT_MANUAL_INPUT
---@field location interaction_target_location_type IT_LOCATION
---@field reference_name string IT_LOCATION:RANDOM_NEARBY_LOCATION
---@field reference_distance integer IT_LOCATION:RANDOM_NEARBY_LOCATION
-- end interaction_target
---@field material_str string[]
---@field mat_type integer
---@field mat_index integer
---@field parent_interaction_index integer
---@field breath_attack_type breath_attack_type
---@field restrictions bitfield
df.interaction_target_materialst = nil

---@class interaction_target_locationst
-- inherited from interaction_target
---@field index integer
---@field name string
---@field manual_input string IT_MANUAL_INPUT
---@field location interaction_target_location_type IT_LOCATION
---@field reference_name string IT_LOCATION:RANDOM_NEARBY_LOCATION
---@field reference_distance integer IT_LOCATION:RANDOM_NEARBY_LOCATION
-- end interaction_target
df.interaction_target_locationst = nil

---@class interaction_instance
---@field id integer
---@field interaction_id integer
---@field unk_1 integer
---@field region_index integer
---@field affected_units integer[] IDs of units affected by the regional interaction
df.interaction_instance = nil


