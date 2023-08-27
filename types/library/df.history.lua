-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.

---@meta

---@class historical_kills
---@field events integer[]
---@field killed_race integer[]
---@field killed_caste integer[]
---@field killed_underground_region integer[]
---@field killed_region integer[]
---@field killed_site integer[]
---@field killed_undead any[]
---@field killed_count integer[]
df.historical_kills = nil

---@class history_hit_item
---@field item integer
---@field item_type item_type
---@field item_subtype integer
---@field mattype integer
---@field matindex integer
---@field shooter_item integer
---@field shooter_item_type item_type
---@field shooter_item_subtype integer
---@field shooter_mattype integer
---@field shooter_matindex integer
df.history_hit_item = nil

---@enum reputation_type
df.reputation_type = {
    Hero = 0,
    AnimalPartner = 1,
    Brawler = 2,
    Psycho = 3,
    TradePartner = 4,
    Friendly = 5,
    Killer = 6,
    Murderer = 7,
    Comrade = 8,
    RespectedGroup = 9,
    HatedGroup = 10,
    EnemyFighter = 11,
    FriendlyFighter = 12,
    Bully = 13,
    Brigand = 14,
    LoyalSoldier = 15,
    Monster = 16,
    Storyteller = 17,
    Poet = 18,
    Bard = 19,
    Dancer = 20,
    Quarreler = 21,
    Flatterer = 22,
    Hunter = 23,
    ProtectorOfWeak = 24,
    TreasureHunter = 25,
    Thief = 26,
    InformationSource = 27,
    PreserverOfKnowledge = 28,
    Intruder = 29,
    Preacher = 30,
}

---@enum whereabouts_type
df.whereabouts_type = {
    NONE = -1,
    wanderer = 0, -- wandering the wilds/region/depths of the world (none/region/underground_region)
    settler = 1, -- site/region, region only for dead 'monsters'
    refugee = 2, -- into region only for dead. refugees and abucted-imprisoned-turned
    army_died = 3, -- either no record of participation in battle, or character died and defender won (character on either side)
    army_survived = 4, -- either no record of participation in battle, or character survived and defender won (character on either side)
    visitor = 5, -- 'visited' as last movement seems to be the key
}

---@enum season
df.season = {
    None = -1,
    Spring = 0,
    Summer = 1,
    Autumn = 2,
    Winter = 3,
}

---@enum death_condition_type
df.death_condition_type = {
    no_statement = 0, -- alive or dead, but death conditions not registered here
    site_battle = 1, -- parameters: site id + optional structure
    region_battle = 2, -- parameters: two unknown values, usually small, with same pair being the same region, but neither is region_id
    wilderness = 3, -- parameters: region_id + -1
    unk_4 = 4,
    entombed = 5, -- same parameters as for site_battle, but structure seems to always be present and be a tomb
    site = 6, -- same parameters as for site_battle, old age and deadly confrontation seen
}

---@enum plot_role_type
df.plot_role_type = {
    None = -1,
    Possible_Threat = 0,
    Rebuffed = 1,
    Source_Of_Funds = 2,
    Source_Of_Funds_For_Master = 3,
    Master = 4,
    Suspected_Criminal = 5,
    Asset = 6,
    Lieutenant = 7,
    Usable_Thief = 8,
    Potential_Employer = 9,
    Indirect_Director = 10, -- Seen as 'possibly unknown director' in actor's exported XML when influencing the plotter using an intermediary in Legends Mode
    Corrupt_Position_Holder = 11,
    Delivery_Target = 12,
    Handler = 13,
    Usable_Assassin = 14,
    Director = 15, -- Seen with no role or 'potential employer' in actor's exported XML, and as the one influencing the plotter in Legends Mode
    Enemy = 16,
    Usable_Snatcher = 17,
    unk_19 = 18,
    unk_20 = 19,
    Underworld_Contact = 20,
    Possibly_Unknown_Director = 21,
}

---@enum plot_strategy_type
df.plot_strategy_type = {
    None = -1,
    Corrupt_And_Pacify = 0,
    Obey = 1,
    Avoid = 2,
    Use = 3,
    Tax = 4,
    Neutralize = 5,
    Monitor = 6,
    Work_If_Suited = 7,
    Torment = 8,
}

---@class plot_agreement
---@field actor_id integer
---@field plot_role plot_role_type
---@field agreement_id integer
---@field agreement_has_messenger boolean
df.plot_agreement = nil

---@class historical_figure_info
---@field spheres integer
---@field skills integer
---@field pets integer
---@field personality integer
---@field masterpieces integer
---@field whereabouts integer
---@field kills historical_kills
---@field wounds integer
---@field known_info integer since v0.34.01
---@field curse integer since v0.34.01
---@field books integer seems to be misnamed. Artifacts seen have been of all kinds, since v0.34.01
---@field reputation integer since v0.34.01
---@field relationships historical_figure_relationships
df.historical_figure_info = nil

---@class historical_figure_relationships
---@field hf_visual any[]
---@field hf_historical any[] since v0.44.01
---@field unk_1 any[] since v0.44.01
---@field identities integer[]
---@field artifact_claims any[] since v0.44.01
---@field unk_2 integer
---@field intrigues integer
df.historical_figure_relationships = nil

---@enum histfig_flags
df.histfig_flags = {
    reveal_artwork = 0,
    equipment_created = 1,
    deity = 2,
    force = 3,
    skeletal_deity = 4,
    rotting_deity = 5,
    worldgen_acted = 6,
    ghost = 7,
    skin_destroyed = 8,
    meat_destroyed = 9,
    bones_destroyed = 10,
    brag_on_kill = 11,
    kill_quest = 12,
    chatworthy = 13,
    flashes = 14,
    never_cull = 15,
}

---@enum histfig_relationship_type
df.histfig_relationship_type = {
    None = -1,
    Mother = 0,
    Father = 1,
    Parent = 2,
    Husband = 3,
    Wife = 4,
    Spouse = 5,
    SonEldest = 6,
    SonEldest2 = 7,
    SonEldest3 = 8,
    SonEldest4 = 9,
    SonEldest5 = 10,
    SonEldest6 = 11,
    SonEldest7 = 12,
    SonEldest8 = 13,
    SonEldest9 = 14,
    SonEldest10 = 15,
    Son = 16,
    SonYoungest = 17,
    SonOnly = 18,
    DaughterEldest = 19,
    DaughterEldest2 = 20,
    DaughterEldest3 = 21,
    DaughterEldest4 = 22,
    DaughterEldest5 = 23,
    DaughterEldest6 = 24,
    DaughterEldest7 = 25,
    DaughterEldest8 = 26,
    DaughterEldest9 = 27,
    DaughterEldest10 = 28,
    Daughter = 29,
    DaughterOnly = 30,
    DaughterYoungest = 31,
    ChildEldest = 32,
    ChildEldest2 = 33,
    ChildEldest3 = 34,
    ChildEldest4 = 35,
    ChildEldest5 = 36,
    ChildEldest6 = 37,
    ChildEldest7 = 38,
    ChildEldest8 = 39,
    ChildEldest9 = 40,
    ChildEldest10 = 41,
    Child = 42,
    ChildYoungest = 43,
    ChildOnly = 44,
    PaternalGrandmother = 45,
    PaternalGrandfather = 46,
    MaternalGrandmother = 47,
    MaternalGrandfather = 48,
    Grandmother = 49,
    Grandfather = 50,
    Grandparent = 51,
    OlderBrother = 52,
    OlderSister = 53,
    OlderSibling = 54,
    YoungerBrother = 55,
    YoungerSister = 56,
    YoungerSibling = 57,
    Cousin = 58,
    Aunt = 59,
    Uncle = 60,
    Niece = 61,
    Nephew = 62,
    Sibling = 63,
    Grandchild = 64,
    OlderHalfBrother = 65, -- 'since' valid for this entry and those below, since v0.47.01
    OlderHalfSister = 66,
    OlderHalfSibling = 67,
    YoungerHalfBrother = 68,
    YoungerHalfSister = 69,
    YoungerHalfSibling = 70,
    HalfSibling = 71,
}

---@enum vague_relationship_type not a great name given that lovers, ex lovers, and lieutenants appear here, but histfig and unit are both used...
df.vague_relationship_type = {
    none = -1,
    childhood_friend = 0,
    war_buddy = 1,
    jealous_obsession = 2,
    jealous_relationship_grudge = 3,
    lover = 4,
    former_lover = 5, -- broke up
    scholar_buddy = 6,
    artistic_buddy = 7,
    athlete_buddy = 8,
    atheletic_rival = 9,
    business_rival = 10,
    religious_persecution_grudge = 11,
    grudge = 12,
    persecution_grudge = 13,
    supernatural_grudge = 14,
    lieutenant = 15,
    worshipped_deity = 16,
    spouse = 17,
    mother = 18,
    father = 19,
    master = 20,
    apprentice = 21,
    companion = 22,
    ex_spouse = 23,
    neighbor = 24,
    shared_entity = 25, -- Religion/PerformanceTroupe/MerchantCompany/Guild
}

---@class historical_figure
---@field profession profession
---@field race integer
---@field caste integer
---@field sex pronoun_type
---@field orientation_flags bitfield
---@field appeared_year integer
---@field born_year integer
---@field born_seconds integer
---@field curse_year integer since v0.34.01
---@field curse_seconds integer since v0.34.01
---@field birth_year_bias integer since v0.34.01
---@field birth_time_bias integer since v0.34.01
---@field old_year integer
---@field old_seconds integer
---@field died_year integer
---@field died_seconds integer
---@field name language_name
---@field civ_id integer
---@field population_id integer
---@field breed_id integer from legends export
---@field cultural_identity integer since v0.40.01
---@field family_head_id integer When a unit is asked about their family in adventure mode, the historical figure corresponding to this ID is called the head of the family or ancestor., since v0.44.01
---@field flags histfig_flags[]
---@field unit_id integer
---@field nemesis_id integer sometimes garbage, since v0.40.01
---@field id integer
---@field unk4 integer
---@field entity_links histfig_entity_link[]
---@field site_links histfig_site_link[]
---@field histfig_links histfig_hf_link[]
---@field info historical_figure_info
---@field vague_relationships integer Do not have to be available mutually, i.e. DF can display Legends relations forming for the other party that does not have an entry (plus time and other conditions not located)
---@field unk_f0 world_site
---@field unk_f4 world_region
---@field unk_f8 world_underground_region
---@field unk_fc integer
---@field unk_v47_2 integer
---@field unk_v47_3 integer
---@field unk_v47_4 integer
---@field unk_v4019_1 integer since v0.40.17-19
---@field unk_5 integer
df.historical_figure = nil

---@enum identity_type
df.identity_type = {
    None = -1, -- Seen on adventurer assuming an identity for reasons unknown
    HidingCurse = 0, -- Inferred from Units.cpp after examining code using 'unk_4c'
    Impersonating = 1, -- Seen where primeval creatures impersonate 'real' gods in modded game
    TrueName = 2, -- E.g. of demonic overlords. Can be used by adventurers to gain sway over them
    FalseIdentity = 3, -- For underhanded purposes
    InfiltrationIdentity = 4, -- A guess. The cases seen all had the HFs fool the same entity that they were members of it, but no actual purpose was seen
    Identity = 5, -- Claim a new official identity, seen when religious appointments are received
}

