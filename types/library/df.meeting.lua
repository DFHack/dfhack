-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.

---@meta

---@class dipscript_info
---@field id integer assigned during Save
---@field script_file string data/dipscript/dwarf_liaison
---@field script_steps script_stepst[]
---@field script_vars script_varst[]
---@field code string DWARF_LIAISON etc
df.dipscript_info = nil

---@class dipscript_popup
---@field meeting_holder_actor integer
---@field meeting_holder_noble integer
---@field activity activity_info
---@field flags bitfield
df.dipscript_popup = nil

---@param idx integer
---@return boolean
function df.script_stepst.setNextStep(idx) end

---@param context meeting_context
---@return integer
function df.script_stepst.execute(context) end

---@param context meeting_context
---@return integer
function df.script_stepst.skip(context) end

---@class script_stepst
---@field next_step_idx integer
df.script_stepst = nil

---@class script_step_setvarst
-- inherited from script_stepst
---@field next_step_idx integer
-- end script_stepst
---@field dest_type string
---@field dest_name string
---@field src_type string
---@field src_name string
df.script_step_setvarst = nil

---@class script_step_simpleactionst
-- inherited from script_stepst
---@field next_step_idx integer
-- end script_stepst
---@field type string
---@field subtype string
df.script_step_simpleactionst = nil

---@class script_step_conditionalst_condition
---@field var1_type string
---@field var1_name string
---@field comparison string
---@field var2_type string
---@field var2_name string

---@class script_step_conditionalst
-- inherited from script_stepst
---@field next_step_idx integer
-- end script_stepst
---@field condition script_step_conditionalst_condition
---@field conditional_next_step_idx integer
df.script_step_conditionalst = nil

---@class script_step_textviewerst
-- inherited from script_stepst
---@field next_step_idx integer
-- end script_stepst
---@field filename string
---@field outvar_name string
df.script_step_textviewerst = nil

---@class script_step_diphistoryst
-- inherited from script_stepst
---@field next_step_idx integer
-- end script_stepst
---@field event string
df.script_step_diphistoryst = nil

---@class script_step_discussst
-- inherited from script_stepst
---@field next_step_idx integer
-- end script_stepst
---@field event string
df.script_step_discussst = nil

---@class script_step_topicdiscussionst
-- inherited from script_stepst
---@field next_step_idx integer
-- end script_stepst
df.script_step_topicdiscussionst = nil

---@class script_step_constructtopiclistst
-- inherited from script_stepst
---@field next_step_idx integer
-- end script_stepst
df.script_step_constructtopiclistst = nil

---@class script_step_dipeventst
-- inherited from script_step_eventst
-- inherited from script_stepst
---@field next_step_idx integer
-- end script_stepst
-- end script_step_eventst
---@field parm1 string
---@field parm2 string
---@field parm3 string
---@field parm4 string
---@field parm5 string
df.script_step_dipeventst = nil

---@class script_step_invasionst
-- inherited from script_step_eventst
-- inherited from script_stepst
---@field next_step_idx integer
-- end script_stepst
-- end script_step_eventst
---@field parm string
df.script_step_invasionst = nil

---@class script_step_eventst
-- inherited from script_stepst
---@field next_step_idx integer
-- end script_stepst
df.script_step_eventst = nil

---@return active_script_varst
function df.script_varst.instantiate() end

---@class script_varst
---@field name string
df.script_varst = nil

---@class script_var_unitst
-- inherited from script_varst
---@field name string
-- end script_varst
df.script_var_unitst = nil

---@class script_var_longst
-- inherited from script_varst
---@field name string
-- end script_varst
df.script_var_longst = nil

function df.active_script_varst.setColor() end

---@param output string
---@param format string
function df.active_script_varst.formatString(output, format) end

---@param int_value integer
---@param ref_value specific_ref
function df.active_script_varst.getValue(int_value, ref_value) end

---@param var meeting_variable
function df.active_script_varst.setValue(var) end

---@param ref_value specific_ref
function df.active_script_varst.removeUnit(ref_value) end

---@param file file_compressorst
function df.active_script_varst.write_file(file) end

