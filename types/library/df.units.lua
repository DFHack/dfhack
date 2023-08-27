-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.

---@meta

---@alias unit_flags1 bitfield

---@alias unit_flags2 bitfield

---@alias unit_flags3 bitfield

---@alias unit_flags4 bitfield

---@alias work_detail_flags bitfield

---@enum value_type
df.value_type = {
    NONE = -1,
    LAW = 0,
    LOYALTY = 1,
    FAMILY = 2,
    FRIENDSHIP = 3,
    POWER = 4,
    TRUTH = 5,
    CUNNING = 6,
    ELOQUENCE = 7,
    FAIRNESS = 8,
    DECORUM = 9,
    TRADITION = 10,
    ARTWORK = 11,
    COOPERATION = 12,
    INDEPENDENCE = 13,
    STOICISM = 14,
    INTROSPECTION = 15,
    SELF_CONTROL = 16,
    TRANQUILITY = 17,
    HARMONY = 18,
    MERRIMENT = 19,
    CRAFTSMANSHIP = 20,
    MARTIAL_PROWESS = 21,
    SKILL = 22,
    HARD_WORK = 23,
    SACRIFICE = 24,
    COMPETITION = 25,
    PERSEVERENCE = 26,
    LEISURE_TIME = 27,
    COMMERCE = 28,
    ROMANCE = 29,
    NATURE = 30,
    PEACE = 31,
    KNOWLEDGE = 32,
}

---@enum goal_type
df.goal_type = {
    STAY_ALIVE = 0,
    MAINTAIN_ENTITY_STATUS = 1,
    START_A_FAMILY = 2,
    RULE_THE_WORLD = 3,
    CREATE_A_GREAT_WORK_OF_ART = 4,
    CRAFT_A_MASTERWORK = 5,
    BRING_PEACE_TO_THE_WORLD = 6,
    BECOME_A_LEGENDARY_WARRIOR = 7,
    MASTER_A_SKILL = 8,
    FALL_IN_LOVE = 9,
    SEE_THE_GREAT_NATURAL_SITES = 10,
    IMMORTALITY = 11,
    MAKE_A_GREAT_DISCOVERY = 12,
    ATTAIN_RANK_IN_SOCIETY = 13, -- since v0.47.01
    BATHE_WORLD_IN_CHAOS = 14, -- since v0.47.01
}

---@enum personality_facet_type
df.personality_facet_type = {
    NONE = -1,
    LOVE_PROPENSITY = 0,
    HATE_PROPENSITY = 1,
    ENVY_PROPENSITY = 2,
    CHEER_PROPENSITY = 3,
    DEPRESSION_PROPENSITY = 4,
    ANGER_PROPENSITY = 5,
    ANXIETY_PROPENSITY = 6,
    LUST_PROPENSITY = 7,
    STRESS_VULNERABILITY = 8,
    GREED = 9,
    IMMODERATION = 10,
    VIOLENT = 11,
    PERSEVERENCE = 12,
    WASTEFULNESS = 13,
    DISCORD = 14,
    FRIENDLINESS = 15,
    POLITENESS = 16,
    DISDAIN_ADVICE = 17,
    BRAVERY = 18,
    CONFIDENCE = 19,
    VANITY = 20,
    AMBITION = 21,
    GRATITUDE = 22,
    IMMODESTY = 23,
    HUMOR = 24,
    VENGEFUL = 25,
    PRIDE = 26,
    CRUELTY = 27,
    SINGLEMINDED = 28,
    HOPEFUL = 29,
    CURIOUS = 30,
    BASHFUL = 31,
    PRIVACY = 32,
    PERFECTIONIST = 33,
    CLOSEMINDED = 34,
    TOLERANT = 35,
    EMOTIONALLY_OBSESSIVE = 36,
    SWAYED_BY_EMOTIONS = 37,
    ALTRUISM = 38,
    DUTIFULNESS = 39,
    THOUGHTLESSNESS = 40,
    ORDERLINESS = 41,
    TRUST = 42,
    GREGARIOUSNESS = 43,
    ASSERTIVENESS = 44,
    ACTIVITY_LEVEL = 45,
    EXCITEMENT_SEEKING = 46,
    IMAGINATION = 47,
    ABSTRACT_INCLINED = 48,
    ART_INCLINED = 49,
}

---@enum physical_attribute_type
df.physical_attribute_type = {
    STRENGTH = 0,
    AGILITY = 1,
    TOUGHNESS = 2,
    ENDURANCE = 3,
    RECUPERATION = 4,
    DISEASE_RESISTANCE = 5,
}

---@enum mental_attribute_type
df.mental_attribute_type = {
    ANALYTICAL_ABILITY = 0,
    FOCUS = 1,
    WILLPOWER = 2,
    CREATIVITY = 3,
    INTUITION = 4,
    PATIENCE = 5,
    MEMORY = 6,
    LINGUISTIC_ABILITY = 7,
    SPATIAL_SENSE = 8,
    MUSICALITY = 9,
    KINESTHETIC_SENSE = 10,
    EMPATHY = 11,
    SOCIAL_AWARENESS = 12,
}

---@enum mood_type
df.mood_type = {
    None = -1,
    Fey = 0,
    Secretive = 1,
    Possessed = 2,
    Macabre = 3,
    Fell = 4,
    Melancholy = 5,
    Raving = 6,
    Berserk = 7,
    Baby = 8,
    Traumatized = 9,
}

---@enum ghost_type
df.ghost_type = {
    None = -1,
    MurderousGhost = 0,
    SadisticGhost = 1,
    SecretivePoltergeist = 2,
    EnergeticPoltergeist = 3,
    AngryGhost = 4,
    ViolentGhost = 5,
    MoaningSpirit = 6,
    HowlingSpirit = 7,
    TroublesomePoltergeist = 8,
    RestlessHaunt = 9,
    ForlornHaunt = 10,
}

