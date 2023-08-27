-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.

---@meta

---@enum reaction_flags
df.reaction_flags = {
    FUEL = 0,
    AUTOMATIC = 1,
    ADVENTURE_MODE_ENABLED = 2,
}

---@class reaction_building
---@field str any[]
---@field type any[]
---@field subtype integer[]
---@field custom integer[]
---@field hotkey integer[]

---@class reaction
---@field code string
---@field name string
---@field flags reaction_flags[]
---@field reagents reaction_reagent[]
---@field products reaction_product[]
---@field skill job_skill
---@field max_multiplier integer since v0.42.01
---@field building reaction_building
---@field index integer
---@field source_hfid integer since v0.42.01
---@field source_enid integer since v0.42.01
---@field raw_strings string[] since v0.42.01
---@field category string since v0.42.01
---@field descriptions reaction_description[] since v0.42.01
---@field quality_adj1 integer since v0.47.02
---@field quality_adj2 integer since v0.47.02
---@field unk_1 integer since v0.47.02
---@field exp_gain integer since v0.47.02
df.reaction = nil

---@class reaction_category
---@field id string
---@field parent string
---@field name string
---@field key integer
---@field description string
df.reaction_category = nil

---@class reaction_description
---@field unk_1 string
---@field item_type item_type
---@field unk_2 string
df.reaction_description = nil

---@enum reaction_reagent_type
df.reaction_reagent_type = {
    item = 0,
}

---@return reaction_reagent_type
function df.reaction_reagent.getType() end

---@param arg_0 integer
---@param arg_1 integer
---@param arg_2 integer
---@param arg_3 integer
---@param arg_4 integer
---@param arg_5 integer
function df.reaction_reagent.unnamed_method(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5) end

---@param reactionID integer
function df.reaction_reagent.resolveTokens(reactionID) end

---@param arg_0 integer
---@param arg_1 integer
---@param arg_2 integer
function df.reaction_reagent.unnamed_method(arg_0, arg_1, arg_2) end

---@param arg_0 item
---@param index integer
---@return boolean
function df.reaction_reagent.matchesRoot(arg_0, index) end

---@param arg_0 item
---@param arg_1 reaction
---@param index integer
---@return boolean
function df.reaction_reagent.matchesChild(arg_0, arg_1, index) end

---@param arg_0 string
---@param index integer
function df.reaction_reagent.getDescription(arg_0, index) end

---@return boolean
function df.reaction_reagent.isLyeBearing() end

---@class reaction_reagent
---@field code string
---@field quantity integer
---@field flags bitfield
df.reaction_reagent = nil

---@alias reaction_reagent_flags bitfield

---@class reaction_reagent_itemst
-- inherited from reaction_reagent
---@field code string
---@field quantity integer
---@field flags bitfield
-- end reaction_reagent
---@field item_type item_type
---@field item_subtype integer
---@field mat_type integer
---@field mat_index integer
---@field reaction_class string
---@field has_material_reaction_product string
---@field flags1 bitfield
---@field flags2 bitfield
---@field flags3 bitfield
---@field flags4 integer
---@field flags5 integer
---@field metal_ore integer
---@field min_dimension integer
---@field contains integer[]
---@field has_tool_use tool_uses
---@field item_str string[]
---@field material_str string[]
---@field metal_ore_str string
---@field contains_str string[]
df.reaction_reagent_itemst = nil

---@enum reaction_product_type
df.reaction_product_type = {
    item = 0,
    improvement = 1,
}

---@return reaction_product_type
function df.reaction_product.getType() end

---@param reactionID integer
function df.reaction_product.resolveTokens(reactionID) end

---@param maker unit
---@param out_products integer
---@param out_items integer
---@param in_reag integer
---@param in_items integer
---@param quantity integer
---@param skill job_skill
---@param job_quality integer
---@param entity historical_entity
---@param site world_site
---@param unk4 integer
function df.reaction_product.produce(maker, out_products, out_items, in_reag, in_items, quantity, skill, job_quality, entity, site, unk4) end

---@param desc string
function df.reaction_product.getDescription(desc) end -- used in Adventurer mode reactions?

---@class reaction_product
---@field product_token string since v0.42.01
---@field product_to_container string
df.reaction_product = nil

---@enum reaction_product_item_flags
df.reaction_product_item_flags = {
    GET_MATERIAL_SAME = 0,
    GET_MATERIAL_PRODUCT = 1,
    FORCE_EDGE = 2,
    PASTE = 3,
    PRESSED = 4,
    CRAFTS = 5,
}

---@class reaction_product_itemst_get_material
---@field reagent_code string
---@field product_code string

---@class reaction_product_itemst
-- inherited from reaction_product
---@field product_token string since v0.42.01
---@field product_to_container string
-- end reaction_product
---@field item_type item_type
---@field item_subtype integer
---@field mat_type integer
---@field mat_index integer
---@field probability integer
---@field count integer
---@field product_dimension integer
---@field flags reaction_product_item_flags[]
---@field get_material reaction_product_itemst_get_material
---@field item_str string[]
---@field material_str string[]
df.reaction_product_itemst = nil

---@enum reaction_product_improvement_flags
df.reaction_product_improvement_flags = {
    GET_MATERIAL_SAME = 0,
    GET_MATERIAL_PRODUCT = 1,
    GLAZED = 2,
}

---@class reaction_product_item_improvementst_get_material
---@field reagent_code string
---@field product_code string

---@class reaction_product_item_improvementst
-- inherited from reaction_product
---@field product_token string since v0.42.01
---@field product_to_container string
-- end reaction_product
---@field target_reagent string
---@field improvement_type improvement_type
---@field improvement_specific_type itemimprovement_specific_type
---@field mat_type integer
---@field mat_index integer
---@field probability integer
---@field flags reaction_product_improvement_flags[]
---@field get_material reaction_product_item_improvementst_get_material
---@field material_str string[]
---@field unk_v4201_2 string since v0.42.01
df.reaction_product_item_improvementst = nil


