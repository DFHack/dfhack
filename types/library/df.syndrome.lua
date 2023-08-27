-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.

---@meta

---@enum creature_interaction_effect_type
df.creature_interaction_effect_type = {
    PAIN = 0,
    SWELLING = 1,
    OOZING = 2,
    BRUISING = 3,
    BLISTERS = 4,
    NUMBNESS = 5,
    PARALYSIS = 6,
    FEVER = 7,
    BLEEDING = 8,
    COUGH_BLOOD = 9,
    VOMIT_BLOOD = 10,
    NAUSEA = 11,
    UNCONSCIOUSNESS = 12,
    NECROSIS = 13,
    IMPAIR_FUNCTION = 14,
    DROWSINESS = 15,
    DIZZINESS = 16,
    ADD_TAG = 17,
    REMOVE_TAG = 18,
    DISPLAY_TILE = 19,
    FLASH_TILE = 20,
    SPEED_CHANGE = 21,
    CAN_DO_INTERACTION = 22,
    SKILL_ROLL_ADJUST = 23,
    BODY_TRANSFORMATION = 24,
    PHYS_ATT_CHANGE = 25,
    MENT_ATT_CHANGE = 26,
    MATERIAL_FORCE_MULTIPLIER = 27,
    BODY_MAT_INTERACTION = 28,
    BODY_APPEARANCE_MODIFIER = 29,
    BP_APPEARANCE_MODIFIER = 30,
    DISPLAY_NAME = 31,
    SENSE_CREATURE_CLASS = 32,
    FEEL_EMOTION = 33,
    CHANGE_PERSONALITY = 34,
    ERRATIC_BEHAVIOR = 35,
    SPECIAL_ATTACK_INTERACTION = 36,
    REGROW_PARTS = 37,
    CLOSE_OPEN_WOUNDS = 38,
    HEAL_TISSUES = 39,
    HEAL_NERVES = 40,
    STOP_BLEEDING = 41,
    REDUCE_PAIN = 42,
    REDUCE_DIZZINESS = 43,
    REDUCE_NAUSEA = 44,
    REDUCE_SWELLING = 45,
    CURE_INFECTION = 46,
    REDUCE_PARALYSIS = 47,
    REDUCE_FEVER = 48,
}

---@alias creature_interaction_effect_flags bitfield

---@alias cie_add_tag_mask1 bitfield

---@alias cie_add_tag_mask2 bitfield

---@enum creature_interaction_effect_target_mode
df.creature_interaction_effect_target_mode = {
    BY_TYPE = 0,
    BY_TOKEN = 1,
    BY_CATEGORY = 2,
}

---@class creature_interaction_effect_target
---@field mode any[]
---@field key string[]
---@field tissue string[]
df.creature_interaction_effect_target = nil

---@return creature_interaction_effect_type
function df.creature_interaction_effect.getType() end

---@return creature_interaction_effect
function df.creature_interaction_effect.clone() end

---@param arg_0 unit
---@param arg_1 unit_syndrome
---@param arg_2 syndrome
---@param intensity integer
---@param bp_index integer
---@param bp_layer integer
---@param arg_6 unit_wound
function df.creature_interaction_effect.doAction(arg_0, arg_1, arg_2, intensity, bp_index, bp_layer, arg_6) end

---@return boolean
function df.creature_interaction_effect.isUntargeted() end

---@return integer
function df.creature_interaction_effect.getTargetModes() end

---@return integer
function df.creature_interaction_effect.getTargetKeys() end

---@return integer
function df.creature_interaction_effect.getTargetTissues() end

---@param arg_0 integer
---@return boolean
function df.creature_interaction_effect.checkAddFlag1(arg_0) end

---@param arg_0 integer
function df.creature_interaction_effect.setBodyMatInteractionName(arg_0) end

---@param type integer
function df.creature_interaction_effect.parseSynAcquireType(type) end

---@param race integer
---@param caste integer
function df.creature_interaction_effect.setBodyTransform(race, caste) end

---@param arg_0 integer
---@param arg_1 integer
---@param arg_2 integer
function df.creature_interaction_effect.addPeriodic(arg_0, arg_1, arg_2) end

---@param arg_0 integer
---@param arg_1 integer
---@param arg_2 integer
---@param arg_3 integer
function df.creature_interaction_effect.addCounterTrigger(arg_0, arg_1, arg_2, arg_3) end

---@param arg_0 integer
function df.creature_interaction_effect.unnamed_method(arg_0) end

function df.creature_interaction_effect.unnamed_method() end

---@return integer
function df.creature_interaction_effect.unnamed_method() end -- can_do_interaction, returns unk_6c

function df.creature_interaction_effect.unnamed_method() end

function df.creature_interaction_effect.unnamed_method() end

function df.creature_interaction_effect.unnamed_method() end

function df.creature_interaction_effect.unnamed_method() end

function df.creature_interaction_effect.unnamed_method() end

function df.creature_interaction_effect.unnamed_method() end

---@class creature_interaction_effect_counter_trigger
---@field counter any[]
---@field minval integer[] ?
---@field maxval integer[] ?
---@field required integer[]