---@enum animal_training_level
df.animal_training_level = {
    SemiWild = 0,
    Trained = 1,
    WellTrained = 2,
    SkilfullyTrained = 3,
    ExpertlyTrained = 4,
    ExceptionallyTrained = 5,
    MasterfullyTrained = 6,
    Domesticated = 7,
    Unk8 = 8,
    WildUntamed = 9, -- Seems to be used as default when not flags1.tame
}

---@enum unit_report_type
df.unit_report_type = {
    Combat = 0,
    Sparring = 1,
    Hunting = 2,
}

---@enum skill_rating
df.skill_rating = {
    Dabbling = 0,
    Novice = 1,
    Adequate = 2,
    Competent = 3,
    Skilled = 4,
    Proficient = 5,
    Talented = 6,
    Adept = 7,
    Expert = 8,
    Professional = 9,
    Accomplished = 10,
    Great = 11,
    Master = 12,
    HighMaster = 13,
    GrandMaster = 14,
    Legendary = 15,
    Legendary1 = 16,
    Legendary2 = 17,
    Legendary3 = 18,
    Legendary4 = 19,
    Legendary5 = 20,
}

---@enum unit_relationship_type Used in unit.relations
df.unit_relationship_type = {
    None = -1,
    Pet = 0,
    Spouse = 1,
    Mother = 2,
    Father = 3,
    LastAttacker = 4,
    GroupLeader = 5,
    Draggee = 6,
    Dragger = 7,
    RiderMount = 8,
    Lover = 9,
    unk10 = 10,
    Sibling = 11,
    Child = 12,
    Friend = 13,
    Grudge = 14,
    Worship = 15,
    AcquaintanceLong = 16,
    AcquaintancePassing = 17,
    Bonded = 18,
    Hero = 19,
    ConsidersViolent = 20,
    ConsidersPsychotic = 21,
    GoodForBusiness = 22,
    FriendlyTerms = 23,
    ConsidersKiller = 24,
    ConsidersMurderer = 25,
    Comrade = 26,
    MemberOfRespectedGroup = 27,
    MemberOfHatedGroup = 28,
    EnemyFighter = 29,
    FriendlyFighter = 30,
    ConsidersBully = 31,
    ConsidersBrigand = 32,
    LoyalSoldier = 33,
    ConsidersMonster = 34,
    ConsidersStoryteller = 35,
    ConsidersPoet = 36,
    ConsidersBard = 37,
    ConsidersDancer = 38,
    Master = 39,
    Apprentice = 40,
    Companion = 41,
    FormerMaster = 42,
    FormerApprentice = 43,
    ConsidersQuarreler = 44,
    ConsidersFlatterer = 45,
    Hunter = 46,
    ProtectorOfTheWeak = 47,
}

---@enum need_type
df.need_type = {
    Socialize = 0,
    DrinkAlcohol = 1,
    PrayOrMeditate = 2,
    StayOccupied = 3,
    BeCreative = 4,
    Excitement = 5,
    LearnSomething = 6,
    BeWithFamily = 7,
    BeWithFriends = 8,
    HearEloquence = 9,
    UpholdTradition = 10,
    SelfExamination = 11,
    MakeMerry = 12,
    CraftObject = 13,
    MartialTraining = 14,
    PracticeSkill = 15,
    TakeItEasy = 16,
    MakeRomance = 17,
    SeeAnimal = 18,
    SeeGreatBeast = 19,
    AcquireObject = 20,
    EatGoodMeal = 21,
    Fight = 22,
    CauseTrouble = 23,
    Argue = 24,
    BeExtravagant = 25,
    Wander = 26,
    HelpSomebody = 27,
    ThinkAbstractly = 28,
    AdmireArt = 29,
}

---@enum pronoun_type
df.pronoun_type = {
    it = -1,
    she = 0,
    he = 1,
}

---@return integer
function df.unit.getCreatureTile() end

---@return integer
function df.unit.getCorpseTile() end

---@return integer
function df.unit.getGlowTile() end

---@class unit_enemy_unk_v40_sub3 since v0.40.01
---@field controller army_controller since v0.40.01
---@field unk_2 integer since v0.40.01
---@field unk_3 integer[] since v0.40.01
---@field unk_4 integer[] since v0.40.01
---@field unk_5 integer[] since v0.40.01
---@field unk_6 integer
---@field visitor_info integer since v0.42.01

---@class unit_enemy
---@field sound_cooldown integer[] since v0.34.01
---@field undead integer since v0.34.01
---@field were_race integer
---@field were_caste integer
---@field normal_race integer
---@field normal_caste integer
---@field interaction integer is set when a RETRACT_INTO_BP interaction is active, since v0.34.01
---@field appearances unit_appearance[]
---@field witness_reports witness_report[]
---@field unk_a5c entity_event[]
---@field gait_index integer[]
---@field unk_unit_id_1 integer[] number of non -1 entries control linked contents in following 4 vectors, rotating, since v0.40.01
---@field unk_v40_1b integer[] since v0.40.01
---@field unk_v40_1c integer[] unused elements probably uninitialized, since v0.40.01
---@field unk_v40_1d integer[] unused elements probably uninitialized, since v0.40.01
---@field unk_v40_1e integer[] unused elements probably uninitialized, since v0.40.01
---@field unk_unit_id_2 integer[] Seen own side, enemy side, not involved (witnesses?). Unused fields not cleared, since v0.40.01
---@field unk_unit_id_2_count integer since v0.40.01
---@field unk_448 integer since v0.40.01
---@field unk_44c integer since v0.40.01
---@field unk_450 integer since v0.40.01
---@field unk_454 integer since v0.40.01
---@field army_controller_id integer since v0.40.01
---@field unk_v40_sub3 unit_enemy_unk_v40_sub3 since v0.40.01
---@field combat_side_id integer
---@field histfig_vector_idx integer
---@field caste_flags caste_raw_flags[] since v0.44.06
---@field enemy_status_slot integer
---@field unk_874_cntr integer
---@field body_part_878 integer[]
---@field body_part_888 integer[]
---@field body_part_relsize integer[] with modifiers
---@field body_part_8a8 integer[]
---@field body_part_base_ins integer[]
---@field body_part_clothing_ins integer[]
---@field body_part_8d8 integer[]
---@field unk_8e8 integer[]
---@field unk_8f8 integer[]

