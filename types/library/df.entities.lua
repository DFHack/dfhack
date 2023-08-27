-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.

---@meta

---@class entity_occasion_info
---@field occasions entity_occasion[] since v0.42.01
---@field next_occasion_id integer
---@field events integer[]
---@field count integer number of elements used in array above
df.entity_occasion_info = nil

---@class entity_occasion some festivals are annual, some single time. unk_1=0 plus unk_3=0 seems to match with single time, which doesn't make much sense. Only frequency seen is yearly
---@field id integer
---@field unk_1 integer 0 and 1 seen
---@field site integer
---@field unk_2 integer only -1 seen, but based on other cases, might be an abstract building
---@field name language_name
---@field start_year_tick integer
---@field end_year_tick integer
---@field unk_3 integer 0-2 seen
---@field event integer
---@field unk_4 integer only seen with unk_3=2, but is usually not set
---@field schedule entity_occasion_schedule[]
---@field unk_5 integer only value seen
df.entity_occasion = nil

---@enum occasion_schedule_type
df.occasion_schedule_type = {
    DANCE_PERFORMANCE = 0,
    MUSICAL_PERFORMANCE = 1,
    POETRY_RECITAL = 2,
    STORYTELLING = 3,
    DANCE_COMPETITION = 4,
    MUSICAL_COMPETITION = 5,
    POETRY_COMPETITION = 6,
    FOOT_RACE = 7,
    unk_8 = 8,
    unk_9 = 9,
    WRESTLING_COMPETITION = 10,
    THROWING_COMPETITION = 11,
    GLADIATORY_COMPETITION = 12,
    PROCESSION = 13,
    CEREMONY = 14,
}

---@class entity_occasion_schedule
---@field type occasion_schedule_type
---@field reference integer art form / event / item_type /procession start abstract building
---@field reference2 integer item_subtype / procession stop abstract building
---@field unk_1 integer
---@field unk_2 integer
---@field unk_3 integer
---@field features entity_occasion_schedule_feature[]
---@field start_year_tick integer
---@field end_year_tick integer
df.entity_occasion_schedule = nil

---@enum occasion_schedule_feature
df.occasion_schedule_feature = {
    unk_0 = 0,
    unk_1 = 1,
    STORYTELLING = 2,
    POETRY_RECITAL = 3,
    MUSICAL_PERFORMANCE = 4,
    DANCE_PERFORMANCE = 5,
    unk_6 = 6,
    CRIERS_IN_FRONT = 7,
    ORDER_OF_PRECEDENCE = 8,
    BANNERS = 9,
    IMAGES = 10,
    unk_11 = 11,
    unk_12 = 12,
    ACROBATS = 13,
    INCENSE_BURNING = 14,
    COSTUMES = 15,
    CANDLES = 16,
    THE_GIVING_OF_ITEMS = 17,
    THE_SACRIFICE_OF_ITEMS = 18,
}

---@class entity_occasion_schedule_feature
---@field feature occasion_schedule_feature
---@field reference integer
---@field unk_1 integer
---@field unk_2 integer
---@field unk_3 integer
df.entity_occasion_schedule_feature = nil

---@class entity_activity_statistics_wealth
---@field total integer
---@field weapons integer
---@field armor integer
---@field furniture integer
---@field other integer
---@field architecture integer
---@field displayed integer
---@field held integer
---@field imported integer
---@field unk_1 integer
---@field exported integer

---@class entity_activity_statistics_food
---@field total integer
---@field meat integer
---@field fish integer
---@field other integer
---@field seeds integer
---@field plant integer
---@field drink integer

---@class entity_activity_statistics
---@field food entity_activity_statistics_food
---@field unit_counts integer[]
---@field population integer
---@field menial_exempt integer
---@field omnivores integer
---@field carnivores integer
---@field trained_animals integer
---@field other_animals integer
---@field potential_soldiers integer
---@field combat_aptitude integer
---@field item_counts integer[]
---@field created_weapons integer[]
---@field wealth entity_activity_statistics_wealth
---@field recent_jobs any[]
---@field excavated_tiles integer unhidden, subterranean, and excluding map features
---@field death_history integer[]
---@field insanity_history integer[]
---@field execution_history integer[]
---@field noble_death_history integer[]
---@field total_deaths integer
---@field total_insanities integer
---@field total_executions integer
---@field unnamed_entity_activity_statistics_23 integer
---@field unnamed_entity_activity_statistics_24 integer
---@field unnamed_entity_activity_statistics_25 integer
---@field unnamed_entity_activity_statistics_26 integer
---@field num_artifacts integer
---@field unk_6 integer in 0.23, total siegers
---@field discovered_creature_foods boolean[]
---@field discovered_creatures boolean[]
---@field discovered_plant_foods boolean[]
---@field discovered_plants boolean[] allows planting of seeds
---@field discovered_water_features integer
---@field discovered_subterranean_features integer
---@field discovered_chasm_features integer unused since 40d
---@field discovered_magma_features integer
---@field discovered_feature_layers integer back in 40d, this counted HFS
---@field migrant_wave_idx integer when >= 2, no migrants
---@field found_minerals integer[] Added after 'you have struck' announcement
---@field found_misc bitfield
df.entity_activity_statistics = nil

