-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.

---@meta

---@enum general_ref_type
df.general_ref_type = {
    ARTIFACT = 0,
    IS_ARTIFACT = 1,
    NEMESIS = 2,
    IS_NEMESIS = 3,
    ITEM = 4,
    ITEM_TYPE = 5,
    COINBATCH = 6,
    MAPSQUARE = 7,
    ENTITY_ART_IMAGE = 8,
    CONTAINS_UNIT = 9,
    CONTAINS_ITEM = 10,
    CONTAINED_IN_ITEM = 11,
    PROJECTILE = 12,
    UNIT = 13,
    UNIT_MILKEE = 14,
    UNIT_TRAINEE = 15,
    UNIT_ITEMOWNER = 16,
    UNIT_TRADEBRINGER = 17,
    UNIT_HOLDER = 18,
    UNIT_WORKER = 19,
    UNIT_CAGEE = 20,
    UNIT_BEATEE = 21,
    UNIT_FOODRECEIVER = 22,
    UNIT_KIDNAPEE = 23,
    UNIT_PATIENT = 24,
    UNIT_INFANT = 25,
    UNIT_SLAUGHTEREE = 26,
    UNIT_SHEAREE = 27,
    UNIT_SUCKEE = 28,
    UNIT_REPORTEE = 29,
    BUILDING = 30,
    BUILDING_CIVZONE_ASSIGNED = 31,
    BUILDING_TRIGGER = 32,
    BUILDING_TRIGGERTARGET = 33,
    BUILDING_CHAIN = 34,
    BUILDING_CAGED = 35,
    BUILDING_HOLDER = 36,
    BUILDING_WELL_TAG = 37,
    BUILDING_USE_TARGET_1 = 38,
    BUILDING_USE_TARGET_2 = 39,
    BUILDING_DESTINATION = 40,
    BUILDING_NEST_BOX = 41,
    ENTITY = 42,
    ENTITY_STOLEN = 43,
    ENTITY_OFFERED = 44,
    ENTITY_ITEMOWNER = 45,
    LOCATION = 46,
    INTERACTION = 47,
    ABSTRACT_BUILDING = 48,
    HISTORICAL_EVENT = 49,
    SPHERE = 50,
    SITE = 51,
    SUBREGION = 52,
    FEATURE_LAYER = 53,
    HISTORICAL_FIGURE = 54,
    ENTITY_POP = 55,
    CREATURE = 56,
    UNIT_RIDER = 57,
    UNIT_CLIMBER = 58,
    UNIT_GELDEE = 59,
    KNOWLEDGE_SCHOLAR_FLAG = 60,
    ACTIVITY_EVENT = 61,
    VALUE_LEVEL = 62,
    LANGUAGE = 63,
    WRITTEN_CONTENT = 64,
    POETIC_FORM = 65,
    MUSICAL_FORM = 66,
    DANCE_FORM = 67,
    BUILDING_DISPLAY_FURNITURE = 68,
    UNIT_INTERROGATEE = 69,
}

---@param file file_compressorst
function df.general_ref.write_file(file) end

---@param file file_compressorst
---@param loadversion save_version
function df.general_ref.read_file(file, loadversion) end

---@return general_ref_type
function df.general_ref.getType() end

---@return item
function df.general_ref.getItem() end

---@return unit
function df.general_ref.getUnit() end

---@return projectile
function df.general_ref.getProjectile() end

---@return building
function df.general_ref.getBuilding() end

---@return historical_entity
function df.general_ref.getEntity() end

---@return artifact_record
function df.general_ref.getArtifact() end

---@return nemesis_record
function df.general_ref.getNemesis() end

---@return activity_event
function df.general_ref.getEvent() end

---@param arg_0 integer
function df.general_ref.setID(arg_0) end

---@return integer
function df.general_ref.getID() end

---@param x integer
---@param y integer
---@param z integer
function df.general_ref.setLocation(x, y, z) end

---@param out_x integer
---@param out_y integer
---@param out_z integer
function df.general_ref.getLocation(out_x, out_y, out_z) end

---@return general_ref
function df.general_ref.clone() end

---@param arg_0 integer
---@param arg_1 string
function df.general_ref.unnamed_method(arg_0, arg_1) end

---@param str string
function df.general_ref.getDescription(str) end

---@param str string
function df.general_ref.getDescription2(str) end

---@param str string
function df.general_ref.getDescription3(str) end -- for scholar flags

---@class general_ref
df.general_ref = nil

---@class general_ref_artifact
-- inherited from general_ref
-- end general_ref
---@field artifact_id integer
df.general_ref_artifact = nil