---@class unit_reports
---@field log any[]
---@field last_year integer[]
---@field last_year_tick integer[]

---@class unit_syndromes
---@field active unit_syndrome[]
---@field reinfection_type integer[]
---@field reinfection_count integer[]

---@class unit_unknown7
---@field unk_7c4 integer[]
---@field unk_c integer[] since v0.34.01

---@class unit_status2
---@field limbs_stand_max integer
---@field limbs_stand_count integer
---@field limbs_grasp_max integer
---@field limbs_grasp_count integer
---@field limbs_fly_max integer
---@field limbs_fly_count integer
---@field body_part_temperature temperaturest[]
---@field add_path_flags integer pathing flags to OR, set to 0 after move
---@field liquid_type bitfield
---@field liquid_depth integer
---@field histeventcol_id integer linked to an active invasion or kidnapping

---@class unit_status
---@field misc_traits unit_misc_trait[]
---@field eat_history integer
---@field demand_timeout integer
---@field mandate_timeout integer
---@field attacker_ids integer[]
---@field attacker_cntdn integer[]
---@field face_direction integer for wagons
---@field artifact_name language_name
---@field souls unit_soul[]
---@field current_soul unit_soul
---@field demands unit_demand[]
---@field labors boolean[]
---@field wrestle_items unit_item_wrestle[]
---@field observed_traps integer[]
---@field complaints unit_complaint[]
---@field parleys unit_parley[] since v0.44.01
---@field requests unit_request[]
---@field coin_debts unit_coin_debt[]
---@field unk_1 any[] since v0.47.01
---@field unk_2 integer since v0.47.01
---@field unk_3 integer since v0.47.01
---@field unk_4 integer[] initialized together with enemy.gait_index, since v0.47.01
---@field unk_5 integer since v0.47.01
---@field adv_sleep_timer integer
---@field recent_job_area coord
---@field recent_jobs coord_path

---@class unit_counters2
---@field paralysis integer
---@field numbness integer
---@field fever integer
---@field exhaustion integer
---@field hunger_timer integer
---@field thirst_timer integer
---@field sleepiness_timer integer
---@field stomach_content integer
---@field stomach_food integer
---@field vomit_timeout integer blocks nausea causing vomit
---@field stored_fat integer hunger leads to death only when 0

---@class unit_curse
---@field unk_0 integer moved from end of counters in 0.43.05
---@field add_tags1 bitfield
---@field rem_tags1 bitfield
---@field add_tags2 bitfield
---@field rem_tags2 bitfield
---@field name_visible boolean since v0.34.01
---@field name string since v0.34.01
---@field name_plural string since v0.34.01
---@field name_adjective string since v0.34.01
---@field sym_and_color1 integer since v0.34.01
---@field sym_and_color2 integer since v0.34.01
---@field flash_period integer since v0.34.01
---@field flash_time2 integer since v0.34.01
---@field body_appearance integer[]
---@field bp_appearance integer[] guess!, since v0.34.01
---@field speed_add integer since v0.34.01
---@field speed_mul_percent integer since v0.34.01
---@field attr_change curse_attr_change since v0.34.01
---@field luck_mul_percent integer since v0.34.01
---@field unk_98 integer since v0.42.01
---@field interaction_id integer[] since v0.34.01
---@field interaction_time integer[] since v0.34.01
---@field interaction_delay integer[] since v0.34.01
---@field time_on_site integer since v0.34.01
---@field own_interaction integer[] since v0.34.01
---@field own_interaction_delay integer[] since v0.34.01

---@enum unit_counters_soldier_mood
df.unit_counters_soldier_mood = {
    None = -1,
    MartialTrance = 0,
    Enraged = 1,
    Tantrum = 2,
    Depressed = 3,
    Oblivious = 4,
}
---@class unit_counters
---@field think_counter integer
---@field job_counter integer
---@field swap_counter integer dec per job_counter reroll, can_swap if 0
---@field death_cause death_type
---@field death_id integer
---@field winded integer
---@field stunned integer
---@field unconscious integer
---@field suffocation integer counts up while winded, results in death
---@field webbed integer
---@field guts_trail1 coord
---@field guts_trail2 coord
---@field soldier_mood_countdown integer plus a random amount
---@field unit_counters_soldier_mood unit_counters_soldier_mood
---@field pain integer
---@field nausea integer
---@field dizziness integer

---@class unit_appearance
---@field body_modifiers integer[]
---@field bp_modifiers integer[]
---@field size_modifier integer product of all H/B/LENGTH body modifiers, in %
---@field tissue_style integer[]
---@field tissue_style_civ_id integer[]
---@field tissue_style_id integer[]
---@field tissue_style_type integer[]
---@field tissue_length integer[] description uses bp_modifiers[style_list_idx[index]]
---@field genes unit_genes
---@field colors integer[]
df.unit_unit_appearance = nil
---@class unit_body
---@field components body_component_info
---@field wounds unit_wound[]
---@field wound_next_id integer
---@field unk_39c any[]
---@field body_plan caste_body_info
---@field weapon_bp integer
---@field physical_attrs unit_attribute[]
---@field size_info body_size_info
---@field blood_max integer
---@field blood_count integer
---@field infection_level integer GETS_INFECTIONS_FROM_ROT sets; DISEASE_RESISTANCE reduces; >=300 causes bleeding
---@field spatters spatter[]

