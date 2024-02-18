local _ENV = mkmodule('plugins.sort')

local gui = require('gui')
local overlay = require('plugins.overlay')
local setbelief = reqscript('modtools/set-belief')
local utils = require('utils')
local widgets = require('gui.widgets')

local GLOBAL_KEY = 'sort'

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

local function get_name(unit)
    return unit and dfhack.toSearchNormalized(dfhack.TranslateName(dfhack.units.getVisibleName(unit)))
end

local function sort_by_name_desc(unit1, unit2)
    local name1 = get_name(unit1)
    local name2 = get_name(unit2)
    return utils.compare_name(name1, name2)
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

local function sort_by_arrival_desc(unit1, unit2)
    local cache = get_active_idx_cache()
    if not cache[unit1.id] then return -1 end
    if not cache[unit2.id] then return 1 end
    return utils.compare(cache[unit2.id], cache[unit1.id])
end

local function sort_by_arrival_asc(unit1, unit2)
    local cache = get_active_idx_cache()
    if not cache[unit1.id] then return -1 end
    if not cache[unit2.id] then return 1 end
    return utils.compare(cache[unit1.id], cache[unit2.id])
end

local function get_stress(unit)
    return unit and
        unit.status.current_soul and
        unit.status.current_soul.personality.stress
end

local function get_stress_rating(unit)
    return get_rating(dfhack.units.getStressCategory(unit), 0, 100, 4, 3, 2, 1)
end

local function sort_by_stress_desc(unit1, unit2)
    local happiness1 = get_stress(unit1)
    local happiness2 = get_stress(unit2)
    if happiness1 == happiness2 then
        return sort_by_name_desc(unit1, unit2)
    end
    if not happiness2 then return -1 end
    if not happiness1 then return 1 end
    return utils.compare(happiness2, happiness1)
end

local function sort_by_stress_asc(unit1, unit2)
    local happiness1 = get_stress(unit1)
    local happiness2 = get_stress(unit2)
    if happiness1 == happiness2 then
        return sort_by_name_desc(unit1, unit2)
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
    return function(unit1, unit2)
        local rating1 = melee_skill_effectiveness(unit1)
        local rating2 = melee_skill_effectiveness(unit2)
        if rating1 == rating2 then return sort_by_name_desc(unit1, unit2) end
        return utils.compare(rating2, rating1)
    end
end

local function make_sort_by_melee_skill_effectiveness_asc()
    return function(unit1, unit2)
        local rating1 = melee_skill_effectiveness(unit1)
        local rating2 = melee_skill_effectiveness(unit2)
        if rating1 == rating2 then return sort_by_name_desc(unit1, unit2) end
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
    return function(unit1, unit2)
        local rating1 = ranged_skill_effectiveness(unit1)
        local rating2 = ranged_skill_effectiveness(unit2)
        if rating1 == rating2 then return sort_by_name_desc(unit1, unit2) end
        return utils.compare(rating2, rating1)
    end
end

local function make_sort_by_ranged_skill_effectiveness_asc()
    return function(unit1, unit2)
        local rating1 = ranged_skill_effectiveness(unit1)
        local rating2 = ranged_skill_effectiveness(unit2)
        if rating1 == rating2 then return sort_by_name_desc(unit1, unit2) end
        return utils.compare(rating1, rating2)
    end
end

local function make_sort_by_skill_desc(sort_skill)
    return function(unit1, unit2)
        local s1 = get_skill(sort_skill, unit1)
        local s2 = get_skill(sort_skill, unit2)
        if s1 == s2 then return sort_by_name_desc(unit1, unit2) end
        if not s2 then return -1 end
        if not s1 then return 1 end
        if s1.rating ~= s2.rating then
            return utils.compare(s2.rating, s1.rating)
        end
        if s1.experience ~= s2.experience then
            return utils.compare(s2.experience, s1.experience)
        end
        return sort_by_name_desc(unit1, unit2)
    end
end

