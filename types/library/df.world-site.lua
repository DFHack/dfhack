-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.

---@meta

---@class scribejob
---@field idmaybe integer
---@field unk_1 integer not locationid
---@field item_id integer
---@field written_content_id integer
---@field unit_id integer
---@field activity_entry_id integer
---@field unk_2 integer
df.scribejob = nil

---@class site_reputation_report
---@field site_id integer
---@field location_id integer
---@field unk_1 integer
---@field unk_2 integer
---@field year integer
---@field tickmaybe integer
---@field unk_3 integer[]
df.site_reputation_report = nil

---@class site_reputation_info
---@field reports site_reputation_report[]
df.site_reputation_info = nil

---@class location_scribe_jobs
---@field scribejobs scribejob[]
---@field nextidmaybe integer
---@field year integer
---@field unk_1 integer
---@field unk_2 integer
---@field unk_3 integer
---@field unk_4 integer
---@field unk_5 integer
df.location_scribe_jobs = nil

---@enum abstract_building_type
df.abstract_building_type = {
    MEAD_HALL = 0,
    KEEP = 1,
    TEMPLE = 2,
    DARK_TOWER = 3,
    MARKET = 4,
    TOMB = 5,
    DUNGEON = 6,
    UNDERWORLD_SPIRE = 7,
    INN_TAVERN = 8,
    LIBRARY = 9,
    COUNTING_HOUSE = 10,
    GUILDHALL = 11,
    TOWER = 12,
    HOSPITAL = 13,
}

---@enum abstract_building_flags
df.abstract_building_flags = {
    Unk0 = 0,
    Unk1 = 1,
    Unk2 = 2, -- gets toggled when an adventurer has visited it.
    Unk3 = 3,
    AllowVisitors = 4,
    AllowResidents = 5,
    OnlyMembers = 6,
    Unk7 = 7,
}

---@class abstract_building_entombed used within Tomb and Dungeon
---@field populations any[]
---@field histfigs integer[]
df.abstract_building_entombed = nil

---@class abstract_building_contents used within Temple, Library, and Inn/Tavern
---@field need_more bitfield
---@field profession profession since v0.47.01
---@field desired_goblets integer
---@field desired_instruments integer
---@field desired_paper integer
---@field desired_splints integer
---@field desired_thread integer times 15000
---@field desired_cloth integer times 10000
---@field desired_crutches integer
---@field desired_powder integer times 150
---@field desired_buckets integer
---@field desired_soap integer times 150
---@field desired_copies integer
---@field location_tier integer
---@field location_value integer
---@field count_goblets integer
---@field count_instruments integer
---@field count_paper integer
---@field count_splints integer
---@field count_thread integer
---@field count_cloth integer
---@field count_crutches integer
---@field count_powder integer
---@field count_buckets integer
---@field count_soap integer
---@field unk_v47_2 integer
---@field unk_v47_3 integer
---@field building_ids integer[]
df.abstract_building_contents = nil

---@return abstract_building_type
function df.abstract_building.getType() end

---@param tile integer
---@param fg integer
---@param bg integer
---@param bright integer
function df.abstract_building.getDisplayTile(tile, fg, bg, bright) end -- on navigation minimap

---@return language_name
function df.abstract_building.getName() end

---@return abstract_building_contents
function df.abstract_building.getContents() end

---@param file file_compressorst
function df.abstract_building.write_file(file) end

---@param file file_compressorst
---@param loadversion save_version
function df.abstract_building.read_file(file, loadversion) end

---@return integer
function df.abstract_building.getReligionID() end

---@return boolean
function df.abstract_building.unnamed_method() end -- returns false for dark tower, dungeon, keep, market, tomb, and underworld spire; true for all others

---@return boolean
function df.abstract_building.unnamed_method() end -- returns true for dark tower, tavern, keep, library, temple, and underworld spire; false for all others

---@return integer
function df.abstract_building.unnamed_method() end -- possibly related to importance; return values include 20, 50, 100

---@return abstract_building_entombed
function df.abstract_building.getEntombed() end

---@param arg_0 stl-fstream
---@param indent integer
function df.abstract_building.generate_xml(arg_0, indent) end

---@class abstract_building
---@field id integer
---@field inhabitants any[]
---@field flags abstract_building_flags[]
---@field unk1 integer in temples; hfig is the god
---@field unk2 integer[]
---@field parent_building_id integer Tombs use this to hold which catacomb they are part of.
---@field child_building_ids integer[] Used by catacombs to hold their tombs
---@field site_owner_id integer entity that constructed the building
---@field scribeinfo location_scribe_jobs since v0.42.01
---@field reputation_reports site_reputation_info since v0.42.01
---@field unk_v42_3 integer since v0.42.01
---@field site_id integer not initialized/saved/loaded, assumed member of base class
---@field pos coord2d since v0.42.01
---@field occupations occupation[] since v0.42.01
df.abstract_building = nil

