local _ENV = mkmodule('plugins.sort')

local gui = require('gui')
local overlay = require('plugins.overlay')
local setbelief = reqscript('modtools/set-belief')
local utils = require('utils')
local widgets = require('gui.widgets')

local CH_UP = string.char(30)
local CH_DN = string.char(31)

local MELEE_WEAPON_SKILLS = {
    df.job_skill.AXE,
    df.job_skill.SWORD,
    df.job_skill.MACE,
    df.job_skill.HAMMER,
    df.job_skill.SPEAR,
}

local RANGED_WEAPON_SKILLS = {
    df.job_skill.CROSSBOW,
}

local function sort_noop(a, b)
    -- this function is used as a marker and never actually gets called
    error('sort_noop should not be called')
end

local function sort_by_name_desc(unit_id_1, unit_id_2)
    if unit_id_1 == unit_id_2 then return 0 end
    local unit1 = df.unit.find(unit_id_1)
    local unit2 = df.unit.find(unit_id_2)
    if not unit1 then return -1 end
    if not unit2 then return 1 end
    local name1 = dfhack.TranslateName(dfhack.units.getVisibleName(unit1))
    local name2 = dfhack.TranslateName(dfhack.units.getVisibleName(unit2))
    return utils.compare_name(name1, name2)
end

local function sort_by_name_asc(unit_id_1, unit_id_2)
    if unit_id_1 == unit_id_2 then return 0 end
    local unit1 = df.unit.find(unit_id_1)
    local unit2 = df.unit.find(unit_id_2)
    if not unit1 then return -1 end
    if not unit2 then return 1 end
    local name1 = dfhack.TranslateName(dfhack.units.getVisibleName(unit1))
    local name2 = dfhack.TranslateName(dfhack.units.getVisibleName(unit2))
    return utils.compare_name(name2, name1)
end

local function sort_by_migrant_wave_desc(unit_id_1, unit_id_2)
    if unit_id_1 == unit_id_2 then return 0 end
    local unit1 = df.unit.find(unit_id_1)
    local unit2 = df.unit.find(unit_id_2)
    if not unit1 then return -1 end
    if not unit2 then return 1 end
    return utils.compare(unit2.id, unit1.id)
end

local function sort_by_migrant_wave_asc(unit_id_1, unit_id_2)
    if unit_id_1 == unit_id_2 then return 0 end
    local unit1 = df.unit.find(unit_id_1)
    local unit2 = df.unit.find(unit_id_2)
    if not unit1 then return -1 end
    if not unit2 then return 1 end
    return utils.compare(unit1.id, unit2.id)
end

local function sort_by_stress_desc(unit_id_1, unit_id_2)
    if unit_id_1 == unit_id_2 then return 0 end
    local unit1 = df.unit.find(unit_id_1)
    local unit2 = df.unit.find(unit_id_2)
    if not unit1 then return -1 end
    if not unit2 then return 1 end
    local happiness1 = unit1.status.current_soul.personality.stress
    local happiness2 = unit2.status.current_soul.personality.stress
    if happiness1 == happiness2 then
        return sort_by_name_desc(unit_id_1, unit_id_2)
    end
    return utils.compare(happiness2, happiness1)
end

local function sort_by_stress_asc(unit_id_1, unit_id_2)
    if unit_id_1 == unit_id_2 then return 0 end
    local unit1 = df.unit.find(unit_id_1)
    local unit2 = df.unit.find(unit_id_2)
    if not unit1 then return -1 end
    if not unit2 then return 1 end
    local happiness1 = unit1.status.current_soul.personality.stress
    local happiness2 = unit2.status.current_soul.personality.stress
    if happiness1 == happiness2 then
        return sort_by_name_desc(unit_id_1, unit_id_2)
    end
    return utils.compare(happiness1, happiness2)
end

local function get_skill(unit_id, skill, unit)
    unit = unit or df.unit.find(unit_id)
    return unit and
        unit.status.current_soul and
        utils.binsearch(unit.status.current_soul.skills, skill, 'id')
end

local function get_max_skill(unit_id, list)
    local unit = df.unit.find(unit_id)
    if not unit or not unit.status.current_soul then return end
    local max
    for _,skill in ipairs(list) do
        local s = get_skill(unit_id, skill, unit)
        if s then
            if not max then
                max = s
            else
                if max.rating == s.rating and max.experience < s.experience then
                    max = s
                elseif max.rating < s.rating then
                    max = s
                end
            end
        end
    end
    return max
end

local function melee_skill_effectiveness(unit, skill_list)
    -- Physical attributes
    local strength = dfhack.units.getPhysicalAttrValue(unit, df.physical_attribute_type.STRENGTH)
    local agility = dfhack.units.getPhysicalAttrValue(unit, df.physical_attribute_type.AGILITY)
    local toughness = dfhack.units.getPhysicalAttrValue(unit, df.physical_attribute_type.TOUGHNESS)
    local endurance = dfhack.units.getPhysicalAttrValue(unit, df.physical_attribute_type.ENDURANCE)
    local body_size_base = unit.body.size_info.size_base

    -- Mental attributes
    local willpower = dfhack.units.getMentalAttrValue(unit, df.mental_attribute_type.WILLPOWER)
    local spatial_sense = dfhack.units.getMentalAttrValue(unit, df.mental_attribute_type.SPATIAL_SENSE)
    local kinesthetic_sense = dfhack.units.getMentalAttrValue(unit, df.mental_attribute_type.KINESTHETIC_SENSE)

    -- Skills
    -- Finding the highest skill
    local skill_rating = 0
    for _, skill in ipairs(skill_list) do
        local melee_skill = dfhack.units.getNominalSkill(unit, skill, true)
        skill_rating = math.max(skill_rating, melee_skill)
    end
    local melee_combat_rating = dfhack.units.getNominalSkill(unit, df.job_skill.MELEE_COMBAT, true)

    local rating = skill_rating * 27000 + melee_combat_rating * 9000
            + strength * 180 + body_size_base * 100 + kinesthetic_sense * 50 + endurance * 50
            + agility * 30 + toughness * 20 + willpower * 20 + spatial_sense * 20
    return rating