---@enum caravan_state_trade_state
df.caravan_state_trade_state = {
    None = 0,
    Approaching = 1,
    AtDepot = 2,
    Leaving = 3,
    Stuck = 4,
}
---@class caravan_state
---@field total_capacity integer
---@field unk_1 integer
---@field caravan_state_trade_state caravan_state_trade_state
---@field depot_notified integer has it warned you that you need a depot
---@field time_remaining integer
---@field entity integer
---@field activity_stats entity_activity_statistics
---@field flags bitfield
---@field import_value integer
---@field export_value_total integer
---@field export_value_personal integer excluding foreign-produced items
---@field offer_value integer
---@field animals integer[]
---@field sell_prices entity_sell_prices
---@field buy_prices entity_buy_prices
---@field goods integer[]
---@field mood integer reflects satisfaction with last trading session
---@field unk_2 integer
df.caravan_state = nil

---@class entity_buy_prices
---@field items entity_buy_requests
---@field price integer[]
df.entity_buy_prices = nil

---@class entity_buy_requests
---@field item_type any[] guess
---@field item_subtype any[] guess
---@field mat_types any[]
---@field mat_indices integer[]
---@field mat_cats job_material_category[]
---@field priority integer[]
df.entity_buy_requests = nil

---@enum entity_sell_category
df.entity_sell_category = {
    Leather = 0,
    ClothPlant = 1,
    ClothSilk = 2,
    Crafts = 3,
    Wood = 4,
    MetalBars = 5,
    SmallCutGems = 6,
    LargeCutGems = 7,
    StoneBlocks = 8,
    Seeds = 9,
    Anvils = 10,
    Weapons = 11,
    TrainingWeapons = 12,
    Ammo = 13,
    TrapComponents = 14,
    DiggingImplements = 15,
    Bodywear = 16,
    Headwear = 17,
    Handwear = 18,
    Footwear = 19,
    Legwear = 20,
    Shields = 21,
    Toys = 22,
    Instruments = 23,
    Pets = 24,
    Drinks = 25,
    Cheese = 26,
    Powders = 27,
    Extracts = 28,
    Meat = 29,
    Fish = 30,
    Plants = 31,
    FruitsNuts = 32,
    GardenVegetables = 33,
    MeatFishRecipes = 34,
    OtherRecipes = 35,
    Stone = 36,
    Cages = 37,
    BagsLeather = 38,
    BagsPlant = 39,
    BagsSilk = 40,
    ThreadPlant = 41,
    ThreadSilk = 42,
    RopesPlant = 43,
    RopesSilk = 44,
    Barrels = 45,
    FlasksWaterskins = 46,
    Quivers = 47,
    Backpacks = 48,
    Sand = 49,
    Glass = 50,
    Miscellaneous = 51,
    Buckets = 52,
    Splints = 53,
    Crutches = 54,
    Eggs = 55,
    BagsYarn = 56,
    RopesYarn = 57,
    ClothYarn = 58,
    ThreadYarn = 59,
    Tools = 60,
    Clay = 61,
    Parchment = 62,
    CupsMugsGoblets = 63,
}

---@class entity_sell_prices
---@field items entity_sell_requests
---@field price any[]
df.entity_sell_prices = nil

---@class entity_sell_requests
---@field priority any[]
df.entity_sell_requests = nil

---@class entity_recipe
---@field subtype integer
---@field item_types any[]
---@field item_subtypes any[]
---@field mat_types any[]
---@field mat_indices integer[]
df.entity_recipe = nil

---@enum historical_entity_type
df.historical_entity_type = {
    Civilization = 0,
    SiteGovernment = 1,
    VesselCrew = 2,
    MigratingGroup = 3,
    NomadicGroup = 4,
    Religion = 5,
    MilitaryUnit = 6,
    Outcast = 7,
    PerformanceTroupe = 8,
    MerchantCompany = 9,
    Guild = 10,
}

---@class honors_type
---@field id integer
---@field flags bitfield
---@field name string
---@field precedence_awarded integer
---@field required_skill job_skill
---@field required_skill_type bitfield
---@field required_skill_points integer
---@field required_kills integer
---@field required_battles integer
---@field required_years_of_membership integer
---@field honored integer[]
---@field required_position integer[]
---@field required_former_position integer[]
df.honors_type = nil