---@class creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
df.creature_interaction_effect = nil

---@class creature_interaction_effect_painst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field sev integer
---@field target creature_interaction_effect_target
df.creature_interaction_effect_painst = nil

---@class creature_interaction_effect_swellingst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field sev integer
---@field target creature_interaction_effect_target
df.creature_interaction_effect_swellingst = nil

---@class creature_interaction_effect_oozingst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field sev integer
---@field target creature_interaction_effect_target
df.creature_interaction_effect_oozingst = nil

---@class creature_interaction_effect_bruisingst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field sev integer
---@field target creature_interaction_effect_target
df.creature_interaction_effect_bruisingst = nil

---@class creature_interaction_effect_blistersst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field sev integer
---@field target creature_interaction_effect_target
df.creature_interaction_effect_blistersst = nil

---@class creature_interaction_effect_numbnessst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field sev integer
---@field target creature_interaction_effect_target
df.creature_interaction_effect_numbnessst = nil

---@class creature_interaction_effect_paralysisst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field sev integer
---@field target creature_interaction_effect_target
df.creature_interaction_effect_paralysisst = nil

---@class creature_interaction_effect_feverst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field sev integer
df.creature_interaction_effect_feverst = nil

---@class creature_interaction_effect_bleedingst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field sev integer
---@field target creature_interaction_effect_target
df.creature_interaction_effect_bleedingst = nil

---@class creature_interaction_effect_cough_bloodst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field sev integer
df.creature_interaction_effect_cough_bloodst = nil

---@class creature_interaction_effect_vomit_bloodst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field sev integer
df.creature_interaction_effect_vomit_bloodst = nil

---@class creature_interaction_effect_nauseast
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field sev integer
df.creature_interaction_effect_nauseast = nil

---@class creature_interaction_effect_unconsciousnessst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field sev integer
df.creature_interaction_effect_unconsciousnessst = nil

---@class creature_interaction_effect_necrosisst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field sev integer
---@field target creature_interaction_effect_target
df.creature_interaction_effect_necrosisst = nil

---@class creature_interaction_effect_impair_functionst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field sev integer
---@field target creature_interaction_effect_target
df.creature_interaction_effect_impair_functionst = nil

---@class creature_interaction_effect_drowsinessst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field sev integer
df.creature_interaction_effect_drowsinessst = nil

---@class creature_interaction_effect_dizzinessst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field sev integer
df.creature_interaction_effect_dizzinessst = nil

---@class creature_interaction_effect_display_namest
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field name string
---@field name_plural string
---@field name_adj string
---@field unk_1 integer
df.creature_interaction_effect_display_namest = nil

---@class creature_interaction_effect_body_appearance_modifierst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field unk_60 integer
---@field unk_64 integer
df.creature_interaction_effect_body_appearance_modifierst = nil

---@class creature_interaction_effect_bp_appearance_modifierst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field unk_6c integer
---@field value integer
---@field target creature_interaction_effect_target
df.creature_interaction_effect_bp_appearance_modifierst = nil

---@class creature_interaction_effect_body_transformationst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field chance integer %
---@field race_str string
---@field caste_str string
---@field race integer[]
---@field caste integer[]
---@field required_creature_flags integer[] contains indexes of flags in creature_raw_flags
---@field forbidden_creature_flags integer[] contains indexes of flags in creature_raw_flags
---@field required_caste_flags integer[] contains indexes of flags in caste_raw_flags
---@field forbidden_caste_flags integer[] contains indexes of flags in caste_raw_flags
---@field unk_1 integer
---@field unk_2 integer
df.creature_interaction_effect_body_transformationst = nil

---@class creature_interaction_effect_skill_roll_adjustst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field multiplier integer % change for skill
---@field chance integer % probability per roll
df.creature_interaction_effect_skill_roll_adjustst = nil

---@class creature_interaction_effect_display_symbolst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field tile integer
---@field color integer
df.creature_interaction_effect_display_symbolst = nil

---@class creature_interaction_effect_flash_symbolst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field sym_color integer[]
---@field period integer
---@field time integer
---@field unk_78 integer
df.creature_interaction_effect_flash_symbolst = nil

---@class creature_interaction_effect_phys_att_changest
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field phys_att_perc integer[]
---@field phys_att_add integer[]
df.creature_interaction_effect_phys_att_changest = nil

---@class creature_interaction_effect_ment_att_changest
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field ment_att_perc integer[]
---@field ment_att_add integer[]
df.creature_interaction_effect_ment_att_changest = nil

---@class creature_interaction_effect_add_simple_flagst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field tags1 bitfield
---@field tags2 bitfield
df.creature_interaction_effect_add_simple_flagst = nil

---@class creature_interaction_effect_remove_simple_flagst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field tags1 bitfield
---@field tags2 bitfield
df.creature_interaction_effect_remove_simple_flagst = nil

---@class creature_interaction_effect_speed_changest
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field bonus_add integer
---@field bonus_perc integer
df.creature_interaction_effect_speed_changest = nil