---@class general_ref_nemesis
-- inherited from general_ref
-- end general_ref
---@field nemesis_id integer
df.general_ref_nemesis = nil

---@class general_ref_item
-- inherited from general_ref
-- end general_ref
---@field item_id integer
---@field cached_index integer lookup optimization, tries before binary search
df.general_ref_item = nil

---@class general_ref_item_type
-- inherited from general_ref
-- end general_ref
---@field type item_type
---@field subtype integer
---@field mat_type integer
---@field mat_index integer
df.general_ref_item_type = nil

---@class general_ref_coinbatch
-- inherited from general_ref
-- end general_ref
---@field batch integer
df.general_ref_coinbatch = nil

---@class general_ref_mapsquare
-- inherited from general_ref
-- end general_ref
---@field tiletype tiletype
---@field mat_type integer
---@field mat_index integer
df.general_ref_mapsquare = nil

---@class general_ref_entity_art_image
-- inherited from general_ref
-- end general_ref
---@field entity_id integer
---@field index integer lookup in entity.resources.art_image_*
df.general_ref_entity_art_image = nil

---@class general_ref_projectile
-- inherited from general_ref
-- end general_ref
---@field projectile_id integer
df.general_ref_projectile = nil

---@class general_ref_unit
-- inherited from general_ref
-- end general_ref
---@field unit_id integer
---@field cached_index integer lookup optimization, tries before binary search
df.general_ref_unit = nil

---@class general_ref_building
-- inherited from general_ref
-- end general_ref
---@field building_id integer
df.general_ref_building = nil

---@class general_ref_entity
-- inherited from general_ref
-- end general_ref
---@field entity_id integer
df.general_ref_entity = nil

---@class general_ref_locationst
-- inherited from general_ref
-- end general_ref
---@field x integer
---@field y integer
---@field z integer
df.general_ref_locationst = nil

---@class general_ref_interactionst
-- inherited from general_ref
-- end general_ref
---@field interaction_id integer
---@field source_id integer
---@field unk_08 integer
---@field unk_0c integer
df.general_ref_interactionst = nil

---@class general_ref_abstract_buildingst
-- inherited from general_ref
-- end general_ref
---@field site_id integer
---@field building_id integer
df.general_ref_abstract_buildingst = nil

---@class general_ref_historical_eventst
-- inherited from general_ref
-- end general_ref
---@field event_id integer
df.general_ref_historical_eventst = nil

---@class general_ref_spherest
-- inherited from general_ref
-- end general_ref
---@field sphere_type sphere_type
df.general_ref_spherest = nil

---@class general_ref_sitest
-- inherited from general_ref
-- end general_ref
---@field site_id integer
df.general_ref_sitest = nil

---@class general_ref_subregionst
-- inherited from general_ref
-- end general_ref
---@field region_id integer
df.general_ref_subregionst = nil

---@class general_ref_feature_layerst
-- inherited from general_ref
-- end general_ref
---@field underground_region_id integer
df.general_ref_feature_layerst = nil

---@class general_ref_historical_figurest
-- inherited from general_ref
-- end general_ref
---@field hist_figure_id integer
df.general_ref_historical_figurest = nil

---@class general_ref_entity_popst
-- inherited from general_ref
-- end general_ref
---@field unk_1 integer
---@field race integer
---@field unk_2 integer
---@field flags bitfield
df.general_ref_entity_popst = nil

---@class general_ref_creaturest
-- inherited from general_ref
-- end general_ref
---@field race integer
---@field caste integer
---@field unk_1 integer
---@field unk_2 integer
---@field flags bitfield
df.general_ref_creaturest = nil

---@class general_ref_knowledge_scholar_flagst
-- inherited from general_ref
-- end general_ref
---@field knowledge knowledge_scholar_category_flag
df.general_ref_knowledge_scholar_flagst = nil

---@class general_ref_activity_eventst
-- inherited from general_ref
-- end general_ref
---@field activity_id integer
---@field event_id integer
df.general_ref_activity_eventst = nil

---@class general_ref_value_levelst
-- inherited from general_ref
-- end general_ref
---@field value value_type
---@field level integer see http://dwarffortresswiki.org/index.php/DF2014:Personality_trait
df.general_ref_value_levelst = nil

---@class general_ref_languagest
-- inherited from general_ref
-- end general_ref
---@field unk_1 integer
df.general_ref_languagest = nil

---@class general_ref_written_contentst
-- inherited from general_ref
-- end general_ref
---@field written_content_id integer
df.general_ref_written_contentst = nil

---@class general_ref_poetic_formst
-- inherited from general_ref
-- end general_ref
---@field poetic_form_id integer
df.general_ref_poetic_formst = nil