---@class unit_job
---@field account integer
---@field satisfaction integer amount earned recently for jobs
---@field hunt_target unit
---@field target_flags integer if set, the unit will try to remove the helmet of their target
---@field destroy_target building
---@field unk_v40_1 integer since v0.40.01
---@field unk_v40_2 integer since v0.40.01
---@field unk_v40_3 integer since v0.40.01
---@field unk_v40_4 integer since v0.40.01
---@field unk_v40_5 integer since v0.40.01
---@field gait_buildup integer
---@field climb_hold coord
---@field unk_v4014_1 integer since v0.40.14
---@field current_job job df_job
---@field mood_skill job_skill can be uninitialized for children and animals
---@field mood_timeout integer counts down from 50000, insanity upon reaching zero
---@field unk_39c integer

---@class unit_opponent
---@field unit_id integer since v0.40.01
---@field unit_pos coord since v0.40.01
---@field unk_c integer since v0.40.01

---@class unit_animal
---@field population world_population_ref
---@field leave_countdown integer once 0, it heads for the edge and leaves
---@field vanish_countdown integer once 0, it vanishes in a puff of smoke

---@class unit_military
---@field squad_id integer
---@field squad_position integer
---@field patrol_cooldown integer
---@field patrol_timer integer
---@field cur_uniform integer
---@field unk_items integer[] since v0.34.06
---@field uniforms any[]
---@field pickup_flags bitfield
---@field uniform_pickup integer[]
---@field uniform_drop integer[]
---@field individual_drills integer[]

---@enum unit_meeting_state
df.unit_meeting_state = {
    SelectNoble = 0,
    FollowNoble = 1,
    DoMeeting = 2,
    LeaveMap = 3,
}
---@class unit_meeting
---@field unit_meeting_state unit_meeting_state
---@field target_entity integer
---@field target_role entity_position_responsibility
---@field pad_1 integer

---@class unit_path
---@field dest coord
---@field goal unit_path_goal
---@field path coord_path

---@class unit_idle_area

---@class unit
---@field name language_name
---@field custom_profession string
---@field profession profession
---@field profession2 profession
---@field race integer
---@field pos coord
---@field idle_area coord
---@field idle_area_threshold integer
---@field idle_area_type unit_station_type
---@field follow_distance integer
---@field path unit_path
---@field flags1 bitfield
---@field flags2 bitfield
---@field flags3 bitfield
---@field flags4 bitfield
---@field meeting unit_meeting
---@field caste integer
---@field sex pronoun_type
---@field id integer
---@field unk_100 integer
---@field training_level animal_training_level
---@field schedule_id integer
---@field civ_id integer
---@field population_id integer
---@field unk_c0 integer since v0.34.01
---@field cultural_identity integer since v0.40.01
---@field invasion_id integer
---@field patrol_route coord_path used by necromancers for corpse locations, siegers etc
---@field patrol_index integer from 23a
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field military unit_military
---@field social_activities integer[]
---@field conversations integer[] since v0.40.01
---@field activities integer[]
---@field unk_1e8 integer[] since v0.40.01
---@field animal unit_animal
---@field opponent unit_opponent
---@field mood mood_type
---@field unk_18e integer
---@field pregnancy_timer integer
---@field pregnancy_genes unit_genes genes from mate
---@field pregnancy_caste integer caste of mate
---@field pregnancy_spouse integer
---@field mood_copy mood_type copied from mood type upon entering strange mood
---@field ghost_info unit_ghost_info
---@field unk_9 integer since v0.34.01
---@field birth_year integer
---@field birth_time integer
---@field curse_year integer since v0.34.01
---@field curse_time integer since v0.34.01
---@field birth_year_bias integer since v0.34.01
---@field birth_time_bias integer since v0.34.01
---@field old_year integer could there be a death of old age time??
---@field old_time integer
---@field following unit
---@field unk_238 integer invalid unless following
---@field relationship_ids integer[]
---@field mount_type integer 0 = riding, 1 = being carried, 2/3 = wagon horses, 4 = wagon merchant
---@field last_hit history_hit_item
---@field inventory unit_inventory_item[]
---@field owned_items integer[]
---@field traded_items integer[] items brought to trade depot
---@field owned_buildings building[]
---@field corpse_parts integer[] entries remain even when items are destroyed
---@field riding_item_id integer since v0.34.08
---@field unnamed_unit_68 integer
---@field job unit_job
---@field body unit_body
---@field appearance unit_appearance
---@field actions unit_action[]
---@field next_action_id integer
---@field counters unit_counters
---@field curse unit_curse
---@field counters2 unit_counters2
---@field status unit_status
---@field hist_figure_id integer
---@field hist_figure_id2 integer used for ghost in AttackedByDead thought
---@field status2 unit_status2
---@field unknown7 unit_unknown7
---@field syndromes unit_syndromes
---@field reports unit_reports
---@field health unit_health_info
---@field used_items unit_item_use[] Contains worn clothes, armor, weapons, arrows fired by archers
---@field enemy unit_enemy
---@field healing_rate integer[]
---@field effective_rate integer
---@field tendons_heal integer
---@field ligaments_heal integer
---@field weight integer
---@field weight_fraction integer 1e-6
---@field burrows integer[]
---@field inactive_burrows integer[]
---@field vision_cone integer
---@field occupations occupation[] since v0.42.01
---@field adjective string from physical descriptions for use in adv
---@field texpos any[]
---@field sheet_icon_texpos integer
---@field texpos_currently_in_use any[]
---@field cached_glowtile_type integer
---@field pool_index integer
---@field mtx integer
df.unit = nil

---@enum witness_report_type
df.witness_report_type = {
    None = -1,
    WitnessedCrime = 0,
    FoundCorpse = 1,
}

---@alias witness_report_flags bitfield

---@class witness_report
---@field death_id integer
---@field crime_id integer
---@field type witness_report_type
---@field year integer
---@field year_tick integer
---@field flags bitfield
---@field unk_18 integer since v0.47.01
---@field unk_1c integer since v0.47.01
---@field unk_20 integer since v0.47.01
---@field unk_24 integer since v0.47.01
---@field unk_28 integer since v0.47.01
---@field unk_2c integer since v0.47.01
---@field unk_30 integer since v0.47.01
---@field unk_34 integer since v0.47.01
---@field pos coord since v0.47.01
df.witness_report = nil