---@class abstract_building_mead_hallst
-- inherited from abstract_building
---@field id integer
---@field inhabitants any[]
---@field flags abstract_building_flags[]
---@field unk1 integer in temples; hfig is the god
---@field unk2 integer[]
---@field parent_building_id integer Tombs use this to hold which catacomb they are part of.
---@field child_building_ids integer[] Used by catacombs to hold their tombs
---@field site_owner_id integer entity that constructed the building
---@field scribeinfo location_scribe_jobs since v0.42.01
---@field reputation_reports site_reputation_info since v0.42.01
---@field unk_v42_3 integer since v0.42.01
---@field site_id integer not initialized/saved/loaded, assumed member of base class
---@field pos coord2d since v0.42.01
---@field occupations occupation[] since v0.42.01
-- end abstract_building
---@field name language_name
---@field item1 site_building_item
---@field item2 site_building_item
df.abstract_building_mead_hallst = nil

---@class abstract_building_keepst
-- inherited from abstract_building
---@field id integer
---@field inhabitants any[]
---@field flags abstract_building_flags[]
---@field unk1 integer in temples; hfig is the god
---@field unk2 integer[]
---@field parent_building_id integer Tombs use this to hold which catacomb they are part of.
---@field child_building_ids integer[] Used by catacombs to hold their tombs
---@field site_owner_id integer entity that constructed the building
---@field scribeinfo location_scribe_jobs since v0.42.01
---@field reputation_reports site_reputation_info since v0.42.01
---@field unk_v42_3 integer since v0.42.01
---@field site_id integer not initialized/saved/loaded, assumed member of base class
---@field pos coord2d since v0.42.01
---@field occupations occupation[] since v0.42.01
-- end abstract_building
---@field name language_name
df.abstract_building_keepst = nil

---@enum temple_deity_type
df.temple_deity_type = {
    None = -1,
    Deity = 0,
    Religion = 1,
}

---@class temple_deity_data
---@field Deity integer
---@field Religion integer
df.temple_deity_data = nil

---@class abstract_building_templest
-- inherited from abstract_building
---@field id integer
---@field inhabitants any[]
---@field flags abstract_building_flags[]
---@field unk1 integer in temples; hfig is the god
---@field unk2 integer[]
---@field parent_building_id integer Tombs use this to hold which catacomb they are part of.
---@field child_building_ids integer[] Used by catacombs to hold their tombs
---@field site_owner_id integer entity that constructed the building
---@field scribeinfo location_scribe_jobs since v0.42.01
---@field reputation_reports site_reputation_info since v0.42.01
---@field unk_v42_3 integer since v0.42.01
---@field site_id integer not initialized/saved/loaded, assumed member of base class
---@field pos coord2d since v0.42.01
---@field occupations occupation[] since v0.42.01
-- end abstract_building
---@field deity_type temple_deity_type
---@field deity_data temple_deity_data
---@field name language_name
---@field contents abstract_building_contents
df.abstract_building_templest = nil

---@class abstract_building_dark_towerst
-- inherited from abstract_building
---@field id integer
---@field inhabitants any[]
---@field flags abstract_building_flags[]
---@field unk1 integer in temples; hfig is the god
---@field unk2 integer[]
---@field parent_building_id integer Tombs use this to hold which catacomb they are part of.
---@field child_building_ids integer[] Used by catacombs to hold their tombs
---@field site_owner_id integer entity that constructed the building
---@field scribeinfo location_scribe_jobs since v0.42.01
---@field reputation_reports site_reputation_info since v0.42.01
---@field unk_v42_3 integer since v0.42.01
---@field site_id integer not initialized/saved/loaded, assumed member of base class
---@field pos coord2d since v0.42.01
---@field occupations occupation[] since v0.42.01
-- end abstract_building
---@field name language_name
df.abstract_building_dark_towerst = nil

---@class abstract_building_marketst
-- inherited from abstract_building
---@field id integer
---@field inhabitants any[]
---@field flags abstract_building_flags[]
---@field unk1 integer in temples; hfig is the god
---@field unk2 integer[]
---@field parent_building_id integer Tombs use this to hold which catacomb they are part of.
---@field child_building_ids integer[] Used by catacombs to hold their tombs
---@field site_owner_id integer entity that constructed the building
---@field scribeinfo location_scribe_jobs since v0.42.01
---@field reputation_reports site_reputation_info since v0.42.01
---@field unk_v42_3 integer since v0.42.01
---@field site_id integer not initialized/saved/loaded, assumed member of base class
---@field pos coord2d since v0.42.01
---@field occupations occupation[] since v0.42.01
-- end abstract_building
---@field name language_name
df.abstract_building_marketst = nil