---@class general_ref_musical_formst
-- inherited from general_ref
-- end general_ref
---@field musical_form_id integer
df.general_ref_musical_formst = nil

---@class general_ref_dance_formst
-- inherited from general_ref
-- end general_ref
---@field dance_form_id integer
df.general_ref_dance_formst = nil

---@class general_ref_is_artifactst
-- inherited from general_ref_artifact
-- inherited from general_ref
-- end general_ref
---@field artifact_id integer
-- end general_ref_artifact
df.general_ref_is_artifactst = nil

---@class general_ref_is_nemesisst
-- inherited from general_ref_nemesis
-- inherited from general_ref
-- end general_ref
---@field nemesis_id integer
-- end general_ref_nemesis
df.general_ref_is_nemesisst = nil

---@class general_ref_contains_unitst
-- inherited from general_ref_unit
-- inherited from general_ref
-- end general_ref
---@field unit_id integer
---@field cached_index integer lookup optimization, tries before binary search
-- end general_ref_unit
df.general_ref_contains_unitst = nil

---@class general_ref_contains_itemst
-- inherited from general_ref_item
-- inherited from general_ref
-- end general_ref
---@field item_id integer
---@field cached_index integer lookup optimization, tries before binary search
-- end general_ref_item
df.general_ref_contains_itemst = nil

---@class general_ref_contained_in_itemst
-- inherited from general_ref_item
-- inherited from general_ref
-- end general_ref
---@field item_id integer
---@field cached_index integer lookup optimization, tries before binary search
-- end general_ref_item
df.general_ref_contained_in_itemst = nil

---@class general_ref_unit_milkeest
-- inherited from general_ref_unit
-- inherited from general_ref
-- end general_ref
---@field unit_id integer
---@field cached_index integer lookup optimization, tries before binary search
-- end general_ref_unit
df.general_ref_unit_milkeest = nil

---@class general_ref_unit_traineest
-- inherited from general_ref_unit
-- inherited from general_ref
-- end general_ref
---@field unit_id integer
---@field cached_index integer lookup optimization, tries before binary search
-- end general_ref_unit
df.general_ref_unit_traineest = nil

---@class general_ref_unit_itemownerst
-- inherited from general_ref_unit
-- inherited from general_ref
-- end general_ref
---@field unit_id integer
---@field cached_index integer lookup optimization, tries before binary search
-- end general_ref_unit
---@field flags bitfield since v0.34.06
df.general_ref_unit_itemownerst = nil

---@class general_ref_unit_tradebringerst
-- inherited from general_ref_unit
-- inherited from general_ref
-- end general_ref
---@field unit_id integer
---@field cached_index integer lookup optimization, tries before binary search
-- end general_ref_unit
df.general_ref_unit_tradebringerst = nil

---@class general_ref_unit_holderst
-- inherited from general_ref_unit
-- inherited from general_ref
-- end general_ref
---@field unit_id integer
---@field cached_index integer lookup optimization, tries before binary search
-- end general_ref_unit
df.general_ref_unit_holderst = nil

---@class general_ref_unit_workerst
-- inherited from general_ref_unit
-- inherited from general_ref
-- end general_ref
---@field unit_id integer
---@field cached_index integer lookup optimization, tries before binary search
-- end general_ref_unit
df.general_ref_unit_workerst = nil

---@class general_ref_unit_cageest
-- inherited from general_ref_unit
-- inherited from general_ref
-- end general_ref
---@field unit_id integer
---@field cached_index integer lookup optimization, tries before binary search
-- end general_ref_unit
df.general_ref_unit_cageest = nil

---@class general_ref_unit_beateest
-- inherited from general_ref_unit
-- inherited from general_ref
-- end general_ref
---@field unit_id integer
---@field cached_index integer lookup optimization, tries before binary search
-- end general_ref_unit
df.general_ref_unit_beateest = nil

---@class general_ref_unit_foodreceiverst
-- inherited from general_ref_unit
-- inherited from general_ref
-- end general_ref
---@field unit_id integer
---@field cached_index integer lookup optimization, tries before binary search
-- end general_ref_unit
df.general_ref_unit_foodreceiverst = nil

---@class general_ref_unit_kidnapeest
-- inherited from general_ref_unit
-- inherited from general_ref
-- end general_ref
---@field unit_id integer
---@field cached_index integer lookup optimization, tries before binary search
-- end general_ref_unit
df.general_ref_unit_kidnapeest = nil

---@class general_ref_unit_patientst
-- inherited from general_ref_unit
-- inherited from general_ref
-- end general_ref
---@field unit_id integer
---@field cached_index integer lookup optimization, tries before binary search
-- end general_ref_unit
df.general_ref_unit_patientst = nil