---@class identity
---@field id integer
---@field name language_name Not used when Impersonating
---@field race integer
---@field caste integer
---@field impersonated_hf integer only when Impersonating
---@field unnamed_identity_7 histfig_id | nemesis_id
---@field type identity_type
---@field birth_year integer the fake one, that is
---@field birth_second integer
---@field unk_2 integer
---@field unk_3 integer
---@field unk_v47_1 integer
---@field unk_v47_2 integer
---@field profession profession
---@field entity_id integer
---@field unk_4 identity_unk_94[]
---@field unk_5 identity_unk_95[]
df.identity = nil

---@class identity_unk_94
---@field unk_0 integer
---@field unk_1 integer[]
---@field unk_2 integer[]
---@field unk_3 integer
---@field unk_4 integer
---@field unk_5 integer
---@field unk_6 integer uninitialized
---@field unk_7 integer
---@field unk_8 integer
---@field unk_9 integer uninitialized
df.identity_unk_94 = nil

---@class identity_unk_95
---@field unk_0 integer
---@field unk_1 integer[]
---@field unk_2 integer[]
---@field unk_3 integer
---@field unk_4 integer
---@field unk_5 integer
df.identity_unk_95 = nil

---@enum mental_picture_property_type
df.mental_picture_property_type = {
    DATE = 0,
    ACTION = 1,
    TOOL = 2,
    EMOTION = 3,
    COLOR_PATTERN = 4,
    SHAPE = 5,
    ADJECTIVE = 6,
    POSITION = 7,
    TIME = 8,
}

---@return mental_picture_property_type
function df.mental_picture_propertyst.getType() end

---@param file file_compressorst
function df.mental_picture_propertyst.write_file(file) end

---@param file file_compressorst
---@param loadversion save_version
function df.mental_picture_propertyst.read_file(file, loadversion) end

---@class mental_picture_propertyst
---@field unk_0 integer
df.mental_picture_propertyst = nil

---@class mental_picture_property_datest
-- inherited from mental_picture_propertyst
---@field unk_0 integer
-- end mental_picture_propertyst
---@field unk_1 integer
---@field unk_2 integer
df.mental_picture_property_datest = nil

---@class mental_picture_property_actionst
-- inherited from mental_picture_propertyst
---@field unk_0 integer
-- end mental_picture_propertyst
---@field unk_1 integer
---@field unk_2 integer
---@field unk_3 integer
---@field unk_4 integer
df.mental_picture_property_actionst = nil

---@class mental_picture_property_toolst
-- inherited from mental_picture_propertyst
---@field unk_0 integer
-- end mental_picture_propertyst
---@field unk_1 integer
---@field unk_2 integer
---@field unk_3 integer
df.mental_picture_property_toolst = nil

---@class mental_picture_property_emotionst
-- inherited from mental_picture_propertyst
---@field unk_0 integer
-- end mental_picture_propertyst
---@field unk_1 integer
---@field unk_2 integer
df.mental_picture_property_emotionst = nil

---@class mental_picture_property_color_patternst
-- inherited from mental_picture_propertyst
---@field unk_0 integer
-- end mental_picture_propertyst
---@field unk_1 integer
---@field unk_2 integer
df.mental_picture_property_color_patternst = nil

---@class mental_picture_property_shapest
-- inherited from mental_picture_propertyst
---@field unk_0 integer
-- end mental_picture_propertyst
---@field unk_1 integer
---@field unk_2 integer
df.mental_picture_property_shapest = nil

---@class mental_picture_property_adjectivest
-- inherited from mental_picture_propertyst
---@field unk_0 integer
-- end mental_picture_propertyst
---@field unk_1 integer
---@field unk_2 integer
df.mental_picture_property_adjectivest = nil

---@class mental_picture_property_positionst
-- inherited from mental_picture_propertyst
---@field unk_0 integer
-- end mental_picture_propertyst
---@field unk_1 integer
---@field unk_2 integer
---@field unk_3 integer
df.mental_picture_property_positionst = nil

---@class mental_picture_property_timest
-- inherited from mental_picture_propertyst
---@field unk_0 integer
-- end mental_picture_propertyst
---@field unk_1 integer
---@field unk_2 integer
---@field unk_3 integer
df.mental_picture_property_timest = nil

---@enum mental_picture_element_type
df.mental_picture_element_type = {
    HF = 0,
    SITE = 1,
    REGION = 2,
}

---@return mental_picture_element_type
function df.mental_picture_elementst.getType() end

---@param file file_compressorst
function df.mental_picture_elementst.write_file(file) end

---@param file file_compressorst
---@param loadversion save_version
function df.mental_picture_elementst.read_file(file, loadversion) end

---@class mental_picture_elementst
---@field unk_1 integer
df.mental_picture_elementst = nil

---@class mental_picture_element_hfst
-- inherited from mental_picture_elementst
---@field unk_1 integer
-- end mental_picture_elementst
---@field unk_1 integer
df.mental_picture_element_hfst = nil

---@class mental_picture_element_sitest
-- inherited from mental_picture_elementst
---@field unk_1 integer
-- end mental_picture_elementst
---@field unk_1 integer
df.mental_picture_element_sitest = nil

---@class mental_picture_element_regionst
-- inherited from mental_picture_elementst
---@field unk_1 integer
-- end mental_picture_elementst
---@field unk_1 integer
df.mental_picture_element_regionst = nil

---@enum history_event_type
df.history_event_type = {
    WAR_ATTACKED_SITE = 0,
    WAR_DESTROYED_SITE = 1,
    CREATED_SITE = 2,
    HIST_FIGURE_DIED = 3,
    ADD_HF_ENTITY_LINK = 4,
    REMOVE_HF_ENTITY_LINK = 5,
    FIRST_CONTACT = 6,
    FIRST_CONTACT_FAILED = 7,
    TOPICAGREEMENT_CONCLUDED = 8,
    TOPICAGREEMENT_REJECTED = 9,
    TOPICAGREEMENT_MADE = 10,
    WAR_PEACE_ACCEPTED = 11,
    WAR_PEACE_REJECTED = 12,
    DIPLOMAT_LOST = 13,
    AGREEMENTS_VOIDED = 14,
    MERCHANT = 15,
    ARTIFACT_HIDDEN = 16,
    ARTIFACT_POSSESSED = 17,
    ARTIFACT_CREATED = 18,
    ARTIFACT_LOST = 19,
    ARTIFACT_FOUND = 20,
    ARTIFACT_RECOVERED = 21,
    ARTIFACT_DROPPED = 22,
    RECLAIM_SITE = 23,
    HF_DESTROYED_SITE = 24,
    SITE_DIED = 25,
    SITE_RETIRED = 26,
    ENTITY_CREATED = 27,
    ENTITY_ACTION = 28,
    ENTITY_INCORPORATED = 29,
    CREATED_BUILDING = 30,
    REPLACED_BUILDING = 31,
    ADD_HF_SITE_LINK = 32,
    REMOVE_HF_SITE_LINK = 33,
    ADD_HF_HF_LINK = 34,
    REMOVE_HF_HF_LINK = 35,
    ENTITY_RAZED_BUILDING = 36,
    MASTERPIECE_CREATED_ARCH_CONSTRUCT = 37,
    MASTERPIECE_CREATED_ITEM = 38,
    MASTERPIECE_CREATED_DYE_ITEM = 39,
    MASTERPIECE_CREATED_ITEM_IMPROVEMENT = 40,
    MASTERPIECE_CREATED_FOOD = 41,
    MASTERPIECE_CREATED_ENGRAVING = 42,
    MASTERPIECE_LOST = 43,
    CHANGE_HF_STATE = 44,
    CHANGE_HF_JOB = 45,
    WAR_FIELD_BATTLE = 46,
    WAR_PLUNDERED_SITE = 47,
    WAR_SITE_NEW_LEADER = 48,
    WAR_SITE_TRIBUTE_FORCED = 49,
    WAR_SITE_TAKEN_OVER = 50,
    BODY_ABUSED = 51,
    HIST_FIGURE_ABDUCTED = 52,
    ITEM_STOLEN = 53,
    HF_RAZED_BUILDING = 54,
    CREATURE_DEVOURED = 55,
    HIST_FIGURE_WOUNDED = 56,
    HIST_FIGURE_SIMPLE_BATTLE_EVENT = 57,
    CREATED_WORLD_CONSTRUCTION = 58,
    HIST_FIGURE_REUNION = 59,
    HIST_FIGURE_REACH_SUMMIT = 60,
    HIST_FIGURE_TRAVEL = 61,
    HIST_FIGURE_NEW_PET = 62,
    ASSUME_IDENTITY = 63,
    CREATE_ENTITY_POSITION = 64,
    CHANGE_CREATURE_TYPE = 65,
    HIST_FIGURE_REVIVED = 66,
    HF_LEARNS_SECRET = 67,
    CHANGE_HF_BODY_STATE = 68,
    HF_ACT_ON_BUILDING = 69,
    HF_DOES_INTERACTION = 70,
    HF_CONFRONTED = 71,
    ENTITY_LAW = 72,
    HF_GAINS_SECRET_GOAL = 73,
    ARTIFACT_STORED = 74,
    AGREEMENT_FORMED = 75,
    SITE_DISPUTE = 76,
    AGREEMENT_CONCLUDED = 77,
    INSURRECTION_STARTED = 78,
    INSURRECTION_ENDED = 79,
    HF_ATTACKED_SITE = 80,
    PERFORMANCE = 81,
    COMPETITION = 82,
    PROCESSION = 83,
    CEREMONY = 84,
    KNOWLEDGE_DISCOVERED = 85,
    ARTIFACT_TRANSFORMED = 86,
    ARTIFACT_DESTROYED = 87,
    HF_RELATIONSHIP_DENIED = 88,
    REGIONPOP_INCORPORATED_INTO_ENTITY = 89,
    POETIC_FORM_CREATED = 90,
    MUSICAL_FORM_CREATED = 91,
    DANCE_FORM_CREATED = 92,
    WRITTEN_CONTENT_COMPOSED = 93,
    CHANGE_HF_MOOD = 94,
    ARTIFACT_CLAIM_FORMED = 95,
    ARTIFACT_GIVEN = 96,
    HF_ACT_ON_ARTIFACT = 97,
    HF_RECRUITED_UNIT_TYPE_FOR_ENTITY = 98,
    HFS_FORMED_REPUTATION_RELATIONSHIP = 99,
    ARTIFACT_COPIED = 100,
    SNEAK_INTO_SITE = 101,
    SPOTTED_LEAVING_SITE = 102,
    ENTITY_SEARCHED_SITE = 103,
    HF_FREED = 104,
    HIST_FIGURE_SIMPLE_ACTION = 105,
    ENTITY_RAMPAGED_IN_SITE = 106,
    ENTITY_FLED_SITE = 107,
    TACTICAL_SITUATION = 108,
    SQUAD_VS_SQUAD = 109,
    SITE_SURRENDERED = 110,
    ENTITY_EXPELS_HF = 111,
    TRADE = 112, -- since v0.47.01
    ADD_ENTITY_SITE_PROFILE_FLAG = 113,
    GAMBLE = 114,
    ADD_HF_ENTITY_HONOR = 115,
    ENTITY_DISSOLVED = 116,
    ENTITY_EQUIPMENT_PURCHASE = 117,
    MODIFIED_BUILDING = 118,
    BUILDING_PROFILE_ACQUIRED = 119,
    HF_PREACH = 120,
    ENTITY_PERSECUTED = 121,
    ENTITY_BREACH_FEATURE_LAYER = 122,
    ENTITY_ALLIANCE_FORMED = 123,
    HF_RANSOMED = 124,
    HF_ENSLAVED = 125,
    SABOTAGE = 126,
    ENTITY_OVERTHROWN = 127,
    HFS_FORMED_INTRIGUE_RELATIONSHIP = 128,
    FAILED_INTRIGUE_CORRUPTION = 129,
    HF_CONVICTED = 130,
    FAILED_FRAME_ATTEMPT = 131,
    HF_INTERROGATED = 132,
}