---@class creature_interaction_effect_body_mat_interactionst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field interaction_name string
---@field interaction_id integer
---@field unk_8c integer
---@field unk_90 integer
---@field unk_94 string
df.creature_interaction_effect_body_mat_interactionst = nil

---@class creature_interaction_effect_material_force_adjustst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field unk_6c string
---@field unk_88 string
---@field unk_a4 string
---@field mat_type integer
---@field mat_index integer
---@field fraction_mul integer
---@field fraction_div integer
df.creature_interaction_effect_material_force_adjustst = nil

---@class creature_interaction_effect_can_do_interactionst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field interaction creature_interaction
df.creature_interaction_effect_can_do_interactionst = nil

---@class creature_interaction_effect_sense_creature_classst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field class_name string
---@field tile integer
---@field color_foreground integer
---@field color_background integer
---@field foreground_brightness integer
df.creature_interaction_effect_sense_creature_classst = nil

---@class creature_interaction_effect_feel_emotionst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field emotion emotion_type
---@field sev integer
df.creature_interaction_effect_feel_emotionst = nil

---@class creature_interaction_effect_change_personalityst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field facets integer[]
df.creature_interaction_effect_change_personalityst = nil

---@class creature_interaction_effect_erratic_behaviorst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field sev integer
df.creature_interaction_effect_erratic_behaviorst = nil

---@class creature_interaction_effect_close_open_woundsst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field unk_1 integer
---@field unk_2 any[]
---@field unk_3 any[]
---@field unk_4 any[]
df.creature_interaction_effect_close_open_woundsst = nil

---@class creature_interaction_effect_cure_infectionsst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field unk_1 integer
---@field unk_2 any[]
---@field unk_3 any[]
---@field unk_4 any[]
df.creature_interaction_effect_cure_infectionsst = nil

---@class creature_interaction_effect_heal_nervesst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field unk_1 integer
---@field unk_2 any[]
---@field unk_3 any[]
---@field unk_4 any[]
df.creature_interaction_effect_heal_nervesst = nil

---@class creature_interaction_effect_heal_tissuesst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field unk_1 integer
---@field unk_2 any[]
---@field unk_3 any[]
---@field unk_4 any[]
df.creature_interaction_effect_heal_tissuesst = nil

---@class creature_interaction_effect_reduce_dizzinessst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field unk_1 integer
df.creature_interaction_effect_reduce_dizzinessst = nil

---@class creature_interaction_effect_reduce_feverst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field unk_1 integer
df.creature_interaction_effect_reduce_feverst = nil

---@class creature_interaction_effect_reduce_nauseast
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field unk_1 integer
df.creature_interaction_effect_reduce_nauseast = nil

---@class creature_interaction_effect_reduce_painst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field unk_1 integer
---@field unk_2 any[]
---@field unk_3 any[]
---@field unk_4 any[]
df.creature_interaction_effect_reduce_painst = nil

---@class creature_interaction_effect_reduce_paralysisst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field unk_1 integer
---@field unk_2 any[]
---@field unk_3 any[]
---@field unk_4 any[]
df.creature_interaction_effect_reduce_paralysisst = nil

---@class creature_interaction_effect_reduce_swellingst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field unk_1 integer
---@field unk_2 any[]
---@field unk_3 any[]
---@field unk_4 any[]
df.creature_interaction_effect_reduce_swellingst = nil

---@class creature_interaction_effect_regrow_partsst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field unk_1 integer
---@field unk_2 any[]
---@field unk_3 any[]
---@field unk_4 any[]
df.creature_interaction_effect_regrow_partsst = nil

---@class creature_interaction_effect_special_attack_interactionst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field unk_1 integer[]
---@field unk_2 string[]
---@field unk_3 string
df.creature_interaction_effect_special_attack_interactionst = nil

---@class creature_interaction_effect_stop_bleedingst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field unk_1 integer
---@field unk_2 any[]
---@field unk_3 any[]
---@field unk_4 any[]
df.creature_interaction_effect_stop_bleedingst = nil

---@class creature_interaction_effect_cure_infectionst
-- inherited from creature_interaction_effect
---@field flags bitfield
---@field prob integer
---@field start integer
---@field peak integer
---@field end integer
---@field dwf_stretch integer
---@field syn_id integer
---@field id integer
---@field syn_index integer index in syndrome
---@field moon_phase_min integer
---@field moon_phase_max integer
---@field counter_trigger creature_interaction_effect_counter_trigger
-- end creature_interaction_effect
---@field unk_1 integer
---@field unk_2 any[]
---@field unk_3 any[]
---@field unk_4 any[]
df.creature_interaction_effect_cure_infectionst = nil

---@alias syndrome_flags bitfield

---@class syndrome
---@field syn_name string
---@field ce creature_interaction_effect[]
---@field syn_affected_class string[]
---@field syn_affected_creature string[]
---@field syn_affected_caste string[]
---@field syn_immune_class string[]
---@field syn_immune_creature string[]
---@field syn_immune_caste string[]
---@field syn_class string[] since v0.34.01
---@field syn_identifier string since v0.42.01
---@field flags bitfield
---@field syn_concentration_added integer[] since v0.42.01
---@field id integer
df.syndrome = nil