---@class abstract_building_tombst
-- inherited from abstract_building
---@field id integer
---@field inhabitants any[]
---@field flags abstract_building_flags[]
---@field unk1 integer in temples; hfig is the god
---@field unk2 integer[]
---@field parent_building_id integer Tombs use this to hold which catacomb they are part of.
---@field child_building_ids integer[] Used by catacombs to hold their tombs
---@field site_owner_id integer entity that constructed the building
---@field scribeinfo location_scribe_jobs since v0.42.01
---@field reputation_reports site_reputation_info since v0.42.01
---@field unk_v42_3 integer since v0.42.01
---@field site_id integer not initialized/saved/loaded, assumed member of base class
---@field pos coord2d since v0.42.01
---@field occupations occupation[] since v0.42.01
-- end abstract_building
---@field name language_name
---@field entombed abstract_building_entombed
---@field precedence integer
df.abstract_building_tombst = nil

---@enum abstract_building_dungeonst_dungeon_type
df.abstract_building_dungeonst_dungeon_type = {
    DUNGEON = 0,
    SEWERS = 1,
    CATACOMBS = 2,
}
---@class abstract_building_dungeonst
-- inherit abstract_building
---@field name language_name
---@field abstract_building_dungeonst_dungeon_type abstract_building_dungeonst_dungeon_type
---@field unk_1 integer
---@field entombed abstract_building_entombed
---@field unk_2 integer
---@field unk_3 integer not saved
---@field unk_4 integer not saved
df.abstract_building_dungeonst = nil

---@class abstract_building_underworld_spirest
-- inherited from abstract_building
---@field id integer
---@field inhabitants any[]
---@field flags abstract_building_flags[]
---@field unk1 integer in temples; hfig is the god
---@field unk2 integer[]
---@field parent_building_id integer Tombs use this to hold which catacomb they are part of.
---@field child_building_ids integer[] Used by catacombs to hold their tombs
---@field site_owner_id integer entity that constructed the building
---@field scribeinfo location_scribe_jobs since v0.42.01
---@field reputation_reports site_reputation_info since v0.42.01
---@field unk_v42_3 integer since v0.42.01
---@field site_id integer not initialized/saved/loaded, assumed member of base class
---@field pos coord2d since v0.42.01
---@field occupations occupation[] since v0.42.01
-- end abstract_building
---@field name language_name
---@field unk_bc integer
df.abstract_building_underworld_spirest = nil

---@class abstract_building_inn_tavernst
-- inherited from abstract_building
---@field id integer
---@field inhabitants any[]
---@field flags abstract_building_flags[]
---@field unk1 integer in temples; hfig is the god
---@field unk2 integer[]
---@field parent_building_id integer Tombs use this to hold which catacomb they are part of.
---@field child_building_ids integer[] Used by catacombs to hold their tombs
---@field site_owner_id integer entity that constructed the building
---@field scribeinfo location_scribe_jobs since v0.42.01
---@field reputation_reports site_reputation_info since v0.42.01
---@field unk_v42_3 integer since v0.42.01
---@field site_id integer not initialized/saved/loaded, assumed member of base class
---@field pos coord2d since v0.42.01
---@field occupations occupation[] since v0.42.01
-- end abstract_building
---@field name language_name
---@field contents abstract_building_contents
---@field room_info any[]
---@field next_room_info_id integer
df.abstract_building_inn_tavernst = nil

---@class abstract_building_libraryst
-- inherited from abstract_building
---@field id integer
---@field inhabitants any[]
---@field flags abstract_building_flags[]
---@field unk1 integer in temples; hfig is the god
---@field unk2 integer[]
---@field parent_building_id integer Tombs use this to hold which catacomb they are part of.
---@field child_building_ids integer[] Used by catacombs to hold their tombs
---@field site_owner_id integer entity that constructed the building
---@field scribeinfo location_scribe_jobs since v0.42.01
---@field reputation_reports site_reputation_info since v0.42.01
---@field unk_v42_3 integer since v0.42.01
---@field site_id integer not initialized/saved/loaded, assumed member of base class
---@field pos coord2d since v0.42.01
---@field occupations occupation[] since v0.42.01
-- end abstract_building
---@field name language_name
---@field copied_artifacts integer[]
---@field unk_1 integer
---@field unk_2 integer
---@field unk_3 integer
---@field unk_4 integer
---@field contents abstract_building_contents
df.abstract_building_libraryst = nil