---@enum history_event_reason Some of these require at least one parameter of varying type. The text is what DF provides without parameter
df.history_event_reason = {
    none = -1,
    insurrection = 0,
    adventure = 1,
    guide = 2,
    rescued = 3,
    sphere_alignment = 4,
    maintain_balance_in_universe = 5,
    highlight_boundaries_between_worlds = 6,
    sow_the_seeds_of_chaos_in_the_world = 7,
    provide_opportunities_for_courage = 8,
    bring_death_to_the_world = 9,
    liked_appearance = 10,
    because_it_was_destined = 11,
    great_fortresses_built_and_tested = 12,
    whim = 13,
    bring_misery_to_the_world = 14,
    bring_murder_to_the_world = 15,
    bring_nightmares_into_reality = 16,
    bring_thralldom_to_the_world = 17,
    bring_torture_to_the_world = 18,
    provide_opportunities_for_acts_of_valor = 19,
    bring_war_to_the_world = 20,
    find_relative = 21,
    offer_condolences = 22,
    be_brought_to_safety = 23,
    help_with_rescue = 24,
    insufficient_work = 25,
    work_request = 26,
    make_weapon = 27,
    vent_at_boss = 28,
    cry_on_boss = 29,
    should_have_reached_goal = 30,
    insufficient_progress_toward_goal = 31,
    going_wrong_direction = 32,
    arrived_at_location = 33,
    entity_no_longer_rules = 34,
    left_site = 35,
    reunited_with_loved_one = 36,
    violent_disagreement = 37,
    adopted = 38,
    true_name_invocation = 39,
    arrived_at_person = 40,
    eradicate_beasts = 41,
    entertain_people = 42,
    make_a_living_as_a_warrior = 43,
    study = 44,
    flight = 45,
    scholarship = 46,
    be_with_master = 47,
    become_citizen = 48,
    prefers_working_alone = 49,
    jealousy = 50,
    glorify_hf = 51,
    have_not_performed = 52,
    prevented_from_leaving = 53,
    curiosity = 54,
    hire_on_as_mercenary = 55,
    hire_on_as_performer = 56,
    hire_on_as_scholar = 57,
    drink = 58,
    admire_architecture = 59,
    pray = 60,
    relax = 61,
    danger = 62,
    cannot_find_artifact = 63,
    failed_mood = 64,
    lack_of_sleep = 65,
    trapped_in_cage = 66,
    great_deal_of_stress = 67,
    unable_to_leave_location = 68,
    sanctify_hf = 69,
    artifact_is_heirloom_of_family_hfid = 70,
    cement_bonds_of_friendship = 71,
    as_a_symbol_of_everlasting_peace = 72,
    on_a_pilgrimage = 73,
    gather_information = 74,
    seek_sanctuary = 75,
    part_of_trade_negotiation = 76,
    artifact_is_symbol_of_entity_position = 77,
    fear_of_persecution = 78, -- The ones below were introduced in 0.47.01 as well, since v0.47.01
    smooth_operation = 79,
    nuance_belief = 80,
    shared_interest = 81,
    envy_living = 82,
    death_panic = 83,
    death_fear = 84,
    avoid_judgement = 85,
    death_pride = 86,
    death_vain = 87,
    death_ambition = 88,
    lack_of_funds = 89,
    battle_losses = 90,
    conviction_exile = 91,
    priest_vent = 92,
    priest_cry = 93,
}

---@class history_event_reason_info
---@field type history_event_reason
---@field data glorify_hf | sanctify_hf | artifact_is_heirloom_of_family_hfid | artifact_is_symbol_of_entity_position
df.history_event_reason_info = nil

---@class history_event_circumstance_info
---@field type unit_thought_type
---@field data Death | Prayer | DreamAbout | Defeated | Murdered | HistEventCollection | AfterAbducting
df.history_event_circumstance_info = nil

---@class history_event_context
---@field flags bitfield
---@field interrogator_relationships historical_figure_relationships
---@field interrogation integer
---@field artifact_id integer
---@field entity_id integer
---@field histfig_id integer
---@field speaker_id integer
---@field site_id integer
---@field region_id integer
---@field layer_id integer
---@field unk_34 integer passed to history_event::isRelatedToAgreementID, but all implementations of that function are broken currently
---@field abstract_building_id integer
---@field sphere sphere_type
---@field architectural_element architectural_element
---@field unk_40 integer
---@field family_relationship histfig_relationship_type not initialized
---@field number integer
---@field unk_48 integer
---@field race integer
---@field unk_4c integer
---@field unk_50 integer
---@field unk_54 integer
---@field caste integer
---@field undead_flags bitfield
---@field unk_5a integer
---@field squad_id integer
---@field formation_id integer ID within world.formations.all
---@field activity_id integer
---@field breed_id integer
---@field battlefield_id integer
---@field interaction_instance_id integer
---@field written_content_id integer
---@field identity_id integer
---@field incident_id integer
---@field crime_id integer
---@field region_weather_id integer
---@field creation_zone_id integer
---@field vehicle_id integer
---@field army_id integer
---@field army_controller_id integer
---@field army_tracking_info_id integer ID within world.army_tracking_info.all
---@field cultural_identity_id integer
---@field agreement_id integer
---@field poetic_form_id integer
---@field musical_form_id integer
---@field dance_form_id integer
---@field scale_id integer
---@field rhythm_id integer
---@field occupation_id integer
---@field belief_system_id integer
---@field image_set_id integer
---@field divination_set_id integer
df.history_event_context = nil

---@enum architectural_element
df.architectural_element = {
    NONE = -1,
    paved_outdoor_area = 0,
    uneven_pillars = 1,
    square_of_pillars = 2,
    pillars_on_the_perimeter = 3,
    upper_floors = 4,
    lower_floors = 5,
    water_pool = 6,
    lava_pool = 7,
    stagnant_pool = 8,
    open_structure = 9,
    paved_indoor_areas = 10,
    detailed_surfaces = 11,
}

---@enum history_event_flags
df.history_event_flags = {
    hidden = 0, -- event is hidden from legends mode when this is set
    unk_1 = 1,
    unk_2 = 2, -- related to intrigues (checked in df::history_event_failed_intrigue_corruptionst::getRelatedHistfigIDs)
}

---@alias merc_role_type bitfield regular if bit not set, since v0.47.02

---@return history_event_type
function df.history_event.getType() end

---@param entity1 integer
---@param entity2 integer
---@return integer
function df.history_event.getWarStatus(entity1, entity2) end

---@param entity1 integer
---@param entity2 integer
---@return integer
function df.history_event.getAngerModifier(entity1, entity2) end

---@param entity1 integer
---@param entity2 integer
---@return integer
function df.history_event.getHappinessModifier(entity1, entity2) end

---@param entity1 integer
---@param entity2 integer
---@param site integer
---@return boolean
function df.history_event.madeFirstContact(entity1, entity2, site) end

---@param killer integer
---@return integer
function df.history_event.getKilledHistfigID(killer) end

---@param victim integer
---@return boolean
function df.history_event.wasHistfigKilled(victim) end

---@param histfig integer
---@return boolean
function df.history_event.wasHistfigRevived(histfig) end

---@return integer
function df.history_event.unnamed_method() end -- returns -1

---@return integer
function df.history_event.unnamed_method() end -- returns -1

---@param vec integer
function df.history_event.getRelatedHistfigIDs(vec) end

---@param vec integer
function df.history_event.getRelatedSiteIDs(vec) end

---@param vec1 integer
---@param vec2 integer
function df.history_event.getRelatedSiteStructureIDs(vec1, vec2) end

---@param vec integer
function df.history_event.getRelatedArtifactIDs(vec) end

---@param vec integer
function df.history_event.getRelatedRegionIDs(vec) end

---@param vec integer
function df.history_event.getRelatedLayerIDs(vec) end

---@param vec integer
function df.history_event.getRelatedEntityIDs(vec) end

---@param histfig integer
---@return boolean
function df.history_event.isRelatedToHistfigID(histfig) end

---@param site integer
---@return boolean
function df.history_event.isRelatedToSiteID(site) end

---@param site integer
---@param structure integer
---@return boolean
function df.history_event.isRelatedToSiteStructure(site, structure) end

---@param artifact integer
---@return boolean
function df.history_event.isRelatedToArtifactID(artifact) end

---@param region integer
---@return boolean
function df.history_event.isRelatedToRegionID(region) end

---@param region integer
---@return boolean
function df.history_event.isRelatedToLayerID(region) end

---@param agreement integer
---@return boolean
function df.history_event.isRelatedToAgreementID(agreement) end -- broken; always returns false

---@param entity integer
---@return boolean
function df.history_event.isRelatedToEntityID(entity) end

---@param arg_0 integer
---@return boolean
function df.history_event.unnamed_method(arg_0) end -- always returns false; seems to be related to breeds at the places where it is called, but is only implemented in events related to agreements, where it is identical to isRelatedToAgreementID

---@param str string
---@param context history_event_context
function df.history_event.getSentence(str, context) end

---@param str string
---@param context history_event_context
function df.history_event.getPhrase(str, context) end

---@param image art_image
function df.history_event.populateArtImage(image) end

---@return integer
function df.history_event.unnamed_method() end -- possibly something related to event importance; return values include 10, 20, 25, 50, 100, 120, 140

---@return integer
function df.history_event.unnamed_method() end

---@param histfig integer
---@return boolean
function df.history_event.isChangedHistfigID(histfig) end

function df.history_event.categorize() end -- inserts event into world_history.events_death if relevant

function df.history_event.uncategorize() end -- removes event from world_history.events_death if relevant

---@param arg_0 stl-fstream
---@param indent integer
function df.history_event.generate_xml(arg_0, indent) end

---@param file file_compressorst
function df.history_event.write_file(file) end

---@param file file_compressorst
---@param loadversion save_version
function df.history_event.read_file(file, loadversion) end

function df.history_event.unnamed_method() end -- records event in historical_entity.unknown1b.diplomacy in both directions if applicable

---@class history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
df.history_event = nil

---@class history_event_war_attacked_sitest
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field attacker_civ integer
---@field defender_civ integer
---@field site_civ integer
---@field site integer
---@field attacker_general_hf integer
---@field defender_general_hf integer
---@field attacker_merc_enid integer since v0.47.02
---@field defender_merc_enid integer since v0.47.02
---@field merc_roles bitfield since v0.47.02
df.history_event_war_attacked_sitest = nil

---@class history_event_war_destroyed_sitest
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field attacker_civ integer
---@field defender_civ integer
---@field site_civ integer
---@field site integer
---@field unk_1 integer
df.history_event_war_destroyed_sitest = nil

---@class history_event_created_sitest
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field civ integer
---@field site_civ integer
---@field resident_civ_id integer since v0.47.01
---@field site integer
---@field builder_hf integer since v0.40.01
df.history_event_created_sitest = nil