---@class artifact_claim
---@field artifact_id integer
---@field claim_type artifact_claim_type
---@field symbol_claim_id integer different small numbers, but all claimed by the greedy necro diplomat, but not complete number range present
---@field claim_year integer Written contents often seem to lack info of being claimed
---@field claim_year_tick integer usually init
---@field unk_1 integer
---@field artifact artifact_record
---@field site integer
---@field structure_local integer
---@field holder_hf integer might be owner_hf. all cases encountered have had both field the same when claimed by entity
---@field subregion integer
---@field feature_layer_id integer
---@field unk_year integer seems to be current year or -1. Matches up with corresponding field of artifact_record
---@field unk_2 integer only other value seen was 0
---@field unk_3 integer uninitialized
---@field unk_4 integer
---@field unk_5 historical_entity
---@field unk_6 historical_entity
df.artifact_claim = nil

---@class entity_unk_v47_1 The 3 first vectors are of the same length and somehow connected
---@field unk_v47_1 integer seen kobold thieves and goblin snatchers, but not all thieves... seen 1 of several persecuted and expelled
---@field unk_v47_2 integer some enum?
---@field unk_v47_3 integer[] some enum?
---@field agreement integer[]
---@field unk_v47_5 integer[] boolean?
---@field unk_v47_6 integer[]
---@field unk_v47_7 integer[]
---@field unk_v47_8 integer[]
---@field unk_v47_9 integer
df.entity_unk_v47_1 = nil

---@class historical_entity_unknown2
---@field metal_proficiency integer best IMPACT_FRACTURE/10000 + MAX_EDGE/1000 for weapon mats plus best IMPACT_FRACTURE/10000 for armor mats
---@field weapon_proficiencies job_skill[]
---@field resource_allotment resource_allotment_data Only for SiteGovernment, but not all
---@field unk_1 poetic_form[] since v0.42.01
---@field unk_2 musical_form[] since v0.42.01
---@field unk_3 dance_form[] since v0.42.01
---@field unk_4 written_content[] since v0.42.01
---@field unk12a integer
---@field unk12b integer uninitialized
---@field unk13 boolean 0
---@field landmass world_landmass
---@field region world_region
---@field unk16 integer uninitialized
---@field unk17 integer 0
---@field unk18 integer[] used during world gen
---@field unk19 integer[] used during world gen
---@field unk20 integer 0
---@field unk21 integer 0
---@field unk22 integer 0
---@field unk23 integer 0
---@field unk24 integer[] used during world gen
---@field unk25 integer[] used during world gen
---@field unk_9C integer
---@field unk_A0 integer
---@field unk_A4 integer
---@field unk_A8 integer
---@field unk_AC integer
---@field unk_B0 integer
---@field unk_B4 integer
---@field unk_B8 integer
---@field unk_BC integer
---@field unk_C0 integer
---@field unk_5 integer since v0.47.01
---@field unk_6 integer since v0.47.01
---@field unk_7 integer since v0.47.01
---@field unk26a integer[]
---@field unk26b integer[]
---@field unk26c integer[]
---@field unk26d integer[]
---@field unk26e integer[]
---@field unk28 any[]
---@field unk_8 integer since v0.47.01
---@field unk29 any[] since v0.34.01
---@field unk_9 integer since v0.47.01
---@field unk_10 integer since v0.47.01
---@field unk_11 integer since v0.47.01

---@class historical_entity_claims
---@field areas coord2d_path in world_data.entity_claims1
---@field unk1 coord2d_path
---@field border coord2d_path

---@class historical_entity_derived_resources
---@field mill_cookable material_vec_ref
---@field mill_dye material_vec_ref
---@field armor_leather integer[]
---@field armor_chain integer[]
---@field armor_plate integer[]
---@field armor_under integer[]
---@field armor_over integer[]
---@field armor_cover integer[]
---@field pants_leather integer[]
---@field pants_chain integer[]
---@field pants_plate integer[]
---@field pants_under integer[]
---@field pants_over integer[]
---@field pants_cover integer[]
---@field helm_leather integer[]
---@field helm_chain integer[]
---@field helm_plate integer[]
---@field helm_under integer[]
---@field helm_over integer[]
---@field helm_cover integer[]
---@field shoes_leather integer[]
---@field shoes_chain integer[]
---@field shoes_plate integer[]
---@field shoes_under integer[]
---@field shoes_over integer[]
---@field shoes_cover integer[]
---@field gloves_leather integer[]
---@field gloves_chain integer[]
---@field gloves_plate integer[]
---@field gloves_under integer[]
---@field gloves_over integer[]
---@field gloves_cover integer[]

---@class historical_entity_unknown1e
---@field unk47 integer in 0.23, last communicate season
---@field unk48 integer in 0.23, last communicate year
---@field imports_from integer
---@field offerings_from integer
---@field offerings_recent integer since the last migrant wave or diplomat visit
---@field offerings_history integer[] rotated yearly at 15th of Timber
---@field hostility_level integer
---@field siege_tier integer
---@field unk_1 integer siege cooldown?, since v0.40.01
---@field unk_2 integer since v0.47.01
---@field unk_3 integer since v0.47.01