---@class general_ref_unit_infantst
-- inherited from general_ref_unit
-- inherited from general_ref
-- end general_ref
---@field unit_id integer
---@field cached_index integer lookup optimization, tries before binary search
-- end general_ref_unit
df.general_ref_unit_infantst = nil

---@class general_ref_unit_slaughtereest
-- inherited from general_ref_unit
-- inherited from general_ref
-- end general_ref
---@field unit_id integer
---@field cached_index integer lookup optimization, tries before binary search
-- end general_ref_unit
df.general_ref_unit_slaughtereest = nil

---@class general_ref_unit_sheareest
-- inherited from general_ref_unit
-- inherited from general_ref
-- end general_ref
---@field unit_id integer
---@field cached_index integer lookup optimization, tries before binary search
-- end general_ref_unit
df.general_ref_unit_sheareest = nil

---@class general_ref_unit_suckeest
-- inherited from general_ref_unit
-- inherited from general_ref
-- end general_ref
---@field unit_id integer
---@field cached_index integer lookup optimization, tries before binary search
-- end general_ref_unit
df.general_ref_unit_suckeest = nil

---@class general_ref_unit_reporteest
-- inherited from general_ref_unit
-- inherited from general_ref
-- end general_ref
---@field unit_id integer
---@field cached_index integer lookup optimization, tries before binary search
-- end general_ref_unit
df.general_ref_unit_reporteest = nil

---@class general_ref_unit_riderst
-- inherited from general_ref_unit
-- inherited from general_ref
-- end general_ref
---@field unit_id integer
---@field cached_index integer lookup optimization, tries before binary search
-- end general_ref_unit
df.general_ref_unit_riderst = nil

---@class general_ref_unit_climberst
-- inherited from general_ref_unit
-- inherited from general_ref
-- end general_ref
---@field unit_id integer
---@field cached_index integer lookup optimization, tries before binary search
-- end general_ref_unit
df.general_ref_unit_climberst = nil

---@class general_ref_unit_geldeest
-- inherited from general_ref_unit
-- inherited from general_ref
-- end general_ref
---@field unit_id integer
---@field cached_index integer lookup optimization, tries before binary search
-- end general_ref_unit
df.general_ref_unit_geldeest = nil

---@class general_ref_unit_interrogateest
-- inherited from general_ref_unit
-- inherited from general_ref
-- end general_ref
---@field unit_id integer
---@field cached_index integer lookup optimization, tries before binary search
-- end general_ref_unit
df.general_ref_unit_interrogateest = nil

---@class general_ref_building_civzone_assignedst
-- inherited from general_ref_building
-- inherited from general_ref
-- end general_ref
---@field building_id integer
-- end general_ref_building
df.general_ref_building_civzone_assignedst = nil

---@class general_ref_building_triggerst
-- inherited from general_ref_building
-- inherited from general_ref
-- end general_ref
---@field building_id integer
-- end general_ref_building
df.general_ref_building_triggerst = nil

---@class general_ref_building_triggertargetst
-- inherited from general_ref_building
-- inherited from general_ref
-- end general_ref
---@field building_id integer
-- end general_ref_building
df.general_ref_building_triggertargetst = nil

---@class general_ref_building_chainst
-- inherited from general_ref_building
-- inherited from general_ref
-- end general_ref
---@field building_id integer
-- end general_ref_building
df.general_ref_building_chainst = nil

---@class general_ref_building_cagedst
-- inherited from general_ref_building
-- inherited from general_ref
-- end general_ref
---@field building_id integer
-- end general_ref_building
df.general_ref_building_cagedst = nil

---@class general_ref_building_holderst
-- inherited from general_ref_building
-- inherited from general_ref
-- end general_ref
---@field building_id integer
-- end general_ref_building
df.general_ref_building_holderst = nil

---@class general_ref_building_well_tag
-- inherited from general_ref_building
-- inherited from general_ref
-- end general_ref
---@field building_id integer
-- end general_ref_building
---@field direction integer
df.general_ref_building_well_tag = nil

---@class general_ref_building_use_target_1st
-- inherited from general_ref_building
-- inherited from general_ref
-- end general_ref
---@field building_id integer
-- end general_ref_building
df.general_ref_building_use_target_1st = nil

---@class general_ref_building_use_target_2st
-- inherited from general_ref_building
-- inherited from general_ref
-- end general_ref
---@field building_id integer
-- end general_ref_building
df.general_ref_building_use_target_2st = nil

---@class general_ref_building_destinationst
-- inherited from general_ref_building
-- inherited from general_ref
-- end general_ref
---@field building_id integer
-- end general_ref_building
df.general_ref_building_destinationst = nil