---@enum death_type
df.death_type = {
    NONE = -1,
    OLD_AGE = 0,
    HUNGER = 1,
    THIRST = 2,
    SHOT = 3,
    BLEED = 4,
    DROWN = 5,
    SUFFOCATE = 6,
    STRUCK_DOWN = 7,
    SCUTTLE = 8, -- stuck wagons
    COLLISION = 9,
    MAGMA = 10, -- does not happen anymore?
    MAGMA_MIST = 11, -- does not happen anymore?
    DRAGONFIRE = 12,
    FIRE = 13,
    SCALD = 14, -- does not happen anymore?
    CAVEIN = 15,
    DRAWBRIDGE = 16,
    FALLING_ROCKS = 17, -- does not happen anymore?
    CHASM = 18,
    CAGE = 19,
    MURDER = 20,
    TRAP = 21,
    VANISH = 22, -- bogeyman
    QUIT = 23, -- Give in to starvation as adventurer
    ABANDON = 24,
    HEAT = 25,
    COLD = 26,
    SPIKE = 27,
    ENCASE_LAVA = 28,
    ENCASE_MAGMA = 29,
    ENCASE_ICE = 30,
    BEHEAD = 31, -- execution during worldgen
    CRUCIFY = 32, -- execution during worldgen
    BURY_ALIVE = 33, -- execution during worldgen
    DROWN_ALT = 34, -- execution during worldgen
    BURN_ALIVE = 35, -- execution during worldgen
    FEED_TO_BEASTS = 36, -- execution during worldgen
    HACK_TO_PIECES = 37, -- execution during worldgen
    LEAVE_OUT_IN_AIR = 38, -- execution during worldgen
    BOIL = 39, -- material state change
    MELT = 40, -- material state change
    CONDENSE = 41, -- material state change
    SOLIDIFY = 42, -- material state change
    INFECTION = 43,
    MEMORIALIZE = 44, -- put to rest
    SCARE = 45,
    DARKNESS = 46, -- died in the dark
    COLLAPSE = 47, -- used in 0.31 for undead
    DRAIN_BLOOD = 48,
    SLAUGHTER = 49,
    VEHICLE = 50,
    FALLING_OBJECT = 51,
    LEAPT_FROM_HEIGHT = 52,
    DROWN_ALT2 = 53,
    EXECUTION_GENERIC = 54,
}

---@class history_event_hist_figure_diedst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field victim_hf integer
---@field slayer_hf integer
---@field slayer_race integer
---@field slayer_caste integer
---@field weapon history_hit_item
---@field site integer
---@field subregion integer
---@field feature_layer integer
---@field death_cause death_type
df.history_event_hist_figure_diedst = nil

---@class history_event_add_hf_entity_linkst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field civ integer
---@field histfig integer
---@field link_type histfig_entity_link_type
---@field position_id integer index into entity.positions.own
---@field appointer_hfid integer since v0.47.02
---@field promise_to_hfid integer since v0.47.02
df.history_event_add_hf_entity_linkst = nil

---@class history_event_remove_hf_entity_linkst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field civ integer
---@field histfig integer
---@field link_type histfig_entity_link_type
---@field position_id integer index into entity.positions.own
df.history_event_remove_hf_entity_linkst = nil

---@class history_event_entity_expels_hfst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field civ integer
---@field expelled integer
---@field site integer
df.history_event_entity_expels_hfst = nil

---@class history_event_first_contactst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field contactor integer
---@field contacted integer
---@field site integer
df.history_event_first_contactst = nil

---@class history_event_first_contact_failedst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field contactor integer
---@field rejector integer
---@field site integer
df.history_event_first_contact_failedst = nil

---@class history_event_topicagreement_concludedst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field source integer
---@field destination integer
---@field site integer
---@field topic meeting_topic
---@field result integer range from -3 to +2
df.history_event_topicagreement_concludedst = nil

---@class history_event_topicagreement_rejectedst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field topic meeting_topic
---@field source integer
---@field destination integer
---@field site integer
df.history_event_topicagreement_rejectedst = nil

---@class history_event_topicagreement_madest
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field topic meeting_topic
---@field source integer
---@field destination integer
---@field site integer
df.history_event_topicagreement_madest = nil

---@class history_event_war_peace_acceptedst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field topic meeting_topic
---@field source integer
---@field destination integer
---@field site integer
df.history_event_war_peace_acceptedst = nil

---@class history_event_war_peace_rejectedst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field topic meeting_topic
---@field source integer
---@field destination integer
---@field site integer
df.history_event_war_peace_rejectedst = nil

---@class history_event_diplomat_lostst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field entity integer
---@field involved integer
---@field site integer
df.history_event_diplomat_lostst = nil

---@class history_event_agreements_voidedst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field source integer
---@field destination integer
df.history_event_agreements_voidedst = nil

---@class history_event_merchantst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field source integer
---@field destination integer
---@field site integer
---@field flags2 history_event_merchant_flags[]
df.history_event_merchantst = nil

---@enum history_event_merchant_flags
df.history_event_merchant_flags = {
    vanished = 0, -- opposite of communicate in caravan_state
    hardship = 1,
    seized = 2,
    offended = 3,
    missing_goods = 4,
    tribute = 5,
}

---@class history_event_artifact_hiddenst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field artifact integer
---@field unit integer
---@field histfig integer
---@field site integer
df.history_event_artifact_hiddenst = nil

---@class history_event_artifact_possessedst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field artifact integer
---@field unit integer
---@field histfig integer
---@field site integer
---@field subregion_id integer
---@field feature_layer_id integer
---@field circumstance history_event_circumstance_info
---@field reason history_event_reason_info
df.history_event_artifact_possessedst = nil

---@class history_event_artifact_createdst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field artifact_id integer
---@field creator_unit_id integer the unit who created the artifact
---@field creator_hfid integer the figure who created the artifact
---@field site integer
---@field flags2 bitfield
---@field circumstance history_event_circumstance_info
---@field reason history_event_reason_info
df.history_event_artifact_createdst = nil

---@class history_event_artifact_lostst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field artifact integer
---@field site integer
---@field site_property_id integer
---@field subregion_id integer
---@field unk_1 integer probably feature_layer_id, based on other events, but haven't seen non -1, since v0.47.01
df.history_event_artifact_lostst = nil

---@class history_event_artifact_foundst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field artifact integer
---@field unit integer
---@field histfig integer
---@field site integer
---@field site_property_id integer since v0.47.02
---@field unk_1 integer probably subregion_id, based on other events, but haven't seen non -1, since v0.47.02
---@field unk_2 integer probably feature_layer_id, based on other events, but haven't seen non -1, since v0.47.02
df.history_event_artifact_foundst = nil

---@class history_event_artifact_recoveredst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field artifact integer
---@field unit integer
---@field histfig integer
---@field site integer
---@field structure integer
---@field region integer
---@field layer integer
df.history_event_artifact_recoveredst = nil

---@class history_event_artifact_droppedst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field artifact integer
---@field unit integer
---@field histfig integer
---@field site integer
---@field flags2 any[]
df.history_event_artifact_droppedst = nil

---@class history_event_reclaim_sitest
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field civ integer
---@field site_civ integer
---@field site integer
---@field flags2 bitfield
df.history_event_reclaim_sitest = nil

---@class history_event_hf_destroyed_sitest
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field attacker_hf integer
---@field defender_civ integer
---@field site_civ integer
---@field site integer
df.history_event_hf_destroyed_sitest = nil

---@class history_event_site_diedst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field civ integer
---@field site_civ integer
---@field site integer
---@field flags2 bitfield
df.history_event_site_diedst = nil

---@class history_event_site_retiredst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field civ integer
---@field site_civ integer
---@field site integer
---@field flags2 bitfield
df.history_event_site_retiredst = nil

---@class history_event_entity_createdst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field entity integer
---@field site integer
---@field structure integer
---@field creator_hfid integer since v0.47.02
df.history_event_entity_createdst = nil

---@enum entity_action_type
df.entity_action_type = {
    entity_primary_criminals = 0,
    entity_relocate = 1,
}

---@class history_event_entity_actionst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field entity integer
---@field site integer
---@field structure integer
---@field action entity_action_type
df.history_event_entity_actionst = nil

---@class history_event_entity_incorporatedst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field migrant_entity integer
---@field join_entity integer
---@field leader_hfid integer since v0.47.02
---@field site integer
---@field partial boolean since v0.47.02
df.history_event_entity_incorporatedst = nil

---@class history_event_created_buildingst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field civ integer
---@field site_civ integer
---@field site integer
---@field structure integer
---@field builder_hf integer
---@field rebuild boolean since v0.47.02
df.history_event_created_buildingst = nil

---@class history_event_replaced_buildingst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field civ integer
---@field site_civ integer
---@field site integer
---@field old_structure integer
---@field new_structure integer
df.history_event_replaced_buildingst = nil

---@class history_event_add_hf_site_linkst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field site integer
---@field structure integer
---@field histfig integer
---@field civ integer
---@field type histfig_site_link_type
df.history_event_add_hf_site_linkst = nil

---@class history_event_remove_hf_site_linkst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field site integer
---@field structure integer
---@field histfig integer
---@field civ integer
---@field type histfig_site_link_type
df.history_event_remove_hf_site_linkst = nil

---@class history_event_add_hf_hf_linkst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field hf integer
---@field hf_target integer
---@field type histfig_hf_link_type
df.history_event_add_hf_hf_linkst = nil

---@class history_event_remove_hf_hf_linkst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field hf integer
---@field hf_target integer
---@field type histfig_hf_link_type
df.history_event_remove_hf_hf_linkst = nil

---@class history_event_entity_razed_buildingst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field civ integer
---@field site integer
---@field structure integer
df.history_event_entity_razed_buildingst = nil

---@class history_event_masterpiece_createdst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field maker integer
---@field maker_entity integer
---@field site integer
---@field skill_at_time skill_rating
df.history_event_masterpiece_createdst = nil

---@class history_event_masterpiece_created_arch_constructst
-- inherited from history_event_masterpiece_createdst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field maker integer
---@field maker_entity integer
---@field site integer
---@field skill_at_time skill_rating
-- end history_event_masterpiece_createdst
---@field building_type integer
---@field building_subtype integer
---@field building_custom integer
---@field unk_2 integer
df.history_event_masterpiece_created_arch_constructst = nil

---@class history_event_masterpiece_created_itemst
-- inherited from history_event_masterpiece_createdst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field maker integer
---@field maker_entity integer
---@field site integer
---@field skill_at_time skill_rating
-- end history_event_masterpiece_createdst
---@field item_type item_type
---@field item_subtype integer
---@field mat_type integer
---@field mat_index integer
---@field item_id integer
df.history_event_masterpiece_created_itemst = nil

---@class history_event_masterpiece_created_dye_itemst
-- inherited from history_event_masterpiece_createdst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field maker integer
---@field maker_entity integer
---@field site integer
---@field skill_at_time skill_rating
-- end history_event_masterpiece_createdst
---@field item_type item_type
---@field item_subtype integer
---@field mat_type integer
---@field mat_index integer
---@field unk_2 integer
---@field dye_mat_type integer
---@field dye_mat_index integer
df.history_event_masterpiece_created_dye_itemst = nil

---@class history_event_masterpiece_created_item_improvementst
-- inherited from history_event_masterpiece_createdst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field maker integer
---@field maker_entity integer
---@field site integer
---@field skill_at_time skill_rating
-- end history_event_masterpiece_createdst
---@field item_type item_type
---@field item_subtype integer
---@field mat_type integer
---@field mat_index integer
---@field unk_2 integer
---@field improvement_type improvement_type
---@field improvement_subtype integer
---@field imp_mat_type integer
---@field imp_mat_index integer
---@field art_id integer
---@field art_subid integer
df.history_event_masterpiece_created_item_improvementst = nil

---@class history_event_masterpiece_created_foodst
-- inherited from history_event_masterpiece_createdst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field maker integer
---@field maker_entity integer
---@field site integer
---@field skill_at_time skill_rating
-- end history_event_masterpiece_createdst
---@field item_subtype integer
---@field item_id integer
df.history_event_masterpiece_created_foodst = nil