---@class historical_entity_tissue_styles
---@field all entity_tissue_style[]
---@field next_style_id integer

---@class historical_entity_positions
---@field own entity_position[]
---@field site entity_position[]
---@field conquered_site entity_position[]
---@field next_position_id integer
---@field assignments entity_position_assignment[]
---@field next_assignment_id integer
---@field possible_evaluate entity_position_assignment[] since v0.40.01
---@field possible_succession entity_position_assignment[] since v0.40.01
---@field possible_appointable entity_position_assignment[] since v0.40.01
---@field possible_elected entity_position_assignment[] since v0.40.01
---@field possible_claimable entity_position_assignment[] since v0.40.01

---@class historical_entity_relations
---@field known_sites integer[] only civs and site government. Fresh player site government has empty vector
---@field deities integer[]
---@field worship integer[] Same length as deities(?). Some kind of relationship strength?
---@field belief_systems integer[] In Religion type entities established by prophets after having developed their own belief system, the ID of this belief system is contained here.
---@field constructions any[] only civs. Usually pairs for source/destination, with destination lacking path and construction. Construction and second entry can be lacking when destination lost(construction destroyed as well?). Also seen only source entry
---@field diplomacy any[]
---@field unk33 integer Non zero seen only on site governments (not all) and one nomadic group. Small values
---@field unk34a integer[] same length as unk34b and unk34c
---@field unk34b integer[]
---@field unk34c integer[]
---@field position integer[] position index (not id)
---@field official integer[] holder of office of corresponding position index

---@class historical_entity_resources_animals
---@field pet_races integer[]
---@field wagon_races integer[]
---@field pack_animal_races integer[]
---@field wagon_puller_races integer[]
---@field mount_races integer[]
---@field minion_races integer[]
---@field exotic_pet_races integer[]
---@field pet_castes integer[]
---@field wagon_castes integer[]
---@field pack_animal_castes integer[]
---@field wagon_puller_castes integer[]
---@field mount_castes integer[]
---@field minion_castes integer[]
---@field exotic_pet_castes integer[]

---@class historical_entity_resources_wood_products lye, charcoal, potash, pearlash, and coke
---@field item_type any[]
---@field item_subtype integer[]
---@field material material_vec_ref

---@class historical_entity_resources_misc_mat
---@field others material_vec_ref amber and coral
---@field glass material_vec_ref
---@field sand material_vec_ref
---@field clay material_vec_ref
---@field crafts material_vec_ref
---@field glass_unused material_vec_ref used for vial extracts on embark
---@field barrels material_vec_ref also buckets, splints, and crutches
---@field flasks material_vec_ref
---@field quivers material_vec_ref
---@field backpacks material_vec_ref
---@field cages material_vec_ref
---@field wood2 material_vec_ref since v0.34.01
---@field rock_metal material_vec_ref since v0.34.01
---@field booze material_vec_ref
---@field cheese material_vec_ref
---@field powders material_vec_ref
---@field extracts material_vec_ref
---@field meat material_vec_ref

---@class historical_entity_resources_refuse
---@field bone material_vec_ref
---@field shell material_vec_ref
---@field pearl material_vec_ref
---@field ivory material_vec_ref
---@field horn material_vec_ref

---@class historical_entity_resources_organic
---@field leather material_vec_ref
---@field parchment material_vec_ref since v0.42.01
---@field fiber material_vec_ref
---@field silk material_vec_ref
---@field wool material_vec_ref
---@field wood material_vec_ref

---@class historical_entity_resources_metal
---@field pick material_vec_ref
---@field weapon material_vec_ref
---@field ranged material_vec_ref
---@field ammo material_vec_ref
---@field ammo2 material_vec_ref maybe intended for siege ammo
---@field armor material_vec_ref also instruments, toys, and tools
---@field anvil material_vec_ref