end

local function make_sort_by_melee_skill_effectiveness_desc(list)
    return function(unit_id_1, unit_id_2)
        if unit_id_1 == unit_id_2 then return 0 end
        local unit1 = df.unit.find(unit_id_1)
        local unit2 = df.unit.find(unit_id_2)
        if not unit1 then return -1 end
        if not unit2 then return 1 end
        local rating1 = melee_skill_effectiveness(unit1, list)
        local rating2 = melee_skill_effectiveness(unit2, list)
        if rating1 == rating2 then return sort_by_name_desc(unit_id_1, unit_id_2) end
        return utils.compare(rating2, rating1)
    end
end

local function make_sort_by_melee_skill_effectiveness_asc(list)
    return function(unit_id_1, unit_id_2)
        if unit_id_1 == unit_id_2 then return 0 end
        local unit1 = df.unit.find(unit_id_1)
        local unit2 = df.unit.find(unit_id_2)
        if not unit1 then return -1 end
        if not unit2 then return 1 end
        local rating1 = melee_skill_effectiveness(unit1, list)
        local rating2 = melee_skill_effectiveness(unit2, list)
        if rating1 == rating2 then return sort_by_name_desc(unit_id_1, unit_id_2) end
        return utils.compare(rating1, rating2)
    end
end

-- FUnction could easily be adapted to different weapon types.
local function ranged_skill_effectiveness(unit, skill_list)
    -- Physical attributes
    local agility = dfhack.units.getPhysicalAttrValue(unit, df.physical_attribute_type.AGILITY)

    -- Mental attributes
    local spatial_sense = dfhack.units.getMentalAttrValue(unit, df.mental_attribute_type.SPATIAL_SENSE)
    local kinesthetic_sense = dfhack.units.getMentalAttrValue(unit, df.mental_attribute_type.KINESTHETIC_SENSE)
    local focus = dfhack.units.getMentalAttrValue(unit, df.mental_attribute_type.FOCUS)

    -- Skills
    -- Finding the highest skill
    local skill_rating = 0
    for _, skill in ipairs(skill_list) do
        local ranged_skill = dfhack.units.getNominalSkill(unit, skill, true)
        skill_rating = math.max(skill_rating, ranged_skill)
    end
    local ranged_combat = dfhack.units.getNominalSkill(unit, df.job_skill.RANGED_COMBAT, true)

    local rating = skill_rating * 24000 + ranged_combat * 8000
    + agility * 15 + spatial_sense * 15 + kinesthetic_sense * 6 + focus * 6
    return rating
end

local function make_sort_by_ranged_skill_effectiveness_desc(list)
    return function(unit_id_1, unit_id_2)
        if unit_id_1 == unit_id_2 then return 0 end
        local unit1 = df.unit.find(unit_id_1)
        local unit2 = df.unit.find(unit_id_2)
        if not unit1 then return -1 end
        if not unit2 then return 1 end
        local rating1 = ranged_skill_effectiveness(unit1, list)
        local rating2 = ranged_skill_effectiveness(unit2, list)
        if rating1 == rating2 then return sort_by_name_desc(unit_id_1, unit_id_2) end
        return utils.compare(rating2, rating1)
    end
end

local function make_sort_by_ranged_skill_effectiveness_asc(list)
    return function(unit_id_1, unit_id_2)
        if unit_id_1 == unit_id_2 then return 0 end
        local unit1 = df.unit.find(unit_id_1)
        local unit2 = df.unit.find(unit_id_2)
        if not unit1 then return -1 end
        if not unit2 then return 1 end
        local rating1 = ranged_skill_effectiveness(unit1, list)
        local rating2 = ranged_skill_effectiveness(unit2, list)
        if rating1 == rating2 then return sort_by_name_desc(unit_id_1, unit_id_2) end
        return utils.compare(rating1, rating2)
    end
end

local function make_sort_by_skill_desc(sort_skill)
    return function(unit_id_1, unit_id_2)
        if unit_id_1 == unit_id_2 then return 0 end
        if unit_id_1 == -1 then return -1 end
        if unit_id_2 == -1 then return 1 end
        local s1 = get_skill(unit_id_1, sort_skill)
        local s2 = get_skill(unit_id_2, sort_skill)
        if s1 == s2 then return sort_by_name_desc(unit_id_1, unit_id_2) end
        if not s2 then return -1 end
        if not s1 then return 1 end
        if s1.rating ~= s2.rating then
            return utils.compare(s2.rating, s1.rating)
        end
        if s1.experience ~= s2.experience then
            return utils.compare(s2.experience, s1.experience)
        end
        return sort_by_name_desc(unit_id_1, unit_id_2)
    end