---@param file file_compressorst
---@param loadversion save_version
function df.active_script_varst.read_file(file, loadversion) end

---@class active_script_varst
---@field name string
df.active_script_varst = nil

---@class active_script_var_unitst
-- inherited from active_script_varst
---@field name string
-- end active_script_varst
---@field unit unit
df.active_script_var_unitst = nil

---@class active_script_var_longst
-- inherited from active_script_varst
---@field name string
-- end active_script_varst
---@field value integer
df.active_script_var_longst = nil

---@class meeting_variable
---@field value integer
---@field ref specific_ref
---@field active_var active_script_varst
df.meeting_variable = nil

---@class meeting_context
---@field meeting meeting_diplomat_info
---@field popup dipscript_popup
---@field unk_2 integer
---@field unk_3 integer
df.meeting_context = nil

---@class meeting_diplomat_info
---@field civ_id integer
---@field unk1 integer maybe is_first_contact
---@field diplomat_id integer
---@field associate_id integer
---@field topic_list any[]
---@field topic_parms integer[]
---@field sell_requests entity_sell_requests
---@field buy_requests entity_buy_requests
---@field dipscript dipscript_info
---@field cur_step integer
---@field active_script_vars active_script_varst[]
---@field unk_50 string
---@field unk_6c string
---@field flags bitfield
---@field events meeting_event[]
---@field agreement_entity integer[]
---@field agreement_topic meeting_topic[]
---@field agreement_year integer[]
---@field agreement_tick integer[]
---@field agreement_outcome integer[]
---@field contact_entity integer[]
---@field contact_year integer[]
---@field contact_tick integer[]
df.meeting_diplomat_info = nil

---@enum meeting_topic
df.meeting_topic = {
    DiscussCurrent = 0,
    RequestPeace = 1,
    TreeQuota = 2,
    BecomeLandHolder = 3,
    PromoteLandHolder = 4,
    ExportAgreement = 5,
    ImportAgreement = 6,
    PleasantPlace = 7,
    WorldStatus = 8,
    TributeAgreement = 9,
}

---@enum meeting_event_type
df.meeting_event_type = {
    AcceptAgreement = 0,
    RejectAgreement = 1,
    AcceptPeace = 2,
    RejectPeace = 3,
    ExportAgreement = 4,
    ImportAgreement = 5,
}

---@class meeting_event
---@field type meeting_event_type
---@field topic meeting_topic
---@field topic_parm integer
---@field unk_1 integer[]
---@field unk_2 integer[]
---@field quota_total integer
---@field quota_remaining integer
---@field year integer
---@field ticks integer
---@field sell_prices entity_sell_prices
---@field buy_prices entity_buy_prices
df.meeting_event = nil

---@class activity_info
---@field id integer assigned during Save
---@field unit_actor integer diplomat or worker
---@field unit_noble integer meeting recipient
---@field place integer
---@field flags bitfield
---@field unk3 integer 3
---@field delay integer 0
---@field tree_quota integer -1
df.activity_info = nil

---@class party_info
---@field units unit[]
---@field location building
---@field timer integer -1 per 10
---@field id integer assigned during Save
df.party_info = nil

---@class room_rent_info
---@field elements building[]
---@field rent_value integer
---@field flags bitfield
df.room_rent_info = nil

---@enum activity_entry_type
df.activity_entry_type = {
    TrainingSession = 0,
    IndividualSkillDrill = 1,
    Conflict = 2,
    unk_3 = 3,
    unk_4 = 4,
    Conversation = 5,
    unk_6 = 6,
    Prayer = 7,
    Socialize = 8,
    Research = 9,
    FillServiceOrder = 10,
    Read = 11,
    Play = 12,
}

---@class activity_entry
---@field id integer
---@field type activity_entry_type
---@field events activity_event[]
---@field next_event_id integer
---@field army_controller integer since v0.40.01
df.activity_entry = nil