---@class abstract_building_counting_housest
-- inherited from abstract_building
---@field id integer
---@field inhabitants any[]
---@field flags abstract_building_flags[]
---@field unk1 integer in temples; hfig is the god
---@field unk2 integer[]
---@field parent_building_id integer Tombs use this to hold which catacomb they are part of.
---@field child_building_ids integer[] Used by catacombs to hold their tombs
---@field site_owner_id integer entity that constructed the building
---@field scribeinfo location_scribe_jobs since v0.42.01
---@field reputation_reports site_reputation_info since v0.42.01
---@field unk_v42_3 integer since v0.42.01
---@field site_id integer not initialized/saved/loaded, assumed member of base class
---@field pos coord2d since v0.42.01
---@field occupations occupation[] since v0.42.01
-- end abstract_building
---@field name language_name
df.abstract_building_counting_housest = nil

---@class abstract_building_guildhallst
-- inherited from abstract_building
---@field id integer
---@field inhabitants any[]
---@field flags abstract_building_flags[]
---@field unk1 integer in temples; hfig is the god
---@field unk2 integer[]
---@field parent_building_id integer Tombs use this to hold which catacomb they are part of.
---@field child_building_ids integer[] Used by catacombs to hold their tombs
---@field site_owner_id integer entity that constructed the building
---@field scribeinfo location_scribe_jobs since v0.42.01
---@field reputation_reports site_reputation_info since v0.42.01
---@field unk_v42_3 integer since v0.42.01
---@field site_id integer not initialized/saved/loaded, assumed member of base class
---@field pos coord2d since v0.42.01
---@field occupations occupation[] since v0.42.01
-- end abstract_building
---@field name language_name
---@field contents abstract_building_contents
df.abstract_building_guildhallst = nil

---@class abstract_building_towerst
-- inherited from abstract_building
---@field id integer
---@field inhabitants any[]
---@field flags abstract_building_flags[]
---@field unk1 integer in temples; hfig is the god
---@field unk2 integer[]
---@field parent_building_id integer Tombs use this to hold which catacomb they are part of.
---@field child_building_ids integer[] Used by catacombs to hold their tombs
---@field site_owner_id integer entity that constructed the building
---@field scribeinfo location_scribe_jobs since v0.42.01
---@field reputation_reports site_reputation_info since v0.42.01
---@field unk_v42_3 integer since v0.42.01
---@field site_id integer not initialized/saved/loaded, assumed member of base class
---@field pos coord2d since v0.42.01
---@field occupations occupation[] since v0.42.01
-- end abstract_building
---@field name language_name
---@field unk_1 integer
df.abstract_building_towerst = nil

---@class abstract_building_hospitalst
-- inherited from abstract_building
---@field id integer
---@field inhabitants any[]
---@field flags abstract_building_flags[]
---@field unk1 integer in temples; hfig is the god
---@field unk2 integer[]
---@field parent_building_id integer Tombs use this to hold which catacomb they are part of.
---@field child_building_ids integer[] Used by catacombs to hold their tombs
---@field site_owner_id integer entity that constructed the building
---@field scribeinfo location_scribe_jobs since v0.42.01
---@field reputation_reports site_reputation_info since v0.42.01
---@field unk_v42_3 integer since v0.42.01
---@field site_id integer not initialized/saved/loaded, assumed member of base class
---@field pos coord2d since v0.42.01
---@field occupations occupation[] since v0.42.01
-- end abstract_building
---@field name language_name
---@field contents abstract_building_contents
df.abstract_building_hospitalst = nil

---@enum world_site_type
df.world_site_type = {
    PlayerFortress = 0,
    DarkFortress = 1,
    Cave = 2,
    MountainHalls = 3,
    ForestRetreat = 4,
    Town = 5,
    ImportantLocation = 6,
    LairShrine = 7,
    Fortress = 8,
    Camp = 9,
    Monument = 10,
}

---@enum world_site_flags
df.world_site_flags = {
    Undiscovered = 0,
    unk_1 = 1,
    unk_2 = 2,
    Town = 3, -- not hamlet
    unk_4 = 4,
    unk_5 = 5,
    unk_6 = 6,
    unk_7 = 7,
    unk_8 = 8,
    CaveCapital = 9, -- set on caves (only) that have capital entity links, i.e. Kobold civs in vanilla
    unk_10 = 10,
}

---@enum fortress_type
df.fortress_type = {
    NONE = -1,
    CASTLE = 0,
    TOWER = 1,
    MONASTERY = 2,
    FORT = 3,
}

---@enum monument_type
df.monument_type = {
    NONE = -1,
    TOMB = 0,
    VAULT = 1,
}

---@enum lair_type
df.lair_type = {
    NONE = -1,
    SIMPLE_MOUND = 0, -- Night creatures
    SIMPLE_BURROW = 1, -- animal, (semi)megabeast, night creature(!)
    LABYRINTH = 2,
    SHRINE = 3,
    WILDERNESS_LOCATION = 4, -- In mountains, hosting Rocs in vanilla
}