---@class history_event_masterpiece_created_engravingst
-- inherited from history_event_masterpiece_createdst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field maker integer
---@field maker_entity integer
---@field site integer
---@field skill_at_time skill_rating
-- end history_event_masterpiece_createdst
---@field art_id integer
---@field art_subid integer
df.history_event_masterpiece_created_engravingst = nil

---@enum masterpiece_loss_type
df.masterpiece_loss_type = {
    MELT = 0,
    MAGMA = 1,
    FORTIFICATION = 2,
    MINING = 3,
    CAVEIN = 4,
    VEGETATION = 5,
}

---@class history_event_masterpiece_lostst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field creation_event integer
---@field histfig integer
---@field site integer
---@field method masterpiece_loss_type
df.history_event_masterpiece_lostst = nil

---@class history_event_change_hf_statest
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field hfid integer
---@field state whereabouts_type
---@field reason history_event_reason
---@field site integer
---@field region integer
---@field layer integer
---@field region_pos coord2d
df.history_event_change_hf_statest = nil

---@class history_event_change_hf_jobst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field hfid integer
---@field new_job profession
---@field old_job profession
---@field site integer
---@field region integer
---@field layer integer
df.history_event_change_hf_jobst = nil

---@class history_event_war_field_battlest
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field attacker_civ integer
---@field defender_civ integer
---@field region integer
---@field layer integer
---@field region_pos coord2d
---@field attacker_general_hf integer
---@field defender_general_hf integer
---@field attacker_merc_enid integer since v0.47.02
---@field defender_merc_enid integer since v0.47.02
---@field merc_roles bitfield since v0.47.02
df.history_event_war_field_battlest = nil

---@class history_event_war_plundered_sitest
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field attacker_civ integer
---@field defender_civ integer
---@field site_civ integer
---@field site integer
---@field unk_1 integer 2=detected, since v0.47.01
df.history_event_war_plundered_sitest = nil

---@class history_event_war_site_new_leaderst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field attacker_civ integer
---@field new_site_civ integer
---@field defender_civ integer
---@field site_civ integer
---@field site integer
---@field new_leaders integer[]
df.history_event_war_site_new_leaderst = nil

---@class history_event_war_site_tribute_forcedst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field attacker_civ integer
---@field defender_civ integer
---@field site_civ integer
---@field site integer
---@field season season
---@field tribute_flags bitfield
df.history_event_war_site_tribute_forcedst = nil

---@class history_event_war_site_taken_overst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field attacker_civ integer
---@field new_site_civ integer
---@field defender_civ integer
---@field site_civ integer
---@field site integer
df.history_event_war_site_taken_overst = nil

---@class history_event_site_surrenderedst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field attacker_civ integer
---@field defender_civ integer
---@field site_civ integer
---@field site integer
df.history_event_site_surrenderedst = nil

---@enum history_event_body_abusedst_abuse_type
df.history_event_body_abusedst_abuse_type = {
    Impaled = 0,
    Piled = 1,
    Flayed = 2,
    Hung = 3,
    Mutilated = 4,
    Animated = 5,
}
---@class history_event_body_abusedst
-- inherit history_event
---@field bodies integer[]
---@field victim_entity integer
---@field civ integer
---@field histfig integer
---@field site integer
---@field region integer
---@field layer integer
---@field region_pos coord2d
---@field history_event_body_abusedst_abuse_type history_event_body_abusedst_abuse_type
---@field abuse_data Impaled | Piled | Flayed | Hung | Animated
df.history_event_body_abusedst = nil

---@class history_event_hist_figure_abductedst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field target integer
---@field snatcher integer
---@field site integer
---@field region integer
---@field layer integer
df.history_event_hist_figure_abductedst = nil

---@enum theft_method_type
df.theft_method_type = {
    Theft = 0,
    Confiscated = 1,
    Looted = 2,
    Recovered = 3,
}

---@class history_event_item_stolenst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field item_type item_type
---@field item_subtype integer
---@field mattype integer
---@field matindex integer
---@field item integer
---@field entity integer
---@field histfig integer
---@field site integer
---@field structure integer
---@field region integer
---@field layer integer
---@field region_pos coord2d
---@field stash_site integer location to which the thief brought the loot
---@field circumstance history_event_circumstance_info
---@field reason history_event_reason_info
---@field theft_method theft_method_type
df.history_event_item_stolenst = nil

---@class history_event_hf_razed_buildingst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field histfig integer
---@field site integer
---@field structure integer
df.history_event_hf_razed_buildingst = nil

---@class history_event_creature_devouredst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field victim integer
---@field race integer
---@field caste integer
---@field eater integer
---@field entity integer
---@field site integer
---@field region integer
---@field layer integer
df.history_event_creature_devouredst = nil

---@enum history_event_hist_figure_woundedst_injury_type
df.history_event_hist_figure_woundedst_injury_type = {
    Smash = 0,
    Slash = 1,
    Stab = 2,
    Rip = 3,
    Burn = 4,
}
---@class history_event_hist_figure_woundedst
-- inherit history_event
---@field woundee integer
---@field wounder integer
---@field site integer
---@field region integer
---@field layer integer
---@field woundee_race integer
---@field woundee_caste integer
---@field body_part integer
---@field history_event_hist_figure_woundedst_injury_type history_event_hist_figure_woundedst_injury_type
---@field part_lost boolean
---@field flags2 bitfield since v0.47.01
df.history_event_hist_figure_woundedst = nil

---@enum history_event_simple_battle_subtype
df.history_event_simple_battle_subtype = {
    SCUFFLE = 0,
    ATTACK = 1,
    SURPRISE = 2,
    AMBUSH = 3,
    HAPPEN_UPON = 4,
    CORNER = 5,
    CONFRONT = 6,
    LOSE_AFTER_RECEIVE_WOUND = 7,
    LOSE_AFTER_INFLICT_WOUND = 8,
    LOSE_AFTER_EXCHANGE_WOUND = 9,
    SUBDUED = 10,
    GOT_INTO_A_BRAWL = 11,
}

---@enum artifact_claim_type
df.artifact_claim_type = {
    Symbol = 0,
    Heirloom = 1,
    Treasure = 2,
    HolyRelic = 3,
}

---@class history_event_hist_figure_simple_battle_eventst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field group1 integer[]
---@field group2 integer[]
---@field site integer
---@field region integer
---@field layer integer
---@field subtype history_event_simple_battle_subtype
df.history_event_hist_figure_simple_battle_eventst = nil

---@class history_event_created_world_constructionst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field civ integer
---@field site_civ integer
---@field construction integer
---@field master_construction integer
---@field site1 integer
---@field site2 integer
df.history_event_created_world_constructionst = nil

---@class history_event_hist_figure_reunionst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field missing integer[]
---@field reunited_with integer[]
---@field assistant integer
---@field site integer
---@field region integer
---@field layer integer
df.history_event_hist_figure_reunionst = nil

---@class history_event_hist_figure_reach_summitst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field group integer[]
---@field region integer
---@field layer integer
---@field region_pos coord2d
df.history_event_hist_figure_reach_summitst = nil

---@enum history_event_hist_figure_travelst_reason
df.history_event_hist_figure_travelst_reason = {
    Journey = 0, -- made a journey to
    Return = 1, -- returned to
    Escape = 2, -- escaped from
}
---@class history_event_hist_figure_travelst
-- inherit history_event
---@field group integer[]
---@field site integer
---@field region integer
---@field layer integer
---@field history_event_hist_figure_travelst_reason history_event_hist_figure_travelst_reason
---@field region_pos coord2d
df.history_event_hist_figure_travelst = nil

---@class history_event_hist_figure_new_petst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field group integer[]
---@field pets integer[]
---@field site integer
---@field region integer
---@field layer integer
---@field region_pos coord2d
df.history_event_hist_figure_new_petst = nil

---@class history_event_assume_identityst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field trickster integer
---@field identity integer
---@field target integer
df.history_event_assume_identityst = nil

---@enum position_creation_reason_type
df.position_creation_reason_type = {
    force_of_argument = 0,
    threat_of_violence = 1,
    collaboration = 2,
    wave_of_popular_support = 3,
    as_a_matter_of_course = 4,
}

---@class history_event_create_entity_positionst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field histfig integer
---@field civ integer
---@field site_civ integer
---@field position integer
---@field reason position_creation_reason_type
df.history_event_create_entity_positionst = nil

---@class history_event_change_creature_typest
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field changee integer
---@field changer integer
---@field old_race integer
---@field old_caste integer
---@field new_race integer
---@field new_caste integer
df.history_event_change_creature_typest = nil

---@class history_event_hist_figure_revivedst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field histfig integer
---@field site integer
---@field region integer
---@field layer integer
---@field ghost_type ghost_type
---@field flags2 bitfield
---@field actor_hfid integer since v0.47.02
---@field interaction integer since v0.47.02
---@field unk_1 integer since v0.47.02
df.history_event_hist_figure_revivedst = nil

---@class history_event_hf_learns_secretst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field student integer
---@field teacher integer
---@field artifact integer
---@field interaction integer
---@field unk_1 integer
df.history_event_hf_learns_secretst = nil

---@enum histfig_body_state
df.histfig_body_state = {
    Active = 0,
    BuriedAtSite = 1,
    UnburiedAtBattlefield = 2,
    UnburiedAtSubregion = 3,
    UnburiedAtFeatureLayer = 4,
    EntombedAtSite = 5,
    UnburiedAtSite = 6,
}

---@class history_event_change_hf_body_statest
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field histfig integer
---@field body_state histfig_body_state
---@field site integer
---@field structure integer
---@field region integer
---@field layer integer
---@field region_pos coord2d
df.history_event_change_hf_body_statest = nil

---@enum history_event_hf_act_on_buildingst_action
df.history_event_hf_act_on_buildingst_action = {
    Profane = 0,
    Disturb = 1,
    PrayedInside = 2,
}
---@class history_event_hf_act_on_buildingst
-- inherit history_event
---@field history_event_hf_act_on_buildingst_action history_event_hf_act_on_buildingst_action
---@field histfig integer
---@field site integer
---@field structure integer
df.history_event_hf_act_on_buildingst = nil

---@class history_event_hf_does_interactionst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field doer integer
---@field target integer
---@field interaction integer
---@field source integer
---@field site integer
---@field region integer
---@field layer integer
df.history_event_hf_does_interactionst = nil

---@class history_event_hf_confrontedst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field target integer
---@field accuser integer
---@field reasons integer[] 0 = ageless, 1 = murder
---@field site integer
---@field region integer
---@field layer integer
---@field region_pos coord2d
df.history_event_hf_confrontedst = nil

---@class history_event_entity_lawst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field entity integer
---@field histfig integer
---@field add_flags bitfield
---@field remove_flags bitfield
df.history_event_entity_lawst = nil

---@class history_event_hf_gains_secret_goalst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field histfig integer
---@field goal goal_type
---@field thought unit_thought_type since v0.47.02
---@field target_hf integer since v0.47.02
---@field reason history_event_reason since v0.47.02
---@field value value_type since v0.47.02
df.history_event_hf_gains_secret_goalst = nil

---@class history_event_artifact_storedst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field artifact integer
---@field unit integer
---@field histfig integer
---@field site integer
---@field building integer Guess. the values seen are low numbers. Legends doesn't provide any additional info
df.history_event_artifact_storedst = nil