end

local function make_sort_by_skill_asc(sort_skill)
    return function(unit_id_1, unit_id_2)
        if unit_id_1 == unit_id_2 then return 0 end
        if unit_id_1 == -1 then return -1 end
        if unit_id_2 == -1 then return 1 end
        local s1 = get_skill(unit_id_1, sort_skill)
        local s2 = get_skill(unit_id_2, sort_skill)
        if s1 == s2 then return sort_by_name_desc(unit_id_1, unit_id_2) end
        if not s2 then return 1 end
        if not s1 then return -1 end
        if s1.rating ~= s2.rating then
            return utils.compare(s1.rating, s2.rating)
        end
        if s1.experience ~= s2.experience then
            return utils.compare(s1.experience, s2.experience)
        end
        return sort_by_name_desc(unit_id_1, unit_id_2)
    end
end

-- Statistical rating that is higher for dwarves that are mentally stable
local function mental_stability(unit)
    local ALTRUISM = unit.status.current_soul.personality.traits.ALTRUISM
    local ANXIETY_PROPENSITY = unit.status.current_soul.personality.traits.ANXIETY_PROPENSITY
    local BRAVERY = unit.status.current_soul.personality.traits.BRAVERY
    local CHEER_PROPENSITY = unit.status.current_soul.personality.traits.CHEER_PROPENSITY
    local CURIOUS = unit.status.current_soul.personality.traits.CURIOUS
    local DISCORD = unit.status.current_soul.personality.traits.DISCORD
    local DUTIFULNESS = unit.status.current_soul.personality.traits.DUTIFULNESS
    local EMOTIONALLY_OBSESSIVE = unit.status.current_soul.personality.traits.EMOTIONALLY_OBSESSIVE
    local HUMOR = unit.status.current_soul.personality.traits.HUMOR
    local LOVE_PROPENSITY = unit.status.current_soul.personality.traits.LOVE_PROPENSITY
    local PERSEVERENCE = unit.status.current_soul.personality.traits.PERSEVERENCE
    local POLITENESS = unit.status.current_soul.personality.traits.POLITENESS
    local PRIVACY = unit.status.current_soul.personality.traits.PRIVACY
    local STRESS_VULNERABILITY = unit.status.current_soul.personality.traits.STRESS_VULNERABILITY
    local TOLERANT = unit.status.current_soul.personality.traits.TOLERANT

    local CRAFTSMANSHIP = setbelief.getUnitBelief(unit, df.value_type['CRAFTSMANSHIP'])
    local FAMILY = setbelief.getUnitBelief(unit, df.value_type['FAMILY'])
    local HARMONY = setbelief.getUnitBelief(unit, df.value_type['HARMONY'])
    local INDEPENDENCE = setbelief.getUnitBelief(unit, df.value_type['INDEPENDENCE'])
    local KNOWLEDGE = setbelief.getUnitBelief(unit, df.value_type['KNOWLEDGE'])
    local LEISURE_TIME = setbelief.getUnitBelief(unit, df.value_type['LEISURE_TIME'])
    local NATURE = setbelief.getUnitBelief(unit, df.value_type['NATURE'])
    local SKILL = setbelief.getUnitBelief(unit, df.value_type['SKILL'])

    -- Calculate the rating using the defined variables
    local rating = (CRAFTSMANSHIP * -0.01) + (FAMILY * -0.09) + (HARMONY * 0.05)
                 + (INDEPENDENCE * 0.06) + (KNOWLEDGE * -0.30) + (LEISURE_TIME * 0.24)
                 + (NATURE * 0.27) + (SKILL * -0.21) + (ALTRUISM * 0.13)
                 + (ANXIETY_PROPENSITY * -0.06) + (BRAVERY * 0.06)
                 + (CHEER_PROPENSITY * 0.41) + (CURIOUS * -0.06) + (DISCORD * 0.14)
                 + (DUTIFULNESS * -0.03) + (EMOTIONALLY_OBSESSIVE * -0.13)
                 + (HUMOR * -0.05) + (LOVE_PROPENSITY * 0.15) + (PERSEVERENCE * -0.07)
                 + (POLITENESS * -0.14) + (PRIVACY * 0.03) + (STRESS_VULNERABILITY * -0.20)
                 + (TOLERANT * -0.11)

    return rating
end

local function sort_by_mental_stability_desc(unit_id_1, unit_id_2)
    if unit_id_1 == unit_id_2 then return 0 end
    local unit1 = df.unit.find(unit_id_1)
    local unit2 = df.unit.find(unit_id_2)
    if not unit1 then return -1 end
    if not unit2 then return 1 end
    local rating1 = mental_stability(unit1)
    local rating2 = mental_stability(unit2)
    if rating1 == rating2 then
        -- sorting by stress is opposite
        -- more mental stable dwarves should have less stress
        return sort_by_stress_asc(unit_id_1, unit_id_2)
    end
    return utils.compare(rating2, rating1)
end

local function sort_by_mental_stability_asc(unit_id_1, unit_id_2)
    if unit_id_1 == unit_id_2 then return 0 end
    local unit1 = df.unit.find(unit_id_1)
    local unit2 = df.unit.find(unit_id_2)
    if not unit1 then return -1 end
    if not unit2 then return 1 end
    local rating1 = mental_stability(unit1)
    local rating2 = mental_stability(unit2)
    if rating1 == rating2 then
        return sort_by_stress_desc(unit_id_1, unit_id_2)
    end
    return utils.compare(rating1, rating2)