---@class property_ownership
---@field index integer
---@field is_concrete_property boolean true if house [property_index = 4 only one seen], or index into buildings
---@field pad_1 integer
---@field property_index integer index into buildings when is_concrete_property is false. Only seen 4 = house with is_concrete_property = true
---@field unk_hfid integer Always same as owner_hfid when set, but not always set when that field is.
---@field owner_entity_id integer Mutually exclusive with owner_hfid. All seen were merchant companies.
---@field owner_hfid integer
---@field unk_owner_entity_id integer Seen only in subset of owner_entity_id case, and always same value
df.property_ownership = nil

---@class world_site_unk_1
---@field nemesis integer[]
---@field artifacts artifact_record[]
---@field animals world_population[]
---@field inhabitants world_site_inhabitant[]
---@field units any[]
---@field unk_d4 integer[]
---@field unk_v40_1a historical_figure[] since v0.40.01
---@field pad_1 integer
---@field unk_v40_1b nemesis_record[] since v0.40.01
---@field unk_v40_1c nemesis_record[] since v0.40.01
---@field unk_v40_1d nemesis_record[] since v0.40.01
---@field unk_v40_1e nemesis_record[] since v0.40.01
---@field unk_v40_1f nemesis_record[] since v0.40.01
---@field unk_v40_1g nemesis_record[] since v0.40.01
---@field unk_v40_1h nemesis_record[] since v0.40.01

---@class world_site
---@field name language_name
---@field civ_id integer
---@field cur_owner_id integer
---@field type world_site_type
---@field pos coord2d
---@field id integer
---@field unk_1 world_site_unk_1
---@field index integer
---@field rgn_min_x integer in embark tiles
---@field rgn_max_x integer
---@field rgn_min_y integer
---@field rgn_max_y integer
---@field rgn_min_z integer
---@field rgn_max_z integer
---@field global_min_x integer in embark tiles
---@field global_min_y integer
---@field global_max_x integer
---@field global_max_y integer
---@field seed1 integer random
---@field seed2 integer random
---@field resident_count integer count living in houses and shops
---@field unk_110 integer
---@field unk_114 integer
---@field unk_118 integer
---@field unk_11c integer Caves have non zero numbers. No others.
---@field unk_120 integer Subset of caves can have non zero.
---@field unk_124 integer Monument 0, LairShrine 5, Camp 20, others varying
---@field unk_128 integer  "site_level" is in here somewhere. Same as for unk_124, but varying ones always less/equal
---@field unk_2 integer[] Has all zero for Fortress, Camp, PlayerFortress, Monument, and LairShrine. Cave can have value, while DarkFortress, MountainHalls, ForestRetreat and Town all have at least one non zero value
---@field unk_13c any[] MountainHall, Town, DarkFortress, but not all
---@field unk_v40_2 any[] forest retreat, since v0.40.01
---@field unk_v47_1 any[] Varying types of habitation can have this. It seems new elements are added to hold all required data as all are full except the last one, since v0.47.01
---@field flags world_site_flags[]
---@field buildings abstract_building[]
---@field next_building_id integer
---@field property_ownership property_ownership[] since v0.47.01
---@field next_property_ownership_id integer since v0.47.01
---@field created_tick integer
---@field created_year integer
---@field unk_170 integer constant 0
---@field unk_174 integer constant 0
---@field unk_178 coord
---@field realization world_site_realization
---@field subtype_info integer
---@field unk_21c any[] since v0.34.01
---@field deaths integer[] killed by rampaging monster, murder, execution, old age seen. Note that most HFs seem to have been culled, since v0.34.01
---@field is_mountain_halls integer since v0.40.01
---@field is_fortress integer since v0.40.01
---@field unk_v47_2 integer only MountainHalls, but only subset of them
---@field unk_v40_4a any[] since v0.40.01
---@field unk_v40_4b any[] since v0.40.01
---@field unk_v40_4c any[] since v0.40.01
---@field unk_v40_4d any[] only seen once, 13 long, corresponding to 13 attacks from the same entity_id resulting in site taken over in 'might bey year', since v0.40.01
---@field unk_v40_4d_next_id integer only single non zero entry, matching vector above. Might guess 'since' is scrambled, since v0.43.01
---@field unk_v43_2 any[] since v0.43.01
---@field unk_v43_3 integer constant 0?, since v0.43.01
---@field unk_v40_5 integer constant -1?, since v0.40.01
---@field unk_188 integer Seen monster in lair, first settler in site, killed defender in site, artifact created in player fortress, (player) created artifact claimed by villain for unrelated cave/villain settled in cave
---@field unk_3a8 integer since v0.44.06
---@field unk_3b0 world_site_unk130 since v0.44.01
---@field unk_18c any[]
---@field unk_19c any[]
---@field entity_links entity_site_link[] since v0.40.01
---@field cultural_identities cultural_identity[] since v0.40.01
---@field unk_v42_1 occupation[] since v0.42.01
---@field unk_v43_4 integer uninitialized, since v0.43.01
---@field unk_3 any[] since v0.44.01
---@field unk_4 historical_figure
---@field unk_5 historical_figure
---@field unk_6 historical_figure
---@field unk_7 historical_figure
---@field unk_8 historical_figure
---@field unk_9 integer
---@field unk_10 integer
---@field unk_11 integer
---@field unk_12 integer
---@field unk_13 integer
---@field unk_14 integer
---@field unk_15 integer
---@field unk_16 integer
---@field unk_17 integer
---@field unk_18 integer
---@field unk_19 integer
---@field unk_20 integer
---@field unk_21 integer
---@field unk_22 integer
---@field unk_23 integer
---@field unk_24 integer
---@field unk_25 integer
df.world_site = nil

