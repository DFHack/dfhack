-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.

---@meta

---@enum machine_type
df.machine_type = {
    standard = 0,
}

---@class machine_info
---@field machine_id integer
---@field flags bitfield
df.machine_info = nil

---@class power_info
---@field produced integer
---@field consumed integer
df.power_info = nil

---@alias machine_conn_modes bitfield

---@class machine_tile_set
---@field tiles coord_path
---@field can_connect any[]
df.machine_tile_set = nil

---@return machine_type
function df.machine.getType() end

---@param x integer
---@param y integer
---@param z integer
function df.machine.moveMachine(x, y, z) end

---@param file file_compressorst
function df.machine.write_file(file) end

---@param file file_compressorst
---@param loadversion save_version
function df.machine.read_file(file, loadversion) end

---@class machine
---@field x integer
---@field y integer
---@field z integer
---@field id integer
---@field components any[]
---@field cur_power integer
---@field min_power integer
---@field visual_phase integer
---@field phase_timer integer
---@field flags bitfield
df.machine = nil

---@class machine_standardst
-- inherited from machine
---@field x integer
---@field y integer
---@field z integer
---@field id integer
---@field components any[]
---@field cur_power integer
---@field min_power integer
---@field visual_phase integer
---@field phase_timer integer
---@field flags bitfield
-- end machine
df.machine_standardst = nil

---@class building_axle_horizontalst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field machine machine_info
---@field is_vertical boolean
df.building_axle_horizontalst = nil

---@class building_axle_verticalst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field machine machine_info
df.building_axle_verticalst = nil

---@class building_gear_assemblyst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field machine machine_info
---@field gear_flags bitfield
df.building_gear_assemblyst = nil

---@class building_windmillst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field machine machine_info
---@field orient_x integer
---@field orient_y integer
---@field is_working integer
---@field visual_rotated boolean
---@field rotate_timer integer
---@field orient_timer integer
df.building_windmillst = nil

---@class building_water_wheelst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field machine machine_info
---@field is_vertical boolean
---@field gives_power boolean
df.building_water_wheelst = nil

---@enum screw_pump_direction
df.screw_pump_direction = {
    FromNorth = 0,
    FromEast = 1,
    FromSouth = 2,
    FromWest = 3,
}

---@class building_screw_pumpst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field machine machine_info
---@field pump_energy integer decreases by 1 every frame. powering or manually pumping maintains near 100
---@field direction screw_pump_direction
---@field pump_manually boolean
df.building_screw_pumpst = nil

---@class building_rollersst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field machine machine_info
---@field direction screw_pump_direction
---@field speed integer
df.building_rollersst = nil