---@enum ghost_goal
df.ghost_goal = {
    None = -1,
    ScareToDeath = 0,
    Stun = 1,
    Batter = 2,
    Possess = 3,
    MisplaceItem = 4,
    Haunt = 5,
    Torment = 6,
    ToppleBuilding = 7,
}

---@class unit_ghost_info
---@field type ghost_type
---@field type2 ghost_type seems to have same value as type
---@field goal ghost_goal
---@field target unit | item | building
---@field misplace_pos coord
---@field action_timer integer time since last action
---@field unk_18 integer
---@field flags bitfield
---@field death_x integer in tiles, global to world
---@field death_y integer
---@field death_z integer
df.unit_ghost_info = nil

---@class unit_genes
---@field appearance integer
---@field colors integer
df.unit_genes = nil

---@enum unit_inventory_item_mode
df.unit_inventory_item_mode = {
    Hauled = 0,
    Weapon = 1, -- also shield, crutch
    Worn = 2, -- quiver
    Piercing = 3,
    Flask = 4, -- attached to clothing
    WrappedAround = 5, -- e.g. bandage
    StuckIn = 6,
    InMouth = 7, -- string descr like Worn
    Pet = 8, -- Left shoulder, right shoulder, or head, selected randomly using pet_seed
    SewnInto = 9,
    Strapped = 10,
}
---@class unit_inventory_item
---@field item item
---@field unit_inventory_item_mode unit_inventory_item_mode
---@field body_part_id integer
---@field pet_seed integer RNG seed for Pet mode
---@field wound_id integer -1 unless suture
df.unit_inventory_item = nil

---@class unit_attribute
---@field value integer effective = value - soft_demotion
---@field max_value integer
---@field improve_counter integer counts to PHYS_ATT_RATES improve cost; then value++
---@field unused_counter integer counts to PHYS_ATT_RATES unused rate; then rust_counter++
---@field soft_demotion integer 0-100; when not 0 blocks improve_counter
---@field rust_counter integer counts to PHYS_ATT_RATES rust; then demotion_counter++
---@field demotion_counter integer counts to PHYS_ATT_RATES demotion; then value--; soft_demotion++
df.unit_attribute = nil

---@class unit_syndrome
---@field type integer
---@field year integer
---@field year_time integer
---@field ticks integer
---@field wounds integer[] refers to unit_wound by id
---@field wound_id integer
---@field symptoms any[]
---@field reinfection_count integer set from unit.reinfection_count[i]++
---@field flags bitfield
---@field unk4 integer[]
df.unit_syndrome = nil

---@enum wound_effect_type
df.wound_effect_type = {
    Bruise = 0,
    Burn = 1,
    Frostbite = 2,
    Burn2 = 3,
    Melting = 4,
    Boiling = 5,
    Freezing = 6,
    Condensation = 7,
    Necrosis = 8,
    Blister = 9,
}

---@alias wound_damage_flags1 bitfield

---@alias wound_damage_flags2 bitfield

---@class unit_wound
---@field id integer
---@field parts any[]
---@field age integer
---@field attacker_unit_id integer
---@field attacker_hist_figure_id integer
---@field flags bitfield
---@field syndrome_id integer
---@field pain integer
---@field nausea integer
---@field dizziness integer
---@field paralysis integer
---@field numbness integer
---@field fever integer
---@field curse wound_curse_info
---@field unk_v42_1 integer since v0.42.01
---@field unk_v42_2 integer since v0.42.01
df.unit_wound = nil

---@class curse_attr_change
---@field phys_att_perc integer[]
---@field phys_att_add integer[]
---@field ment_att_perc integer[]
---@field ment_att_add integer[]
df.curse_attr_change = nil

---@class wound_curse_info_timing
---@field interaction_time integer[]
---@field time_counter integer

---@class wound_curse_info
---@field unk_v40_1 integer since v0.40.01
---@field add_tags1 bitfield
---@field rem_tags1 bitfield
---@field add_tags2 bitfield
---@field rem_tags2 bitfield
---@field name_visible boolean
---@field name string
---@field name_plural string
---@field name_adjective string
---@field sym_and_color1 integer
---@field sym_and_color2 integer
---@field flash_period integer
---@field flash_time2 integer
---@field speed_add integer
---@field speed_mul_percent integer
---@field attr_change curse_attr_change
---@field unk_v42_1 integer since v0.42.01
---@field luck_mul_percent integer
---@field unk_v42_2 integer since v0.42.01
---@field interaction_id integer[]
---@field timing wound_curse_info_timing
---@field were_race integer
---@field were_caste integer
---@field body_appearance integer[]
---@field bp_appearance integer[]
df.wound_curse_info = nil