---@enum activity_event_type
df.activity_event_type = {
    TrainingSession = 0,
    CombatTraining = 1,
    SkillDemonstration = 2,
    IndividualSkillDrill = 3,
    Sparring = 4,
    RangedPractice = 5,
    Harassment = 6,
    Conversation = 7,
    Conflict = 8,
    Guard = 9,
    Reunion = 10,
    Prayer = 11,
    Socialize = 12,
    Worship = 13,
    Performance = 14,
    Research = 15,
    PonderTopic = 16,
    DiscussTopic = 17,
    Read = 18,
    FillServiceOrder = 19,
    Write = 20,
    CopyWrittenContent = 21,
    TeachTopic = 22,
    Play = 23,
    MakeBelieve = 24,
    PlayWithToy = 25,
    Encounter = 26,
    StoreObject = 27,
}

---@class activity_event_participants
---@field histfigs any[]
---@field units any[]
---@field free_histfigs any[]
---@field free_units any[]
---@field activity_id integer
---@field event_id integer
df.activity_event_participants = nil

---@return activity_event_type
function df.activity_event.getType() end

---@param file file_compressorst
function df.activity_event.write_file(file) end

---@param file file_compressorst
---@param loadversion save_version
function df.activity_event.read_file(file, loadversion) end

---@return boolean
function df.activity_event.isEmpty() end -- returns true if hist_figure_ids empty or if various subclass fields are uninitialized

---@return integer
function df.activity_event.unnamed_method() end -- returns -1

---@param arg_0 integer
function df.activity_event.unnamed_method(arg_0) end -- does nothing

---@return activity_event_participants
function df.activity_event.getParticipantInfo() end

---@param children_too boolean
function df.activity_event.dismiss(children_too) end

---@param dx integer
---@param dy integer
---@param dz integer
function df.activity_event.move(dx, dy, dz) end

---@param histfig integer
---@param unit integer
---@param arg_2 boolean
function df.activity_event.removeParticipant(histfig, unit, arg_2) end

---@param arg_0 process_unit_aux
---@param unit unit
function df.activity_event.process(arg_0, unit) end

---@param unit unit
---@return integer
function df.activity_event.checkDrillInvalid(unit) end

---@param arg_0 integer
---@return boolean
function df.activity_event.decUniformLock(arg_0) end

---@return squad_event_type
function df.activity_event.getSquadEventType() end

---@param skill job_skill
function df.activity_event.setDemoSkill(skill) end

---@param wait_countdown integer
---@param train_rounds integer
---@param train_countdown integer
function df.activity_event.setSkillDemoTimers(wait_countdown, train_rounds, train_countdown) end

---@param amount integer
function df.activity_event.adjustOrganizeCounter(amount) end

---@param hist_figure_id integer
---@param unit_id integer
function df.activity_event.getOrganizer(hist_figure_id, unit_id) end -- or perhaps somebody else - only works for combat_training and skill_demonstration

---@return integer
function df.activity_event.getBuilding() end -- returns pointer to building_id

---@return boolean
function df.activity_event.isSparring() end

---@return integer
function df.activity_event.getUniformType() end

---@param unit_id integer
---@param str string
function df.activity_event.getName(unit_id, str) end

---@class activity_event
---@field event_id integer mostly, but not always, the index in activity.events
---@field activity_id integer
---@field parent_event_id integer
---@field flags bitfield
---@field unk_v42_1 any[]
---@field unk_v42_2 any[]
df.activity_event = nil

---@class activity_event_training_sessionst
-- inherited from activity_event
---@field event_id integer mostly, but not always, the index in activity.events
---@field activity_id integer
---@field parent_event_id integer
---@field flags bitfield
---@field unk_v42_1 any[]
---@field unk_v42_2 any[]
-- end activity_event
---@field participants activity_event_participants
df.activity_event_training_sessionst = nil

---@class activity_event_combat_trainingst
-- inherited from activity_event
---@field event_id integer mostly, but not always, the index in activity.events
---@field activity_id integer
---@field parent_event_id integer
---@field flags bitfield
---@field unk_v42_1 any[]
---@field unk_v42_2 any[]
-- end activity_event
---@field participants activity_event_participants
---@field building_id integer
---@field hist_figure_id integer
---@field unit_id integer
---@field organize_counter integer gt 0 => organizing, lt 0 => done
df.activity_event_combat_trainingst = nil