---@class historical_entity_resources
---@field digger_type integer[]
---@field weapon_type integer[]
---@field training_weapon_type integer[]
---@field armor_type integer[]
---@field ammo_type integer[]
---@field helm_type integer[]
---@field gloves_type integer[]
---@field shoes_type integer[]
---@field pants_type integer[]
---@field shield_type integer[]
---@field trapcomp_type integer[]
---@field toy_type integer[]
---@field instrument_type integer[]
---@field siegeammo_type integer[]
---@field tool_type integer[]
---@field unk_1 integer[] since v0.42.01
---@field metal historical_entity_resources_metal
---@field organic historical_entity_resources_organic
---@field metals integer[] bars
---@field stones integer[] boulders and blocks
---@field gems integer[] small and large cut
---@field refuse historical_entity_resources_refuse
---@field misc_mat historical_entity_resources_misc_mat
---@field fish_races integer[]
---@field fish_castes integer[]
---@field egg_races integer[]
---@field egg_castes integer[]
---@field plants material_vec_ref
---@field tree_fruit_plants integer[]
---@field tree_fruit_growths integer[]
---@field shrub_fruit_plants integer[]
---@field shrub_fruit_growths integer[]
---@field seeds material_vec_ref
---@field wood_products historical_entity_resources_wood_products lye, charcoal, potash, pearlash, and coke
---@field animals historical_entity_resources_animals
---@field meat_fish_recipes entity_recipe[]
---@field other_recipes entity_recipe[]
---@field unk13 any[] in 0.23, these were material/matgloss pairs, never used for anything
---@field unk14 item[] in 0.23, items that would be equipped by the arriving King, never used
---@field unk15a integer in 0.23, minimum temperature
---@field unk15b integer in 0.23, maximum temperature
---@field ethic any[]
---@field values integer[]
---@field unk_2 integer since v0.42.01
---@field permitted_skill boolean[]
---@field art_image_types integer[] 0 = civilization symbol
---@field art_image_ids integer[]
---@field art_image_subids integer[]
---@field color_ref_type any[]
---@field foreground_color_curses any[]
---@field foreground_color_curses_bright boolean[]
---@field background_color_curses any[]
---@field foreground_color integer[] foreground color used for the entity symbol in legends mode and the historical maps.
---@field background_color integer[] background color used for the entity symbol in legends mode and the historical maps.

---@class historical_entity
---@field type historical_entity_type
---@field id integer index in the array
---@field entity_raw entity_raw
---@field unk_v50_10 integer
---@field save_file_id integer changes once has 100 entries
---@field next_member_idx integer
---@field name language_name
---@field race integer
---@field flags bitfield
---@field guild_professions any[] Only seen 1, and only for guilds, since v0.47.01
---@field entity_links entity_entity_link[]
---@field site_links entity_site_link[]
---@field histfig_ids integer[]
---@field populations integer[] 1st entry copies to unit.population_id for Adventurer?
---@field nemesis_ids integer[]
---@field resources historical_entity_resources
---@field uniforms entity_uniform[]
---@field next_uniform_id integer
---@field relations historical_entity_relations
---@field positions historical_entity_positions
---@field tissue_styles historical_entity_tissue_styles
---@field squads integer[]
---@field global_event_knowledge_year integer
---@field local_known_events integer[] since the above year
---@field production_zone_id integer not sure what this refers to
---@field conquered_site_group_flags bitfield actually lives inside a class, since v0.34.01
---@field worldgen_can_make_guildhall integer[] since v0.34.01
---@field training_knowledge integer since v0.34.06
---@field events entity_event[]
---@field unk_v40_1a integer since v0.40.01
---@field unk_v40_1b integer since v0.40.01
---@field unk_v40_1c integer since v0.40.01
---@field unk_v40_1d integer since v0.40.01
---@field unk_v40_1e integer since v0.40.01
---@field performed_poetic_forms integer[] since v0.42.01
---@field performed_musical_forms integer[] since v0.42.01
---@field performed_dance_forms integer[] since v0.42.01
---@field major_civ_number integer[] Single item increasing from 0 linearly as major race civs are created, i.e. all civs except cavern and Kobold, but including spire breach released goblin ones, since v0.42.01
---@field major_civ_number_2 integer[] Identical to major_civ_number on the same entries. Presumably some properties only major civs have, such as e.g. accepting outsiders as members, engage in conquest, etc., since v0.42.01
---@field unk_1 integer[] Only on major civs, but not all, since v0.42.01
---@field occasion_info entity_occasion_info only seen on Civilization, SiteGovernment, and Religion, but not all
---@field artifact_claims artifact_claim[] sorted on artifact id, since v0.44.01
---@field honors honors_type[] Only merc companies. Matches #Honors groups in Legends Viewer, since v0.47.01
---@field next_honors_index integer since v0.47.01
---@field equipment_purchases integer only seen on military units, since v0.47.01
---@field attack integer only seen on military units, since v0.47.01
---@field total_battles integer attacks + defenses. Only seen on military units, since v0.47.01
---@field unk_v47_1 integer since v0.47.01
---@field divination_sets integer[] Guess. Only on religions, but not all. start at 350 and added sequentially in Religion formation order. Last religion # = last divination set index, since v0.47.01
---@field founding_site_government integer All cases examined refered to site government of site of founding. Perf troop and merc lack site info but seems reasonable., since v0.47.01
---@field meeting_events meeting_event[]
---@field activity_stats entity_activity_statistics
---@field unknown1e historical_entity_unknown1e
---@field armies army[] since v0.40.01
---@field army_controllers army_controller[] since v0.40.01
---@field hist_figures historical_figure[]
---@field nemesis nemesis_record[]
---@field derived_resources historical_entity_derived_resources
---@field assignments_by_type any[]
---@field claims historical_entity_claims
---@field children integer[] includes self
---@field unknown2 historical_entity_unknown2
df.historical_entity = nil