end

-- Statistical rating that is higher for more potent dwarves in long run melee military training
-- Rating considers fighting melee opponents
-- Wounds are not considered!
local function melee_combat_potential(unit)
    -- Physical attributes
    local strength = unit.body.physical_attrs.STRENGTH.max_value
    local agility = unit.body.physical_attrs.AGILITY.max_value
    local toughness = unit.body.physical_attrs.TOUGHNESS.max_value
    local endurance = unit.body.physical_attrs.ENDURANCE.max_value
    local body_size_base = unit.body.size_info.size_base

    -- Mental attributes
    local willpower = unit.status.current_soul.mental_attrs.WILLPOWER.max_value
    local spatial_sense = unit.status.current_soul.mental_attrs.SPATIAL_SENSE.max_value
    local kinesthetic_sense = unit.status.current_soul.mental_attrs.KINESTHETIC_SENSE.max_value

    -- melee combat potential rating
    local rating = strength * 264 + endurance * 84 + body_size_base * 77 + kinesthetic_sense * 74
            + agility * 33 + willpower * 31 + spatial_sense * 27 + toughness * 25
    return rating
end

local function sort_by_melee_combat_potential_desc(unit_id_1, unit_id_2)
    if unit_id_1 == unit_id_2 then return 0 end
    local unit1 = df.unit.find(unit_id_1)
    local unit2 = df.unit.find(unit_id_2)
    if not unit1 then return -1 end
    if not unit2 then return 1 end
    local rating1 = melee_combat_potential(unit1)
    local rating2 = melee_combat_potential(unit2)
    if rating1 == rating2 then
        return sort_by_mental_stability_desc(unit_id_1, unit_id_2)
    end
    return utils.compare(rating2, rating1)
end

local function sort_by_melee_combat_potential_asc(unit_id_1, unit_id_2)
    if unit_id_1 == unit_id_2 then return 0 end
    local unit1 = df.unit.find(unit_id_1)
    local unit2 = df.unit.find(unit_id_2)
    if not unit1 then return -1 end
    if not unit2 then return 1 end
    local rating1 = melee_combat_potential(unit1)
    local rating2 = melee_combat_potential(unit2)
    if rating1 == rating2 then
        return sort_by_mental_stability_asc(unit_id_1, unit_id_2)
    end
    return utils.compare(rating1, rating2)
end

-- Statistical rating that is higher for more potent dwarves in long run ranged military training
-- Wounds are not considered!
local function ranged_combat_potential(unit)
    -- Physical attributes
    local agility = unit.body.physical_attrs.AGILITY.max_value
    local toughness = unit.body.physical_attrs.TOUGHNESS.max_value
    local endurance = unit.body.physical_attrs.ENDURANCE.max_value

    -- Mental attributes
    local focus = unit.status.current_soul.mental_attrs.FOCUS.max_value
    local willpower = unit.status.current_soul.mental_attrs.WILLPOWER.max_value
    local spatial_sense = unit.status.current_soul.mental_attrs.SPATIAL_SENSE.max_value
    local kinesthetic_sense = unit.status.current_soul.mental_attrs.KINESTHETIC_SENSE.max_value

    -- ranged combat potential formula
    local rating = agility * 5 + kinesthetic_sense * 5 + spatial_sense * 2 + focus * 2
    return rating
end

local function sort_by_ranged_combat_potential_desc(unit_id_1, unit_id_2)
    if unit_id_1 == unit_id_2 then return 0 end
    local unit1 = df.unit.find(unit_id_1)
    local unit2 = df.unit.find(unit_id_2)
    if not unit1 then return -1 end
    if not unit2 then return 1 end
    local rating1 = ranged_combat_potential(unit1)
    local rating2 = ranged_combat_potential(unit2)
    if rating1 == rating2 then
        return sort_by_mental_stability_desc(unit_id_1, unit_id_2)
    end
    return utils.compare(rating2, rating1)
end

local function sort_by_ranged_combat_potential_asc(unit_id_1, unit_id_2)
    if unit_id_1 == unit_id_2 then return 0 end
    local unit1 = df.unit.find(unit_id_1)
    local unit2 = df.unit.find(unit_id_2)
    if not unit1 then return -1 end
    if not unit2 then return 1 end
    local rating1 = ranged_combat_potential(unit1)
    local rating2 = ranged_combat_potential(unit2)
    if rating1 == rating2 then
        return sort_by_mental_stability_asc(unit_id_1, unit_id_2)
    end
    return utils.compare(rating1, rating2)
end

