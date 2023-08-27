-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.

---@meta

---@alias vermin_flags bitfield

---@enum vermin_category
df.vermin_category = {
    None = -1,
    Eater = 0,
    Grounder = 1,
    Rotter = 2,
    Swamper = 3,
    Searched = 4,
    Disturbed = 5,
    Dropped = 6,
    Underworld = 7, -- last used in 40d for vermin in eerie glowing pits
}

---@class vermin
---@field race integer
---@field caste integer
---@field pos coord
---@field visible boolean 1 = visible vermin
---@field countdown integer
---@field item item
---@field flags bitfield
---@field amount integer The total number of vermin in this object. Decimal constant 10000001 means infinity (probably).
---@field population world_population_ref
---@field category vermin_category
---@field id integer assigned during Save
df.vermin = nil