---@class history_event_agreement_formedst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field agreement_id integer
---@field delegated boolean since v0.47.02
df.history_event_agreement_formedst = nil

---@enum site_dispute_type
df.site_dispute_type = {
    Territory = 0,
    WaterRights = 1,
    GrazingRights = 2,
    FishingRights = 3,
    RightsOfWay = 4,
    LivestockOwnership = 5,
}

---@class history_event_site_disputest
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field dispute_type site_dispute_type
---@field entity_1 integer
---@field entity_2 integer
---@field site_1 integer
---@field site_2 integer
df.history_event_site_disputest = nil

---@class history_event_agreement_concludedst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field agreement_id integer
---@field subject_id integer
---@field reason history_event_reason
---@field concluder_hf integer
df.history_event_agreement_concludedst = nil

---@class history_event_insurrection_startedst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field target_civ integer
---@field site integer
df.history_event_insurrection_startedst = nil

---@enum insurrection_outcome
df.insurrection_outcome = {
    LeadershipOverthrown = 0,
    PopulationGone = 1,
    Crushed = 2,
}

---@class history_event_insurrection_endedst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field target_civ integer
---@field site integer
---@field outcome insurrection_outcome
df.history_event_insurrection_endedst = nil

---@class history_event_hf_attacked_sitest
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field attacker_hf integer
---@field defender_civ integer
---@field site_civ integer
---@field site integer
df.history_event_hf_attacked_sitest = nil

---@class history_event_performancest
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field entity integer
---@field occasion integer
---@field schedule integer
---@field site integer
---@field region integer
---@field layer integer
df.history_event_performancest = nil

---@class history_event_competitionst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field entity integer
---@field occasion integer
---@field schedule integer
---@field site integer
---@field region integer
---@field layer integer
---@field competitor_hf integer[]
---@field winner_hf integer[]
df.history_event_competitionst = nil

---@class history_event_processionst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field entity integer
---@field occasion integer
---@field schedule integer
---@field site integer
---@field region integer
---@field layer integer
df.history_event_processionst = nil

---@class history_event_ceremonyst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field entity integer
---@field occasion integer
---@field schedule integer
---@field site integer
---@field region integer
---@field layer integer
df.history_event_ceremonyst = nil

---@class history_event_knowledge_discoveredst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field hf integer
---@field knowledge knowledge_scholar_category_flag
---@field first integer
df.history_event_knowledge_discoveredst = nil

---@class history_event_artifact_transformedst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field new_artifact integer
---@field old_artifact integer[]
---@field unit integer
---@field histfig integer
---@field site integer
df.history_event_artifact_transformedst = nil

---@class history_event_artifact_destroyedst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field artifact integer
---@field site integer
---@field destroyer_hf integer
---@field destroyer_civ integer
df.history_event_artifact_destroyedst = nil

---@class history_event_hf_relationship_deniedst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field seeker_hf integer
---@field target_hf integer
---@field type unit_relationship_type
---@field reason history_event_reason
---@field reason_id integer the historical figure that the reason describes
---@field site integer
---@field region integer
---@field layer integer
df.history_event_hf_relationship_deniedst = nil

---@class history_event_regionpop_incorporated_into_entityst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field pop_race integer
---@field number_moved integer
---@field pop_region integer
---@field pop_layer integer
---@field join_entity integer
---@field site integer
df.history_event_regionpop_incorporated_into_entityst = nil

---@class history_event_poetic_form_createdst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field histfig integer
---@field form integer
---@field site integer
---@field region integer
---@field layer integer
---@field circumstance history_event_circumstance_info
---@field reason history_event_reason_info
df.history_event_poetic_form_createdst = nil

---@class history_event_musical_form_createdst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field histfig integer
---@field form integer
---@field site integer
---@field region integer
---@field layer integer
---@field circumstance history_event_circumstance_info
---@field reason history_event_reason_info
df.history_event_musical_form_createdst = nil

---@class history_event_dance_form_createdst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field histfig integer
---@field form integer
---@field site integer
---@field region integer
---@field layer integer
---@field circumstance history_event_circumstance_info
---@field reason history_event_reason_info
df.history_event_dance_form_createdst = nil

---@class history_event_written_content_composedst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field histfig integer
---@field content integer
---@field site integer
---@field region integer
---@field layer integer
---@field circumstance history_event_circumstance_info
---@field reason history_event_reason_info
df.history_event_written_content_composedst = nil

---@class history_event_change_hf_moodst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field histfig integer
---@field mood mood_type
---@field reason history_event_reason
---@field site integer
---@field region integer
---@field layer integer
---@field region_pos coord2d
df.history_event_change_hf_moodst = nil

---@class history_event_artifact_claim_formedst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field artifact integer
---@field histfig integer
---@field entity integer
---@field position_profile integer
---@field claim_type artifact_claim_type
---@field circumstance history_event_circumstance_info
---@field reason history_event_reason_info
df.history_event_artifact_claim_formedst = nil

---@class history_event_artifact_givenst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field artifact integer
---@field giver_hf integer
---@field giver_entity integer
---@field receiver_hf integer
---@field receiver_entity integer
---@field circumstance history_event_circumstance_info
---@field reason history_event_reason_info
---@field inherited boolean since v0.47.02
df.history_event_artifact_givenst = nil

---@enum history_event_hf_act_on_artifactst_action
df.history_event_hf_act_on_artifactst_action = {
    View = 0,
    AskAbout = 1,
}
---@class history_event_hf_act_on_artifactst
-- inherit history_event
---@field history_event_hf_act_on_artifactst_action history_event_hf_act_on_artifactst_action
---@field artifact integer
---@field histfig integer
---@field site integer
---@field structure integer
df.history_event_hf_act_on_artifactst = nil

---@class history_event_hf_recruited_unit_type_for_entityst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field entity integer
---@field histfig integer
---@field profession profession
---@field site integer
---@field region integer
---@field layer integer
df.history_event_hf_recruited_unit_type_for_entityst = nil

---@class history_event_hfs_formed_reputation_relationshipst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field histfig1 integer
---@field identity1 integer
---@field histfig2 integer
---@field identity2 integer
---@field rep1 reputation_type
---@field rep2 reputation_type
---@field site integer
---@field region integer
---@field layer integer
df.history_event_hfs_formed_reputation_relationshipst = nil

---@class history_event_artifact_copiedst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field artifact integer
---@field entity_dest integer
---@field entity_src integer
---@field site_dest integer
---@field site_src integer
---@field structure_dest integer
---@field structure_src integer
---@field flags2 bitfield
df.history_event_artifact_copiedst = nil

---@class history_event_sneak_into_sitest
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field attacker_civ integer
---@field defender_civ integer
---@field site_civ integer
---@field site integer
df.history_event_sneak_into_sitest = nil

---@class history_event_spotted_leaving_sitest
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field spotter_hf integer
---@field leaver_civ integer
---@field site_civ integer
---@field site integer
df.history_event_spotted_leaving_sitest = nil

---@class history_event_entity_searched_sitest
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field searcher_civ integer
---@field site integer
---@field result integer 0 = found nothing
df.history_event_entity_searched_sitest = nil

---@class history_event_hf_freedst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field freeing_civ integer
---@field freeing_hf integer
---@field holding_civ integer
---@field site_civ integer
---@field site integer
---@field rescued_hfs integer[]
df.history_event_hf_freedst = nil

---@enum simple_action_type
df.simple_action_type = {
    carouse = 0,
    purchase_well_crafted_equipment = 1,
    purchase_finely_crafted_equipment = 2,
    purchase_superior_equipment = 3,
    purchase_exceptional_equipment = 4,
    purchase_masterwork_equipment = 5,
    performe_horrible_experiments = 6,
}

---@class history_event_hist_figure_simple_actionst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field group_hfs integer[]
---@field type simple_action_type
---@field site integer
---@field structure integer
---@field region integer
---@field layer integer
df.history_event_hist_figure_simple_actionst = nil

---@class history_event_entity_rampaged_in_sitest
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field rampage_civ_id integer
---@field site_id integer
df.history_event_entity_rampaged_in_sitest = nil

---@class history_event_entity_fled_sitest
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field fled_civ_id integer
---@field site_id integer
df.history_event_entity_fled_sitest = nil

---@enum tactical_situation
df.tactical_situation = {
    attacker_strongly_favored = 0,
    attacker_favored = 1,
    attacker_slightly_favored = 2,
    defender_strongly_favored = 3,
    defender_favored = 4,
    defender_slightly_favored = 5,
    neither_favored = 6,
}

---@class history_event_tactical_situationst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field a_tactician_hfid integer
---@field d_tactician_hfid integer
---@field a_tactics_roll integer
---@field d_tactics_roll integer
---@field situation tactical_situation
---@field site integer
---@field structure integer
---@field subregion integer
---@field feature_layer integer
---@field tactics_flags bitfield
df.history_event_tactical_situationst = nil

---@class history_event_squad_vs_squadst
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field a_leader_hfid integer
---@field a_leadership_roll integer
---@field a_hfid integer[]
---@field a_squad_id integer
---@field a_race integer
---@field a_interaction integer
---@field a_effect integer
---@field a_number integer
---@field a_slain integer
---@field d_leader_hfid integer
---@field d_leadership_roll integer
---@field d_hfid integer[]
---@field d_squad_id integer
---@field d_race integer
---@field d_interaction integer
---@field d_effect integer
---@field d_number integer
---@field d_slain integer
---@field site integer
---@field structure integer
---@field subregion integer
---@field feature_layer integer
df.history_event_squad_vs_squadst = nil

---@class history_event_tradest since v0.47.01
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field hf integer
---@field entity integer the guild to which the figure belongs?
---@field source_site integer
---@field dest_site integer
---@field production_zone integer
---@field allotment integer
---@field allotment_index integer
---@field account_shift integer
df.history_event_tradest = nil

---@class history_event_add_entity_site_profile_flagst since v0.47.01
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field entity integer
---@field site integer
---@field added_flags bitfield
df.history_event_add_entity_site_profile_flagst = nil

---@class history_event_gamblest since v0.47.01
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field hf integer
---@field site integer
---@field structure integer
---@field account_before integer
---@field account_after integer
df.history_event_gamblest = nil

---@class history_event_add_hf_entity_honorst since v0.47.01
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field entity_id integer
---@field hfid integer
---@field honor_id integer index into historical_entity.honors
df.history_event_add_hf_entity_honorst = nil

---@class history_event_entity_dissolvedst since v0.47.01
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field entity integer
---@field circumstance history_event_circumstance_info
---@field reason history_event_reason_info
df.history_event_entity_dissolvedst = nil

---@class history_event_entity_equipment_purchasest since v0.47.01
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field entity integer
---@field quality item_quality
---@field hfs integer[]
df.history_event_entity_equipment_purchasest = nil

---@class history_event_modified_buildingst since v0.47.01
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field site integer
---@field structure integer index into world_site.buildings
---@field hf integer
---@field unk_1 integer
---@field modification bitfield
df.history_event_modified_buildingst = nil

---@class history_event_building_profile_acquiredst since v0.47.01
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field site integer
---@field building_profile integer
---@field acquirer_hf integer
---@field acquirer_entity integer
---@field acquisition_type integer 0: purchased, 1: inherited, 2: rebuilt. Doesn't match. Seen purchased_unowned, inherited, and rebuilt_ruined together when value = 0
---@field previous_owner_hf integer
---@field unk_1 integer
df.history_event_building_profile_acquiredst = nil

---@class history_event_hf_preachst since v0.47.01
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field speaker_hf integer
---@field site integer
---@field topic reputation_type
---@field entity1 integer
---@field entity2 integer
df.history_event_hf_preachst = nil