local function make_sort_by_skill_asc(sort_skill)
    return function(unit1, unit2)
        local s1 = get_skill(sort_skill, unit1)
        local s2 = get_skill(sort_skill, unit2)
        if s1 == s2 then return sort_by_name_desc(unit1, unit2) end
        if not s2 then return 1 end
        if not s1 then return -1 end
        if s1.rating ~= s2.rating then
            return utils.compare(s1.rating, s2.rating)
        end
        if s1.experience ~= s2.experience then
            return utils.compare(s1.experience, s2.experience)
        end
        return sort_by_name_desc(unit1, unit2)
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

local function sort_by_mental_stability_desc(unit1, unit2)
    local rating1 = get_mental_stability(unit1)
    local rating2 = get_mental_stability(unit2)
    if rating1 == rating2 then
        -- sorting by stress is opposite
        -- more mental stable dwarves should have less stress
        return sort_by_stress_asc(unit1, unit2)
    end
    return utils.compare(rating2, rating1)
end

local function sort_by_mental_stability_asc(unit1, unit2)
    local rating1 = get_mental_stability(unit1)
    local rating2 = get_mental_stability(unit2)
    if rating1 == rating2 then
        return sort_by_stress_desc(unit1, unit2)
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

local function sort_by_melee_combat_potential_desc(unit1, unit2)
    local rating1 = get_melee_combat_potential(unit1)
    local rating2 = get_melee_combat_potential(unit2)
    if rating1 == rating2 then
        return sort_by_mental_stability_desc(unit1, unit2)
    end
    return utils.compare(rating2, rating1)
end

local function sort_by_melee_combat_potential_asc(unit1, unit2)
    local rating1 = get_melee_combat_potential(unit1)
    local rating2 = get_melee_combat_potential(unit2)
    if rating1 == rating2 then
        return sort_by_mental_stability_asc(unit1, unit2)
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

local function sort_by_ranged_combat_potential_desc(unit1, unit2)
    local rating1 = get_ranged_combat_potential(unit1)
    local rating2 = get_ranged_combat_potential(unit2)
    if rating1 == rating2 then
        return sort_by_mental_stability_desc(unit1, unit2)
    end
    return utils.compare(rating2, rating1)
end

local function sort_by_ranged_combat_potential_asc(unit1, unit2)
    local rating1 = get_ranged_combat_potential(unit1)
    local rating2 = get_ranged_combat_potential(unit2)
    if rating1 == rating2 then
        return sort_by_mental_stability_asc(unit1, unit2)
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

local function sort_by_need_desc(unit1, unit2)
    local rating1 = get_need(unit1)
    local rating2 = get_need(unit2)
    if rating1 == rating2 then
        return sort_by_stress_desc(unit1, unit2)
    end
    if not rating2 then return -1 end
    if not rating1 then return 1 end
    return utils.compare(rating2, rating1)
end