---@class activity_event_skill_demonstrationst
-- inherited from activity_event
---@field event_id integer mostly, but not always, the index in activity.events
---@field activity_id integer
---@field parent_event_id integer
---@field flags bitfield
---@field unk_v42_1 any[]
---@field unk_v42_2 any[]
-- end activity_event
---@field participants activity_event_participants
---@field building_id integer
---@field hist_figure_id integer
---@field unit_id integer
---@field skill job_skill
---@field organize_counter integer
---@field wait_countdown integer
---@field train_rounds integer
---@field train_countdown integer
df.activity_event_skill_demonstrationst = nil

---@class activity_event_individual_skill_drillst
-- inherited from activity_event
---@field event_id integer mostly, but not always, the index in activity.events
---@field activity_id integer
---@field parent_event_id integer
---@field flags bitfield
---@field unk_v42_1 any[]
---@field unk_v42_2 any[]
-- end activity_event
---@field participants activity_event_participants
---@field building_id integer
---@field countdown integer
df.activity_event_individual_skill_drillst = nil

---@class activity_event_sparringst
-- inherited from activity_event
---@field event_id integer mostly, but not always, the index in activity.events
---@field activity_id integer
---@field parent_event_id integer
---@field flags bitfield
---@field unk_v42_1 any[]
---@field unk_v42_2 any[]
-- end activity_event
---@field participants activity_event_participants
---@field building_id integer
---@field groups any[]
---@field countdown integer
df.activity_event_sparringst = nil

---@class activity_event_ranged_practicest
-- inherited from activity_event
---@field event_id integer mostly, but not always, the index in activity.events
---@field activity_id integer
---@field parent_event_id integer
---@field flags bitfield
---@field unk_v42_1 any[]
---@field unk_v42_2 any[]
-- end activity_event
---@field participants activity_event_participants
---@field building_id integer
---@field unnamed_activity_event_ranged_practicest_3 integer
---@field unnamed_activity_event_ranged_practicest_4 integer
---@field unnamed_activity_event_ranged_practicest_5 integer
df.activity_event_ranged_practicest = nil

---@class activity_event_harassmentst
-- inherited from activity_event
---@field event_id integer mostly, but not always, the index in activity.events
---@field activity_id integer
---@field parent_event_id integer
---@field flags bitfield
---@field unk_v42_1 any[]
---@field unk_v42_2 any[]
-- end activity_event
---@field unk_1 integer[]
---@field unk_2 any[]
---@field unk_3 integer
---@field unk_4 integer
---@field unk_5 integer
---@field unk_6 integer
---@field unk_7 integer
---@field unk_8 integer
df.activity_event_harassmentst = nil

---@enum conversation_menu
df.conversation_menu = {
    None = -1,
    RespondGreeting = 0,
    MainMenu = 1,
    unk_3 = 2,
    unk_4 = 3,
    unk_5 = 4,
    unk_6 = 5,
    unk_7 = 6,
    RespondGoodbye = 7,
    unk_9 = 8,
    DenyPermissionSleep = 9,
    AskJoin = 10,
    RespondJoin = 11,
    DiscussRescue = 12,
    DiscussAgreement = 13,
    DiscussTrade = 14,
    DiscussSurroundingArea = 15,
    RespondAccusation = 16,
    DiscussFamily = 17,
    RespondArmistice = 18,
    RespondDemandYield = 19,
    unk_21 = 20,
    unk_22 = 21,
    unk_23 = 22,
    AskDirections = 23,
    unk_25 = 24,
    unk_26 = 25,
    unk_27 = 26,
    unk_28 = 27,
    unk_29 = 28,
    unk_30 = 29,
    Demand = 30,
    unk_32 = 31,
    unk_33 = 32,
    unk_34 = 33,
    Barter = 34,
    DiscussHearthpersonDuties = 35,
    unk_37 = 36,
    DiscussJourney = 37,
    DiscussGroup = 38,
    DiscussConflict = 39,
    DiscussSite = 40,
    RespondDemand = 41,
    unk_43 = 42,
    RespondTributeDemand = 43,
    RespondTributeOffer = 44,
    DiscussTradeCancellation = 45,
    RespondPeaceOffer = 46,
    DiscussAgreementConclusion = 47,
    RespondAdoptionRequest = 48,
    unk_50 = 49,
    unk_51 = 50,
    RespondPositionOffer = 51,
    RespondInvocation = 52,
    unk_54 = 53,
    AskAboutPerson = 54,
    unk_56 = 55,
    DiscussFeelings = 56,
    unk_58 = 57,
    unk_59 = 58,
    unk_60 = 59,
    unk_61 = 60,
    unk_62 = 61,
    unk_63 = 62,
    unk_64 = 63,
    unk_65 = 64,
    StateGeneralThoughts = 65,
    DiscussValues = 66,
    RespondValues = 67,
    RespondPassiveReply = 68,
    RespondFlattery = 69,
    RespondDismissal = 70,
}

