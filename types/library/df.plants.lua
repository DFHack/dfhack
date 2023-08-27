-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.

---@meta

---@alias plant_flags bitfield

---@class plant
---@field flags bitfield
---@field material integer
---@field pos coord
---@field grow_counter integer
---@field damage_flags bitfield
---@field hitpoints integer
---@field update_order integer
---@field site_id integer
---@field srb_id integer
---@field contaminants spatter_common[]
---@field tree_info plant_tree_info
df.plant = nil

---@alias plant_tree_tile bitfield

---@alias plant_root_tile bitfield

---@class plant_tree_info
---@field body any[] dimension body_height
---@field extent_east any[] dimension body_height
---@field extent_south any[] dimension body_height
---@field extent_west any[] dimension body_height
---@field extent_north any[] dimension body_height
---@field body_height integer
---@field dim_x integer
---@field dim_y integer
---@field roots any[] dimension roots_depth
---@field roots_depth integer
---@field unk6 integer
df.plant_tree_info = nil