---@class entity_tissue_style
---@field name string
---@field preferred_shapings integer[]
---@field unk_1 integer[] maybe probability?
---@field maintain_length_min integer
---@field maintain_length_max integer
---@field id integer
df.entity_tissue_style = nil

---@enum training_knowledge_level
df.training_knowledge_level = {
    None = 0,
    FewFacts = 1,
    GeneralFamiliarity = 2,
    Knowledgeable = 3,
    Expert = 4,
    Domesticated = 5,
}

---@enum entity_position_flags
df.entity_position_flags = {
    IS_LAW_MAKER = 0,
    ELECTED = 1,
    DUTY_BOUND = 2,
    MILITARY_SCREEN_ONLY = 3,
    GENDER_MALE = 4,
    GENDER_FEMALE = 5,
    SUCCESSION_BY_HEIR = 6,
    HAS_RESPONSIBILITIES = 7,
    FLASHES = 8,
    BRAG_ON_KILL = 9,
    CHAT_WORTHY = 10,
    DO_NOT_CULL = 11,
    KILL_QUEST = 12,
    IS_LEADER = 13,
    IS_DIPLOMAT = 14,
    EXPORTED_IN_LEGENDS = 15,
    DETERMINES_COIN_DESIGN = 16,
    ACCOUNT_EXEMPT = 17,
    unk_12 = 18,
    unk_13 = 19,
    COLOR = 20,
    RULES_FROM_LOCATION = 21,
    MENIAL_WORK_EXEMPTION = 22,
    MENIAL_WORK_EXEMPTION_SPOUSE = 23,
    SLEEP_PRETENSION = 24,
    PUNISHMENT_EXEMPTION = 25,
    unk_1a = 26,
    unk_1b = 27,
    QUEST_GIVER = 28,
    SPECIAL_BURIAL = 29,
    REQUIRES_MARKET = 30,
    unk_1f = 31,
}

---@class entity_position
---@field code string
---@field id integer
---@field flags entity_position_flags[]
---@field allowed_creature integer[]
---@field allowed_class string[]
---@field rejected_creature integer[]
---@field rejected_class string[]
---@field name string[]
---@field name_female string[]
---@field name_male string[]
---@field spouse string[]
---@field spouse_female string[]
---@field spouse_male string[]
---@field squad string[]
---@field land_name string
---@field squad_size integer
---@field commander_id integer[]
---@field commander_civ integer[]
---@field commander_types integer[]
---@field land_holder integer
---@field requires_population integer
---@field unk_1 integer since v0.34.01
---@field precedence integer
---@field replaced_by integer
---@field number integer
---@field appointed_by integer[]
---@field appointed_by_civ integer[]
---@field succession_by_position integer[]
---@field responsibilities boolean[]
---@field unk_v50_358 string
---@field color integer[]
---@field required_boxes integer
---@field required_cabinets integer
---@field required_racks integer
---@field required_stands integer
---@field required_office integer
---@field required_bedroom integer
---@field required_dining integer
---@field required_tomb integer
---@field mandate_max integer
---@field demand_max integer
---@field unk_2 integer since v0.47.01
df.entity_position = nil

---@class entity_position_assignment
---@field id integer
---@field histfig integer
---@field histfig2 integer since v0.40.01
---@field position_id integer position within relevant entity
---@field position_vector_idx integer
---@field flags any[]
---@field squad_id integer
---@field unk_1 integer
---@field unk_2 integer
---@field unk_3 integer since v0.40.01
---@field unk_4 integer since v0.40.01
---@field unk_5 any[] not saved, since v0.40.01
---@field unk_6 integer unknown size, not initialized or saved, since v0.47.01
df.entity_position_assignment = nil

---@enum entity_material_category
df.entity_material_category = {
    None = -1,
    Clothing = 0, -- cloth or leather
    Leather = 1, -- organic.leather
    Cloth = 2, -- any cloth
    Wood = 3, -- organic.wood, used for training weapons
    Crafts = 4, -- misc_mat.crafts
    Stone = 5, -- stones
    Improvement = 6, -- misc_mat.crafts
    Glass = 7, -- misc_mat.glass_unused, used for extract vials
    Wood2 = 8, -- misc_mat.barrels, also used for buckets
    Bag = 9, -- cloth/leather
    Cage = 10, -- misc_mat.cages
    WeaponMelee = 11, -- metal.weapon
    WeaponRanged = 12, -- metal.ranged
    Ammo = 13, -- metal.ammo
    Ammo2 = 14, -- metal.ammo2
    Pick = 15, -- metal.pick
    Armor = 16, -- metal.armor, also used for shields, tools, instruments, and toys
    Gem = 17, -- gems
    Bone = 18, -- refuse.bone
    Shell = 19, -- refuse.shell
    Pearl = 20, -- refuse.pearl
    Ivory = 21, -- refuse.ivory
    Horn = 22, -- refuse.horn
    Other = 23, -- misc_mat.others
    Anvil = 24, -- metal.anvil
    Booze = 25, -- misc_mat.booze
    Metal = 26, -- metals with ITEMS_HARD, used for chains
    PlantFiber = 27, -- organic.fiber
    Silk = 28, -- organic.silk
    Wool = 29, -- organic.wool
    Furniture = 30, -- misc_mat.rock_metal
    MiscWood2 = 31, -- misc_mat.wood2
}