---@class general_ref_building_nest_boxst
-- inherited from general_ref_building
-- inherited from general_ref
-- end general_ref
---@field building_id integer
-- end general_ref_building
df.general_ref_building_nest_boxst = nil

---@class general_ref_building_display_furniturest
-- inherited from general_ref_building
-- inherited from general_ref
-- end general_ref
---@field building_id integer
-- end general_ref_building
df.general_ref_building_display_furniturest = nil

---@class general_ref_entity_stolenst
-- inherited from general_ref_entity
-- inherited from general_ref
-- end general_ref
---@field entity_id integer
-- end general_ref_entity
df.general_ref_entity_stolenst = nil

---@class general_ref_entity_offeredst
-- inherited from general_ref_entity
-- inherited from general_ref
-- end general_ref
---@field entity_id integer
-- end general_ref_entity
df.general_ref_entity_offeredst = nil

---@class general_ref_entity_itemownerst
-- inherited from general_ref_entity
-- inherited from general_ref
-- end general_ref
---@field entity_id integer
-- end general_ref_entity
df.general_ref_entity_itemownerst = nil

---@enum specific_ref_type
df.specific_ref_type = {
    NONE = -1,
    unk_2 = 0,
    UNIT = 1,
    JOB = 2,
    ACTIVITY = 3,
    ITEM_GENERAL = 4,
    EFFECT = 5,
    VERMIN_EVENT = 6,
    VERMIN_ESCAPED_PET = 7,
    ENTITY = 8,
    PLOT_INFO = 9,
    VIEWSCREEN = 10,
    UNIT_ITEM_WRESTLE = 11,
    NULL_REF = 12,
    HIST_FIG = 13,
    SITE = 14,
    ARTIFACT = 15,
    ITEM_IMPROVEMENT = 16,
    COIN_FRONT = 17,
    COIN_BACK = 18,
    DETAIL_EVENT = 19,
    SUBREGION = 20,
    FEATURE_LAYER = 21,
    ART_IMAGE = 22,
    CREATURE_DEF = 23,
    ENTITY_ART_IMAGE = 24, -- unused?
    unk_27 = 25,
    ENTITY_POPULATION = 26,
    BREED = 27,
}

---@class specific_ref
---@field type specific_ref_type
---@field data object | unit | activity | screen | effect | vermin | job | histfig | entity | wrestle
df.specific_ref = nil

---@enum histfig_entity_link_type
df.histfig_entity_link_type = {
    MEMBER = 0,
    FORMER_MEMBER = 1,
    MERCENARY = 2,
    FORMER_MERCENARY = 3,
    SLAVE = 4,
    FORMER_SLAVE = 5,
    PRISONER = 6,
    FORMER_PRISONER = 7,
    ENEMY = 8,
    CRIMINAL = 9,
    POSITION = 10,
    FORMER_POSITION = 11,
    POSITION_CLAIM = 12,
    SQUAD = 13,
    FORMER_SQUAD = 14,
    OCCUPATION = 15,
    FORMER_OCCUPATION = 16,
}

---@return histfig_entity_link_type
function df.histfig_entity_link.getType() end

---@param file file_compressorst
function df.histfig_entity_link.write_file(file) end

---@param file file_compressorst
---@param loadversion save_version
function df.histfig_entity_link.read_file(file, loadversion) end

function df.histfig_entity_link.unnamed_method() end

function df.histfig_entity_link.unnamed_method() end

---@return integer
function df.histfig_entity_link.getPosition() end

---@return integer
function df.histfig_entity_link.getOccupation() end

---@return integer
function df.histfig_entity_link.getPositionStartYear() end

---@return integer
function df.histfig_entity_link.getPositionEndYear() end

---@param arg_0 stl-fstream
---@param indent integer
function df.histfig_entity_link.generate_xml(arg_0, indent) end

---@class histfig_entity_link
---@field entity_id integer
---@field entity_vector_idx integer
---@field link_strength integer
df.histfig_entity_link = nil

---@class histfig_entity_link_memberst
-- inherited from histfig_entity_link
---@field entity_id integer
---@field entity_vector_idx integer
---@field link_strength integer
-- end histfig_entity_link
df.histfig_entity_link_memberst = nil

---@class histfig_entity_link_former_memberst
-- inherited from histfig_entity_link
---@field entity_id integer
---@field entity_vector_idx integer
---@field link_strength integer
-- end histfig_entity_link
df.histfig_entity_link_former_memberst = nil

---@class histfig_entity_link_mercenaryst
-- inherited from histfig_entity_link
---@field entity_id integer
---@field entity_vector_idx integer
---@field link_strength integer
-- end histfig_entity_link
df.histfig_entity_link_mercenaryst = nil