local SORT_FNS = {
    sort_by_any_melee_desc=make_sort_by_melee_skill_effectiveness_desc(MELEE_WEAPON_SKILLS),
    sort_by_any_melee_asc=make_sort_by_melee_skill_effectiveness_asc(MELEE_WEAPON_SKILLS),
    sort_by_any_ranged_desc=make_sort_by_ranged_skill_effectiveness_desc(RANGED_WEAPON_SKILLS),
    sort_by_any_ranged_asc=make_sort_by_ranged_skill_effectiveness_asc(RANGED_WEAPON_SKILLS),
    sort_by_teacher_desc=make_sort_by_skill_desc(df.job_skill.TEACHING),
    sort_by_teacher_asc=make_sort_by_skill_asc(df.job_skill.TEACHING),
    sort_by_tactics_desc=make_sort_by_skill_desc(df.job_skill.MILITARY_TACTICS),
    sort_by_tactics_asc=make_sort_by_skill_asc(df.job_skill.MILITARY_TACTICS),
    sort_by_axe_desc=make_sort_by_skill_desc(df.job_skill.AXE),
    sort_by_axe_asc=make_sort_by_skill_asc(df.job_skill.AXE),
    sort_by_sword_desc=make_sort_by_skill_desc(df.job_skill.SWORD),
    sort_by_sword_asc=make_sort_by_skill_asc(df.job_skill.SWORD),
    sort_by_mace_desc=make_sort_by_skill_desc(df.job_skill.MACE),
    sort_by_mace_asc=make_sort_by_skill_asc(df.job_skill.MACE),
    sort_by_hammer_desc=make_sort_by_skill_desc(df.job_skill.HAMMER),
    sort_by_hammer_asc=make_sort_by_skill_asc(df.job_skill.HAMMER),
    sort_by_spear_desc=make_sort_by_skill_desc(df.job_skill.SPEAR),
    sort_by_spear_asc=make_sort_by_skill_asc(df.job_skill.SPEAR),
    sort_by_crossbow_desc=make_sort_by_skill_desc(df.job_skill.CROSSBOW),
    sort_by_crossbow_asc=make_sort_by_skill_asc(df.job_skill.CROSSBOW),
}

-- ----------------------
-- SquadAssignmentOverlay
--

SquadAssignmentOverlay = defclass(SquadAssignmentOverlay, overlay.OverlayWidget)
SquadAssignmentOverlay.ATTRS{
    default_pos={x=23, y=5},
    default_enabled=true,
    viewscreens='dwarfmode/UnitSelector/SQUAD_FILL_POSITION',
    frame={w=38, h=25},
    frame_style=gui.FRAME_PANEL,
    frame_background=gui.CLEAR_PEN,
    autoarrange_subviews=true,
    autoarrange_gap=1,
}

