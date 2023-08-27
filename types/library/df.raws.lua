-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.

---@meta

---@param creature_index integer
---@param caste_index integer
---@param alt boolean
---@param soldier boolean
---@return integer
function df.creature_handler.getTile(creature_index, caste_index, alt, soldier) end

---@param creature_index integer
---@param caste_index integer
---@return integer
function df.creature_handler.getGlowTile(creature_index, caste_index) end

---@class creature_handler
---@field alphabetic creature_raw[]
---@field all creature_raw[]
---@field num_caste integer seems equal to length of vectors below
---@field list_creature integer[] Together with list_caste, a list of all caste indexes in order.
---@field list_caste integer[]
---@field action_strings string[] since v0.40.01
df.creature_handler = nil

---@class world_raws_effects
---@field mat_types any[]
---@field mat_indexes integer[]
---@field interactions any[]
---@field all creature_interaction_effect[] since v0.34.01

---@class world_raws_syndromes
---@field mat_types any[]
---@field mat_indexes integer[]
---@field interactions any[]
---@field all syndrome[] since v0.34.01

---@class world_raws_unk_v50_3
---@field unnamed_world_raws_unk_v50_3_1 any[]

---@class world_raws_unk_v50_2
---@field unnamed_world_raws_unk_v50_2_1 any[]

---@class world_raws_unk_v50_1
---@field unnamed_world_raws_unk_v50_1_1 any[]
---@field unnamed_world_raws_unk_v50_1_2 integer[]

---@class world_raws_buildings
---@field all building_def[]
---@field workshops building_def_workshopst[]
---@field furnaces building_def_furnacest[]
---@field next_id integer
---@field unnamed_world_raws_buildings_5 any[]
---@field unnamed_world_raws_buildings_6 any[]
---@field unnamed_world_raws_buildings_7 any[]
---@field unnamed_world_raws_buildings_8 any[]
---@field unnamed_world_raws_buildings_9 any[]
---@field unnamed_world_raws_buildings_10 any[]
---@field unnamed_world_raws_buildings_11 any[]
---@field unnamed_world_raws_buildings_12 any[]
---@field unnamed_world_raws_buildings_13 any[]
---@field unnamed_world_raws_buildings_14 any[]
---@field unnamed_world_raws_buildings_15 any[]
---@field unnamed_world_raws_buildings_16 any[]
---@field unnamed_world_raws_buildings_17 any[]
---@field unnamed_world_raws_buildings_18 any[]
---@field unnamed_world_raws_buildings_19 any[]
---@field unnamed_world_raws_buildings_20 any[]
---@field unnamed_world_raws_buildings_21 any[]
---@field unnamed_world_raws_buildings_22 any[]
---@field unnamed_world_raws_buildings_23 any[]

---@class world_raws_reactions
---@field reactions reaction[]
---@field reaction_categories reaction_category[]

---@class world_raws_descriptors
---@field colors descriptor_color[]
---@field shapes descriptor_shape[]
---@field patterns descriptor_pattern[]
---@field unk_1 integer[] since v0.47.01
---@field unk_2 integer[] since v0.47.01
---@field unk_3 integer[] since v0.47.01
---@field unnamed_world_raws_descriptors_7 any[]
---@field unnamed_world_raws_descriptors_8 any[]
---@field unnamed_world_raws_descriptors_9 any[]
---@field unnamed_world_raws_descriptors_10 any[]
---@field unnamed_world_raws_descriptors_11 any[]
---@field unnamed_world_raws_descriptors_12 any[]
---@field unnamed_world_raws_descriptors_13 any[]
---@field unnamed_world_raws_descriptors_14 any[]
---@field unnamed_world_raws_descriptors_15 any[]
---@field unnamed_world_raws_descriptors_16 any[]
---@field unnamed_world_raws_descriptors_17 any[]

---@class world_raws_language
---@field words language_word[]
---@field symbols language_symbol[]
---@field translations language_translation[]
---@field word_table any[]

---@class world_raws_itemdefs
---@field all itemdef[]
---@field weapons itemdef_weaponst[]
---@field trapcomps itemdef_trapcompst[]
---@field toys itemdef_toyst[]
---@field tools itemdef_toolst[]
---@field tools_by_type any[]
---@field instruments itemdef_instrumentst[]
---@field armor itemdef_armorst[]
---@field ammo itemdef_ammost[]
---@field siege_ammo itemdef_siegeammost[]
---@field gloves itemdef_glovesst[]
---@field shoes itemdef_shoesst[]
---@field shields itemdef_shieldst[]
---@field helms itemdef_helmst[]
---@field pants itemdef_pantsst[]
---@field food itemdef_foodst[]
---@field unnamed_world_raws_itemdefs_17 any[]
---@field unnamed_world_raws_itemdefs_18 any[]

---@class world_raws_plants
---@field all plant_raw[]
---@field bushes plant_raw[]
---@field bushes_idx any[]
---@field trees plant_raw[]
---@field trees_idx any[]
---@field grasses plant_raw[]
---@field grasses_idx any[]
---@field unnamed_world_raws_plants_8 any[]
---@field unnamed_world_raws_plants_9 any[]

---@class world_raws
---@field material_templates material_template[]
---@field inorganics inorganic_raw[]
---@field inorganics_subset inorganic_raw[] all inorganics with value less than 4
---@field plants world_raws_plants
---@field tissue_templates tissue_template[]
---@field body_detail_plans body_detail_plan[]
---@field body_templates body_template[]
---@field bodyglosses any[]
---@field creature_variations creature_variation[]
---@field creatures creature_handler
---@field itemdefs world_raws_itemdefs
---@field entities entity_raw[]
---@field language world_raws_language
---@field descriptors world_raws_descriptors
---@field reactions world_raws_reactions
---@field buildings world_raws_buildings
---@field interactions interaction[] since v0.34.01
---@field unk_v50_1 world_raws_unk_v50_1
---@field unk_v50_2 world_raws_unk_v50_2
---@field unk_v50_3 world_raws_unk_v50_3
---@field mat_table special_mat_table
---@field syndromes world_raws_syndromes
---@field effects world_raws_effects
df.world_raws = nil


