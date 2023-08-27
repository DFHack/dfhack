-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.

---@meta

---@enum pattern_type
df.pattern_type = {
    MONOTONE = 0,
    STRIPES = 1,
    IRIS_EYE = 2,
    SPOTS = 3,
    PUPIL_EYE = 4,
    MOTTLED = 5,
}

---@class descriptor_color
---@field id string
---@field word_unk string[]
---@field words integer[]
---@field name string
---@field color curses_color
---@field bold integer
---@field red number
---@field green number
---@field blue number
---@field unk_v50_1 integer[]
df.descriptor_color = nil

---@class descriptor_shape
---@field id string
---@field words_str string[] temporary storage before resolving to language_word
---@field words integer[]
---@field name string
---@field name_plural string
---@field adj string[]
---@field gems_use bitfield
---@field category string[] since v0.47.01
---@field faces integer since v0.47.01
---@field tile integer
---@field unk_v50_1 integer
---@field unk_v50_2 integer[]
---@field unk_v50_3 integer[]
---@field unk_v50_4 integer
---@field unk_v50_5 integer
---@field unk_v50_6 integer
df.descriptor_shape = nil

---@class descriptor_pattern
---@field id string
---@field colors integer[]
---@field pattern pattern_type
---@field cp_color string[]
df.descriptor_pattern = nil