---@class activity_event_conversationst_unk2
---@field unk_1 incident[]
---@field unk_2 integer
---@field unk_3 integer
---@field unk_4 integer[]
---@field unk_5 integer[]
---@field unk_6 integer[]
---@field unk_7 integer[]
---@field unk_8 integer[]
---@field unk_9 integer[]
---@field unk_10 integer[]
---@field unk_11 integer[]
---@field unk_12 integer[]
---@field unk_13 integer[]
---@field unk_14 integer[]
---@field unk_15 integer[]
---@field unk_16 integer[]
---@field unk_17 integer[]
---@field unk_18 integer
---@field unk_19 integer
---@field unk_20 integer
---@field unk_21 integer
---@field unk_22 integer
---@field unk_23 integer
---@field unk_24 integer
---@field unk_25 integer
---@field unk_26 integer
---@field unk_27 integer
---@field unk_28 integer
---@field unk_29 integer

---@class activity_event_conversationst_unk_b4
---@field unk_1 integer
---@field unk_2 integer
---@field unk_3 integer
---@field unk_4 integer[]
---@field unk_5 integer since v0.47.01

---@class activity_event_conversationst
-- inherited from activity_event
---@field event_id integer mostly, but not always, the index in activity.events
---@field activity_id integer
---@field parent_event_id integer
---@field flags bitfield
---@field unk_v42_1 any[]
---@field unk_v42_2 any[]
-- end activity_event
---@field participants any[]
---@field menu conversation_menu
---@field unk1 entity_event
---@field unk_1 integer
---@field unk_2 integer
---@field unk_3 integer
---@field unk_4 integer
---@field unk_v42_3 integer since v0.42.01
---@field unk_v42_4 integer[] since v0.42.01
---@field unk_5 integer[]
---@field unk_6 any[]
---@field unk_7 integer[]
---@field unk_8 integer[]
---@field unk_b4 activity_event_conversationst_unk_b4
---@field turns any[]
---@field floor_holder integer -1 = no one's turn
---@field floor_holder_hfid integer -1 = no one's turn
---@field pause integer ticks since the last turn
---@field flags2 bitfield
---@field unk2 activity_event_conversationst_unk2
---@field choices talk_choice[]
---@field unk3 conversation_menu
---@field unk4 integer[] uninitialized
df.activity_event_conversationst = nil

---@class activity_event_conflictst
-- inherited from activity_event
---@field event_id integer mostly, but not always, the index in activity.events
---@field activity_id integer
---@field parent_event_id integer
---@field flags bitfield
---@field unk_v42_1 any[]
---@field unk_v42_2 any[]
-- end activity_event
---@field sides any[]
---@field unk_1 integer
---@field unk_2 integer
---@field unk_3 integer
---@field unk_v42_3 integer since v0.42.01
---@field unnamed_activity_event_conflictst_6 integer since v0.50.01
df.activity_event_conflictst = nil