---@class cultural_identity
---@field id integer
---@field site_id integer
---@field civ_id integer
---@field group_log any[] the circumstances of groups joining or leaving this culture
---@field ethic any[]
---@field values integer[]
---@field events entity_event[]
---@field unk_d8 integer
---@field unk_dc integer[]
---@field unk_ec integer
---@field unk_f0 integer
---@field unk_f4 integer 0 or 800000
---@field unk_1 any[]
---@field unk_2 any[]
---@field unk_f8 integer
df.cultural_identity = nil

---@class world_site_inhabitant
---@field count integer
---@field race integer
---@field population_id integer
---@field entity_id integer can be Religion, Civilization, and SiteGovernment as well as Outcast
---@field unk_10 integer since v0.40.01
---@field cultural_identity_id integer since v0.40.01
---@field interaction_id integer since v0.40.01
---@field interaction_effect_idx integer index into the above interaction, usually refers to an ANIMATE effect, since v0.40.01
---@field related_entity_id integer Founder if outcast_id=-1, else Outcast and equal to outcast_id, since v0.40.01
---@field unk_24 integer 0 and 1 seen, since v0.40.01
---@field unk_28 integer since v0.40.01
df.world_site_inhabitant = nil

---@class world_site_realization
---@field buildings site_realization_building[]
---@field num_buildings integer
---@field unk_14 integer
---@field num_areas integer
---@field mini_rivers any[]
---@field mini_tiles any[]
---@field mini_colors any[]
---@field road_map any[]
---@field river_map any[]
---@field unk_55e8 any[]
---@field building_map any[]
---@field flags_map any[] since v0.40.01
---@field zoom_tiles any[]
---@field zoom_colors any[]
---@field zoom_movemask any[]
---@field area_map any[]
---@field areas any[]
---@field unk_1 integer
---@field army_controller_pos_x integer
---@field army_controller_pos_y integer
---@field unk_193bc any[]
---@field num_unk_193bc integer
---@field unk_2 integer
---@field unk_3 integer
---@field unk_4 any[] since v0.47.01
---@field unk_5 integer since v0.47.01
---@field unk_6 integer[]
---@field unk_7 integer
---@field unk_8 integer[]
---@field unk_9 integer
---@field unk_10 integer[]
---@field unk_11 integer
---@field unk_12 integer[]
---@field unk_13 integer
---@field unk_15 integer[]
---@field unk_16 integer
---@field unk_17 integer[]
---@field unk_18 integer
---@field unk_19 integer[]
---@field unk_20 integer
---@field unk_21 integer[]
---@field unk_22 integer
---@field building_well site_realization_building[]
---@field num_building_well integer
---@field building_temple site_realization_building[]
---@field num_building_temple integer
---@field building_type22 site_realization_building[]
---@field num_building_type22 integer
---@field building_type21 site_realization_building[]
---@field num_building_type21 integer
---@field unk_23 integer[]
---@field unk_24 integer
---@field unk_wsr_vector any[]
df.world_site_realization = nil

---@class site_realization_crossroads
---@field road_min_y integer[]
---@field road_max_y integer[]
---@field road_min_x integer[]
---@field road_max_x integer[]
---@field idx_x integer
---@field idx_y integer
---@field tile_width integer
---@field tile_height integer
---@field unk_310 integer
---@field unk_314 integer
---@field unk_318 integer
---@field unk_31c integer
---@field unk_320 integer
---@field unk_324 integer
---@field unk_328 integer
---@field unk_32c integer
---@field center_x_tile integer
---@field center_y_tile integer
---@field up site_realization_crossroads
---@field down site_realization_crossroads
---@field right site_realization_crossroads
---@field left site_realization_crossroads
---@field unk_348 integer
---@field unk_349 integer
---@field unk_34c integer
---@field unk_1 integer
---@field unk_2 integer
---@field unk_3 integer
---@field unk_4 integer
---@field unk_5 integer
---@field unk_6 integer
---@field unk_7 integer since v0.47.01
---@field unk_8 integer since v0.47.01
---@field unk_370 integer[]
---@field unk_3d0 integer[]
df.site_realization_crossroads = nil