---@class history_event_entity_persecutedst since v0.47.01
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field persecuting_hf integer
---@field persecuting_entity integer
---@field target_entity integer
---@field site integer
---@field property_confiscated_from_hfs integer[]
---@field destroyed_structures integer[]
---@field shrines_destroyed integer
---@field expelled_hfs integer[]
---@field expelled_populations integer[]
---@field expelled_races integer[]
---@field expelled_counts integer[]
df.history_event_entity_persecutedst = nil

---@class history_event_entity_breach_feature_layerst since v0.47.01
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field site integer
---@field site_entity integer
---@field civ_entity integer
---@field layer integer
df.history_event_entity_breach_feature_layerst = nil

---@class history_event_entity_alliance_formedst since v0.47.01
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field entity integer
---@field joining_entities integer[]
df.history_event_entity_alliance_formedst = nil

---@class history_event_hf_ransomedst since v0.47.01
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field ransomed_hf integer
---@field ransomer_hf integer
---@field payer_hf integer
---@field payer_entity integer
---@field moved_to_site integer
df.history_event_hf_ransomedst = nil

---@class history_event_hf_enslavedst since v0.47.01
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field enslaved_hf integer
---@field seller_hf integer
---@field payer_entity integer
---@field moved_to_site integer
df.history_event_hf_enslavedst = nil

---@class history_event_sabotagest since v0.47.01
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field saboteur_hf integer
---@field target_hf integer
---@field target_entity integer
---@field site integer
df.history_event_sabotagest = nil

---@class history_event_entity_overthrownst since v0.47.01
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field overthrown_hf integer
---@field position_taker_hf integer
---@field instigator_hf integer
---@field entity integer
---@field position_profile_id integer
---@field conspirator_hfs integer[]
---@field site integer
df.history_event_entity_overthrownst = nil

---@class history_event_hfs_formed_intrigue_relationshipst since v0.47.01
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field corruptor_hf integer
---@field corruptor_identity integer
---@field target_hf integer
---@field target_identity integer
---@field target_role integer
---@field corruptor_role integer
---@field site integer
---@field region integer since v0.47.02
---@field layer integer since v0.47.02
df.history_event_hfs_formed_intrigue_relationshipst = nil

---@class history_event_failed_intrigue_corruptionst since v0.47.01
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field corruptor_hf integer
---@field corruptor_identity integer
---@field target_hf integer
---@field target_identity integer
---@field site integer
---@field region integer since v0.47.02
---@field layer integer since v0.47.02
df.history_event_failed_intrigue_corruptionst = nil

---@class history_event_hf_convictedst since v0.47.01
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field convicted_hf integer
---@field convicter_entity integer
---@field recognized_by_entity integer
---@field recognized_by_hf integer
---@field implicated_hfs integer[]
---@field corrupt_hf integer
---@field behest_of_hf integer
---@field fooled_hf integer
---@field framer_hf integer
---@field surveillance_hf integer
---@field co_conspirator_hf integer
---@field target_hf integer
---@field crime integer references crime::T_mode
---@field hammerstrokes integer
---@field prison_months integer
---@field punishment_flags bitfield
---@field plot_flags bitfield
df.history_event_hf_convictedst = nil

---@class history_event_failed_frame_attemptst since v0.47.01
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field target_hf integer
---@field convicter_entity integer
---@field plotter_hf integer
---@field fooled_hf integer
---@field framer_hf integer
---@field crime integer references crime::T_mode
df.history_event_failed_frame_attemptst = nil

---@class history_event_hf_interrogatedst since v0.47.01
-- inherited from history_event
---@field year integer
---@field seconds integer
---@field flags history_event_flags[]
---@field id integer
-- end history_event
---@field target_hf integer
---@field arresting_entity integer
---@field interrogator_hf integer
---@field implicated_hfs integer[]
---@field interrogation_flags bitfield
df.history_event_hf_interrogatedst = nil

---@enum history_event_collection_type
df.history_event_collection_type = {
    WAR = 0,
    BATTLE = 1,
    DUEL = 2,
    SITE_CONQUERED = 3,
    ABDUCTION = 4,
    THEFT = 5,
    BEAST_ATTACK = 6,
    JOURNEY = 7,
    INSURRECTION = 8,
    OCCASION = 9,
    PERFORMANCE = 10,
    COMPETITION = 11,
    PROCESSION = 12,
    CEREMONY = 13,
    PURGE = 14,
    RAID = 15,
    PERSECUTION = 16,
    ENTITY_OVERTHROWN = 17,
}

---@return history_event_collection_type
function df.history_event_collection.getType() end

---@param arg_0 stl-fstream
---@param indent integer
function df.history_event_collection.generate_xml(arg_0, indent) end

---@param file file_compressorst
function df.history_event_collection.write_file(file) end

---@param file file_compressorst
---@param loadversion save_version
function df.history_event_collection.read_file(file, loadversion) end

function df.history_event_collection.categorize() end

function df.history_event_collection.uncategorize() end

---@param string string
function df.history_event_collection.getName(string) end

---@param x integer
---@param y integer
function df.history_event_collection.getRegionCoords(x, y) end

---@return integer
function df.history_event_collection.getParent() end

---@return integer
function df.history_event_collection.unnamed_method() end

---@param defender_civ integer
---@param attacker_civ integer
---@return integer
function df.history_event_collection.isBetweenEntities(defender_civ, attacker_civ) end

function df.history_event_collection.updateEndTime() end

---@class history_event_collection
---@field events integer[]
---@field collections integer[]
---@field start_year integer
---@field end_year integer
---@field start_seconds integer
---@field end_seconds integer
---@field flags any[]
---@field id integer
df.history_event_collection = nil

---@class history_event_collection_warst_unk
---@field unk_1 integer[] These 5 vectors are the same length,0 or 1. Only 0 seen
---@field attacker_entity_leader integer[]
---@field unk_2 integer[] 25, 25, 46 seen. All on the first few (oldest) collections.
---@field unk_3 integer[] only -1 seen
---@field unk_4 integer[] -5,-6, -14 -15 seen
---@field unk_5 integer same as previous vector single element or zero. Sum?
---@field ethics_unk1 integer[] these three vectors are of the same length
---@field disputed_ethics ethic_type[]
---@field ethics_unk3 integer[] not seen other value
---@field dispute_severities integer[]
---@field accumulated_ethics_severity integer
---@field event_unk integer[] values 5 and 10 seen. These three vectors are the same length
---@field negative_events integer[] Site dispute, war attack site, created site, and culled seen
---@field event_severities integer[] Site dispute:-9/-10, war attack site:-2/-4/-5, created site: -20, culled: -20 (guess failed settlement)
---@field accumulated_event_severity integer sum of previous vector values

---@class history_event_collection_warst
-- inherited from history_event_collection
---@field events integer[]
---@field collections integer[]
---@field start_year integer
---@field end_year integer
---@field start_seconds integer
---@field end_seconds integer
---@field flags any[]
---@field id integer
-- end history_event_collection
---@field name language_name
---@field attacker_civ integer[]
---@field defender_civ integer[]
---@field unk_1 integer[] when length 2 attacker/defender entity. When longer seems to contain unrelated civs at varying locations
---@field unk history_event_collection_warst_unk
df.history_event_collection_warst = nil

---@class history_event_collection_battlest
-- inherited from history_event_collection
---@field events integer[]
---@field collections integer[]
---@field start_year integer
---@field end_year integer
---@field start_seconds integer
---@field end_seconds integer
---@field flags any[]
---@field id integer
-- end history_event_collection
---@field name language_name
---@field parent_collection integer
---@field region integer
---@field layer integer
---@field site integer
---@field region_pos coord2d
---@field attacker_civ integer[]
---@field defender_civ integer[]
---@field attacker_hf integer[]
---@field attacker_role integer[] Tentatively 0: regular, 1/2 merc
---@field defender_hf integer[]
---@field defender_role integer[] same as for attacker role, i.e. 0-2, with 1/2 being mercs
---@field noncombat_hf integer[] saw being beheaded, but that's only one checked
---@field merc_roles bitfield
---@field attacker_mercs integer
---@field defender_mercs integer
---@field attacker_merc_hfs integer[]
---@field defender_merc_hfs integer[]
---@field attacker_squad_entity_pop integer[]
---@field attacker_squad_counts integer[]
---@field attacker_squad_deaths integer[]
---@field attacker_squad_races integer[]
---@field attacker_squad_sites integer[]
---@field unk_3 integer[] probably a boolean, as only 0/1 seen
---@field defender_squad_entity_pops integer[]
---@field defender_squad_counts integer[]
---@field defender_squad_deaths integer[]
---@field defender_squad_races integer[]
---@field defender_squad_sites integer[]
---@field unk_4 integer[] probably a boolean, as only 0/1 seen
---@field outcome integer 0 = attacker won, 1 = defender won
df.history_event_collection_battlest = nil

---@class history_event_collection_duelst
-- inherited from history_event_collection
---@field events integer[]
---@field collections integer[]
---@field start_year integer
---@field end_year integer
---@field start_seconds integer
---@field end_seconds integer
---@field flags any[]
---@field id integer
-- end history_event_collection
---@field parent_collection integer
---@field region integer
---@field layer integer
---@field site integer
---@field region_pos coord2d
---@field attacker_hf integer
---@field defender_hf integer
---@field ordinal integer
---@field unk_1 integer probably boolean. Only 0/1 seen. Looks like winner, with all '1' examined showing defeat of defender, from unscathed to death, and '0' showing no result at all or death of attacker
df.history_event_collection_duelst = nil

---@class history_event_collection_site_conqueredst
-- inherited from history_event_collection
---@field events integer[]
---@field collections integer[]
---@field start_year integer
---@field end_year integer
---@field start_seconds integer
---@field end_seconds integer
---@field flags any[]
---@field id integer
-- end history_event_collection
---@field parent_collection integer
---@field site integer
---@field attacker_civ integer[]
---@field defender_civ integer[]
---@field unk_1 integer uninitialized
---@field ordinal integer
df.history_event_collection_site_conqueredst = nil

---@class history_event_collection_abductionst
-- inherited from history_event_collection
---@field events integer[]
---@field collections integer[]
---@field start_year integer
---@field end_year integer
---@field start_seconds integer
---@field end_seconds integer
---@field flags any[]
---@field id integer
-- end history_event_collection
---@field parent_collection integer
---@field region integer
---@field layer integer
---@field site integer
---@field region_pos coord2d
---@field attacker_civ integer
---@field defender_civ integer
---@field snatcher_hf integer[]
---@field victim_hf integer[]
---@field unk_1 integer[]
---@field ordinal integer
df.history_event_collection_abductionst = nil

---@class history_event_collection_theftst
-- inherited from history_event_collection
---@field events integer[]
---@field collections integer[]
---@field start_year integer
---@field end_year integer
---@field start_seconds integer
---@field end_seconds integer
---@field flags any[]
---@field id integer
-- end history_event_collection
---@field parent_collection integer
---@field region integer
---@field layer integer
---@field site integer
---@field region_pos coord2d
---@field thief_civ integer
---@field victim_civ integer
---@field thief_hf integer[]
---@field stolen_item_types any[]
---@field stolen_item_subtypes any[]
---@field stolen_mat_types any[]
---@field stolen_mat_indices integer[]
---@field stolen_item_ids integer[]
---@field unk_1 integer[]
---@field unk_2 integer[]
---@field unk_3 integer[]
---@field unk_4 integer[]
---@field unk_5 integer[]
---@field unk_6 integer[]
---@field unk_7 integer[]
---@field ordinal integer
df.history_event_collection_theftst = nil