---@class histfig_entity_link_former_mercenaryst
-- inherited from histfig_entity_link
---@field entity_id integer
---@field entity_vector_idx integer
---@field link_strength integer
-- end histfig_entity_link
df.histfig_entity_link_former_mercenaryst = nil

---@class histfig_entity_link_slavest
-- inherited from histfig_entity_link
---@field entity_id integer
---@field entity_vector_idx integer
---@field link_strength integer
-- end histfig_entity_link
df.histfig_entity_link_slavest = nil

---@class histfig_entity_link_former_slavest
-- inherited from histfig_entity_link
---@field entity_id integer
---@field entity_vector_idx integer
---@field link_strength integer
-- end histfig_entity_link
df.histfig_entity_link_former_slavest = nil

---@class histfig_entity_link_prisonerst
-- inherited from histfig_entity_link
---@field entity_id integer
---@field entity_vector_idx integer
---@field link_strength integer
-- end histfig_entity_link
df.histfig_entity_link_prisonerst = nil

---@class histfig_entity_link_former_prisonerst
-- inherited from histfig_entity_link
---@field entity_id integer
---@field entity_vector_idx integer
---@field link_strength integer
-- end histfig_entity_link
df.histfig_entity_link_former_prisonerst = nil

---@class histfig_entity_link_enemyst
-- inherited from histfig_entity_link
---@field entity_id integer
---@field entity_vector_idx integer
---@field link_strength integer
-- end histfig_entity_link
df.histfig_entity_link_enemyst = nil

---@class histfig_entity_link_criminalst
-- inherited from histfig_entity_link
---@field entity_id integer
---@field entity_vector_idx integer
---@field link_strength integer
-- end histfig_entity_link
df.histfig_entity_link_criminalst = nil

---@class histfig_entity_link_positionst
-- inherited from histfig_entity_link
---@field entity_id integer
---@field entity_vector_idx integer
---@field link_strength integer
-- end histfig_entity_link
---@field assignment_id integer
---@field assignment_vector_idx integer
---@field start_year integer
df.histfig_entity_link_positionst = nil

---@class histfig_entity_link_former_positionst
-- inherited from histfig_entity_link
---@field entity_id integer
---@field entity_vector_idx integer
---@field link_strength integer
-- end histfig_entity_link
---@field assignment_id integer
---@field start_year integer
---@field end_year integer
df.histfig_entity_link_former_positionst = nil

---@class histfig_entity_link_position_claimst
-- inherited from histfig_entity_link
---@field entity_id integer
---@field entity_vector_idx integer
---@field link_strength integer
-- end histfig_entity_link
---@field assignment_id integer
---@field start_year integer
df.histfig_entity_link_position_claimst = nil

---@class histfig_entity_link_squadst
-- inherited from histfig_entity_link
---@field entity_id integer
---@field entity_vector_idx integer
---@field link_strength integer
-- end histfig_entity_link
---@field squad_id integer
---@field squad_position integer
---@field start_year integer
df.histfig_entity_link_squadst = nil

---@class histfig_entity_link_former_squadst
-- inherited from histfig_entity_link
---@field entity_id integer
---@field entity_vector_idx integer
---@field link_strength integer
-- end histfig_entity_link
---@field squad_id integer
---@field start_year integer
---@field end_year integer
df.histfig_entity_link_former_squadst = nil

---@class histfig_entity_link_occupationst
-- inherited from histfig_entity_link
---@field entity_id integer
---@field entity_vector_idx integer
---@field link_strength integer
-- end histfig_entity_link
---@field occupation_id integer
---@field start_year integer
df.histfig_entity_link_occupationst = nil

---@class histfig_entity_link_former_occupationst
-- inherited from histfig_entity_link
---@field entity_id integer
---@field entity_vector_idx integer
---@field link_strength integer
-- end histfig_entity_link
---@field occupation_id integer
---@field start_year integer
---@field end_year integer
df.histfig_entity_link_former_occupationst = nil

---@enum histfig_site_link_type
df.histfig_site_link_type = {
    OCCUPATION = 0,
    SEAT_OF_POWER = 1,
    HANGOUT = 2,
    HOME_SITE_ABSTRACT_BUILDING = 3,
    HOME_SITE_REALIZATION_BUILDING = 4,
    LAIR = 5,
    HOME_SITE_REALIZATION_SUL = 6,
    HOME_SITE_SAVED_CIVZONE = 7,
    PRISON_ABSTRACT_BUILDING = 8,
    PRISON_SITE_BUILDING_PROFILE = 9,
}

---@return histfig_site_link_type
function df.histfig_site_link.getType() end