---@enum misc_trait_type
df.misc_trait_type = {
    RequestWaterCooldown = 0,
    RequestFoodCooldown = 1,
    RequestRescueCooldown = 2,
    RequestHealthcareCooldown = 3,
    GetDrinkCooldown = 4, -- auto-decrement
    GetFoodCooldown = 5, -- auto-decrement
    CleanSelfCooldown = 6, -- auto-decrement
    Migrant = 7, -- auto-decrement
    RoomComplaint = 8, -- auto-decrement
    UnnamedResident = 9, -- upon reaching zero, resident creature gets named
    RentBedroomCooldown = 10, -- auto-decrement
    ClaimTrinketCooldown = 11, -- auto-decrement
    ClaimClothingCooldown = 12, -- auto-decrement
    WantsDrink = 13, -- auto-increment to 403200
    unk_15 = 14,
    PrepareToDie = 15, -- auto-decrement
    CaveAdapt = 16,
    unk_18 = 17, -- auto-decrement
    unk_19 = 18, -- auto-decrement
    unk_20 = 19,
    unk_21 = 20, -- auto-decrement
    FollowUnitCooldown = 21, -- 0-20, 200 on failed path, auto-decrement
    unk_23 = 22, -- auto-decrement
    unk_24 = 23, -- auto-decrement
    unk_25 = 24,
    DangerousTerrainCooldown = 25, -- created at 200, blocks repath?, auto-decrement
    Beaching = 26, -- triggered by BEACH_FREQUENCY, auto-decrement
    IdleAreaCooldown = 27, -- auto-decrement
    unk_29 = 28, -- auto-decrement
    DiagnosePatientCooldown = 29, -- 0-2000, auto-decrement
    DressWoundCooldown = 30, -- auto-decrement
    CleanPatientCooldown = 31, -- auto-decrement
    SurgeryCooldown = 32, -- auto-decrement
    SutureCooldown = 33, -- auto-decrement
    SetBoneCooldown = 34, -- auto-decrement
    PlaceInTractionCooldown = 35, -- auto-decrement
    ApplyCastCooldown = 36, -- auto-decrement
    ImmobilizeBreakCooldown = 37, -- auto-decrement
    BringCrutchCooldown = 38, -- auto-decrement
    unk_40 = 39, -- auto-decrement, set military pickup flag upon reaching zero
    MilkCounter = 40, -- auto-decrement
    HadDrill = 41, -- auto-decrement
    CompletedDrill = 42, -- auto-decrement
    EggSpent = 43, -- auto-decrement
    GroundedAnimalAnger = 44, -- auto-decrement
    unk_46 = 45, -- auto-decrement
    TimeSinceSuckedBlood = 46,
    DrinkingBlood = 47, -- auto-decrement
    unk_49 = 48, -- auto-decrement
    unk_50 = 49, -- auto-decrement
    RevertWildTimer = 50, -- one trigger => --training_level, auto-decrement
    unk_52 = 51, -- auto-decrement
    NoPantsAnger = 52, -- auto-decrement
    NoShirtAnger = 53, -- auto-decrement
    NoShoesAnger = 54, -- auto-decrement
    unk_56 = 55, -- auto-decrement
    unk_57 = 56, -- auto-decrement
    unk_58 = 57,
    unk_59 = 58, -- auto-decrement
    unk_60 = 59, -- auto-decrement
    unk_61 = 60, -- auto-decrement
    unk_62 = 61, -- auto-decrement
    unk_63 = 62, -- auto-decrement
    unk_64 = 63, -- auto-decrement
    CitizenshipCooldown = 64, -- starts at 1 year, unit will not re-request citizenship during this time, auto-decrement
    unk_66 = 65, -- auto-decrement
    unk_67 = 66, -- auto-decrement
    unk_68 = 67, -- auto-decrement
    unk_69 = 68, -- related to (job_type)0xf1
}

---@class unit_misc_trait
---@field id misc_trait_type
---@field value integer
df.unit_misc_trait = nil

---@class unit_item_wrestle
---@field unit integer
---@field self_bp integer
---@field other_bp integer
---@field unk_c integer
---@field unk_10 integer
---@field item1 integer
---@field item2 integer
---@field unk_1c integer
---@field unk_1e integer 1 grabs, -1 grabbed
---@field unk_20 integer
df.unit_item_wrestle = nil

---@class unit_item_use
---@field id integer
---@field time_in_use integer
---@field has_grown_attached integer
---@field affection_level integer min 50 for attached, 1000 for name
df.unit_item_use = nil

---@alias unit_health_flags bitfield

---@alias unit_bp_health_flags bitfield

---@class unit_health_info
---@field unit_id integer
---@field flags bitfield
---@field body_part_flags unit_bp_health_flags[]
---@field unk_18_cntdn integer
---@field immobilize_cntdn integer
---@field dressing_cntdn integer
---@field suture_cntdn integer
---@field crutch_cntdn integer
---@field op_history any[]
---@field unk_34 any[]
df.unit_health_info = nil

---@alias orientation_flags bitfield

---@class unit_soul
---@field id integer
---@field name language_name
---@field race integer
---@field sex pronoun_type
---@field caste integer
---@field orientation_flags bitfield
---@field unk2 integer
---@field unk3 integer
---@field unk4 integer
---@field unk_1 integer since v0.34.01
---@field unk_2 integer since v0.34.01
---@field unk_3 integer since v0.34.01
---@field unk_4 integer since v0.34.01
---@field unk_5 integer since v0.40.01
---@field mental_attrs unit_attribute[]
---@field skills unit_skill[]
---@field preferences unit_preference[]
---@field personality unit_personality
---@field performance_skills integer since v0.42.01
df.unit_soul = nil

---@class unit_instrument_skill
---@field id integer
---@field rating skill_rating
---@field experience integer
df.unit_instrument_skill = nil

---@class unit_poetic_skill
---@field id integer
---@field rating skill_rating
---@field experience integer
df.unit_poetic_skill = nil

---@class unit_musical_skill
---@field id integer
---@field rating skill_rating
---@field experience integer
df.unit_musical_skill = nil

---@class unit_dance_skill
---@field id integer
---@field rating skill_rating
---@field experience integer
df.unit_dance_skill = nil

---@class unit_emotion_memory
---@field type emotion_type
---@field unk2 integer
---@field strength integer
---@field thought unit_thought_type
---@field subthought integer for certain thoughts
---@field severity integer
---@field unk_1 integer
---@field year integer
---@field year_tick integer
---@field unk_v50_1 integer since v0.50.01
---@field unk_v50_2 integer since v0.50.01
df.unit_emotion_memory = nil