function SquadAssignmentOverlay:init()
    self.dirty = true

    self:addviews{
        widgets.EditField{
            view_id='search',
            frame={l=0},
            label_text='Search: ',
            on_char=function(ch) return ch:match('[%l _-]') end,
            on_change=function() self:refresh_list() end,
        },
        widgets.Panel{
            frame={l=0, r=0, h=15},
            frame_style=gui.FRAME_INTERIOR,
            subviews={
                widgets.CycleHotkeyLabel{
                    view_id='sort',
                    frame={t=0, l=0},
                    label='Sort by:',
                    key='CUSTOM_SHIFT_S',
                    options={
                        {label='melee effectiveness'..CH_DN, value=SORT_FNS.sort_by_any_melee_desc, pen=COLOR_GREEN},
                        {label='melee effectiveness'..CH_UP, value=SORT_FNS.sort_by_any_melee_asc, pen=COLOR_YELLOW},
                        {label='ranged effectiveness'..CH_DN, value=SORT_FNS.sort_by_any_ranged_desc, pen=COLOR_GREEN},
                        {label='ranged effectiveness'..CH_UP, value=SORT_FNS.sort_by_any_ranged_asc, pen=COLOR_YELLOW},
                        {label='name'..CH_DN, value=sort_by_name_desc, pen=COLOR_GREEN},
                        {label='name'..CH_UP, value=sort_by_name_asc, pen=COLOR_YELLOW},
                        {label='teacher skill'..CH_DN, value=SORT_FNS.sort_by_teacher_desc, pen=COLOR_GREEN},
                        {label='teacher skill'..CH_UP, value=SORT_FNS.sort_by_teacher_asc, pen=COLOR_YELLOW},
                        {label='tactics skill'..CH_DN, value=SORT_FNS.sort_by_tactics_desc, pen=COLOR_GREEN},
                        {label='tactics skill'..CH_UP, value=SORT_FNS.sort_by_tactics_asc, pen=COLOR_YELLOW},
                        {label='migrant wave'..CH_DN, value=sort_by_migrant_wave_desc, pen=COLOR_GREEN},
                        {label='migrant wave'..CH_UP, value=sort_by_migrant_wave_asc, pen=COLOR_YELLOW},
                        {label='stress'..CH_DN, value=sort_by_stress_desc, pen=COLOR_GREEN},
                        {label='stress'..CH_UP, value=sort_by_stress_asc, pen=COLOR_YELLOW},
                        {label='mental stability'..CH_DN, value=sort_by_mental_stability_desc, pen=COLOR_GREEN},
                        {label='mental stability'..CH_UP, value=sort_by_mental_stability_asc, pen=COLOR_YELLOW},
                        {label='axe skill'..CH_DN, value=SORT_FNS.sort_by_axe_desc, pen=COLOR_GREEN},
                        {label='axe skill'..CH_UP, value=SORT_FNS.sort_by_axe_asc, pen=COLOR_YELLOW},
                        {label='sword skill'..CH_DN, value=SORT_FNS.sort_by_sword_desc, pen=COLOR_GREEN},
                        {label='sword skill'..CH_UP, value=SORT_FNS.sort_by_sword_asc, pen=COLOR_YELLOW},
                        {label='mace skill'..CH_DN, value=SORT_FNS.sort_by_mace_desc, pen=COLOR_GREEN},
                        {label='mace skill'..CH_UP, value=SORT_FNS.sort_by_mace_asc, pen=COLOR_YELLOW},
                        {label='hammer skill'..CH_DN, value=SORT_FNS.sort_by_hammer_desc, pen=COLOR_GREEN},
                        {label='hammer skill'..CH_UP, value=SORT_FNS.sort_by_hammer_asc, pen=COLOR_YELLOW},
                        {label='spear skill'..CH_DN, value=SORT_FNS.sort_by_spear_desc, pen=COLOR_GREEN},
                        {label='spear skill'..CH_UP, value=SORT_FNS.sort_by_spear_asc, pen=COLOR_YELLOW},
                        {label='crossbow skill'..CH_DN, value=SORT_FNS.sort_by_crossbow_desc, pen=COLOR_GREEN},
                        {label='crossbow skill'..CH_UP, value=SORT_FNS.sort_by_crossbow_asc, pen=COLOR_YELLOW},
                        {label='melee potential'..CH_DN, value=sort_by_melee_combat_potential_desc, pen=COLOR_GREEN},
                        {label='melee potential'..CH_UP, value=sort_by_melee_combat_potential_asc, pen=COLOR_YELLOW},
                        {label='ranged potential'..CH_DN, value=sort_by_ranged_combat_potential_desc, pen=COLOR_GREEN},
                        {label='ranged potential'..CH_UP, value=sort_by_ranged_combat_potential_asc, pen=COLOR_YELLOW},
                    },
                    initial_option=SORT_FNS.sort_by_any_melee_desc,
                    on_change=self:callback('refresh_list', 'sort'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_any_melee',
                    frame={t=2, l=0, w=10},
                    options={
                        {label='any melee', value=sort_noop},
                        {label='any melee'..CH_DN, value=SORT_FNS.sort_by_any_melee_desc, pen=COLOR_GREEN},
                        {label='any melee'..CH_UP, value=SORT_FNS.sort_by_any_melee_asc, pen=COLOR_YELLOW},
                    },
                    initial_option=SORT_FNS.sort_by_any_melee_desc,
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_any_melee'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_any_ranged',
                    frame={t=2, r=9, w=11},
                    options={
                        {label='any ranged', value=sort_noop},
                        {label='any ranged'..CH_DN, value=SORT_FNS.sort_by_any_ranged_desc, pen=COLOR_GREEN},
                        {label='any ranged'..CH_UP, value=SORT_FNS.sort_by_any_ranged_asc, pen=COLOR_YELLOW},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_any_ranged'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_name',
                    frame={t=2, r=0, w=5},
                    options={
                        {label='name', value=sort_noop},
                        {label='name'..CH_DN, value=sort_by_name_desc, pen=COLOR_GREEN},
                        {label='name'..CH_UP, value=sort_by_name_asc, pen=COLOR_YELLOW},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_name'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_teacher',
                    frame={t=4, l=0, w=8},
                    options={
                        {label='teacher', value=sort_noop},
                        {label='teacher'..CH_DN, value=SORT_FNS.sort_by_teacher_desc, pen=COLOR_GREEN},
                        {label='teacher'..CH_UP, value=SORT_FNS.sort_by_teacher_asc, pen=COLOR_YELLOW},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_teacher'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_tactics',
                    frame={t=4, l=10, w=8},
                    options={
                        {label='tactics', value=sort_noop},
                        {label='tactics'..CH_DN, value=SORT_FNS.sort_by_tactics_desc, pen=COLOR_GREEN},
                        {label='tactics'..CH_UP, value=SORT_FNS.sort_by_tactics_asc, pen=COLOR_YELLOW},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_tactics'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_migrant_wave',
                    frame={t=4, r=0, w=13},
                    options={
                        {label='migrant wave', value=sort_noop},
                        {label='migrant wave'..CH_DN, value=sort_by_migrant_wave_desc, pen=COLOR_GREEN},
                        {label='migrant wave'..CH_UP, value=sort_by_migrant_wave_asc, pen=COLOR_YELLOW},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_migrant_wave'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_stress',
                    frame={t=6, l=0, w=7},
                    options={
                        {label='stress', value=sort_noop},
                        {label='stress'..CH_DN, value=sort_by_stress_desc, pen=COLOR_GREEN},
                        {label='stress'..CH_UP, value=sort_by_stress_asc, pen=COLOR_YELLOW},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_stress'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_mental_stability',
                    frame={t=6, r=0, w=17},
                    options={
                        {label='mental stability', value=sort_noop},
                        {label='mental stability'..CH_DN, value=sort_by_mental_stability_desc, pen=COLOR_GREEN},
                        {label='mental stability'..CH_UP, value=sort_by_mental_stability_asc, pen=COLOR_YELLOW},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_mental_stability'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_axe',
                    frame={t=8, l=0, w=4},
                    options={
                        {label='axe', value=sort_noop},
                        {label='axe'..CH_DN, value=SORT_FNS.sort_by_axe_desc, pen=COLOR_GREEN},
                        {label='axe'..CH_UP, value=SORT_FNS.sort_by_axe_asc, pen=COLOR_YELLOW},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_axe'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_sword',
                    frame={t=8, w=6},
                    options={
                        {label='sword', value=sort_noop},
                        {label='sword'..CH_DN, value=SORT_FNS.sort_by_sword_desc, pen=COLOR_GREEN},
                        {label='sword'..CH_UP, value=SORT_FNS.sort_by_sword_asc, pen=COLOR_YELLOW},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_sword'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_mace',
                    frame={t=8, r=0, w=5},
                    options={
                        {label='mace', value=sort_noop},
                        {label='mace'..CH_DN, value=SORT_FNS.sort_by_mace_desc, pen=COLOR_GREEN},
                        {label='mace'..CH_UP, value=SORT_FNS.sort_by_mace_asc, pen=COLOR_YELLOW},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_mace'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_hammer',
                    frame={t=10, l=0, w=7},
                    options={
                        {label='hammer', value=sort_noop},
                        {label='hammer'..CH_DN, value=SORT_FNS.sort_by_hammer_desc, pen=COLOR_GREEN},
                        {label='hammer'..CH_UP, value=SORT_FNS.sort_by_hammer_asc, pen=COLOR_YELLOW},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_hammer'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_spear',
                    frame={t=10, w=6},
                    options={
                        {label='spear', value=sort_noop},
                        {label='spear'..CH_DN, value=SORT_FNS.sort_by_spear_desc, pen=COLOR_GREEN},
                        {label='spear'..CH_UP, value=SORT_FNS.sort_by_spear_asc, pen=COLOR_YELLOW},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_spear'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_crossbow',
                    frame={t=10, r=0, w=9},
                    options={
                        {label='crossbow', value=sort_noop},
                        {label='crossbow'..CH_DN, value=SORT_FNS.sort_by_crossbow_desc, pen=COLOR_GREEN},
                        {label='crossbow'..CH_UP, value=SORT_FNS.sort_by_crossbow_asc, pen=COLOR_YELLOW},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_crossbow'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_melee_combat_potential',
                    frame={t=12, l=0, w=16},
                    options={
                        {label='melee potential', value=sort_noop},
                        {label='melee potential'..CH_DN, value=sort_by_melee_combat_potential_desc, pen=COLOR_GREEN},
                        {label='melee potential'..CH_UP, value=sort_by_melee_combat_potential_asc, pen=COLOR_YELLOW},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_melee_combat_potential'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_ranged_combat_potential',
                    frame={t=12, r=0, w=17},
                    options={
                        {label='ranged potential', value=sort_noop},
                        {label='ranged potential'..CH_DN, value=sort_by_ranged_combat_potential_desc, pen=COLOR_GREEN},
                        {label='ranged potential'..CH_UP, value=sort_by_ranged_combat_potential_asc, pen=COLOR_YELLOW},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_ranged_combat_potential'),
                },
            },
        },
        widgets.CycleHotkeyLabel{
            view_id='military',
            frame={l=0},
            key='CUSTOM_SHIFT_Q',
            label='Units in other squads:',
            options={
                {label='Include', value='include', pen=COLOR_GREEN},
                {label='Only', value='only', pen=COLOR_YELLOW},
                {label='Exclude', value='exclude', pen=COLOR_RED},
            },
            initial_option='include',
            on_change=function() self:refresh_list() end,
        },
        widgets.CycleHotkeyLabel{
            view_id='officials',
            frame={l=0},
            key='CUSTOM_SHIFT_O',
            label='Appointed officials:',
            options={
                {label='Include', value='include', pen=COLOR_GREEN},
                {label='Only', value='only', pen=COLOR_YELLOW},
                {label='Exclude', value='exclude', pen=COLOR_RED},
            },
            initial_option='include',
            on_change=function() self:refresh_list() end,
        },
        widgets.CycleHotkeyLabel{
            view_id='nobles',
            frame={l=0, w=20},
            key='CUSTOM_SHIFT_N',
            label='Nobility:',
            options={
                {label='Include', value='include', pen=COLOR_GREEN},
                {label='Only', value='only', pen=COLOR_YELLOW},
                {label='Exclude', value='exclude', pen=COLOR_RED},
            },
            initial_option='include',
            on_change=function() self:refresh_list() end,
        },
    }
end

local function normalize_search_key(search_key)
    local out = ''
    for c in dfhack.toSearchNormalized(search_key):gmatch("[%w%s]") do
        out = out .. c:lower()
    end
    return out
end

local function is_in_military(unit)
    return unit.military.squad_id > -1
end

local function is_elected_or_appointed_official(unit)
    if #unit.occupations > 0 then return true end
    for _, noble_pos in ipairs(dfhack.units.getNoblePositions(unit) or {}) do
        if noble_pos.position.flags.ELECTED or
            (noble_pos.position.mandate_max == 0 and noble_pos.position.demand_max == 0)
        then
            return true
        end
    end
    return false
end

local function is_nobility(unit)
    for _, noble_pos in ipairs(dfhack.units.getNoblePositions(unit) or {}) do
        if not noble_pos.position.flags.ELECTED and
            (noble_pos.position.mandate_max > 0 or noble_pos.position.demand_max > 0)
        then
            return true
        end
    end
    return false
end

local function filter_matches(unit_id, filter)
    if unit_id == -1 then return true end
    local unit = df.unit.find(unit_id)
    if not unit then return false end
    if filter.military == 'only' and not is_in_military(unit) then return false end
    if filter.military == 'exclude' and is_in_military(unit) then return false end
    if filter.officials == 'only' and not is_elected_or_appointed_official(unit) then return false end
    if filter.officials == 'exclude' and is_elected_or_appointed_official(unit) then return false end
    if filter.nobles == 'only' and not is_nobility(unit) then return false end
    if filter.nobles == 'exclude' and is_nobility(unit) then return false end
    if #filter.search == 0 then return true end
    local search_key = dfhack.TranslateName(dfhack.units.getVisibleName(unit))
    return normalize_search_key(search_key):find(dfhack.toSearchNormalized(filter.search))
end

local function is_noop_filter(filter)
    return #filter.search == 0 and
        filter.military == 'include' and
        filter.officials == 'include' and
        filter.nobles == 'include'
end

local function is_filter_equal(a, b)
    return a.search == b.search and
        a.military == b.military and
        a.officials == b.officials and
        a.nobles == b.nobles
end

local unit_selector = df.global.game.main_interface.unit_selector

-- this function uses the unused itemid and selected vectors to keep state,
-- taking advantage of the fact that they are reset by DF when the list of units changes
local function filter_vector(filter, prev_filter)
    local unid_is_filtered = #unit_selector.selected >= 0 and unit_selector.selected[0] ~= 0
    if is_noop_filter(filter) or #unit_selector.selected == 0 then
        if not unid_is_filtered then
            -- we haven't modified the unid vector; nothing to do here
            return
        end
        -- restore the unid vector
        unit_selector.unid:assign(unit_selector.itemid)
        -- clear our "we meddled" flag
        unit_selector.selected[0] = 0
        return
    end
    if unid_is_filtered and is_filter_equal(filter, prev_filter) then
        -- filter hasn't changed; we don't need to refilter
        return
    end
    if unid_is_filtered then
        -- restore the unid vector
        unit_selector.unid:assign(unit_selector.itemid)
    else
        -- save the unid vector and set our meddle flag
        unit_selector.itemid:assign(unit_selector.unid)
        unit_selector.selected[0] = 1
    end
    -- do the actual filtering
    for idx=#unit_selector.unid-1,0,-1 do
        if not filter_matches(unit_selector.unid[idx], filter) then
            unit_selector.unid:erase(idx)
        end
    end
end

local SORT_WIDGET_NAMES = {
    'sort',
    'sort_any_melee',
    'sort_any_ranged',
    'sort_name',
    'sort_teacher',
    'sort_tactics',
    'sort_migrant_wave',
    'sort_stress',
    'sort_mental_stability',
    'sort_axe',
    'sort_sword',
    'sort_mace',
    'sort_hammer',
    'sort_spear',
    'sort_crossbow',
    'sort_melee_combat_potential',
    'sort_ranged_combat_potential',
}

function SquadAssignmentOverlay:refresh_list(sort_widget, sort_fn)
    sort_widget = sort_widget or 'sort'
    sort_fn = sort_fn or self.subviews.sort:getOptionValue()
    if sort_fn == sort_noop then
        self.subviews[sort_widget]:cycle()
        return
    end
    for _,widget_name in ipairs(SORT_WIDGET_NAMES) do
        self.subviews[widget_name]:setOption(sort_fn)
    end
    local filter = {
        search=self.subviews.search.text,
        military=self.subviews.military:getOptionValue(),
        officials=self.subviews.officials:getOptionValue(),
        nobles=self.subviews.nobles:getOptionValue(),
    }
    filter_vector(filter, self.prev_filter or {})
    self.prev_filter = filter
    utils.sort_vector(unit_selector.unid, nil, sort_fn)
end

function SquadAssignmentOverlay:onInput(keys)
    if keys._MOUSE_R_DOWN or
        keys._MOUSE_L_DOWN and not self:getMouseFramePos()
    then
        -- if any click is made outside of our window, we may need to refresh our list
        self.dirty = true
    end
    return SquadAssignmentOverlay.super.onInput(self, keys)
end

function SquadAssignmentOverlay:onRenderFrame(dc, frame_rect)
    SquadAssignmentOverlay.super.onRenderFrame(self, dc, frame_rect)
    if self.dirty then
        self:refresh_list()
        self.dirty = false
    end
end

OVERLAY_WIDGETS = {
    squad_assignment=SquadAssignmentOverlay,
}

--[[
local utils = require('utils')
local units = require('plugins.sort.units')
local items = require('plugins.sort.items')

orders = orders or {}
orders.units = units.orders
orders.items = items.orders

function parse_ordering_spec(type,...)
    local group = orders[type]
    if group == nil then
        dfhack.printerr('Invalid ordering class: '..tostring(type))
        return nil
    end

    local specs = table.pack(...)
    local rv = { }

    for _,spec in ipairs(specs) do
        local nil_first = false
        if string.sub(spec,1,1) == '<' then
            nil_first = true
            spec = string.sub(spec,2)
        end

        local reverse = false
        if string.sub(spec,1,1) == '>' then
            reverse = true
            spec = string.sub(spec,2)
        end

        local cm = group[spec]

        if cm == nil then
            dfhack.printerr('Unknown order for '..type..': '..tostring(spec))
            return nil
        end

        if nil_first or reverse then
            cm = copyall(cm)
            cm.nil_first = nil_first
            cm.reverse = reverse
        end

        rv[#rv+1] = cm
    end

    return rv
end

make_sort_order = utils.make_sort_order
]]

return _ENV