local function sort_by_need_asc(unit1, unit2)
    local rating1 = get_need(unit1)
    local rating2 = get_need(unit2)
    if rating1 == rating2 then
        return sort_by_stress_asc(unit1, unit2)
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
local sort_by_pick_desc=make_sort_by_skill_desc(df.job_skill.MINING)
local sort_by_pick_asc=make_sort_by_skill_asc(df.job_skill.MINING)
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
    {label='melee effectiveness', widget='sort_any_melee', desc_fn=sort_by_any_melee_desc, asc_fn=sort_by_any_melee_asc, rating_fn=get_melee_skill_effectiveness_rating},
    {label='ranged effectiveness', widget='sort_any_ranged', desc_fn=sort_by_any_ranged_desc, asc_fn=sort_by_any_ranged_asc, rating_fn=get_ranged_skill_effectiveness_rating},
    {label='teacher skill', widget='sort_teacher', desc_fn=sort_by_teacher_desc, asc_fn=sort_by_teacher_asc, rating_fn=curry(get_skill_rating, df.job_skill.TEACHING)},
    {label='stress level', widget='sort_stress', desc_fn=sort_by_stress_desc, asc_fn=sort_by_stress_asc, rating_fn=get_stress_rating, use_stress_faces=true},
    {label='arrival order', widget='sort_arrival', desc_fn=sort_by_arrival_desc, asc_fn=sort_by_arrival_asc, rating_fn=get_arrival_rating},
    {label='tactics skill', widget='sort_tactics', desc_fn=sort_by_tactics_desc, asc_fn=sort_by_tactics_asc, rating_fn=curry(get_skill_rating, df.job_skill.MILITARY_TACTICS)},
    {label='need for training', widget='sort_need', desc_fn=sort_by_need_desc, asc_fn=sort_by_need_asc, rating_fn=get_need_rating, use_stress_faces=true},
    {label='pick (mining) skill', widget='sort_pick', desc_fn=sort_by_pick_desc, asc_fn=sort_by_pick_asc, rating_fn=curry(get_skill_rating, df.job_skill.MINING)},
    {label='axe skill', widget='sort_axe', desc_fn=sort_by_axe_desc, asc_fn=sort_by_axe_asc, rating_fn=curry(get_skill_rating, df.job_skill.AXE)},
    {label='sword skill', widget='sort_sword', desc_fn=sort_by_sword_desc, asc_fn=sort_by_sword_asc, rating_fn=curry(get_skill_rating, df.job_skill.SWORD)},
    {label='mace skill', widget='sort_mace', desc_fn=sort_by_mace_desc, asc_fn=sort_by_mace_asc, rating_fn=curry(get_skill_rating, df.job_skill.MACE)},
    {label='hammer skill', widget='sort_hammer', desc_fn=sort_by_hammer_desc, asc_fn=sort_by_hammer_asc, rating_fn=curry(get_skill_rating, df.job_skill.HAMMER)},
    {label='spear skill', widget='sort_spear', desc_fn=sort_by_spear_desc, asc_fn=sort_by_spear_asc, rating_fn=curry(get_skill_rating, df.job_skill.SPEAR)},
    {label='crossbow skill', widget='sort_crossbow', desc_fn=sort_by_crossbow_desc, asc_fn=sort_by_crossbow_asc, rating_fn=curry(get_skill_rating, df.job_skill.CROSSBOW)},
    {label='melee potential', widget='sort_melee_combat_potential', desc_fn=sort_by_melee_combat_potential_desc, asc_fn=sort_by_melee_combat_potential_asc, rating_fn=get_melee_combat_potential_rating},
    {label='ranged potential', widget='sort_ranged_combat_potential', desc_fn=sort_by_ranged_combat_potential_desc, asc_fn=sort_by_ranged_combat_potential_asc, rating_fn=get_ranged_combat_potential_rating},
}
for _, v in ipairs(SORT_LIBRARY) do
    SORT_LIBRARY[v.widget] = v
end

local unit_selector = df.global.game.main_interface.unit_selector

local use_stress_faces = false
local rating_annotations = {}

local function get_unit_selector()
    return dfhack.gui.getWidget(unit_selector, 'Unit selector')
end

local function get_scroll_rows()
    return dfhack.gui.getWidget(unit_selector, 'Unit selector', 'Unit List', 1)
end

local function get_scroll_pos(scroll_rows)
    return (scroll_rows or get_scroll_rows()).scroll
end

local function get_num_slots(screen_height)
    if not screen_height then
        _, screen_height = dfhack.screen.getWindowSize()
    end
    return (screen_height - 20) // 3
end