---@class history_event_collection_beast_attackst
-- inherited from history_event_collection
---@field events integer[]
---@field collections integer[]
---@field start_year integer
---@field end_year integer
---@field start_seconds integer
---@field end_seconds integer
---@field flags any[]
---@field id integer
-- end history_event_collection
---@field parent_collection integer
---@field region integer
---@field layer integer
---@field site integer
---@field region_pos coord2d
---@field defender_civ integer
---@field attacker_hf integer[]
---@field ordinal integer
df.history_event_collection_beast_attackst = nil

---@class history_event_collection_journeyst
-- inherited from history_event_collection
---@field events integer[]
---@field collections integer[]
---@field start_year integer
---@field end_year integer
---@field start_seconds integer
---@field end_seconds integer
---@field flags any[]
---@field id integer
-- end history_event_collection
---@field traveler_hf integer[]
---@field ordinal integer
df.history_event_collection_journeyst = nil

---@class history_event_collection_insurrectionst
-- inherited from history_event_collection
---@field events integer[]
---@field collections integer[]
---@field start_year integer
---@field end_year integer
---@field start_seconds integer
---@field end_seconds integer
---@field flags any[]
---@field id integer
-- end history_event_collection
---@field site integer
---@field target_civ integer
---@field ordinal integer
df.history_event_collection_insurrectionst = nil

---@class history_event_collection_occasionst
-- inherited from history_event_collection
---@field events integer[]
---@field collections integer[]
---@field start_year integer
---@field end_year integer
---@field start_seconds integer
---@field end_seconds integer
---@field flags any[]
---@field id integer
-- end history_event_collection
---@field civ integer
---@field occasion integer
---@field ordinal integer
df.history_event_collection_occasionst = nil

---@class history_event_collection_performancest
-- inherited from history_event_collection
---@field events integer[]
---@field collections integer[]
---@field start_year integer
---@field end_year integer
---@field start_seconds integer
---@field end_seconds integer
---@field flags any[]
---@field id integer
-- end history_event_collection
---@field parent_collection integer all seen were occasions
---@field civ integer
---@field unk_1 integer 0-11 seen
---@field unk_2 integer 0-9 seen
---@field ordinal integer
df.history_event_collection_performancest = nil

---@class history_event_collection_competitionst
-- inherited from history_event_collection
---@field events integer[]
---@field collections integer[]
---@field start_year integer
---@field end_year integer
---@field start_seconds integer
---@field end_seconds integer
---@field flags any[]
---@field id integer
-- end history_event_collection
---@field parent_collection integer all seen were occasions
---@field civ integer
---@field unk_1 integer 0-13 seen
---@field unk_2 integer 0-9 seen
---@field ordinal integer
df.history_event_collection_competitionst = nil

---@class history_event_collection_processionst
-- inherited from history_event_collection
---@field events integer[]
---@field collections integer[]
---@field start_year integer
---@field end_year integer
---@field start_seconds integer
---@field end_seconds integer
---@field flags any[]
---@field id integer
-- end history_event_collection
---@field parent_collection integer all seen were occasions
---@field civ integer
---@field unk_1 integer 0-14 seen
---@field unk_2 integer 0-9 seen
---@field ordinal integer
df.history_event_collection_processionst = nil

---@class history_event_collection_ceremonyst
-- inherited from history_event_collection
---@field events integer[]
---@field collections integer[]
---@field start_year integer
---@field end_year integer
---@field start_seconds integer
---@field end_seconds integer
---@field flags any[]
---@field id integer
-- end history_event_collection
---@field parent_collection integer all seen were occasions
---@field civ integer
---@field unk_1 integer 0-14 seen
---@field unk_2 integer 0-10 seen
---@field ordinal integer
df.history_event_collection_ceremonyst = nil

---@class history_event_collection_purgest since v0.42.04
-- inherited from history_event_collection
---@field events integer[]
---@field collections integer[]
---@field start_year integer
---@field end_year integer
---@field start_seconds integer
---@field end_seconds integer
---@field flags any[]
---@field id integer
-- end history_event_collection
---@field site integer
---@field adjective string
---@field ordinal integer
df.history_event_collection_purgest = nil

---@class history_event_collection_raidst since v0.42.04
-- inherited from history_event_collection
---@field events integer[]
---@field collections integer[]
---@field start_year integer
---@field end_year integer
---@field start_seconds integer
---@field end_seconds integer
---@field flags any[]
---@field id integer
-- end history_event_collection
---@field parent_collection integer
---@field region integer
---@field layer integer
---@field site integer
---@field region_pos coord2d
---@field attacker_civ integer
---@field defender_civ integer
---@field thieves integer[] all of the ones examined were mentioned stealing things during the same raid on the site
---@field ordinal integer
df.history_event_collection_raidst = nil

---@class history_event_collection_persecutionst since v0.47.01
-- inherited from history_event_collection
---@field events integer[]
---@field collections integer[]
---@field start_year integer
---@field end_year integer
---@field start_seconds integer
---@field end_seconds integer
---@field flags any[]
---@field id integer
-- end history_event_collection
---@field site integer
---@field entity integer
---@field ordinal integer
df.history_event_collection_persecutionst = nil

---@class history_event_collection_entity_overthrownst since v0.47.01
-- inherited from history_event_collection
---@field events integer[]
---@field collections integer[]
---@field start_year integer
---@field end_year integer
---@field start_seconds integer
---@field end_seconds integer
---@field flags any[]
---@field id integer
-- end history_event_collection
---@field site integer
---@field entity integer
---@field ordinal integer
df.history_event_collection_entity_overthrownst = nil

---@enum era_type
df.era_type = {
    ThreePowers = 0,
    TwoPowers = 1,
    OnePower = 2,
    Myth = 3,
    Legends = 4,
    Twilight = 5,
    FairyTales = 6,
    Race = 7,
    Heroes = 8,
    Golden = 9,
    Death = 10,
    Civilization = 11,
    Emptiness = 12,
}

---@class history_era_details
---@field living_powers integer
---@field living_megabeasts integer
---@field living_semimegabeasts integer
---@field power_hf1 integer
---@field power_hf2 integer
---@field power_hf3 integer
---@field civilized_races integer[]
---@field civilized_total integer
---@field civilized_mundane integer

---@class history_era_title
---@field type era_type
---@field histfig_1 integer
---@field histfig_2 integer
---@field ordinal integer
---@field name string
---@field percent integer either percentage of single race or percentage of mundane

---@class history_era
---@field year integer
---@field title history_era_title
---@field details history_era_details
df.history_era = nil

---@class relationship_event
---@field event integer[] not included in the main list
---@field relationship vague_relationship_type[]
---@field source_hf integer[]
---@field target_hf integer[]
---@field year integer[]
---@field next_element integer 1024 for all vectors except the last one
---@field start_year integer first year of the events contained in the element
df.relationship_event = nil

---@class relationship_event_supplement
---@field event integer can be found in the relationship_events
---@field occasion_type integer only 245/246 seen. 245:scholarly lecture, 246: performance
---@field site integer
---@field unk_1 integer only 81 seen
---@field profession profession
df.relationship_event_supplement = nil

---@class world_history_event_collections
---@field all history_event_collection[]
---@field other any[]

---@class world_history
---@field events history_event[]
---@field events_death history_event[]
---@field relationship_events relationship_event[] since v0.47.01
---@field relationship_event_supplements relationship_event_supplement[] supplemental info for artistic/scholar buddies, since v0.47.01
---@field figures historical_figure[]
---@field event_collections world_history_event_collections
---@field eras history_era[]
---@field discovered_art_image_id integer[]
---@field discovered_art_image_subid integer[]
---@field total_unk integer
---@field total_powers integer also includes megabeasts
---@field total_megabeasts integer
---@field total_semimegabeasts integer
---@field unk_14 any[]
---@field unk_v42_1 integer[] since v0.42.01
---@field intrigues intrigue[] since v0.47.01
---@field live_megabeasts historical_figure[]
---@field live_semimegabeasts historical_figure[]
---@field unk_histfig_3 historical_figure[]
---@field unk_histfig_4 historical_figure[]
---@field unk_histfig_5 historical_figure[]
---@field unk_1 historical_figure[] since v0.47.01
---@field unk_v40_1 any[] since v0.40.01
---@field unk_histfig_6 historical_figure[] since v0.42.01
---@field unk_histfig_7 historical_figure[] since v0.42.01
---@field unk_histfig_8 historical_figure[] since v0.42.01
---@field unk_histfig_9 historical_figure[] since v0.42.01
---@field unk_histfig_10 historical_figure[] since v0.42.01
---@field unk_histfig_11 historical_figure[] since v0.40.01
---@field unk_histfig_12 historical_figure[] since v0.44.01
---@field unk_histfig_13 historical_figure[] since v0.44.01
---@field unk_3 historical_figure[] since v0.44.01
---@field unk_4 any[] since v0.47.01
---@field unk_5 historical_figure[] since v0.47.01
---@field unk_6 any[] since v0.47.01
---@field unk_7 integer[] since v0.47.01
---@field unk_8 integer
---@field active_event_collections history_event_collection[]
---@field unk_10 integer
---@field unk_11 integer
---@field unk_12 integer
---@field active_mission mission_report
df.world_history = nil

---@class intrigue
---@field event_id integer NOTE: can be culled. Seen: failed_intrigue_corruption, event_agreement_formed, hfs_formed_intrigue_relationship
---@field corruption intrigue_corruption Mutually exclusive with circumstance. Exactly one is present. Presumably 'bring into network' action doesn't provide membership
---@field reason history_event_reason_info
---@field circumstance history_event_circumstance_info
df.intrigue = nil

---@enum intrigue_corruption_manipulated_emotion
df.intrigue_corruption_manipulated_emotion = {
    Trust = 0,
    Loyalty = 1,
    Love = 2,
    Fear = 3,
    Respect = 4,
}
---@enum intrigue_corruption_manipulation_type
df.intrigue_corruption_manipulation_type = {
    Threat = 0,
    Flattery = 1,
    Authority = 2,
    BlackmailForEmbezzlement = 3,
    Bribery = 4,
    Sympathy = 5,
    Revenge = 6,
    Immortality = 7,
}
---@class intrigue_corruption
---@field crime crime_type
---@field corruptor_id integer
---@field target_id integer
---@field target_relationship vague_relationship_type set if and only if action = BringIntoNetwork
---@field target_relationship_entity_id integer Only set when relation = CommonEntity. Common Religion/PerformanceTroupe/MerchantCompany/Guild seen.
---@field lurer_id integer Can be set with action = CorruptInPlace, not otherwise
---@field intrigue_corruption_manipulation_type intrigue_corruption_manipulation_type
---@field unk_4 integer -16 to 315 seen
---@field unk_5 integer -141 to 351 seen
---@field manipulated_facet personality_facet_type
---@field facet_rating integer
---@field facet_roll integer
---@field manipulated_value value_type
---@field value_rating integer
---@field value_roll integer
---@field intrigue_corruption_manipulated_emotion intrigue_corruption_manipulated_emotion
---@field emotion_rating integer -100 to 125 seen
---@field emotion_roll integer -10 to 12 seen
---@field flags bitfield
---@field position_entity_id integer Used to pull rank
---@field position_assignment_id integer
---@field offered_id integer deity or revenge target
---@field offered_relationship vague_relationship_type
---@field corruptor_ally_roll integer
---@field target_ally_roll integer
df.intrigue_corruption = nil