---@class unit_personality
---@field values any[] since v0.40.01
---@field ethics any[] since v0.40.01
---@field emotions any[] since v0.40.01
---@field dreams any[] since v0.40.01
---@field next_dream_id integer since v0.40.01
---@field unk_v40_6 any[] since v0.40.01
---@field traits integer[]
---@field civ_id integer since v0.40.01
---@field cultural_identity integer since v0.40.01
---@field mannerism any[]
---@field habit integer[]
---@field stress integer
---@field time_without_distress integer range 0-806400, higher values cause stress to decrease quicker, since v0.40.14
---@field time_without_eustress integer range 0-806400, higher values cause stress to increase quicker, since v0.40.14
---@field likes_outdoors integer migrated from misc_traits
---@field combat_hardened integer migrated from misc_traits
---@field outdoor_dislike_counter integer incremented when unit is in rain, since v0.47.05
---@field needs any[] since v0.42.01
---@field flags bitfield since v0.42.01
---@field temporary_trait_changes integer sum of inebriation or so personality changing effects
---@field slack_end_year integer since v0.42.01
---@field slack_end_year_tick integer since v0.42.01
---@field memories integer
---@field temptation_greed integer 0-100, for corruption, since v0.47.01
---@field temptation_lust integer since v0.47.01
---@field temptation_power integer since v0.47.01
---@field temptation_anger integer since v0.47.01
---@field longterm_stress integer since v0.50.01
---@field current_focus integer weighted sum of needs focus_level-s
---@field undistracted_focus integer usually number of needs multiplied by 4
---@field unnamed_unit_personality_31 integer
---@field unnamed_unit_personality_32 integer[]
---@field unnamed_unit_personality_33 integer
---@field unnamed_unit_personality_34 integer
---@field unnamed_unit_personality_35 integer
---@field unnamed_unit_personality_36 integer
df.unit_personality = nil

---@enum unit_action_type_group for the action timer API, not in DF
df.unit_action_type_group = {
    All = 0,
    Movement = 1,
    MovementFeet = 2,
    Combat = 3,
    Work = 4,
}

---@enum unit_action_type
df.unit_action_type = {
    None = -1,
    Move = 0,
    Attack = 1,
    Jump = 2,
    HoldTerrain = 3,
    ReleaseTerrain = 4,
    Climb = 5,
    Job = 6,
    Talk = 7,
    Unsteady = 8,
    Parry = 9,
    Block = 10,
    Dodge = 11,
    Recover = 12,
    StandUp = 13,
    LieDown = 14,
    Job2 = 15,
    PushObject = 16,
    SuckBlood = 17,
    HoldItem = 18,
    ReleaseItem = 19,
    Unk20 = 20,
    Unk21 = 21,
    Unk22 = 22,
    Unk23 = 23,
}

---@class unit_action
---@field type unit_action_type
---@field id integer
---@field data raw_data | move | attack | jump | holdterrain | releaseterrain | climb | job | talk | unsteady | parry | block | dodge | recover | standup | liedown | job2 | pushobject | suckblood | holditem | releaseitem | unk20 | unk21 | unk22 | unk23
df.unit_action = nil

---@class unit_action_data_move
---@field x integer
---@field y integer
---@field z integer
---@field timer integer
---@field timer_init integer
---@field fatigue integer
---@field flags bitfield
df.unit_action_data_move = nil

---@enum unit_action_data_attack_unk_4_wrestle_type
df.unit_action_data_attack_unk_4_wrestle_type = {
    Wrestle = 0,
    Grab = 1,
}
---@class unit_action_data_attack_unk_4
---@field unit_action_data_attack_unk_4_wrestle_type unit_action_data_attack_unk_4_wrestle_type
---@field unk_4 integer
---@field unk_6 integer
---@field unk_8 integer
---@field unk_c integer
---@field unk_10 integer
---@field unk_14 integer

---@class unit_action_data_attack
---@field target_unit_id integer
---@field unk_4 unit_action_data_attack_unk_4
---@field attack_item_id integer
---@field target_body_part_id integer
---@field attack_body_part_id integer
---@field attack_id integer refers to weapon_attack or caste_attack
---@field unk_28 integer
---@field unk_2c integer
---@field attack_velocity integer
---@field flags bitfield
---@field attack_skill job_skill
---@field attack_accuracy integer
---@field timer1 integer prepare
---@field timer2 integer recover
df.unit_action_data_attack = nil

---@class unit_action_data_jump
---@field x1 integer
---@field y1 integer
---@field z1 integer
---@field x2 integer
---@field y2 integer
---@field z2 integer
df.unit_action_data_jump = nil

---@class unit_action_data_hold_terrain
---@field x1 integer
---@field y1 integer
---@field z1 integer
---@field x2 integer
---@field y2 integer
---@field z2 integer
---@field x3 integer
---@field y3 integer
---@field z3 integer
---@field timer integer
---@field fatigue integer
df.unit_action_data_hold_terrain = nil

---@class unit_action_data_release_terrain
---@field x integer
---@field y integer
---@field z integer
df.unit_action_data_release_terrain = nil

---@class unit_action_data_climb
---@field x1 integer
---@field y1 integer
---@field z1 integer
---@field x2 integer
---@field y2 integer
---@field z2 integer
---@field x3 integer
---@field y3 integer
---@field z3 integer
---@field timer integer
---@field timer_init integer
---@field fatigue integer
df.unit_action_data_climb = nil

---@class unit_action_data_job
---@field x integer
---@field y integer
---@field z integer
---@field timer integer
df.unit_action_data_job = nil

---@class unit_action_data_talk
---@field unk_0 integer
---@field activity_id integer
---@field activity_event_idx integer
---@field event entity_event
---@field unk_34 integer
---@field timer integer
---@field unk_3c integer
---@field unk_40 integer
---@field unk_44 integer
---@field unk_48 integer
---@field unk_4c integer
---@field unk_50 integer
---@field unk_54 integer
df.unit_action_data_talk = nil

---@class unit_action_data_unsteady
---@field timer integer
df.unit_action_data_unsteady = nil

---@class unit_action_data_parry
---@field unit_id integer
---@field target_action integer
---@field parry_item_id integer
df.unit_action_data_parry = nil