---@param file file_compressorst
function df.histfig_site_link.write_file(file) end

---@param file file_compressorst
---@param loadversion save_version
function df.histfig_site_link.read_file(file, loadversion) end

---@param arg_0 stl-fstream
---@param indent integer
function df.histfig_site_link.generate_xml(arg_0, indent) end

---@class histfig_site_link
---@field site integer
---@field sub_id integer from XML
---@field entity integer
df.histfig_site_link = nil

---@class histfig_site_link_occupationst
-- inherited from histfig_site_link
---@field site integer
---@field sub_id integer from XML
---@field entity integer
-- end histfig_site_link
---@field unk_1 integer
df.histfig_site_link_occupationst = nil

---@class histfig_site_link_seat_of_powerst
-- inherited from histfig_site_link
---@field site integer
---@field sub_id integer from XML
---@field entity integer
-- end histfig_site_link
df.histfig_site_link_seat_of_powerst = nil

---@class histfig_site_link_hangoutst
-- inherited from histfig_site_link
---@field site integer
---@field sub_id integer from XML
---@field entity integer
-- end histfig_site_link
df.histfig_site_link_hangoutst = nil

---@class histfig_site_link_home_site_abstract_buildingst
-- inherited from histfig_site_link
---@field site integer
---@field sub_id integer from XML
---@field entity integer
-- end histfig_site_link
df.histfig_site_link_home_site_abstract_buildingst = nil

---@class histfig_site_link_home_site_realization_buildingst
-- inherited from histfig_site_link
---@field site integer
---@field sub_id integer from XML
---@field entity integer
-- end histfig_site_link
df.histfig_site_link_home_site_realization_buildingst = nil

---@class histfig_site_link_lairst
-- inherited from histfig_site_link
---@field site integer
---@field sub_id integer from XML
---@field entity integer
-- end histfig_site_link
df.histfig_site_link_lairst = nil

---@class histfig_site_link_home_site_realization_sulst
-- inherited from histfig_site_link
---@field site integer
---@field sub_id integer from XML
---@field entity integer
-- end histfig_site_link
df.histfig_site_link_home_site_realization_sulst = nil

---@class histfig_site_link_home_site_saved_civzonest
-- inherited from histfig_site_link
---@field site integer
---@field sub_id integer from XML
---@field entity integer
-- end histfig_site_link
df.histfig_site_link_home_site_saved_civzonest = nil

---@class histfig_site_link_prison_abstract_buildingst
-- inherited from histfig_site_link
---@field site integer
---@field sub_id integer from XML
---@field entity integer
-- end histfig_site_link
df.histfig_site_link_prison_abstract_buildingst = nil

---@class histfig_site_link_prison_site_building_profilest
-- inherited from histfig_site_link
---@field site integer
---@field sub_id integer from XML
---@field entity integer
-- end histfig_site_link
df.histfig_site_link_prison_site_building_profilest = nil

---@enum histfig_hf_link_type
df.histfig_hf_link_type = {
    MOTHER = 0,
    FATHER = 1,
    SPOUSE = 2,
    CHILD = 3,
    DEITY = 4,
    LOVER = 5,
    PRISONER = 6,
    IMPRISONER = 7,
    MASTER = 8,
    APPRENTICE = 9,
    COMPANION = 10,
    FORMER_MASTER = 11,
    FORMER_APPRENTICE = 12,
    PET_OWNER = 13,
    FORMER_SPOUSE = 14,
    DECEASED_SPOUSE = 15,
}

---@return histfig_hf_link_type
function df.histfig_hf_link.getType() end

---@param file file_compressorst
function df.histfig_hf_link.write_file(file) end

---@param file file_compressorst
---@param loadversion save_version
function df.histfig_hf_link.read_file(file, loadversion) end

---@param arg_0 stl-fstream
---@param indent integer
function df.histfig_hf_link.generate_xml(arg_0, indent) end

---@class histfig_hf_link
---@field target_hf integer
---@field link_strength integer
df.histfig_hf_link = nil

---@class histfig_hf_link_motherst
-- inherited from histfig_hf_link
---@field target_hf integer
---@field link_strength integer
-- end histfig_hf_link
df.histfig_hf_link_motherst = nil

---@class histfig_hf_link_fatherst
-- inherited from histfig_hf_link
---@field target_hf integer
---@field link_strength integer
-- end histfig_hf_link
df.histfig_hf_link_fatherst = nil

---@class histfig_hf_link_spousest
-- inherited from histfig_hf_link
---@field target_hf integer
---@field link_strength integer
-- end histfig_hf_link
df.histfig_hf_link_spousest = nil

