-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.

---@meta

---@enum projectile_type
df.projectile_type = {
    Item = 0,
    Unit = 1,
    Magic = 2,
}

---@alias proj_list_link projectile[]

---@alias projectile_flags bitfield

---@return projectile_type
function df.projectile.getType() end

---@param noDamageFloor boolean
---@return boolean
function df.projectile.checkImpact(noDamageFloor) end

---@return boolean
function df.projectile.checkMovement() end

---@param file file_compressorst
---@param loadversion save_version
function df.projectile.read_file(file, loadversion) end

---@param file file_compressorst
function df.projectile.write_file(file) end

---@param arg_0 integer
function df.projectile.unnamed_method(arg_0) end

---@return boolean
function df.projectile.isObjectLost() end

---@return integer
function df.projectile.unnamed_method() end

---@class projectile
---@field link proj_list_link
---@field id integer
---@field firer unit
---@field origin_pos coord
---@field target_pos coord
---@field cur_pos coord
---@field prev_pos coord
---@field distance_flown integer
---@field fall_threshold integer
---@field min_hit_distance integer
---@field min_ground_distance integer
---@field flags bitfield
---@field fall_counter integer counts down from delay to 0, then it moves
---@field fall_delay integer
---@field hit_rating integer
---@field unk21 integer
---@field unk22 integer
---@field bow_id integer
---@field unk_item_id integer
---@field unk_unit_id integer
---@field unk_v40_1 integer uninitialized+saved, since v0.40.01
---@field pos_x integer
---@field pos_y integer
---@field pos_z integer
---@field speed_x integer
---@field speed_y integer
---@field speed_z integer
---@field accel_x integer
---@field accel_y integer
---@field accel_z integer
df.projectile = nil

---@class proj_itemst
-- inherited from projectile
---@field link proj_list_link
---@field id integer
---@field firer unit
---@field origin_pos coord
---@field target_pos coord
---@field cur_pos coord
---@field prev_pos coord
---@field distance_flown integer
---@field fall_threshold integer
---@field min_hit_distance integer
---@field min_ground_distance integer
---@field flags bitfield
---@field fall_counter integer counts down from delay to 0, then it moves
---@field fall_delay integer
---@field hit_rating integer
---@field unk21 integer
---@field unk22 integer
---@field bow_id integer
---@field unk_item_id integer
---@field unk_unit_id integer
---@field unk_v40_1 integer uninitialized+saved, since v0.40.01
---@field pos_x integer
---@field pos_y integer
---@field pos_z integer
---@field speed_x integer
---@field speed_y integer
---@field speed_z integer
---@field accel_x integer
---@field accel_y integer
---@field accel_z integer
-- end projectile
---@field item item
df.proj_itemst = nil

---@class proj_unitst
-- inherited from projectile
---@field link proj_list_link
---@field id integer
---@field firer unit
---@field origin_pos coord
---@field target_pos coord
---@field cur_pos coord
---@field prev_pos coord
---@field distance_flown integer
---@field fall_threshold integer
---@field min_hit_distance integer
---@field min_ground_distance integer
---@field flags bitfield
---@field fall_counter integer counts down from delay to 0, then it moves
---@field fall_delay integer
---@field hit_rating integer
---@field unk21 integer
---@field unk22 integer
---@field bow_id integer
---@field unk_item_id integer
---@field unk_unit_id integer
---@field unk_v40_1 integer uninitialized+saved, since v0.40.01
---@field pos_x integer
---@field pos_y integer
---@field pos_z integer
---@field speed_x integer
---@field speed_y integer
---@field speed_z integer
---@field accel_x integer
---@field accel_y integer
---@field accel_z integer
-- end projectile
---@field unit unit ?
df.proj_unitst = nil

---@class proj_magicst
-- inherited from projectile
---@field link proj_list_link
---@field id integer
---@field firer unit
---@field origin_pos coord
---@field target_pos coord
---@field cur_pos coord
---@field prev_pos coord
---@field distance_flown integer
---@field fall_threshold integer
---@field min_hit_distance integer
---@field min_ground_distance integer
---@field flags bitfield
---@field fall_counter integer counts down from delay to 0, then it moves
---@field fall_delay integer
---@field hit_rating integer
---@field unk21 integer
---@field unk22 integer
---@field bow_id integer
---@field unk_item_id integer
---@field unk_unit_id integer
---@field unk_v40_1 integer uninitialized+saved, since v0.40.01
---@field pos_x integer
---@field pos_y integer
---@field pos_z integer
---@field speed_x integer
---@field speed_y integer
---@field speed_z integer
---@field accel_x integer
---@field accel_y integer
---@field accel_z integer
-- end projectile
---@field type integer
---@field damage integer
df.proj_magicst = nil