local function annotate_visible_units(sort_id)
    local opt = SORT_LIBRARY[sort_id]
    use_stress_faces = opt.use_stress_faces
    rating_annotations = {}
    rating_fn = opt.rating_fn
    local scroll_rows = get_scroll_rows()
    local rows = dfhack.gui.getWidgetChildren(scroll_rows)
    local scroll_pos = get_scroll_pos(scroll_rows)
    local max_idx = math.min(#rows, scroll_pos+scroll_rows.num_visible)
    for idx = scroll_pos+1, max_idx do
        local annotation_idx = idx - scroll_pos - 1
        rating_annotations[annotation_idx] = nil
        local row = rows[idx]
        if rating_fn and df.widget_container:is_instance(row) then
            local unit = dfhack.gui.getWidget(row, 0).u
            local val, color = rating_fn(unit)
            if val then
                rating_annotations[annotation_idx] = {val=val, color=color}
            end
        end
    end
end

-- ----------------------
-- SortButton
--

SortButton = defclass(SortButton, widgets.Panel)
SortButton.ATTRS{
    on_click=DEFAULT_NIL,
}

local function make_sort_pens(ascending, selected)
    local to_pen = dfhack.pen.parse
    local init = df.global.init
    local name = ('texpos_sort_%s_%s'):format(
        ascending and 'ascending' or 'descending',
        selected and 'active' or 'inactive'
    )
    -- this is backwards, but it matches vanilla
    local ch = ascending and 31 or 30
    local fg = selected and COLOR_WHITE or COLOR_BLACK
    return {
        to_pen{tile=init[name][0], ch=32, fg=COLOR_BLACK, bg=COLOR_GRAY},
        to_pen{tile=init[name][1], ch=ch, fg=fg, bg=COLOR_GRAY},
        to_pen{tile=init[name][2], ch=ch, fg=fg, bg=COLOR_GRAY},
        to_pen{tile=init[name][3], ch=32, fg=COLOR_BLACK, bg=COLOR_GRAY},
    }
end

local SORT_PENS = {
    [true]={[true]=make_sort_pens(true, true), [false]=make_sort_pens(true, false)},
    [false]={[true]=make_sort_pens(false, true), [false]=make_sort_pens(false, false)},
}

local function sort_button_get_tile(self, idx)
    return SORT_PENS[self.ascending][self.selected][idx]
end

function SortButton:init()
    self.frame = self.frame or {}
    self.frame.w, self.frame.h = 4, 1

    self.ascending = false
    self.selected = false

    self:addviews{
        widgets.Label{
            text={
                {width=1, tile=curry(sort_button_get_tile, self, 1)},
                {width=1, tile=curry(sort_button_get_tile, self, 2)},
                {width=1, tile=curry(sort_button_get_tile, self, 3)},
                {width=1, tile=curry(sort_button_get_tile, self, 4)},
            },
            on_click=function()
                if not self.selected then
                    self.selected = true
                else
                    self.ascending = not self.ascending
                end
                self.on_click()
            end,
        }
    }
end

-- ----------------------
-- SquadAnnotationOverlay
--

annotation_instance = nil

SquadAnnotationOverlay = defclass(SquadAnnotationOverlay, overlay.OverlayWidget)
SquadAnnotationOverlay.ATTRS{
    desc='Adds sort and annotation capabilities to the squad assignment panel.',
    default_pos={x=15, y=5},
    version='2',
    default_enabled=true,
    viewscreens='dwarfmode/UnitSelector/SQUAD_FILL_POSITION',
    frame={w=82, h=35},
}

function get_annotation_text(idx)
    local elem = rating_annotations[idx]
    if not elem or not tonumber(elem.val) then return ' -- ' end
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
        ensure_key(face, 0)[0] = to_pen{tile=FACE_TILES[idx-1][0][0], ch=1, fg=color, keep_lower=true}
        ensure_key(face, 0)[1] = to_pen{tile=FACE_TILES[idx-1][0][1], ch='\\', fg=color, keep_lower=true}
        ensure_key(face, 1)[0] = to_pen{tile=FACE_TILES[idx-1][1][0], ch='\\', fg=color, keep_lower=true}
        ensure_key(face, 1)[1] = to_pen{tile=FACE_TILES[idx-1][1][1], ch='/', fg=color, keep_lower=true}
        FACE_TILES[idx-1] = face
    end
end
init_face_tiles()

function get_stress_face_tile(idx, x, y)
    local elem = rating_annotations[idx]
    if not elem or not elem.val or elem.val < 0 then
        return y == 1 and DASH_PEN or gui.CLEAR_PEN
    end
    local val = math.min(6, elem.val)
    return safe_index(FACE_TILES, val, y, x)
end

local function get_label_text(screen_height)
    local text = {}
    for idx=1, get_num_slots(screen_height) do
        table.insert(text, NEWLINE)
        if use_stress_faces then
            table.insert(text, {gap=1, width=1, tile=curry(get_stress_face_tile, idx, 0, 0)})
            table.insert(text, {width=1, tile=curry(get_stress_face_tile, idx, 1, 0)})
            table.insert(text, NEWLINE)
            table.insert(text, {gap=1, width=1, tile=curry(get_stress_face_tile, idx, 0, 1)})
            table.insert(text, {width=1, tile=curry(get_stress_face_tile, idx, 1, 1)})
            table.insert(text, NEWLINE)
        else
            table.insert(text, {
                text=curry(get_annotation_text, idx),
                pen=curry(get_annotation_color, idx),
                width=3,
                rjustify=true,
            })
            table.insert(text, NEWLINE)
            table.insert(text, NEWLINE)
        end
    end
    return text
end

local function make_options(label, view_id)
    return {
        {label=label, value='noop'},
        {label=label, value=view_id, pen=COLOR_GREEN},
    }
end

function SquadAnnotationOverlay:init()
    annotation_instance = self
    self.dirty = true

    local sort_options = {}
    for _, opt in ipairs(SORT_LIBRARY) do
        table.insert(sort_options, {
            label=opt.label,
            value=opt.widget,
            pen=COLOR_GREEN,
        })
    end

    self:addviews{
        widgets.Panel{
            view_id='annotation_panel',
            frame={l=0, w=6, t=0, b=0},
            frame_style=gui.FRAME_INTERIOR_MEDIUM,
            frame_background=gui.CLEAR_PEN,
            subviews={
                SortButton{
                    view_id='sort_button',
                    frame={t=0, l=0},
                    on_click=self:callback('sync_widgets'),
                },
                widgets.Label{
                    view_id='label',
                    frame={t=5, l=0, r=0, b=0},
                    auto_width=false,
                    auto_height=false,
                },
            }
        },
        widgets.BannerPanel{
            view_id='banner_panel',
            frame={t=1, r=0, w=32, h=1},
            frame_background=gui.CLEAR_PEN,
        },
        widgets.CycleHotkeyLabel{
            view_id='sort',
            frame={t=1, r=1, w=30},
            label='Show:',
            key='CUSTOM_SHIFT_S',
            options=sort_options,
            initial_option='sort_any_melee',
            on_change=self:callback('sync_widgets', 'sort'),
        },
        widgets.Panel{
            view_id='hover_panel',
            frame={t=0, r=0, w=33, h=16},
            frame_style=gui.FRAME_MEDIUM,
            visible=false,
            subviews={
                widgets.Panel{
                    frame={t=1, l=0, r=0, b=0},
                    frame_background=gui.CLEAR_PEN,
                    subviews={
                        widgets.Divider{
                            frame={t=0, l=0, r=0, h=1},
                            frame_style=gui.FRAME_THIN,
                            frame_style_l=false,
                            frame_style_r=false,
                        },
                        widgets.CycleHotkeyLabel{
                            view_id='sort_any_melee',
                            frame={t=1, l=0, w=13},
                            options=make_options('melee effect.', 'sort_any_melee'),
                            initial_option='sort_any_melee',
                            option_gap=0,
                            on_change=self:callback('sync_widgets', 'sort_any_melee'),
                        },
                        widgets.CycleHotkeyLabel{
                            view_id='sort_any_ranged',
                            frame={t=1, r=0, w=14},
                            options=make_options('ranged effect.', 'sort_any_ranged'),
                            option_gap=0,
                            on_change=self:callback('sync_widgets', 'sort_any_ranged'),
                        },
                        widgets.CycleHotkeyLabel{
                            view_id='sort_teacher',
                            frame={t=3, l=0, w=7},
                            options=make_options('teacher', 'sort_teacher'),
                            option_gap=0,
                            on_change=self:callback('sync_widgets', 'sort_teacher'),
                        },
                        widgets.CycleHotkeyLabel{
                            view_id='sort_stress',
                            frame={t=3, l=10, w=6},
                            options=make_options('stress', 'sort_stress'),
                            option_gap=0,
                            on_change=self:callback('sync_widgets', 'sort_stress'),
                        },
                        widgets.CycleHotkeyLabel{
                            view_id='sort_arrival',
                            frame={t=3, r=0, w=13},
                            options=make_options('arrival order', 'sort_arrival'),
                            option_gap=0,
                            on_change=self:callback('sync_widgets', 'sort_arrival'),
                        },
                        widgets.CycleHotkeyLabel{
                            view_id='sort_tactics',
                            frame={t=5, l=0, w=7},
                            options=make_options('tactics', 'sort_tactics'),
                            option_gap=0,
                            on_change=self:callback('sync_widgets', 'sort_tactics'),
                        },
                        widgets.CycleHotkeyLabel{
                            view_id='sort_need',
                            frame={t=5, r=0, w=17},
                            options=make_options('need for training', 'sort_need'),
                            option_gap=0,
                            on_change=self:callback('sync_widgets', 'sort_need'),
                        },
                        widgets.CycleHotkeyLabel{
                            view_id='sort_pick',
                            frame={t=7, l=0, w=4},
                            options=make_options('pick', 'sort_pick'),
                            option_gap=0,
                            on_change=self:callback('sync_widgets', 'sort_pick'),
                        },
                        widgets.CycleHotkeyLabel{
                            view_id='sort_axe',
                            frame={t=7, l=8, w=3},
                            options=make_options('axe', 'sort_axe'),
                            option_gap=0,
                            on_change=self:callback('sync_widgets', 'sort_axe'),
                        },
                        widgets.CycleHotkeyLabel{
                            view_id='sort_sword',
                            frame={t=7, l=16, w=5},
                            options=make_options('sword', 'sort_sword'),
                            option_gap=0,
                            on_change=self:callback('sync_widgets', 'sort_sword'),
                        },
                        widgets.CycleHotkeyLabel{
                            view_id='sort_mace',
                            frame={t=7, r=0, w=4},
                            options=make_options('mace', 'sort_mace'),
                            option_gap=0,
                            on_change=self:callback('sync_widgets', 'sort_mace'),
                        },
                        widgets.CycleHotkeyLabel{
                            view_id='sort_hammer',
                            frame={t=9, l=0, w=6},
                            options=make_options('hammer', 'sort_hammer'),
                            option_gap=0,
                            on_change=self:callback('sync_widgets', 'sort_hammer'),
                        },
                        widgets.CycleHotkeyLabel{
                            view_id='sort_spear',
                            frame={t=9, l=12, w=5},
                            options=make_options('spear', 'sort_spear'),
                            option_gap=0,
                            on_change=self:callback('sync_widgets', 'sort_spear'),
                        },
                        widgets.CycleHotkeyLabel{
                            view_id='sort_crossbow',
                            frame={t=9, r=0, w=8},
                            options=make_options('crossbow', 'sort_crossbow'),
                            option_gap=0,
                            on_change=self:callback('sync_widgets', 'sort_crossbow'),
                        },
                        widgets.CycleHotkeyLabel{
                            view_id='sort_melee_combat_potential',
                            frame={t=11, l=0, w=12},
                            options=make_options('melee poten.', 'sort_melee_combat_potential'),
                            option_gap=0,
                            on_change=self:callback('sync_widgets', 'sort_melee_combat_potential'),
                        },
                        widgets.CycleHotkeyLabel{
                            view_id='sort_ranged_combat_potential',
                            frame={t=11, r=0, w=13},
                            options=make_options('ranged poten.', 'sort_ranged_combat_potential'),
                            option_gap=0,
                            on_change=self:callback('sync_widgets', 'sort_ranged_combat_potential'),
                        },
                    },
                },
            },
        },
    }
end

function SquadAnnotationOverlay:sync_widgets(sort_widget, sort_id)
    if sort_widget then
        if sort_id == 'noop' then
            self.subviews[sort_widget]:cycle()
            return
        end
        self.subviews.sort:setOption(sort_id)
        for _,opt in ipairs(SORT_LIBRARY) do
            self.subviews[opt.widget]:setOption(sort_id)
        end
    end
    sort_set_sort_fn()
    self.dirty = true
end

function do_sort(a, b)
    local self = annotation_instance
    local opt = SORT_LIBRARY[self.subviews.sort:getOptionValue()]
    local fn = self.subviews.sort_button.ascending and opt.asc_fn or opt.desc_fn
    return fn(a, b) < 0
end

function SquadAnnotationOverlay:mouse_over_ours()
    if self.subviews.annotation_panel:getMouseFramePos() then return true end
    if self.subviews.hover_panel.visible and self.subviews.hover_panel:getMouseFramePos() then
        return true
    end
    return self.subviews.banner_panel:getMouseFramePos()
end

function SquadAnnotationOverlay:onInput(keys)
    local clicked_on_us = keys._MOUSE_L and self:mouse_over_ours()
    if keys._MOUSE_R or (keys._MOUSE_L and not clicked_on_us) then
        -- if any click is made, we may need to refresh our annotations
        self.dirty = true
    end
    return SquadAnnotationOverlay.super.onInput(self, keys) or clicked_on_us
end

function SquadAnnotationOverlay:onRenderFrame(dc, rect)
    if self.dirty or
        self.saved_scroll_position ~= get_scroll_pos() or
        self.saved_num_visible ~= get_scroll_rows().num_visible
    then
        self.subviews.sort_button.selected = sort_get_sort_active()
        annotate_visible_units(self.subviews.sort:getOptionValue())
        self.saved_scroll_position = get_scroll_pos()
        self.dirty = false
    end
    self.subviews.label:setText(get_label_text(self.frame_parent_rect.height))
    if self.subviews.sort:getMousePos() then
        self.subviews.hover_panel.visible = true
    elseif self.subviews.hover_panel.visible and
        not self.subviews.hover_panel:getMouseFramePos()
    then
        self.subviews.hover_panel.visible = false
    end
    SquadAnnotationOverlay.super.onRenderFrame(self, dc, rect)
end

function SquadAnnotationOverlay:preUpdateLayout(parent_rect)
    self.frame.h = get_num_slots(parent_rect.height) * 3 + 7
end

-- ----------------------
-- SquadFilterOverlay
--

local function poke_list()
    get_unit_selector().sort_flags.NEEDS_RESORTED = true
end

filter_instance = nil

SquadFilterOverlay = defclass(SquadFilterOverlay, overlay.OverlayWidget)
SquadFilterOverlay.ATTRS{
    desc='Adds filter capabilities to the squad assignment panel.',
    default_pos={x=36, y=-5},
    default_enabled=true,
    viewscreens='dwarfmode/UnitSelector/SQUAD_FILL_POSITION',
    frame={w=57, h=6},
}

function SquadFilterOverlay:init()
    filter_instance = self

    local main_panel = widgets.BannerPanel{
        frame={t=0, b=0, r=0, w=29},
        subviews={
            widgets.CycleHotkeyLabel{
                view_id='military',
                frame={t=0, l=1},
                key='CUSTOM_SHIFT_Q',
                label='   Other squads:',
                options={
                    {label='Include', value='include', pen=COLOR_GREEN},
                    {label='Only', value='only', pen=COLOR_YELLOW},
                    {label='Exclude', value='exclude', pen=COLOR_LIGHTRED},
                },
                initial_option='include',
                on_change=poke_list,
            },
            widgets.CycleHotkeyLabel{
                view_id='officials',
                frame={t=1, l=1},
                key='CUSTOM_SHIFT_O',
                label='      Officials:',
                options={
                    {label='Include', value='include', pen=COLOR_GREEN},
                    {label='Only', value='only', pen=COLOR_YELLOW},
                    {label='Exclude', value='exclude', pen=COLOR_LIGHTRED},
                },
                initial_option='include',
                on_change=poke_list,
            },
            widgets.CycleHotkeyLabel{
                view_id='nobles',
                frame={t=2, l=1},
                key='CUSTOM_SHIFT_N',
                label='       Nobility:',
                options={
                    {label='Include', value='include', pen=COLOR_GREEN},
                    {label='Only', value='only', pen=COLOR_YELLOW},
                    {label='Exclude', value='exclude', pen=COLOR_LIGHTRED},
                },
                initial_option='include',
                on_change=poke_list,
            },
            widgets.CycleHotkeyLabel{
                view_id='infant',
                frame={t=3, l=1},
                key='CUSTOM_SHIFT_M',
                label='Infant carriers:',
                options={
                    {label='Include', value='include', pen=COLOR_GREEN},
                    {label='Only', value='only', pen=COLOR_YELLOW},
                    {label='Exclude', value='exclude', pen=COLOR_LIGHTRED},
                },
                initial_option='include',
                on_change=poke_list,
            },
            widgets.CycleHotkeyLabel{
                view_id='unstable',
                frame={t=4, l=1},
                key='CUSTOM_SHIFT_D',
                label='   Hates combat:',
                options={
                    {label='Include', value='include', pen=COLOR_GREEN},
                    {label='Only', value='only', pen=COLOR_YELLOW},
                    {label='Exclude', value='exclude', pen=COLOR_LIGHTRED},
                },
                initial_option='include',
                on_change=poke_list,
            },
            widgets.CycleHotkeyLabel{
                view_id='maimed',
                frame={t=5, l=1},
                key='CUSTOM_SHIFT_I',
                label='         Maimed:',
                options={
                    {label='Include', value='include', pen=COLOR_GREEN},
                    {label='Only', value='only', pen=COLOR_YELLOW},
                    {label='Exclude', value='exclude', pen=COLOR_LIGHTRED},
                },
                initial_option='include',
                on_change=poke_list,
            },
        },
    }

    self:addviews{
        widgets.BannerPanel{
            frame={t=1, l=0, w=27, h=1},
            subviews={
                widgets.HelpButton{
                    frame={t=0, l=1},
                    command='sort',
                },
                widgets.HotkeyLabel{
                    frame={t=0, l=5},
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
                        poke_list()
                    end,
                },
            },
        },
        main_panel,
    }
end

function SquadFilterOverlay:render(dc)
    sort_set_filter_fn()
    SquadFilterOverlay.super.render(self, dc)
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

local function filter_matches(unit, filter)
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
    return true
end

function do_filter(unit)
    if annotation_instance then
        annotation_instance.dirty = true
    end
    local self = filter_instance
    local filter = {
        military=self.subviews.military:getOptionValue(),
        officials=self.subviews.officials:getOptionValue(),
        nobles=self.subviews.nobles:getOptionValue(),
        infant=self.subviews.infant:getOptionValue(),
        unstable=self.subviews.unstable:getOptionValue(),
        maimed=self.subviews.maimed:getOptionValue(),
    }
    return filter_matches(unit, filter)
end

OVERLAY_WIDGETS = {
    -- TODO: rewrite for 50.12
    squad_annotation=SquadAnnotationOverlay,
    squad_filter=SquadFilterOverlay,
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
    elevate_barony=require('plugins.sort.diplomacy').DiplomacyOverlay,
    elevate_barony_preferences=require('plugins.sort.diplomacy').PreferenceOverlay,
}

dfhack.onStateChange[GLOBAL_KEY] = function(sc)
    if sc ~= SC_MAP_LOADED or df.global.gamemode ~= df.game_mode.DWARF then
        return
    end
    init_face_tiles()
end

return _ENV