---@class activity_event_guardst
-- inherited from activity_event
---@field event_id integer mostly, but not always, the index in activity.events
---@field activity_id integer
---@field parent_event_id integer
---@field flags bitfield
---@field unk_v42_1 any[]
---@field unk_v42_2 any[]
-- end activity_event
---@field unk_1 integer[]
---@field unk_2 coord
---@field unk_3 integer
df.activity_event_guardst = nil

---@class activity_event_reunionst
-- inherited from activity_event
---@field event_id integer mostly, but not always, the index in activity.events
---@field activity_id integer
---@field parent_event_id integer
---@field flags bitfield
---@field unk_v42_1 any[]
---@field unk_v42_2 any[]
-- end activity_event
---@field unk_1 integer[]
---@field unk_2 integer[]
---@field unk_3 integer
---@field unk_4 integer
---@field unk_5 integer
---@field unk_6 integer
---@field unk_7 integer
df.activity_event_reunionst = nil

---@class activity_event_prayerst
-- inherited from activity_event
---@field event_id integer mostly, but not always, the index in activity.events
---@field activity_id integer
---@field parent_event_id integer
---@field flags bitfield
---@field unk_v42_1 any[]
---@field unk_v42_2 any[]
-- end activity_event
---@field participants activity_event_participants
---@field histfig_id integer deity
---@field topic sphere_type -1 when praying
---@field site_id integer
---@field location_id integer
---@field building_id integer
---@field timer integer
df.activity_event_prayerst = nil

---@class activity_event_socializest
-- inherited from activity_event
---@field event_id integer mostly, but not always, the index in activity.events
---@field activity_id integer
---@field parent_event_id integer
---@field flags bitfield
---@field unk_v42_1 any[]
---@field unk_v42_2 any[]
-- end activity_event
---@field participants activity_event_participants
---@field site_id integer
---@field location_id integer
---@field building_id integer
---@field unk_1 integer
df.activity_event_socializest = nil

---@class activity_event_worshipst
-- inherited from activity_event
---@field event_id integer mostly, but not always, the index in activity.events
---@field activity_id integer
---@field parent_event_id integer
---@field flags bitfield
---@field unk_v42_1 any[]
---@field unk_v42_2 any[]
-- end activity_event
---@field participants activity_event_participants
---@field site_id integer
---@field location_id integer
---@field building_id integer
---@field unk_1 integer
df.activity_event_worshipst = nil

---@enum performance_event_type
df.performance_event_type = {
    STORY = 0,
    POETRY = 1,
    MUSIC = 2,
    DANCE = 3,
    SERMON_EVENT = 4,
    SERMON_SPHERE = 5,
    SERMON_PROMOTE_VALUE = 6,
    SERMON_INVEIGH_AGAINST_VALUE = 7,
}

---@enum performance_participant_type
df.performance_participant_type = {
    TELL_STORY = 0,
    RECITE_POETRY = 1,
    MAKE_MUSIC = 2,
    PERFORM_DANCE = 3,
    LISTEN = 4,
    HEAR = 5,
}

---@class activity_event_performancest
-- inherited from activity_event
---@field event_id integer mostly, but not always, the index in activity.events
---@field activity_id integer
---@field parent_event_id integer
---@field flags bitfield
---@field unk_v42_1 any[]
---@field unk_v42_2 any[]
-- end activity_event
---@field participants activity_event_participants
---@field type performance_event_type
---@field event integer used for story
---@field written_content_id integer
---@field poetic_form integer
---@field music_form integer
---@field dance_form integer
---@field unk_1 integer
---@field unk_2 integer
---@field unk_3 integer
---@field unk_4 integer
---@field unk_5 integer
---@field unk_6 integer
---@field participant_actions any[]
---@field pos_performer_2d coord2d
---@field pos_performer coord
---@field unk_pos_1_x0 integer
---@field unk_pos_1_y0 integer
---@field unk_pos_1_x1 integer
---@field unk_pos_1_y1 integer
---@field unk_pos_1_z integer
---@field unk_pos_2_x0 integer
---@field unk_pos_2_y0 integer
---@field unk_pos_2_x1 integer
---@field unk_pos_2_y1 integer
---@field unk_pos_2_z integer
---@field play_orders performance_play_orderst[]
---@field unk_11 integer
---@field unk_12 integer[]
---@field unk_13 coord
---@field unk_16 integer
---@field unk_17 integer
---@field unk_18 integer
df.activity_event_performancest = nil