---@class unit_action_data_block
---@field unit_id integer
---@field target_action integer
---@field block_item_id integer
df.unit_action_data_block = nil

---@class unit_action_data_dodge
---@field x1 integer
---@field y1 integer
---@field z1 integer
---@field timer integer
---@field x2 integer
---@field y2 integer
---@field z2 integer
df.unit_action_data_dodge = nil

---@class unit_action_data_recover
---@field timer integer
---@field unk_4 integer
df.unit_action_data_recover = nil

---@class unit_action_data_stand_up
---@field timer integer
df.unit_action_data_stand_up = nil

---@class unit_action_data_lie_down
---@field timer integer
df.unit_action_data_lie_down = nil

---@class unit_action_data_job2
---@field timer integer
df.unit_action_data_job2 = nil

---@class unit_action_data_push_object
---@field x1 integer
---@field y1 integer
---@field z1 integer
---@field x2 integer
---@field y2 integer
---@field z2 integer
---@field x3 integer
---@field y3 integer
---@field z3 integer
---@field timer integer
---@field unk_18 integer
df.unit_action_data_push_object = nil

---@class unit_action_data_suck_blood
---@field unit_id integer
---@field timer integer
df.unit_action_data_suck_blood = nil

---@class unit_action_data_hold_item
---@field x1 integer
---@field y1 integer
---@field z1 integer
---@field x2 integer
---@field y2 integer
---@field z2 integer
---@field unk_c integer
---@field unk_10 integer
---@field unk_14 integer
df.unit_action_data_hold_item = nil

---@class unit_action_data_release_item
---@field unk_0 integer
df.unit_action_data_release_item = nil

---@class unit_action_data_unk_sub_20
---@field unk_0 integer[]
---@field unk_1 integer[]
df.unit_action_data_unk_sub_20 = nil

---@class unit_action_data_unk_sub_21
---@field unk_0 integer[]
---@field unk_1 integer[]
df.unit_action_data_unk_sub_21 = nil

---@class unit_action_data_unk_sub_22
---@field unk_0 integer
df.unit_action_data_unk_sub_22 = nil

---@class unit_action_data_unk_sub_23
---@field unk_0 integer
df.unit_action_data_unk_sub_23 = nil

---@class unit_skill
---@field id job_skill
---@field rating skill_rating
---@field experience integer
---@field unused_counter integer
---@field rusty integer
---@field rust_counter integer
---@field demotion_counter integer
---@field natural_skill_lvl integer This is the NATURAL_SKILL level for the caste in the raws. This skill cannot rust below this level.
df.unit_skill = nil

---@enum unit_preference_type
df.unit_preference_type = {
    LikeMaterial = 0,
    LikeCreature = 1,
    LikeFood = 2,
    HateCreature = 3,
    LikeItem = 4,
    LikePlant = 5,
    LikeTree = 6,
    LikeColor = 7,
    LikeShape = 8,
    LikePoeticForm = 9,
    LikeMusicalForm = 10,
    LikeDanceForm = 11,
}
---@class unit_preference
---@field unit_preference_type unit_preference_type
---@field unnamed_unit_preference_3 item_type | creature_id | color_id | shape_id | plant_id | poetic_form_id | musical_form_id | dance_form_id
---@field item_subtype integer
---@field mattype integer
---@field matindex integer
---@field mat_state matter_state
---@field active boolean
---@field prefstring_seed integer feeds into a simple RNG to choose which prefstring to use
df.unit_preference = nil

---@class unit_complaint
---@field type history_event_reason
---@field age integer
df.unit_complaint = nil

---@class unit_parley
---@field invasion integer
---@field speaker integer
---@field artifact integer
---@field flags bitfield
df.unit_parley = nil

---@enum unit_request_type
df.unit_request_type = {
    DoGuildJobs = 0,
}
---@class unit_request
---@field unit_request_type unit_request_type
---@field source integer
---@field count integer
df.unit_request = nil

---@class unit_coin_debt
---@field recipient integer
---@field amount integer
df.unit_coin_debt = nil

---@class unit_chunk
---@field id integer unit_*.dat
---@field units any[]
df.unit_chunk = nil

---@class unit_appearance physical_formst
---@field unk_1 integer
---@field caste_index integer also refers to $global.world.raws.creatures.list_caste[$]
---@field unk_3 integer
---@field physical_attributes unit_attribute[]
---@field unk_5 integer
---@field body_modifiers integer[]
---@field bp_modifiers integer[]
---@field unk_8 integer
---@field tissue_style integer[]
---@field tissue_style_civ_id integer[]
---@field tissue_style_id integer[]
---@field tissue_style_type integer[]
---@field tissue_length integer[]
---@field appearance_genes integer
---@field color_genes integer
---@field color_modifiers integer[]
---@field unk_18 integer
---@field unk_19 integer
df.unit_appearance = nil

---@class work_detail
---@field name string
---@field work_detail_flags bitfield
---@field unnamed_work_detail_3 integer always 0 when loading a save, complete nonsense in a save?
---@field assigned_units integer[]
---@field allowed_labors boolean[]
---@field unnamed_work_detail_6 integer similarly problematic
---@field icon integer
df.work_detail = nil

---@class process_unit_aux
---@field unit unit
---@field unnamed_process_unit_aux_2 integer
---@field flags bitfield
---@field unnamed_process_unit_aux_4 integer
---@field unnamed_process_unit_aux_5 integer
---@field unnamed_process_unit_aux_6 integer
---@field unitlist any[]
---@field unnamed_process_unit_aux_8 integer
---@field unnamed_process_unit_aux_9 integer
---@field unnamed_process_unit_aux_10 integer
---@field unnamed_process_unit_aux_11 integer
---@field unnamed_process_unit_aux_12 integer
---@field unnamed_process_unit_aux_13 integer
---@field unnamed_process_unit_aux_14 integer
---@field unnamed_process_unit_aux_15 integer
df.process_unit_aux = nil


