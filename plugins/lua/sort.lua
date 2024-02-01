local _ENV = mkmodule('plugins.sort')

local gui = require('gui')
local overlay = require('plugins.overlay')
local setbelief = reqscript('modtools/set-belief')
local sortoverlay = require('plugins.sort.sortoverlay')
local utils = require('utils')
local widgets = require('gui.widgets')

local GLOBAL_KEY = 'sort'

local CH_UP = string.char(30)
local CH_DN = string.char(31)

local function get_rating(val, baseline, range, highest, high, med, low)
    val = val - (baseline or 0)
    range = range or 100
    local percentile = (math.min(range, val) * 100) // range
    if percentile < (low or 25) then return percentile, COLOR_RED end
    if percentile < (med or 50) then return percentile, COLOR_LIGHTRED end
    if percentile < (high or 75) then return percentile, COLOR_YELLOW end
    if percentile < (highest or 90) then return percentile, COLOR_GREEN end
    return percentile, COLOR_LIGHTGREEN
end

local function sort_noop(a, b)
    -- this function is used as a marker and never actually gets called
    error('sort_noop should not be called')
end

local function get_name(unit)
    return unit and dfhack.toSearchNormalized(dfhack.TranslateName(dfhack.units.getVisibleName(unit)))
end

local function sort_by_name_desc(unit_id_1, unit_id_2)
    if unit_id_1 == unit_id_2 then return 0 end
    local unit1 = df.unit.find(unit_id_1)
    local unit2 = df.unit.find(unit_id_2)
    if not unit1 then return -1 end
    if not unit2 then return 1 end
    local name1 = get_name(unit1)
    local name2 = get_name(unit2)
    return utils.compare_name(name1, name2)
end

local function sort_by_name_asc(unit_id_1, unit_id_2)
    if unit_id_1 == unit_id_2 then return 0 end
    local unit1 = df.unit.find(unit_id_1)
    local unit2 = df.unit.find(unit_id_2)
    if not unit1 then return -1 end
    if not unit2 then return 1 end
    local name1 = get_name(unit1)
    local name2 = get_name(unit2)
    return utils.compare_name(name2, name1)
end

local active_units = df.global.world.units.active
local active_idx_cache = {}
local function get_active_idx_cache()
    local num_active_units = #active_units
    if num_active_units == 0 or active_idx_cache[active_units[num_active_units-1].id] ~= num_active_units-1 then
        active_idx_cache = {}
        for i,active_unit in ipairs(active_units) do
            active_idx_cache[active_unit.id] = i
        end
    end
    return active_idx_cache
end

local function is_original_dwarf(unit)
    return df.global.plotinfo.fortress_age == unit.curse.time_on_site // 10
end

local WAVE_END_GAP = 10000

local function get_most_recent_wave_oldest_active_idx(cache)
    local oldest_unit
    for idx=#active_units-1,0,-1 do
        local unit = active_units[idx]
        if not dfhack.units.isCitizen(unit) then goto continue end
        if oldest_unit and unit.curse.time_on_site - oldest_unit.curse.time_on_site > WAVE_END_GAP then
            return cache[oldest_unit.id]
        else
            oldest_unit = unit
        end
        ::continue::
    end
end