---@param file file_compressorst
function df.performance_play_orderst.write_file(file) end

---@param file file_compressorst
---@param loadversion save_version
function df.performance_play_orderst.read_file(file, loadversion) end

---@class performance_play_orderst
---@field unk_1 integer
---@field unk_2 integer
---@field unk_3 integer
---@field unk_4 any[]
---@field unk_5 integer
df.performance_play_orderst = nil

---@class activity_event_researchst
-- inherited from activity_event
---@field event_id integer mostly, but not always, the index in activity.events
---@field activity_id integer
---@field parent_event_id integer
---@field flags bitfield
---@field unk_v42_1 any[]
---@field unk_v42_2 any[]
-- end activity_event
---@field participants activity_event_participants
---@field site_id integer
---@field location_id integer
---@field building_id integer
df.activity_event_researchst = nil

---@class activity_event_ponder_topicst
-- inherited from activity_event
---@field event_id integer mostly, but not always, the index in activity.events
---@field activity_id integer
---@field parent_event_id integer
---@field flags bitfield
---@field unk_v42_1 any[]
---@field unk_v42_2 any[]
-- end activity_event
---@field participants activity_event_participants
---@field site_id integer
---@field location_id integer
---@field building_id integer
---@field unk_1 integer
---@field knowledge knowledge_scholar_category_flag
---@field timer integer
df.activity_event_ponder_topicst = nil

---@class activity_event_discuss_topicst
-- inherited from activity_event
---@field event_id integer mostly, but not always, the index in activity.events
---@field activity_id integer
---@field parent_event_id integer
---@field flags bitfield
---@field unk_v42_1 any[]
---@field unk_v42_2 any[]
-- end activity_event
---@field participants activity_event_participants
---@field site_id integer
---@field location_id integer
---@field building_id integer
---@field unk_1 integer
---@field knowledge knowledge_scholar_category_flag
---@field timer integer
---@field unk_2 integer
---@field unk_3 integer
df.activity_event_discuss_topicst = nil

---@class activity_event_readst
-- inherited from activity_event
---@field event_id integer mostly, but not always, the index in activity.events
---@field activity_id integer
---@field parent_event_id integer
---@field flags bitfield
---@field unk_v42_1 any[]
---@field unk_v42_2 any[]
-- end activity_event
---@field participants activity_event_participants
---@field building_id integer
---@field site_id integer
---@field location_id integer
---@field state integer 0 if not in progress, 1 if reading
---@field timer integer
df.activity_event_readst = nil

---@class activity_event_fill_service_orderst
-- inherited from activity_event
---@field event_id integer mostly, but not always, the index in activity.events
---@field activity_id integer
---@field parent_event_id integer
---@field flags bitfield
---@field unk_v42_1 any[]
---@field unk_v42_2 any[]
-- end activity_event
---@field histfig_id integer
---@field unit_id integer
---@field occupation_id integer
---@field unk_1 integer
df.activity_event_fill_service_orderst = nil

---@enum activity_event_writest_mode
df.activity_event_writest_mode = {
    WriteAboutKnowledge = 0,
}
---@class activity_event_writest
-- inherit activity_event
---@field participants activity_event_participants
---@field building_id integer
---@field site_id integer
---@field location_id integer
---@field unk_1 bitfield
---@field timer integer
---@field unk_2 integer
---@field unk_3 integer
---@field activity_event_writest_mode activity_event_writest_mode
---@field knowledge knowledge_scholar_category_flag
df.activity_event_writest = nil

