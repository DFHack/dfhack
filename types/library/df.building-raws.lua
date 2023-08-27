-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.

---@meta

---@param arg_0 integer
---@param arg_1 integer
---@param arg_2 integer
---@param arg_3 integer
function df.building_def.parseRaws(arg_0, arg_1, arg_2, arg_3) end

function df.building_def.categorize() end -- add to world.raws.buildings.whatever

function df.building_def.finalize() end

---@class building_def
---@field code string
---@field id integer
---@field name string
---@field building_type building_type
---@field building_subtype integer
---@field name_color integer[]
---@field tile any[]
---@field tile_color any[]
---@field tile_block any[]
---@field graphics_normal any[]
---@field graphics_overlay any[]
---@field build_key number
---@field needs_magma boolean
---@field build_items building_def_item[]
---@field dim_x integer
---@field dim_y integer
---@field workloc_x integer
---@field workloc_y integer
---@field build_labors any[]
---@field labor_description string
---@field build_stages integer
---@field unnamed_building_def_23 string[]
df.building_def = nil

---@class building_def_item
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
---@field quantity integer
---@field has_tool_use tool_uses
---@field item_str string[]
---@field material_str string[]
---@field metal_ore_str string
df.building_def_item = nil

---@class building_def_workshopst
-- inherited from building_def
---@field code string
---@field id integer
---@field name string
---@field building_type building_type
---@field building_subtype integer
---@field name_color integer[]
---@field tile any[]
---@field tile_color any[]
---@field tile_block any[]
---@field graphics_normal any[]
---@field graphics_overlay any[]
---@field build_key number
---@field needs_magma boolean
---@field build_items building_def_item[]
---@field dim_x integer
---@field dim_y integer
---@field workloc_x integer
---@field workloc_y integer
---@field build_labors any[]
---@field labor_description string
---@field build_stages integer
---@field unnamed_building_def_23 string[]
-- end building_def
---@field unnamed_building_def_workshopst_1 integer
df.building_def_workshopst = nil

---@class building_def_furnacest
-- inherited from building_def
---@field code string
---@field id integer
---@field name string
---@field building_type building_type
---@field building_subtype integer
---@field name_color integer[]
---@field tile any[]
---@field tile_color any[]
---@field tile_block any[]
---@field graphics_normal any[]
---@field graphics_overlay any[]
---@field build_key number
---@field needs_magma boolean
---@field build_items building_def_item[]
---@field dim_x integer
---@field dim_y integer
---@field workloc_x integer
---@field workloc_y integer
---@field build_labors any[]
---@field labor_description string
---@field build_stages integer
---@field unnamed_building_def_23 string[]
-- end building_def
---@field unnamed_building_def_furnacest_1 integer
df.building_def_furnacest = nil