-- return green for most recent wave, red for the first wave, yellow for all others
-- rating is a three digit number that indicates the (potentially approximate) order
local function get_arrival_rating(unit)
    local cache = get_active_idx_cache()
    local unit_active_idx = cache[unit.id]
    if not unit_active_idx then return end
    local most_recent_wave_oldest_active_idx = get_most_recent_wave_oldest_active_idx(cache)
    if not most_recent_wave_oldest_active_idx then return end
    local num_active_units = #active_units
    local rating = num_active_units < 1000 and unit_active_idx or ((unit_active_idx * 1000) // #active_units)
    if most_recent_wave_oldest_active_idx < unit_active_idx then
        return rating, COLOR_LIGHTGREEN
    end
    if is_original_dwarf(unit) then
        return rating, COLOR_RED
    end
    return rating, COLOR_YELLOW
end

local function sort_by_arrival_desc(unit_id_1, unit_id_2)
    if unit_id_1 == unit_id_2 then return 0 end
    local cache = get_active_idx_cache()
    if not cache[unit_id_1] then return -1 end
    if not cache[unit_id_2] then return 1 end
    return utils.compare(cache[unit_id_2], cache[unit_id_1])
end

local function sort_by_arrival_asc(unit_id_1, unit_id_2)
    if unit_id_1 == unit_id_2 then return 0 end
    local cache = get_active_idx_cache()
    if not cache[unit_id_1] then return -1 end
    if not cache[unit_id_2] then return 1 end
    return utils.compare(cache[unit_id_1], cache[unit_id_2])
end

local function get_stress(unit)
    return unit and
        unit.status.current_soul and
        unit.status.current_soul.personality.stress
end

local function get_stress_rating(unit)
    return get_rating(dfhack.units.getStressCategory(unit), 0, 100, 4, 3, 2, 1)
end

local function sort_by_stress_desc(unit_id_1, unit_id_2)
    if unit_id_1 == unit_id_2 then return 0 end
    local unit1 = df.unit.find(unit_id_1)
    local unit2 = df.unit.find(unit_id_2)
    if not unit1 then return -1 end
    if not unit2 then return 1 end
    local happiness1 = get_stress(unit1)
    local happiness2 = get_stress(unit2)
    if happiness1 == happiness2 then
        return sort_by_name_desc(unit_id_1, unit_id_2)
    end
    if not happiness2 then return -1 end
    if not happiness1 then return 1 end
    return utils.compare(happiness2, happiness1)
end

local function sort_by_stress_asc(unit_id_1, unit_id_2)
    if unit_id_1 == unit_id_2 then return 0 end
    local unit1 = df.unit.find(unit_id_1)
    local unit2 = df.unit.find(unit_id_2)
    if not unit1 then return -1 end
    if not unit2 then return 1 end
    local happiness1 = get_stress(unit1)
    local happiness2 = get_stress(unit2)
    if happiness1 == happiness2 then
        return sort_by_name_desc(unit_id_1, unit_id_2)
    end
    if not happiness2 then return 1 end
    if not happiness1 then return -1 end
    return utils.compare(happiness1, happiness2)
end

local function get_skill(skill, unit)
    return unit and
        unit.status.current_soul and
        (utils.binsearch(unit.status.current_soul.skills, skill, 'id'))
end

local function get_skill_rating(skill, unit)
    local uskill = get_skill(skill, unit)
    if not uskill then return nil end
    return get_rating(uskill.rating, 0, 100, 10, 5, 1, 0)
end

local MELEE_WEAPON_SKILLS = {
    df.job_skill.AXE,
    df.job_skill.SWORD,
    df.job_skill.MACE,
    df.job_skill.HAMMER,
    df.job_skill.SPEAR,
}

local function melee_skill_effectiveness(unit)
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
    for _, skill in ipairs(MELEE_WEAPON_SKILLS) do
        local melee_skill = dfhack.units.getNominalSkill(unit, skill, true)
        skill_rating = math.max(skill_rating, melee_skill)
    end
    local melee_combat_rating = dfhack.units.getNominalSkill(unit, df.job_skill.MELEE_COMBAT, true)

    local rating = skill_rating * 27000 + melee_combat_rating * 9000
            + strength * 180 + body_size_base * 100 + kinesthetic_sense * 50 + endurance * 50
            + agility * 30 + toughness * 20 + willpower * 20 + spatial_sense * 20
    return rating
end

local function get_melee_skill_effectiveness_rating(unit)
    return get_rating(melee_skill_effectiveness(unit), 350000, 2750000, 64, 52, 40, 28)
end

local function make_sort_by_melee_skill_effectiveness_desc()
    return function(unit_id_1, unit_id_2)
        if unit_id_1 == unit_id_2 then return 0 end
        local unit1 = df.unit.find(unit_id_1)
        local unit2 = df.unit.find(unit_id_2)
        if not unit1 then return -1 end
        if not unit2 then return 1 end
        local rating1 = melee_skill_effectiveness(unit1)
        local rating2 = melee_skill_effectiveness(unit2)
        if rating1 == rating2 then return sort_by_name_desc(unit_id_1, unit_id_2) end
        return utils.compare(rating2, rating1)
    end
end

local function make_sort_by_melee_skill_effectiveness_asc()
    return function(unit_id_1, unit_id_2)
        if unit_id_1 == unit_id_2 then return 0 end
        local unit1 = df.unit.find(unit_id_1)
        local unit2 = df.unit.find(unit_id_2)
        if not unit1 then return -1 end
        if not unit2 then return 1 end
        local rating1 = melee_skill_effectiveness(unit1)
        local rating2 = melee_skill_effectiveness(unit2)
        if rating1 == rating2 then return sort_by_name_desc(unit_id_1, unit_id_2) end
        return utils.compare(rating1, rating2)
    end
end

local RANGED_WEAPON_SKILLS = {
    df.job_skill.CROSSBOW,
}

-- Function could easily be adapted to different weapon types.
local function ranged_skill_effectiveness(unit)
    -- Physical attributes
    local agility = dfhack.units.getPhysicalAttrValue(unit, df.physical_attribute_type.AGILITY)

    -- Mental attributes
    local spatial_sense = dfhack.units.getMentalAttrValue(unit, df.mental_attribute_type.SPATIAL_SENSE)
    local kinesthetic_sense = dfhack.units.getMentalAttrValue(unit, df.mental_attribute_type.KINESTHETIC_SENSE)
    local focus = dfhack.units.getMentalAttrValue(unit, df.mental_attribute_type.FOCUS)

    -- Skills
    -- Finding the highest skill
    local skill_rating = 0
    for _, skill in ipairs(RANGED_WEAPON_SKILLS) do
        local ranged_skill = dfhack.units.getNominalSkill(unit, skill, true)
        skill_rating = math.max(skill_rating, ranged_skill)
    end
    local ranged_combat = dfhack.units.getNominalSkill(unit, df.job_skill.RANGED_COMBAT, true)

    local rating = skill_rating * 24000 + ranged_combat * 8000
    + agility * 15 + spatial_sense * 15 + kinesthetic_sense * 6 + focus * 6
    return rating
end

local function get_ranged_skill_effectiveness_rating(unit)
    return get_rating(ranged_skill_effectiveness(unit), 0, 800000, 72, 52, 31, 11)
end

local function make_sort_by_ranged_skill_effectiveness_desc()
    return function(unit_id_1, unit_id_2)
        if unit_id_1 == unit_id_2 then return 0 end
        local unit1 = df.unit.find(unit_id_1)
        local unit2 = df.unit.find(unit_id_2)
        if not unit1 then return -1 end
        if not unit2 then return 1 end
        local rating1 = ranged_skill_effectiveness(unit1)
        local rating2 = ranged_skill_effectiveness(unit2)
        if rating1 == rating2 then return sort_by_name_desc(unit_id_1, unit_id_2) end
        return utils.compare(rating2, rating1)
    end
end

local function make_sort_by_ranged_skill_effectiveness_asc()
    return function(unit_id_1, unit_id_2)
        if unit_id_1 == unit_id_2 then return 0 end
        local unit1 = df.unit.find(unit_id_1)
        local unit2 = df.unit.find(unit_id_2)
        if not unit1 then return -1 end
        if not unit2 then return 1 end
        local rating1 = ranged_skill_effectiveness(unit1)
        local rating2 = ranged_skill_effectiveness(unit2)
        if rating1 == rating2 then return sort_by_name_desc(unit_id_1, unit_id_2) end
        return utils.compare(rating1, rating2)
    end
end

local function make_sort_by_skill_desc(sort_skill)
    return function(unit_id_1, unit_id_2)
        if unit_id_1 == unit_id_2 then return 0 end
        if unit_id_1 == -1 then return -1 end
        if unit_id_2 == -1 then return 1 end
        local s1 = get_skill(sort_skill, df.unit.find(unit_id_1))
        local s2 = get_skill(sort_skill, df.unit.find(unit_id_2))
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
        local s1 = get_skill(sort_skill, df.unit.find(unit_id_1))
        local s2 = get_skill(sort_skill, df.unit.find(unit_id_2))
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
local function get_mental_stability(unit)
    local altruism = unit.status.current_soul.personality.traits.ALTRUISM
    local anxiety_propensity = unit.status.current_soul.personality.traits.ANXIETY_PROPENSITY
    local bravery = unit.status.current_soul.personality.traits.BRAVERY
    local cheer_propensity = unit.status.current_soul.personality.traits.CHEER_PROPENSITY
    local curious = unit.status.current_soul.personality.traits.CURIOUS
    local discord = unit.status.current_soul.personality.traits.DISCORD
    local dutifulness = unit.status.current_soul.personality.traits.DUTIFULNESS
    local emotionally_obsessive = unit.status.current_soul.personality.traits.EMOTIONALLY_OBSESSIVE
    local humor = unit.status.current_soul.personality.traits.HUMOR
    local love_propensity = unit.status.current_soul.personality.traits.LOVE_PROPENSITY
    local perseverence = unit.status.current_soul.personality.traits.PERSEVERENCE
    local politeness = unit.status.current_soul.personality.traits.POLITENESS
    local privacy = unit.status.current_soul.personality.traits.PRIVACY
    local stress_vulnerability = unit.status.current_soul.personality.traits.STRESS_VULNERABILITY
    local tolerant = unit.status.current_soul.personality.traits.TOLERANT

    local craftsmanship = setbelief.getUnitBelief(unit, df.value_type['CRAFTSMANSHIP'])
    local family = setbelief.getUnitBelief(unit, df.value_type['FAMILY'])
    local harmony = setbelief.getUnitBelief(unit, df.value_type['HARMONY'])
    local independence = setbelief.getUnitBelief(unit, df.value_type['INDEPENDENCE'])
    local knowledge = setbelief.getUnitBelief(unit, df.value_type['KNOWLEDGE'])
    local leisure_time = setbelief.getUnitBelief(unit, df.value_type['LEISURE_TIME'])
    local nature = setbelief.getUnitBelief(unit, df.value_type['NATURE'])
    local skill = setbelief.getUnitBelief(unit, df.value_type['SKILL'])

    -- calculate the rating using the defined variables
    local rating = (craftsmanship * -0.01) + (family * -0.09) + (harmony * 0.05)
                 + (independence * 0.06) + (knowledge * -0.30) + (leisure_time * 0.24)
                 + (nature * 0.27) + (skill * -0.21) + (altruism * 0.13)
                 + (anxiety_propensity * -0.06) + (bravery * 0.06)
                 + (cheer_propensity * 0.41) + (curious * -0.06) + (discord * 0.14)
                 + (dutifulness * -0.03) + (emotionally_obsessive * -0.13)
                 + (humor * -0.05) + (love_propensity * 0.15) + (perseverence * -0.07)
                 + (politeness * -0.14) + (privacy * 0.03) + (stress_vulnerability * -0.20)
                 + (tolerant * -0.11)

    return rating
end

local function sort_by_mental_stability_desc(unit_id_1, unit_id_2)
    if unit_id_1 == unit_id_2 then return 0 end
    local unit1 = df.unit.find(unit_id_1)
    local unit2 = df.unit.find(unit_id_2)
    if not unit1 then return -1 end
    if not unit2 then return 1 end
    local rating1 = get_mental_stability(unit1)
    local rating2 = get_mental_stability(unit2)
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
    local rating1 = get_mental_stability(unit1)
    local rating2 = get_mental_stability(unit2)
    if rating1 == rating2 then
        return sort_by_stress_desc(unit_id_1, unit_id_2)
    end
    return utils.compare(rating1, rating2)
end

-- Statistical rating that is higher for more potent dwarves in long run melee military training
-- Rating considers fighting melee opponents
-- Wounds are not considered!
local function get_melee_combat_potential(unit)
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

    -- assume highest skill ratings
    local skill_rating = df.skill_rating.Legendary5
    local melee_combat_rating = df.skill_rating.Legendary5

    -- melee combat potential rating
    local rating = skill_rating * 27000 + melee_combat_rating * 9000
            + strength * 180 + body_size_base * 100 + kinesthetic_sense * 50 + endurance * 50
            + agility * 30 + toughness * 20 + willpower * 20 + spatial_sense * 20
    return rating
end

local function get_melee_combat_potential_rating(unit)
    return get_rating(get_melee_combat_potential(unit), 350000, 2750000, 64, 52, 40, 28)
end

local function sort_by_melee_combat_potential_desc(unit_id_1, unit_id_2)
    if unit_id_1 == unit_id_2 then return 0 end
    local unit1 = df.unit.find(unit_id_1)
    local unit2 = df.unit.find(unit_id_2)
    if not unit1 then return -1 end
    if not unit2 then return 1 end
    local rating1 = get_melee_combat_potential(unit1)
    local rating2 = get_melee_combat_potential(unit2)
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
    local rating1 = get_melee_combat_potential(unit1)
    local rating2 = get_melee_combat_potential(unit2)
    if rating1 == rating2 then
        return sort_by_mental_stability_asc(unit_id_1, unit_id_2)
    end
    return utils.compare(rating1, rating2)
end

-- Statistical rating that is higher for more potent dwarves in long run ranged military training
-- Wounds are not considered!
local function get_ranged_combat_potential(unit)
    -- Physical attributes
    local agility = unit.body.physical_attrs.AGILITY.max_value

    -- Mental attributes
    local focus = unit.status.current_soul.mental_attrs.FOCUS.max_value
    local spatial_sense = unit.status.current_soul.mental_attrs.SPATIAL_SENSE.max_value
    local kinesthetic_sense = unit.status.current_soul.mental_attrs.KINESTHETIC_SENSE.max_value

    -- assume highest skill ratings
    local skill_rating = df.skill_rating.Legendary5
    local ranged_combat = df.skill_rating.Legendary5

    -- ranged combat potential formula
    local rating = skill_rating * 24000 + ranged_combat * 8000
            + agility * 15 + spatial_sense * 15 + kinesthetic_sense * 6 + focus * 6
    return rating
end

local function get_ranged_combat_potential_rating(unit)
    return get_rating(get_ranged_combat_potential(unit), 0, 800000, 72, 52, 31, 11)
end

local function sort_by_ranged_combat_potential_desc(unit_id_1, unit_id_2)
    if unit_id_1 == unit_id_2 then return 0 end
    local unit1 = df.unit.find(unit_id_1)
    local unit2 = df.unit.find(unit_id_2)
    if not unit1 then return -1 end
    if not unit2 then return 1 end
    local rating1 = get_ranged_combat_potential(unit1)
    local rating2 = get_ranged_combat_potential(unit2)
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
    local rating1 = get_ranged_combat_potential(unit1)
    local rating2 = get_ranged_combat_potential(unit2)
    if rating1 == rating2 then
        return sort_by_mental_stability_asc(unit_id_1, unit_id_2)
    end
    return utils.compare(rating1, rating2)
end

local function get_need(unit)
    if not unit or not unit.status.current_soul then return end
    for _, need in ipairs(unit.status.current_soul.personality.needs) do
        if need.id == df.need_type.MartialTraining and need.focus_level < 0 then
            return -need.focus_level
        end
    end
end

local function get_need_rating(unit)
    local focus_level = get_need(unit)
    if not focus_level then return end
    -- convert to stress ratings so we can use stress faces as labels
    if focus_level > 100000 then return 0 end
    if focus_level > 10000 then return 1 end
    if focus_level > 1000 then return 2 end
    if focus_level > 100 then return 3 end
    return 6
end

local function sort_by_need_desc(unit_id_1, unit_id_2)
    if unit_id_1 == unit_id_2 then return 0 end
    local unit1 = df.unit.find(unit_id_1)
    local unit2 = df.unit.find(unit_id_2)
    if not unit1 then return -1 end
    if not unit2 then return 1 end
    local rating1 = get_need(unit1)
    local rating2 = get_need(unit2)
    if rating1 == rating2 then
        return sort_by_stress_desc(unit_id_1, unit_id_2)
    end
    if not rating2 then return -1 end
    if not rating1 then return 1 end
    return utils.compare(rating2, rating1)
end

local function sort_by_need_asc(unit_id_1, unit_id_2)
    if unit_id_1 == unit_id_2 then return 0 end
    local unit1 = df.unit.find(unit_id_1)
    local unit2 = df.unit.find(unit_id_2)
    if not unit1 then return -1 end
    if not unit2 then return 1 end
    local rating1 = get_need(unit1)
    local rating2 = get_need(unit2)
    if rating1 == rating2 then
        return sort_by_stress_asc(unit_id_1, unit_id_2)
    end
    if not rating2 then return 1 end
    if not rating1 then return -1 end
    return utils.compare(rating1, rating2)
end

local sort_by_any_melee_desc=make_sort_by_melee_skill_effectiveness_desc()
local sort_by_any_melee_asc=make_sort_by_melee_skill_effectiveness_asc()
local sort_by_any_ranged_desc=make_sort_by_ranged_skill_effectiveness_desc()
local sort_by_any_ranged_asc=make_sort_by_ranged_skill_effectiveness_asc()
local sort_by_teacher_desc=make_sort_by_skill_desc(df.job_skill.TEACHING)
local sort_by_teacher_asc=make_sort_by_skill_asc(df.job_skill.TEACHING)
local sort_by_tactics_desc=make_sort_by_skill_desc(df.job_skill.MILITARY_TACTICS)
local sort_by_tactics_asc=make_sort_by_skill_asc(df.job_skill.MILITARY_TACTICS)
local sort_by_axe_desc=make_sort_by_skill_desc(df.job_skill.AXE)
local sort_by_axe_asc=make_sort_by_skill_asc(df.job_skill.AXE)
local sort_by_sword_desc=make_sort_by_skill_desc(df.job_skill.SWORD)
local sort_by_sword_asc=make_sort_by_skill_asc(df.job_skill.SWORD)
local sort_by_mace_desc=make_sort_by_skill_desc(df.job_skill.MACE)
local sort_by_mace_asc=make_sort_by_skill_asc(df.job_skill.MACE)
local sort_by_hammer_desc=make_sort_by_skill_desc(df.job_skill.HAMMER)
local sort_by_hammer_asc=make_sort_by_skill_asc(df.job_skill.HAMMER)
local sort_by_spear_desc=make_sort_by_skill_desc(df.job_skill.SPEAR)
local sort_by_spear_asc=make_sort_by_skill_asc(df.job_skill.SPEAR)
local sort_by_crossbow_desc=make_sort_by_skill_desc(df.job_skill.CROSSBOW)
local sort_by_crossbow_asc=make_sort_by_skill_asc(df.job_skill.CROSSBOW)

local SORT_LIBRARY = {
    {label='melee effectiveness', desc_fn=sort_by_any_melee_desc, asc_fn=sort_by_any_melee_asc, rating_fn=get_melee_skill_effectiveness_rating},
    {label='ranged effectiveness', desc_fn=sort_by_any_ranged_desc, asc_fn=sort_by_any_ranged_asc, rating_fn=get_ranged_skill_effectiveness_rating},
    {label='name', desc_fn=sort_by_name_desc, asc_fn=sort_by_name_asc},
    {label='teacher skill', desc_fn=sort_by_teacher_desc, asc_fn=sort_by_teacher_asc, rating_fn=curry(get_skill_rating, df.job_skill.TEACHING)},
    {label='tactics skill', desc_fn=sort_by_tactics_desc, asc_fn=sort_by_tactics_asc, rating_fn=curry(get_skill_rating, df.job_skill.MILITARY_TACTICS)},
    {label='arrival order', desc_fn=sort_by_arrival_desc, asc_fn=sort_by_arrival_asc, rating_fn=get_arrival_rating},
    {label='stress level', desc_fn=sort_by_stress_desc, asc_fn=sort_by_stress_asc, rating_fn=get_stress_rating, use_stress_faces=true},
    {label='need for training', desc_fn=sort_by_need_desc, asc_fn=sort_by_need_asc, rating_fn=get_need_rating, use_stress_faces=true},
    {label='axe skill', desc_fn=sort_by_axe_desc, asc_fn=sort_by_axe_asc, rating_fn=curry(get_skill_rating, df.job_skill.AXE)},
    {label='sword skill', desc_fn=sort_by_sword_desc, asc_fn=sort_by_sword_asc, rating_fn=curry(get_skill_rating, df.job_skill.SWORD)},
    {label='mace skill', desc_fn=sort_by_mace_desc, asc_fn=sort_by_mace_asc, rating_fn=curry(get_skill_rating, df.job_skill.MACE)},
    {label='hammer skill', desc_fn=sort_by_hammer_desc, asc_fn=sort_by_hammer_asc, rating_fn=curry(get_skill_rating, df.job_skill.HAMMER)},
    {label='spear skill', desc_fn=sort_by_spear_desc, asc_fn=sort_by_spear_asc, rating_fn=curry(get_skill_rating, df.job_skill.SPEAR)},
    {label='crossbow skill', desc_fn=sort_by_crossbow_desc, asc_fn=sort_by_crossbow_asc, rating_fn=curry(get_skill_rating, df.job_skill.CROSSBOW)},
    {label='melee potential', desc_fn=sort_by_melee_combat_potential_desc, asc_fn=sort_by_melee_combat_potential_asc, rating_fn=get_melee_combat_potential_rating},
    {label='ranged potential', desc_fn=sort_by_ranged_combat_potential_desc, asc_fn=sort_by_ranged_combat_potential_asc, rating_fn=get_ranged_combat_potential_rating},
}

local RATING_FNS = {}
local STRESS_FACE_FNS = {}
for _, opt in ipairs(SORT_LIBRARY) do
    RATING_FNS[opt.desc_fn] = opt.rating_fn
    RATING_FNS[opt.asc_fn] = opt.rating_fn
    if opt.use_stress_faces then
        STRESS_FACE_FNS[opt.desc_fn] = true
        STRESS_FACE_FNS[opt.asc_fn] = true
    end
end

-- ----------------------
-- SquadAssignmentOverlay
--

SquadAssignmentOverlay = defclass(SquadAssignmentOverlay, overlay.OverlayWidget)
SquadAssignmentOverlay.ATTRS{
    desc='Adds search, sort, and filter capabilities to the squad assignment screen.',
    default_pos={x=18, y=5},
    default_enabled=true,
    viewscreens='dwarfmode/UnitSelector/SQUAD_FILL_POSITION',
    version='2',
    frame={w=38, h=33},
}

-- allow initial spacebar or two successive spacebars to fall through and
-- pause/unpause the game
local function search_on_char(ch, text)
    if ch == ' ' then return text:match('%S$') end
    return ch:match('[%l _-]')
end

function SquadAssignmentOverlay:init()
    self.dirty = true

    local sort_options = {}
    for _, opt in ipairs(SORT_LIBRARY) do
        table.insert(sort_options, {
            label=opt.label..CH_DN,
            value=opt.desc_fn,
            pen=COLOR_GREEN,
        })
        table.insert(sort_options, {
            label=opt.label..CH_UP,
            value=opt.asc_fn,
            pen=COLOR_YELLOW,
        })
    end

    local main_panel = widgets.Panel{
        frame={l=0, r=0, t=0, b=0},
        frame_style=gui.FRAME_PANEL,
        frame_background=gui.CLEAR_PEN,
        autoarrange_subviews=true,
        autoarrange_gap=1,
    }
    main_panel:addviews{
        widgets.EditField{
            view_id='search',
            frame={l=0},
            label_text='Search: ',
            on_char=search_on_char,
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
                    options=sort_options,
                    initial_option=sort_by_any_melee_desc,
                    on_change=self:callback('refresh_list', 'sort'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_any_melee',
                    frame={t=2, l=0, w=11},
                    options={
                        {label='melee eff.', value=sort_noop},
                        {label='melee eff.'..CH_DN, value=sort_by_any_melee_desc, pen=COLOR_GREEN},
                        {label='melee eff.'..CH_UP, value=sort_by_any_melee_asc, pen=COLOR_YELLOW},
                    },
                    initial_option=sort_by_any_melee_desc,
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_any_melee'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_any_ranged',
                    frame={t=2, r=8, w=12},
                    options={
                        {label='ranged eff.', value=sort_noop},
                        {label='ranged eff.'..CH_DN, value=sort_by_any_ranged_desc, pen=COLOR_GREEN},
                        {label='ranged eff.'..CH_UP, value=sort_by_any_ranged_asc, pen=COLOR_YELLOW},
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
                        {label='teacher'..CH_DN, value=sort_by_teacher_desc, pen=COLOR_GREEN},
                        {label='teacher'..CH_UP, value=sort_by_teacher_asc, pen=COLOR_YELLOW},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_teacher'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_tactics',
                    frame={t=4, l=10, w=8},
                    options={
                        {label='tactics', value=sort_noop},
                        {label='tactics'..CH_DN, value=sort_by_tactics_desc, pen=COLOR_GREEN},
                        {label='tactics'..CH_UP, value=sort_by_tactics_asc, pen=COLOR_YELLOW},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_tactics'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_arrival',
                    frame={t=4, r=0, w=14},
                    options={
                        {label='arrival order', value=sort_noop},
                        {label='arrival order'..CH_DN, value=sort_by_arrival_desc, pen=COLOR_GREEN},
                        {label='arrival order'..CH_UP, value=sort_by_arrival_asc, pen=COLOR_YELLOW},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_arrival'),
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
                    view_id='sort_need',
                    frame={t=6, r=0, w=18},
                    options={
                        {label='need for training', value=sort_noop},
                        {label='need for training'..CH_DN, value=sort_by_need_desc, pen=COLOR_GREEN},
                        {label='need for training'..CH_UP, value=sort_by_need_asc, pen=COLOR_YELLOW},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_need'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_axe',
                    frame={t=8, l=0, w=4},
                    options={
                        {label='axe', value=sort_noop},
                        {label='axe'..CH_DN, value=sort_by_axe_desc, pen=COLOR_GREEN},
                        {label='axe'..CH_UP, value=sort_by_axe_asc, pen=COLOR_YELLOW},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_axe'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_sword',
                    frame={t=8, w=6},
                    options={
                        {label='sword', value=sort_noop},
                        {label='sword'..CH_DN, value=sort_by_sword_desc, pen=COLOR_GREEN},
                        {label='sword'..CH_UP, value=sort_by_sword_asc, pen=COLOR_YELLOW},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_sword'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_mace',
                    frame={t=8, r=0, w=5},
                    options={
                        {label='mace', value=sort_noop},
                        {label='mace'..CH_DN, value=sort_by_mace_desc, pen=COLOR_GREEN},
                        {label='mace'..CH_UP, value=sort_by_mace_asc, pen=COLOR_YELLOW},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_mace'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_hammer',
                    frame={t=10, l=0, w=7},
                    options={
                        {label='hammer', value=sort_noop},
                        {label='hammer'..CH_DN, value=sort_by_hammer_desc, pen=COLOR_GREEN},
                        {label='hammer'..CH_UP, value=sort_by_hammer_asc, pen=COLOR_YELLOW},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_hammer'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_spear',
                    frame={t=10, w=6},
                    options={
                        {label='spear', value=sort_noop},
                        {label='spear'..CH_DN, value=sort_by_spear_desc, pen=COLOR_GREEN},
                        {label='spear'..CH_UP, value=sort_by_spear_asc, pen=COLOR_YELLOW},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_spear'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_crossbow',
                    frame={t=10, r=0, w=9},
                    options={
                        {label='crossbow', value=sort_noop},
                        {label='crossbow'..CH_DN, value=sort_by_crossbow_desc, pen=COLOR_GREEN},
                        {label='crossbow'..CH_UP, value=sort_by_crossbow_asc, pen=COLOR_YELLOW},
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
            label='  Appointed officials:',
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
            frame={l=0},
            key='CUSTOM_SHIFT_N',
            label='             Nobility:',
            options={
                {label='Include', value='include', pen=COLOR_GREEN},
                {label='Only', value='only', pen=COLOR_YELLOW},
                {label='Exclude', value='exclude', pen=COLOR_RED},
            },
            initial_option='include',
            on_change=function() self:refresh_list() end,
        },
        widgets.CycleHotkeyLabel{
            view_id='infant',
            frame={l=0},
            key='CUSTOM_SHIFT_M',
            label=' Mothers with infants:',
            options={
                {label='Include', value='include', pen=COLOR_GREEN},
                {label='Only', value='only', pen=COLOR_YELLOW},
                {label='Exclude', value='exclude', pen=COLOR_RED},
            },
            initial_option='include',
            on_change=function() self:refresh_list() end,
        },
        widgets.CycleHotkeyLabel{
            view_id='unstable',
            frame={l=0},
            key='CUSTOM_SHIFT_D',
            label='      Dislikes combat:',
            options={
                {label='Include', value='include', pen=COLOR_GREEN},
                {label='Only', value='only', pen=COLOR_YELLOW},
                {label='Exclude', value='exclude', pen=COLOR_RED},
            },
            initial_option='include',
            on_change=function() self:refresh_list() end,
        },
        widgets.CycleHotkeyLabel{
            view_id='maimed',
            frame={l=0},
            key='CUSTOM_SHIFT_I',
            label='   Critically injured:',
            options={
                {label='Include', value='include', pen=COLOR_GREEN},
                {label='Only', value='only', pen=COLOR_YELLOW},
                {label='Exclude', value='exclude', pen=COLOR_RED},
            },
            initial_option='include',
            on_change=function() self:refresh_list() end,
        },
        widgets.HotkeyLabel{
            key='CUSTOM_SHIFT_A',
            label='Toggle all filters',
            on_activate=function()
                local target = self.subviews.military:getOptionValue() == 'exclude' and 'include' or 'exclude'
                self.subviews.military:setOption(target)
                self.subviews.officials:setOption(target)
                self.subviews.nobles:setOption(target)
                self.subviews.infant:setOption(target)
                self.subviews.unstable:setOption(target)
                self.subviews.maimed:setOption(target)
                self:refresh_list()
            end,
        },
    }

    self:addviews{
        main_panel,
        widgets.HelpButton{
            frame={t=0, r=1},
            command='sort',
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
    for _,occupation in ipairs(unit.occupations) do
        if occupation.type ~= df.occupation_type.MERCENARY then
            return true
        end
    end
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

local function has_infant(unit)
    for _, baby in ipairs(df.global.world.units.other.ANY_BABY2) do
        if baby.relationship_ids.Mother == unit.id then
            return true
        end
    end
    return false
end

local function is_unstable(unit)
    -- stddev percentiles are 61, 48, 35, 23
    -- let's go with one stddev below the mean (35) as the cutoff
    local _, color = get_rating(get_mental_stability(unit), -40, 80, 35, 0, 0, 0)
    return color ~= COLOR_LIGHTGREEN
end

local function is_maimed(unit)
    return not unit.flags2.vision_good or
        unit.status2.limbs_grasp_count < 2 or
        unit.status2.limbs_stand_count == 0
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
    if filter.infant == 'only' and not has_infant(unit) then return false end
    if filter.infant == 'exclude' and has_infant(unit) then return false end
    if filter.unstable == 'only' and not is_unstable(unit) then return false end
    if filter.unstable == 'exclude' and is_unstable(unit) then return false end
    if filter.maimed == 'only' and not is_maimed(unit) then return false end
    if filter.maimed == 'exclude' and is_maimed(unit) then return false end
    if #filter.search == 0 then return true end
    local search_key = sortoverlay.get_unit_search_key(unit)
    return normalize_search_key(search_key):find(dfhack.toSearchNormalized(filter.search))
end

local function is_noop_filter(filter)
    return #filter.search == 0 and
        filter.military == 'include' and
        filter.officials == 'include' and
        filter.nobles == 'include' and
        filter.infant == 'include' and
        filter.unstable == 'include' and
        filter.maimed == 'include'
end

local function is_filter_equal(a, b)
    return a.search == b.search and
        a.military == b.military and
        a.officials == b.officials and
        a.nobles == b.nobles and
        a.infant == b.infant and
        a.unstable == b.unstable and
        a.maimed == b.maimed
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
    -- fix up scroll position if it would be off the end of the list
    if unit_selector.scroll_position + 10 > #unit_selector.unid then
        unit_selector.scroll_position = math.max(0, #unit_selector.unid - 10)
    end
end

local use_stress_faces = false
local rating_annotations = {}

local function annotate_visible_units(sort_fn)
    use_stress_faces = STRESS_FACE_FNS[sort_fn]
    rating_annotations = {}
    rating_fn = RATING_FNS[sort_fn]
    local max_idx = math.min(#unit_selector.unid-1, unit_selector.scroll_position+9)
    for idx = unit_selector.scroll_position, max_idx do
        local annotation_idx = idx - unit_selector.scroll_position + 1
        local unit = df.unit.find(unit_selector.unid[idx])
        rating_annotations[annotation_idx] = nil
        if unit and rating_fn then
            local val, color = rating_fn(unit)
            if val then
                rating_annotations[annotation_idx] = {val=val, color=color}
            end
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
    'sort_arrival',
    'sort_stress',
    'sort_need',
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
        infant=self.subviews.infant:getOptionValue(),
        unstable=self.subviews.unstable:getOptionValue(),
        maimed=self.subviews.maimed:getOptionValue(),
    }
    filter_vector(filter, self.prev_filter or {})
    self.prev_filter = filter
    utils.sort_vector(unit_selector.unid, nil, sort_fn)
    annotate_visible_units(sort_fn)
    self.saved_scroll_position = unit_selector.scroll_position
end

function SquadAssignmentOverlay:onInput(keys)
    if keys._MOUSE_R or (keys._MOUSE_L and not self:getMouseFramePos()) then
        -- if any click is made outside of our window, we may need to refresh our list
        self.dirty = true
    end
    return SquadAssignmentOverlay.super.onInput(self, keys) or
        (keys._MOUSE_L and self:getMouseFramePos())
end

function SquadAssignmentOverlay:onRenderFrame(dc, frame_rect)
    SquadAssignmentOverlay.super.onRenderFrame(self, dc, frame_rect)
    if self.dirty then
        self:refresh_list()
        self.dirty = false
    elseif self.saved_scroll_position ~= unit_selector.scroll_position then
        annotate_visible_units(self.subviews.sort:getOptionValue())
        self.saved_scroll_position = unit_selector.scroll_position
    end
end

-- ----------------------
-- SquadAnnotationOverlay
--

SquadAnnotationOverlay = defclass(SquadAnnotationOverlay, overlay.OverlayWidget)
SquadAnnotationOverlay.ATTRS{
    desc='Annotates squad selection candidates with the values of the current sort.',
    default_pos={x=56, y=5},
    default_enabled=true,
    viewscreens='dwarfmode/UnitSelector/SQUAD_FILL_POSITION',
    frame={w=5, h=35},
    frame_style=gui.FRAME_INTERIOR_MEDIUM,
    frame_background=gui.CLEAR_PEN,
}

function get_annotation_text(idx)
    local elem = rating_annotations[idx]
    if not elem or not tonumber(elem.val) then return ' - ' end

    return tostring(math.tointeger(elem.val))
end

function get_annotation_color(idx)
    local elem = rating_annotations[idx]
    return elem and elem.color or nil
end

local to_pen = dfhack.pen.parse
local DASH_PEN = to_pen{ch='-', fg=COLOR_WHITE, keep_lower=true}

local FACE_TILES = {}
local function init_face_tiles()
    for idx=0,6 do
        FACE_TILES[idx] = {}
        local face_off = (6 - idx) * 2
        for y=0,1 do
            for x=0,1 do
                local tile = dfhack.screen.findGraphicsTile('INTERFACE_BITS', 32 + face_off + x, 6 + y)
                ensure_key(FACE_TILES[idx], y)[x] = tile
            end
        end
    end

    for idx,color in ipairs{COLOR_RED, COLOR_LIGHTRED, COLOR_YELLOW, COLOR_WHITE, COLOR_GREEN, COLOR_LIGHTGREEN, COLOR_LIGHTCYAN} do
        local face = {}
        ensure_key(face, 0)[0] = to_pen{tile=FACE_TILES[idx-1][0][0], ch=1, fg=color}
        ensure_key(face, 0)[1] = to_pen{tile=FACE_TILES[idx-1][0][1], ch='\\', fg=color}
        ensure_key(face, 1)[0] = to_pen{tile=FACE_TILES[idx-1][1][0], ch='\\', fg=color}
        ensure_key(face, 1)[1] = to_pen{tile=FACE_TILES[idx-1][1][1], ch='/', fg=color}
        FACE_TILES[idx-1] = face
    end
end
init_face_tiles()

function get_stress_face_tile(idx, x, y)
    local elem = rating_annotations[idx]
    if not elem or not elem.val or elem.val < 0 then
        return x == 0 and y == 1 and DASH_PEN or gui.CLEAR_PEN
    end
    local val = math.min(6, elem.val)
    return safe_index(FACE_TILES, val, y, x)
end

function SquadAnnotationOverlay:init()
    for idx = 1, 10 do
        self:addviews{
            widgets.Label{
                frame={t=idx*3+1, h=1, w=3},
                text={
                    {
                        text=curry(get_annotation_text, idx),
                        pen=curry(get_annotation_color, idx),
                        width=3,
                        rjustify=true,
                    },
                },
                visible=function() return not use_stress_faces end,
            },
            widgets.Label{
                frame={t=idx*3, r=0, h=2, w=2},
                auto_height=false,
                text={
                    {width=1, tile=curry(get_stress_face_tile, idx, 0, 0)},
                    {width=1, tile=curry(get_stress_face_tile, idx, 1, 0)},
                    NEWLINE,
                    {width=1, tile=curry(get_stress_face_tile, idx, 0, 1)},
                    {width=1, tile=curry(get_stress_face_tile, idx, 1, 1)},
                },
                visible=function() return use_stress_faces end,
            },
        }
    end
end

function SquadAnnotationOverlay:onInput(keys)
    return SquadAnnotationOverlay.super.onInput(self, keys) or
        (keys._MOUSE_L and self:getMouseFramePos())
end

OVERLAY_WIDGETS = {
    -- TODO: rewrite for 50.12
    -- squad_assignment=SquadAssignmentOverlay,
    -- squad_annotation=SquadAnnotationOverlay,
    -- info=require('plugins.sort.info').InfoOverlay,
    -- workanimals=require('plugins.sort.info').WorkAnimalOverlay,
    candidates=require('plugins.sort.info').CandidatesOverlay,
    -- interrogation=require('plugins.sort.info').InterrogationOverlay,
    location_selector=require('plugins.sort.locationselector').LocationSelectorOverlay,
    -- unit_selector=require('plugins.sort.unitselector').UnitSelectorOverlay,
    -- worker_assignment=require('plugins.sort.unitselector').WorkerAssignmentOverlay,
    -- burrow_assignment=require('plugins.sort.unitselector').BurrowAssignmentOverlay,
    slab=require('plugins.sort.slab').SlabOverlay,
    world=require('plugins.sort.world').WorldOverlay,
    places=require('plugins.sort.places').PlacesOverlay,
    -- elevate_barony=require('plugins.sort.diplomacy').DiplomacyOverlay,
    -- elevate_barony_preferences=require('plugins.sort.diplomacy').PreferenceOverlay,
}

dfhack.onStateChange[GLOBAL_KEY] = function(sc)
    if sc ~= SC_MAP_LOADED or df.global.gamemode ~= df.game_mode.DWARF then
        return
    end

    init_face_tiles()
end

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