---@enum site_realization_building_type
df.site_realization_building_type = {
    cottage_plot = 0,
    castle_wall = 1,
    castle_tower = 2,
    castle_courtyard = 3,
    house = 4,
    temple = 5,
    tomb = 6,
    shop_house = 7,
    warehouse = 8,
    market_square = 9,
    pasture = 10,
    waste = 11,
    courtyard = 12,
    well = 13,
    vault = 14,
    great_tower = 15,
    trenches = 16,
    tree_house = 17,
    hillock_house = 18,
    mead_hall = 19,
    fortress_entrance = 20,
    library = 21,
    tavern = 22,
    counting_house = 23,
    guild_hall = 24,
    city_tower = 25,
    shrine = 26,
    unk_27 = 27,
    dormitory = 28,
    dininghall = 29,
    necromancer_tower = 30,
    barrow = 31,
}

---@class site_realization_building
---@field id integer
---@field type site_realization_building_type
---@field min_x integer in tiles relative to site
---@field min_y integer
---@field max_x integer
---@field max_y integer
---@field unk_18 integer
---@field inhabitants world_site_inhabitant[]
---@field unk_2c integer
---@field item site_building_item
---@field abstract_building_id integer used for temple and mead hall
---@field unk_44 integer
---@field building_info site_realization_building_infost
---@field unk_4c any[]
---@field unk_5c integer bit 0x01 == abandoned
---@field unk_60 any[]
---@field unk_v40_1 integer since v0.40.01
df.site_realization_building = nil

---@return site_realization_building_type
function df.site_realization_building_infost.getType() end

---@param file file_compressorst
function df.site_realization_building_infost.write_file(file) end

---@param file file_compressorst
---@param loadversion save_version
function df.site_realization_building_infost.read_file(file, loadversion) end

---@class site_realization_building_infost
df.site_realization_building_infost = nil

---@class site_building_item
---@field race integer
---@field item_type item_type
---@field item_subtype integer
---@field mat_type integer
---@field mat_index integer
df.site_building_item = nil

---@alias tower_shape bitfield

---@class site_realization_building_info_castle_wallst
-- inherited from site_realization_building_infost
-- end site_realization_building_infost
---@field length integer
---@field door_pos integer
---@field start_x integer
---@field start_y integer
---@field start_z integer
---@field end_x integer
---@field end_y integer
---@field end_z integer
---@field wall_item site_building_item
---@field door_item site_building_item
df.site_realization_building_info_castle_wallst = nil

---@class site_realization_building_info_castle_towerst
-- inherited from site_realization_building_infost
-- end site_realization_building_infost
---@field roof_z integer
---@field base_z integer can be below ground, but not above ground
---@field door_n_elevation integer
---@field door_s_elevation integer
---@field door_e_elevation integer
---@field door_w_elevation integer
---@field door_item site_building_item
---@field wall_item site_building_item
---@field shape bitfield
---@field unk_40 integer
---@field unk_44 integer
df.site_realization_building_info_castle_towerst = nil

---@class site_realization_building_info_castle_courtyardst
-- inherited from site_realization_building_infost
-- end site_realization_building_infost
df.site_realization_building_info_castle_courtyardst = nil

---@enum site_shop_type
df.site_shop_type = {
    GeneralImports = 0,
    FoodImports = 1,
    ClothingImports = 2,
    Cloth = 3,
    Leather = 4,
    WovenClothing = 5,
    LeatherClothing = 6,
    BoneCarver = 7,
    GemCutter = 8,
    Weaponsmith = 9,
    Bowyer = 10,
    Blacksmith = 11,
    Armorsmith = 12,
    MetalCraft = 13,
    LeatherGoods = 14,
    Carpenter = 15,
    StoneFurniture = 16,
    MetalFurniture = 17,
    ImportedGoodsMarket = 18,
    ImportedFoodMarket = 19,
    ImportedClothingMarket = 20,
    MeatMarket = 21,
    FruitAndVegetableMarket = 22,
    CheeseMarket = 23,
    ProcessedGoodsMarket = 24,
    Tavern = 25,
}