---@class entity_uniform_item
---@field random_dye integer
---@field armorlevel integer
---@field item_color integer
---@field art_image_id integer
---@field art_image_subid integer
---@field image_thread_color integer
---@field image_material_class entity_material_category
---@field maker_race integer
---@field indiv_choice bitfield
---@field mattype integer
---@field matindex integer
---@field material_class entity_material_category
df.entity_uniform_item = nil

---@class entity_uniform
---@field id integer
---@field unk_4 integer
---@field uniform_item_types any[]
---@field uniform_item_subtypes any[]
---@field uniform_item_info any[]
---@field name string
---@field flags bitfield
df.entity_uniform = nil

---@enum entity_event_type
df.entity_event_type = {
    invasion = 0,
    abduction = 1,
    incident = 2,
    occupation = 3,
    beast = 4,
    group = 5,
    harass = 6,
    flee = 7,
    abandon = 8,
    reclaimed = 9,
    founded = 10,
    reclaiming = 11,
    founding = 12,
    leave = 13,
    insurrection = 14,
    insurrection_end = 15,
    succession = 16,
    claim = 17,
    accept_tribute_offer = 18,
    refuse_tribute_offer = 19,
    accept_tribute_demand = 20,
    refuse_tribute_demand = 21,
    accept_peace_offer = 22,
    refuse_peace_offer = 23,
    cease_tribute_offer = 24,
    artifact_in_site = 25,
    artifact_in_subregion = 26,
    artifact_in_feature_layer = 27,
    artifact_in_inventory = 28,
    artifact_not_in_site = 29,
    artifact_not_in_subregion = 30,
    artifact_not_in_feature_layer = 31,
    artifact_not_in_inventory = 32,
    artifact_was_destroyed = 33,
}

---@class entity_event
---@field data invasion | abduction | incident | occupation | beast | group | harass | flee | abandon | reclaimed | founded | reclaiming | founding | leave | insurrection | insurrection_end | succession | claim | accept_tribute_offer | refuse_tribute_offer | accept_tribute_demand | refuse_tribute_demand | accept_peace_offer | refuse_peace_offer | cease_tribute_offer | artifact_in_site | artifact_in_subregion | artifact_in_feature_layer | artifact_in_inventory | artifact_not_in_site | artifact_not_in_subregion | artifact_not_in_feature_layer | artifact_not_in_inventory | artifact_destroyed
---@field unk_year integer often the same as the other year/tick. Start/stop time?
---@field unk_year_tick integer
---@field year integer
---@field year_tick integer
---@field unk_1 integer
---@field type entity_event_type
df.entity_event = nil

---@class agreement
---@field id integer
---@field parties agreement_party[]
---@field next_party_id integer
---@field details agreement_details[]
---@field next_details_id integer
---@field unk_1 integer
---@field unk_2 integer
---@field flags bitfield since v0.42.01
df.agreement = nil

---@class agreement_party
---@field id integer
---@field histfig_ids integer[]
---@field entity_ids integer[]
---@field unk_1 any[]
df.agreement_party = nil

---@enum crime_type
df.crime_type = {
    NONE = -1,
    Bribery = 0,
    BringIntoNetwork = 1,
    Corruption = 2,
    Embezzlement = 3,
}

---@enum agreement_details_type
df.agreement_details_type = {
    JoinParty = 0,
    DemonicBinding = 1,
    Residency = 2,
    Citizenship = 3,
    Parley = 4,
    PositionCorruption = 5, -- Embezzlement and accepting bribes seen. For own gain and for 'sponsor'
    PlotStealArtifact = 6,
    PromisePosition = 7,
    PlotAssassination = 8,
    PlotAbduct = 9,
    PlotSabotage = 10,
    PlotConviction = 11,
    Location = 12,
    PlotInfiltrationCoup = 13,
    PlotFrameTreason = 14,
    PlotInduceWar = 15,
}

---@class agreement_details
---@field id integer
---@field year integer
---@field year_tick integer
---@field data JoinParty | DemonicBinding | Residency | Citizenship | Parley | PositionCorruption | PlotStealArtifact | PromisePosition | PlotAssassination | PlotAbduct | PlotSabotage | PlotConviction | Location | PlotInfiltrationCoup | PlotFrameTreason | PlotInduceWar
---@field type agreement_details_type
df.agreement_details = nil