---@class histfig_hf_link_childst
-- inherited from histfig_hf_link
---@field target_hf integer
---@field link_strength integer
-- end histfig_hf_link
df.histfig_hf_link_childst = nil

---@class histfig_hf_link_deityst
-- inherited from histfig_hf_link
---@field target_hf integer
---@field link_strength integer
-- end histfig_hf_link
df.histfig_hf_link_deityst = nil

---@class histfig_hf_link_loverst
-- inherited from histfig_hf_link
---@field target_hf integer
---@field link_strength integer
-- end histfig_hf_link
df.histfig_hf_link_loverst = nil

---@class histfig_hf_link_prisonerst
-- inherited from histfig_hf_link
---@field target_hf integer
---@field link_strength integer
-- end histfig_hf_link
df.histfig_hf_link_prisonerst = nil

---@class histfig_hf_link_imprisonerst
-- inherited from histfig_hf_link
---@field target_hf integer
---@field link_strength integer
-- end histfig_hf_link
df.histfig_hf_link_imprisonerst = nil

---@class histfig_hf_link_masterst
-- inherited from histfig_hf_link
---@field target_hf integer
---@field link_strength integer
-- end histfig_hf_link
df.histfig_hf_link_masterst = nil

---@class histfig_hf_link_apprenticest
-- inherited from histfig_hf_link
---@field target_hf integer
---@field link_strength integer
-- end histfig_hf_link
df.histfig_hf_link_apprenticest = nil

---@class histfig_hf_link_companionst
-- inherited from histfig_hf_link
---@field target_hf integer
---@field link_strength integer
-- end histfig_hf_link
---@field unk_1 integer
---@field unk_2 integer
df.histfig_hf_link_companionst = nil

---@class histfig_hf_link_former_apprenticest
-- inherited from histfig_hf_link
---@field target_hf integer
---@field link_strength integer
-- end histfig_hf_link
df.histfig_hf_link_former_apprenticest = nil

---@class histfig_hf_link_former_masterst
-- inherited from histfig_hf_link
---@field target_hf integer
---@field link_strength integer
-- end histfig_hf_link
df.histfig_hf_link_former_masterst = nil

---@class histfig_hf_link_pet_ownerst
-- inherited from histfig_hf_link
---@field target_hf integer
---@field link_strength integer
-- end histfig_hf_link
df.histfig_hf_link_pet_ownerst = nil

---@class histfig_hf_link_former_spousest
-- inherited from histfig_hf_link
---@field target_hf integer
---@field link_strength integer
-- end histfig_hf_link
df.histfig_hf_link_former_spousest = nil

---@class histfig_hf_link_deceased_spousest
-- inherited from histfig_hf_link
---@field target_hf integer
---@field link_strength integer
-- end histfig_hf_link
df.histfig_hf_link_deceased_spousest = nil

---@enum entity_entity_link_type
df.entity_entity_link_type = {
    PARENT = 0,
    CHILD = 1,
    RELIGIOUS = 2, -- Seen between religion and merc company., since v0.47.01
}

---@class entity_entity_link
---@field type entity_entity_link_type
---@field target integer
---@field strength integer
df.entity_entity_link = nil

---@enum entity_site_link_type Enum names updated per Putnam
df.entity_site_link_type = {
    None = -1,
    All = 0,
    Inside_Wall = 1,
    Outside_Wall = 2,
    Outskirts = 3,
    Local_Activity = 4,
}

---@alias entity_site_link_flags bitfield

---@class entity_site_link
---@field target integer
---@field entity_id integer
---@field entity_cache_index integer not saved
---@field position_profile_id integer index into entity.positions.assignments of Civilization (?)
---@field type entity_site_link_type called location in df source
---@field start_hr integer
---@field end_hr integer
---@field flags bitfield
---@field former_flag bitfield
---@field link_strength integer
---@field initial_controlling_population integer all non zero cases are SiteGovernments with type = Claim, status = 0, and flags.residence = true. All examined were formed as forced administrations
---@field last_check_controlling_population integer same value as previous field
---@field ab_profile any[] When a single element the first value makes sense as an abstract building related to the entity, but longer lists do not, including numbers larger than the number of abstract buildings
---@field target_site_x integer target site world coordinate x
---@field target_site_y integer target site world coordinate y
---@field last_checked_army_year integer all cases seen were NomadicGroup with criminal_gang flag set, unk_4 = 0 and type = Foreign_Crime, except for cases with type = Claim and residence flag set as well, since v0.43.01
---@field last_checked_army_year_tick integer paired with the previous field. Could be year/year_tick pair set to the start of play for all of these as all have the same number pair in the same save, since v0.43.01
df.entity_site_link = nil

---@alias undead_flags bitfield