---@enum town_labor_type
df.town_labor_type = {
    NONE = -1,
    CLOTH = 0,
    TANNING = 1,
    CLOTHING_CLOTH = 2,
    CLOTHING_LEATHER = 3,
    CRAFTS_BONE_CARVER = 4,
    GEM_CUTTER = 5,
    METAL_WEAPON_SMITH = 6,
    WOOD_WEAPON_SMITH = 7,
    BLACK_SMITH = 8,
    METAL_ARMOR_SMITH = 9,
    METAL_CRAFTER = 10,
    LEATHER_ACCESSORIES = 11,
    FURNITURE_WOOD = 12,
    FURNITURE_STONE = 13,
    FURNITURE_METAL = 14,
}

---@class site_realization_building_info_shop_housest
-- inherited from site_realization_building_infost
-- end site_realization_building_infost
---@field type site_shop_type
---@field name language_name
df.site_realization_building_info_shop_housest = nil

---@class site_realization_building_info_market_squarest
-- inherited from site_realization_building_infost
-- end site_realization_building_infost
---@field type site_shop_type
df.site_realization_building_info_market_squarest = nil

---@class site_realization_building_info_trenchesst
-- inherited from site_realization_building_infost
-- end site_realization_building_infost
---@field unk_4 integer
---@field spokes any[] N, S, E, W
df.site_realization_building_info_trenchesst = nil

---@enum tree_house_type
df.tree_house_type = {
    TreeHouse = 0,
    HomeTree = 1,
    ShapingTree = 2,
    MarketTree = 3,
    Unknown1 = 4,
    Unknown2 = 5,
}

---@class site_realization_building_info_tree_housest
-- inherited from site_realization_building_infost
-- end site_realization_building_infost
---@field type tree_house_type
---@field unk_8 integer
---@field name language_name
df.site_realization_building_info_tree_housest = nil

---@enum hillock_house_type
df.hillock_house_type = {
    unk_0 = 0,
    CivicMound = 1,
    CastleMound = 2,
    DrinkingMound = 3,
}

---@class site_realization_building_info_hillock_housest
-- inherited from site_realization_building_infost
-- end site_realization_building_infost
---@field type hillock_house_type
df.site_realization_building_info_hillock_housest = nil

---@class site_realization_building_info_shrinest
-- inherited from site_realization_building_infost
-- end site_realization_building_infost
---@field unk_1 integer
---@field unk_2 integer
df.site_realization_building_info_shrinest = nil

---@enum creation_zone_pwg_alteration_type
df.creation_zone_pwg_alteration_type = {
    location_death = 0,
    camp = 1,
    srb_ruined = 2,
    srp_ruined = 3,
}

---@return creation_zone_pwg_alteration_type
function df.creation_zone_pwg_alterationst.getType() end

---@param file file_compressorst
function df.creation_zone_pwg_alterationst.write_file(file) end

---@param file file_compressorst
---@param loadversion save_version
function df.creation_zone_pwg_alterationst.read_file(file, loadversion) end

---@param arg_0 integer
function df.creation_zone_pwg_alterationst.unnamed_method(arg_0) end

---@return integer
function df.creation_zone_pwg_alterationst.unnamed_method() end

---@class creation_zone_pwg_alterationst
---@field unk_0 integer
df.creation_zone_pwg_alterationst = nil

---@class creation_zone_pwg_alteration_location_deathst_unk_1
---@field unk_1a any[]
---@field unk_2a integer[]

---@class creation_zone_pwg_alteration_location_deathst
-- inherited from creation_zone_pwg_alterationst
---@field unk_0 integer
-- end creation_zone_pwg_alterationst
---@field unk_1 creation_zone_pwg_alteration_location_deathst_unk_1
---@field unk_2 integer
df.creation_zone_pwg_alteration_location_deathst = nil

---@class creation_zone_pwg_alteration_campst
-- inherited from creation_zone_pwg_alterationst
---@field unk_0 integer
-- end creation_zone_pwg_alterationst
---@field unk_1 integer
---@field x1 integer
---@field y1 integer
---@field x2 integer
---@field y2 integer
---@field unk_2 integer
---@field unk_3 integer
---@field unk_4 integer
---@field unk_5 integer
---@field unk_6 integer
---@field unk_7 integer
df.creation_zone_pwg_alteration_campst = nil

---@class creation_zone_pwg_alteration_srb_ruinedst
-- inherited from creation_zone_pwg_alterationst
---@field unk_0 integer
-- end creation_zone_pwg_alterationst
---@field site_id integer
---@field building_id integer
df.creation_zone_pwg_alteration_srb_ruinedst = nil

---@class creation_zone_pwg_alteration_srp_ruinedst
-- inherited from creation_zone_pwg_alterationst
---@field unk_0 integer
-- end creation_zone_pwg_alterationst
---@field unk_1 integer
---@field unk_2 integer
df.creation_zone_pwg_alteration_srp_ruinedst = nil