---@class agreement_details_data_join_party
---@field reason history_event_reason
---@field member integer
---@field party integer
---@field site integer
---@field entity integer
---@field figure integer this is a value_type when reason == sphere_alignment
---@field unk_v50_1 integer
---@field unk_v50_2 integer
df.agreement_details_data_join_party = nil

---@class agreement_details_data_demonic_binding
---@field reason history_event_reason
---@field demon integer
---@field summoner integer
---@field site integer
---@field artifact integer
---@field sphere sphere_type
df.agreement_details_data_demonic_binding = nil

---@class agreement_details_data_residency
---@field reason history_event_reason
---@field applicant integer
---@field government integer
---@field site integer
---@field unk_v50_1 integer
---@field unk_v50_2 integer
df.agreement_details_data_residency = nil

---@class agreement_details_data_citizenship
---@field applicant integer
---@field government integer
---@field site integer
---@field unk_v50_1 integer
---@field unk_v50_2 integer
df.agreement_details_data_citizenship = nil

---@class agreement_details_data_parley
---@field unk_1 integer
---@field party_id integer
---@field unk_v50_1 integer
---@field unk_v50_2 integer
---@field unk_v50_3 integer
---@field unk_v50_4 integer
df.agreement_details_data_parley = nil

---@class agreement_details_data_position_corruption
---@field unk_1 integer 247-249 seen
---@field actor_index integer agreement.parties index
---@field influencer_index integer agreement.parties index
---@field intermediary_index integer agreement.parties index
---@field target_id integer
---@field position_id integer position index in the entity's Own entity_position vector
df.agreement_details_data_position_corruption = nil

---@class agreement_details_data_plot_steal_artifact
---@field actor_index integer agreement.parties index
---@field influencer_index integer agreement.parties index
---@field intermediary_index integer agreement.parties index
---@field artifact_id integer
df.agreement_details_data_plot_steal_artifact = nil

---@class agreement_details_data_promise_position
---@field beneficiary_index integer agreement.parties index
---@field actor_index integer agreement.parties index
---@field promisee_index integer agreement.parties index
---@field influencer_index integer agreement.parties index. May be swapped with beneficiary
---@field intermediary_indices integer[] agreement.parties index
---@field entity_id integer
df.agreement_details_data_promise_position = nil

---@class agreement_details_data_plot_assassination
---@field actor_index integer agreement.parties index
---@field influencer_index integer agreement.parties index
---@field intermediary_index integer agreement.parties index
---@field target_id integer
df.agreement_details_data_plot_assassination = nil

---@class agreement_details_data_plot_abduct
---@field actor_index integer agreement.parties index
---@field intermediary_index integer agreement.parties index
---@field target_id integer
---@field unk_v50_1 integer
df.agreement_details_data_plot_abduct = nil

---@class agreement_details_data_plot_sabotage
---@field plotter_index integer agreement.parties index
---@field actor_index integer agreement.parties index
---@field intermediary_index integer agreement.parties index. A guess, as no intermediary cases have been seen
---@field victim_id integer
---@field unk_1 integer
---@field unk_2 integer
df.agreement_details_data_plot_sabotage = nil

---@class agreement_details_data_plot_conviction
---@field criminal_indices integer[] agreement.parties index. All indices listed, regardless of confessions
---@field crime crime_type
df.agreement_details_data_plot_conviction = nil

---@class agreement_details_data_location
---@field applicant integer
---@field government integer
---@field site integer
---@field type abstract_building_type
---@field deity_type temple_deity_type
---@field deity_data temple_deity_data
---@field profession profession
---@field tier integer 1 = temple or guildhall, 2 = temple complex or grand guildhall; matches location_tier in abstract_building_contents
---@field unk_v50_1 integer
df.agreement_details_data_location = nil

---@class agreement_details_data_plot_infiltration_coup
---@field actor_index integer agreement.parties index
---@field influencer_index integer agreement.parties index
---@field target integer action=8: site id, 9: entity id
---@field action integer 8 and 9 seen. Probably matches up with corresponding hist fig Infiltrate_Society action
df.agreement_details_data_plot_infiltration_coup = nil

---@class agreement_details_data_plot_frame_treason
---@field actor_index integer agreement.parties index
---@field influencer_index integer agreement.parties index
---@field victim_id integer
---@field fool_id integer
---@field unk_1 integer only same as fool_id seen, and so may be swapped. Guess it would be sentencer if different from fooled hf, though
df.agreement_details_data_plot_frame_treason = nil

---@class agreement_details_data_plot_induce_war
---@field actor_index integer agreement.parties index
---@field influencer_index integer agreement.parties index
---@field attacker integer
---@field defender integer
df.agreement_details_data_plot_induce_war = nil


