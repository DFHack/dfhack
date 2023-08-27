-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.

---@meta

---@class invasion_info
---@field id integer
---@field civ_id integer
---@field active_size1 integer 0 unless active
---@field active_size2 integer
---@field size integer
---@field duration_counter integer
---@field flags bitfield
---@field unk4b integer
---@field unk_1 integer since v0.44.01
---@field unk_2 integer since v0.44.01
---@field unk_3 integer since v0.44.01
---@field unk_4 integer since v0.44.01
---@field unk_5 integer since v0.44.01
df.invasion_info = nil

---@class entity_population_unk4
---@field unk_1 any[] all 3 vectors share a single index series, with the third being interleaved with at least the second one
---@field unk_2 any[]
---@field unk_3 any[]
df.entity_population_unk4 = nil

---@class entity_population
---@field name language_name
---@field races any[] all the 3 vectors are always the same length, and thus coupled
---@field counts integer[]
---@field unk3 integer[] Set only for cave civs. When set, >= counts. Pre first embark all those are equal
---@field unk4 any[]
---@field unk5 integer
---@field layer_id integer
---@field id integer
---@field flags integer ?; layer_id == -1
---@field civ_id integer
df.entity_population = nil

---@enum nemesis_flags
df.nemesis_flags = {
    ACTIVE_ADVENTURER = 0, -- used when loading save. Swapping the player character via tactical mode disables this flag on the old player character and sets it for the new one.
    RETIRED_ADVENTURER = 1, -- allows resuming play
    ADVENTURER = 2, -- blue color and guided by forces unknown description in legends mode
    unk_3 = 3,
    unk_4 = 4,
    unk_5 = 5,
    unk_6 = 6,
    unk_7 = 7, -- Causes the unit tile to flash between dark and light.
    unk_8 = 8,
    HERO = 9, -- Set after assigning the Hero status during adventure mode character creation, produces the vanguard of destiny description in legends mode.
    DEMIGOD = 10, -- Set after assigning the Demigod status during adventure mode character creation, produces the divine parentage description in legends mode.
}

---@class nemesis_record
---@field id integer sequential index in the array
---@field unit_id integer
---@field save_file_id integer unit-*.dat
---@field member_idx integer index in the file
---@field figure historical_figure
---@field unit unit
---@field group_leader_id integer
---@field companions any[]
---@field unk10 integer
---@field unk11 integer
---@field unk12 integer
---@field unk_v47_1 integer
---@field unk_v47_2 integer
---@field flags nemesis_flags[]
df.nemesis_record = nil

---@class artifact_record
---@field id integer
---@field name language_name
---@field flags any[]
---@field item item
---@field abs_tile_x integer
---@field abs_tile_y integer
---@field abs_tile_z integer
---@field unk_1 integer since v0.44.01
---@field site integer
---@field structure_local integer
---@field unk_2 integer since v0.47.01
---@field subregion integer
---@field feature_layer integer
---@field owner_hf integer namer/creator does not seem to require a claim to be shown
---@field remote_claims integer[] all afar, heirloom from afar seen, since v0.44.01
---@field entity_claims integer[] since v0.44.01
---@field direct_claims integer[] since v0.44.01
---@field storage_site integer since v0.44.01
---@field storage_structure_local integer since v0.44.01
---@field loss_region integer since v0.44.01
---@field unk_3 integer since v0.44.01
---@field holder_hf integer doesn't seem to require a claim, since v0.44.01
---@field year integer seems to be current year or -1, since v0.44.01
---@field unk_4 integer since v0.44.01
---@field unk_5 integer Small set of non zero fairly small numbers seen?
df.artifact_record = nil