---@class activity_event_copy_written_contentst
-- inherited from activity_event
---@field event_id integer mostly, but not always, the index in activity.events
---@field activity_id integer
---@field parent_event_id integer
---@field flags bitfield
---@field unk_v42_1 any[]
---@field unk_v42_2 any[]
-- end activity_event
---@field unit_id integer
---@field histfig_id integer
---@field occupation_id integer
---@field building_id integer
---@field site_id integer
---@field location_id integer
---@field flagsmaybe bitfield
---@field unk_1 integer
---@field timer integer
---@field unnamed_activity_event_copy_written_contentst_10 integer since v0.50.01
df.activity_event_copy_written_contentst = nil

---@class activity_event_teach_topicst
-- inherited from activity_event
---@field event_id integer mostly, but not always, the index in activity.events
---@field activity_id integer
---@field parent_event_id integer
---@field flags bitfield
---@field unk_v42_1 any[]
---@field unk_v42_2 any[]
-- end activity_event
---@field participants activity_event_participants
---@field unk_1 integer
---@field unk_2 integer
---@field unk_3 integer
---@field unk_4 integer
---@field unk_5 integer
---@field unk_6 integer
---@field unk_7 integer
---@field unk_8 integer
---@field unk_9 integer
df.activity_event_teach_topicst = nil

---@class activity_event_playst
-- inherited from activity_event
---@field event_id integer mostly, but not always, the index in activity.events
---@field activity_id integer
---@field parent_event_id integer
---@field flags bitfield
---@field unk_v42_1 any[]
---@field unk_v42_2 any[]
-- end activity_event
---@field participants activity_event_participants
---@field unk_1 integer
---@field unk_2 integer[]
---@field unk_3 coord
df.activity_event_playst = nil

---@class activity_event_make_believest
-- inherited from activity_event
---@field event_id integer mostly, but not always, the index in activity.events
---@field activity_id integer
---@field parent_event_id integer
---@field flags bitfield
---@field unk_v42_1 any[]
---@field unk_v42_2 any[]
-- end activity_event
---@field participants activity_event_participants
---@field unk_1 integer
---@field unk_2 integer
---@field unk_3 integer
---@field unk_4 integer[]
---@field unk_5 coord
df.activity_event_make_believest = nil

---@class activity_event_play_with_toyst_unk
---@field unk_1 integer[]
---@field unk_2 coord

---@class activity_event_play_with_toyst
-- inherited from activity_event
---@field event_id integer mostly, but not always, the index in activity.events
---@field activity_id integer
---@field parent_event_id integer
---@field flags bitfield
---@field unk_v42_1 any[]
---@field unk_v42_2 any[]
-- end activity_event
---@field participants activity_event_participants
---@field unk_1 integer
---@field unk_2 integer
---@field unk_3 integer
---@field unk activity_event_play_with_toyst_unk
---@field unk_4 integer
---@field unk_5 integer[]
df.activity_event_play_with_toyst = nil

---@class activity_event_encounterst
-- inherited from activity_event
---@field event_id integer mostly, but not always, the index in activity.events
---@field activity_id integer
---@field parent_event_id integer
---@field flags bitfield
---@field unk_v42_1 any[]
---@field unk_v42_2 any[]
-- end activity_event
---@field unk_1 any[]
---@field unk_2 any[]
---@field unk_3 integer[]
---@field unk_4 integer[]
---@field unk_5 integer
---@field unk_6 integer
---@field unk_7 integer
---@field unk_8 integer
---@field unk_9 integer
df.activity_event_encounterst = nil

---@class activity_event_store_objectst
-- inherited from activity_event
---@field event_id integer mostly, but not always, the index in activity.events
---@field activity_id integer
---@field parent_event_id integer
---@field flags bitfield
---@field unk_v42_1 any[]
---@field unk_v42_2 any[]
-- end activity_event
---@field unk_1 integer
---@field unk_2 coord
---@field building_id integer
---@field unk_3 integer
---@field unk_4 integer
df.activity_event_store_objectst = nil

---@class schedule_info
---@field id integer
---@field unk_1 integer
---@field slots schedule_slot[]
df.schedule_info = nil

---@class schedule_slot
---@field type integer 0:Eat, 1:Sleep, 2-4:???
---@field start_time integer
---@field end_time integer
---@field unk_1 integer
---@field processed integer
df.schedule_slot = nil


