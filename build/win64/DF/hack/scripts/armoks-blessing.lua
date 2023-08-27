-- Adjust all attributes of all dwarves to an ideal
-- by vjek
--[====[

armoks-blessing
===============
Runs the equivalent of `rejuvenate`, `elevate-physical`, `elevate-mental`, and
`brainwash` on all dwarves currently on the map.  This is an extreme change,
which sets every stat and trait to an ideal easy-to-satisfy preference.

Without providing arguments, only attributes, age, and personalities will be adjusted.
Adding arguments allows for skills or classes to be adjusted to legendary (maximum).

Arguments:

- ``list``
   Prints list of all skills

- ``classes``
   Prints list of all classes

- ``all``
   Set all skills, for all Dwarves, to legendary

- ``<skill name>``
   Set a specific skill, for all Dwarves, to legendary

   example: ``armoks-blessing RANGED_COMBAT``

   All Dwarves become a Legendary Archer

- ``<class name>``
   Set a specific class (group of skills), for all Dwarves, to legendary

   example: ``armoks-blessing Medical``

   All Dwarves will have all medical related skills set to legendary

]====]
local utils = require 'utils'
function rejuvenate(unit)
    if unit==nil then
        print ("No unit available!  Aborting with extreme prejudice.")
        return
    end

    local current_year=df.global.cur_year
    local newbirthyear=current_year - 20
    if unit.birth_year < newbirthyear then
        unit.birth_year=newbirthyear
    end
    if unit.old_year < current_year+100 then
        unit.old_year=current_year+100
    end

end
-- ---------------------------------------------------------------------------
function brainwash_unit(unit)
    if unit==nil then
        print ("No unit available!  Aborting with extreme prejudice.")
        return
    end
--  ~unit.status.current_soul.personality.traits
    unit.status.current_soul.personality.traits.LOVE_PROPENSITY          = 75
    unit.status.current_soul.personality.traits.HATE_PROPENSITY          = 25
    unit.status.current_soul.personality.traits.ENVY_PROPENSITY          = 25
    unit.status.current_soul.personality.traits.CHEER_PROPENSITY         = 75
    unit.status.current_soul.personality.traits.DEPRESSION_PROPENSITY    = 25
    unit.status.current_soul.personality.traits.ANGER_PROPENSITY         = 25
    unit.status.current_soul.personality.traits.ANXIETY_PROPENSITY       = 25
    unit.status.current_soul.personality.traits.LUST_PROPENSITY          = 99
    unit.status.current_soul.personality.traits.STRESS_VULNERABILITY     = 25
    unit.status.current_soul.personality.traits.GREED                    = 25
    unit.status.current_soul.personality.traits.IMMODERATION             = 25
    unit.status.current_soul.personality.traits.VIOLENT                  = 50
    unit.status.current_soul.personality.traits.PERSEVERENCE             = 75
    unit.status.current_soul.personality.traits.WASTEFULNESS             = 50
    unit.status.current_soul.personality.traits.DISCORD                  = 25
    unit.status.current_soul.personality.traits.FRIENDLINESS             = 75
    unit.status.current_soul.personality.traits.POLITENESS               = 75
    unit.status.current_soul.personality.traits.DISDAIN_ADVICE           = 50
    unit.status.current_soul.personality.traits.BRAVERY                  = 75
    unit.status.current_soul.personality.traits.CONFIDENCE               = 75
    unit.status.current_soul.personality.traits.VANITY                   = 25
    unit.status.current_soul.personality.traits.AMBITION                 = 75
    unit.status.current_soul.personality.traits.GRATITUDE                = 75
    unit.status.current_soul.personality.traits.IMMODESTY                = 50
    unit.status.current_soul.personality.traits.HUMOR                    = 75
    unit.status.current_soul.personality.traits.VENGEFUL                 = 25
    unit.status.current_soul.personality.traits.PRIDE                    = 50
    unit.status.current_soul.personality.traits.CRUELTY                  = 25
    unit.status.current_soul.personality.traits.SINGLEMINDED             = 75
    unit.status.current_soul.personality.traits.HOPEFUL                  = 75
    unit.status.current_soul.personality.traits.CURIOUS                  = 75
    unit.status.current_soul.personality.traits.BASHFUL                  = 25
    unit.status.current_soul.personality.traits.PRIVACY                  = 75
    unit.status.current_soul.personality.traits.PERFECTIONIST            = 75
    unit.status.current_soul.personality.traits.CLOSEMINDED              = 25
    unit.status.current_soul.personality.traits.TOLERANT                 = 75
    unit.status.current_soul.personality.traits.EMOTIONALLY_OBSESSIVE    = 25
    unit.status.current_soul.personality.traits.SWAYED_BY_EMOTIONS       = 25
    unit.status.current_soul.personality.traits.ALTRUISM                 = 75
    unit.status.current_soul.personality.traits.DUTIFULNESS              = 75
    unit.status.current_soul.personality.traits.THOUGHTLESSNESS          = 25
    unit.status.current_soul.personality.traits.ORDERLINESS              = 75
    unit.status.current_soul.personality.traits.TRUST                    = 75
    unit.status.current_soul.personality.traits.GREGARIOUSNESS           = 75
    unit.status.current_soul.personality.traits.ASSERTIVENESS            = 25
    unit.status.current_soul.personality.traits.ACTIVITY_LEVEL           = 75
    unit.status.current_soul.personality.traits.EXCITEMENT_SEEKING       = 75
    unit.status.current_soul.personality.traits.IMAGINATION              = 25
    unit.status.current_soul.personality.traits.ABSTRACT_INCLINED        = 25
    unit.status.current_soul.personality.traits.ART_INCLINED             = 50

    unit.status.current_soul.personality.values:resize(0)
    local list_of_values={
        [df.value_type.LAW]=-11,
        [df.value_type.FAMILY]=11,
        [df.value_type.FRIENDSHIP]=41,
        [df.value_type.DECORUM]=11,
        [df.value_type.TRADITION]=11,
        [df.value_type.ARTWORK]=41,
        [df.value_type.COOPERATION]=41,
        [df.value_type.STOICISM]=11,
        [df.value_type.INTROSPECTION]=41,
        [df.value_type.SELF_CONTROL]=41,
        [df.value_type.HARMONY]=11,
        [df.value_type.MERRIMENT]=21,
        [df.value_type.SKILL]=41,
        [df.value_type.HARD_WORK]=41,
        [df.value_type.SACRIFICE]=41,
        [df.value_type.COMPETITION]=-41,
        [df.value_type.PERSEVERENCE]=41,
        [df.value_type.LEISURE_TIME]=-11,
        [df.value_type.COMMERCE]=41,
        [df.value_type.ROMANCE]=41,
        [df.value_type.PEACE]=-11,
        [df.value_type.KNOWLEDGE]=41,
    }
    for k,v in pairs(list_of_values) do
        unit.status.current_soul.personality.values:insert("#",{new=true,type=k,strength=v})
    end

    unit.status.current_soul.personality.needs:resize(0)
    local list_of_needs={
        [df.need_type.Socialize]=2,
        [df.need_type.BeWithFriends]=2,
        [df.need_type.TakeItEasy]=2,
        [df.need_type.MakeMerry]=2,
        [df.need_type.AdmireArt]=2,
    --  [df.need_type.EatGoodMeal]=0, -- open bug 10262 as of 20200218
        [df.need_type.PrayOrMeditate]=0,
        [df.need_type.DrinkAlcohol]=1,
    }
    for k,v in pairs(list_of_needs) do
        if k == df.need_type.PrayOrMeditate then -- handle deity id, otherwise, the deity is set to -1
            if unit.hist_figure_id ~= -1 then
                for index, link in ipairs(df.historical_figure.find(unit.hist_figure_id).histfig_links) do
                    if df.histfig_hf_link_deityst:is_instance(link) and link.target_hf ~= nil then
                        unit.status.current_soul.personality.needs:insert("#",{new=true,id=k,deity_id=link.target_hf,need_level=v})
                        link.link_strength=1 -- casual worshipper
                    end
                end
            end
        else
            unit.status.current_soul.personality.needs:insert("#",{new=true,id=k,need_level=v})
        end
    end

end
-- ---------------------------------------------------------------------------
function elevate_attributes(unit)
    if unit==nil then
        print ("No unit available!  Aborting with extreme prejudice.")
        return
    end

    if unit.status.current_soul then
        for k,v in pairs(unit.status.current_soul.mental_attrs) do
            v.value=v.max_value
        end
    end

    for k,v in pairs(unit.body.physical_attrs) do
        v.value=v.max_value
    end
end
-- ---------------------------------------------------------------------------
-- this function will return the number of elements, starting at zero.
-- useful for counting things where #foo doesn't work
function count_this(to_be_counted)
    local count = -1
    local var1 = ""
    while var1 ~= nil do
        count = count + 1
        var1 = (to_be_counted[count])
    end
    count=count-1
    return count
end
-- ---------------------------------------------------------------------------
function make_legendary(skillname,unit)
    local skillnamenoun,skillnum

    if unit==nil then
        print ("No unit available!  Aborting with extreme prejudice.")
        return
    end

    if (df.job_skill[skillname]) then
        skillnamenoun = df.job_skill.attrs[df.job_skill[skillname]].caption_noun
    else
        print ("The skill name provided is not in the list.")
        return
    end

    if skillnamenoun ~= nil then
        skillnum = df.job_skill[skillname]
        utils.insert_or_update(unit.status.current_soul.skills, { new = true, id = skillnum, rating = 20 }, 'id')
        print (unit.name.first_name.." is now a Legendary "..skillnamenoun)
    else
        print ("Empty skill name noun, bailing out!")
        return
    end
end
-- ---------------------------------------------------------------------------
function BreathOfArmok(unit)

    if unit==nil then
        print ("No unit available!  Aborting with extreme prejudice.")
        return
    end
    local i

    local count_max = count_this(df.job_skill)
    for i=0, count_max do
        utils.insert_or_update(unit.status.current_soul.skills, { new = true, id = i, rating = 20 }, 'id')
    end
    print ("The breath of Armok has engulfed "..unit.name.first_name)
end
-- ---------------------------------------------------------------------------
function LegendaryByClass(skilltype,v)
    local unit=v
    if unit==nil then
        print ("No unit available!  Aborting with extreme prejudice.")
        return
    end

    local i
    local skillclass
    local count_max = count_this(df.job_skill)
    for i=0, count_max do
        skillclass = df.job_skill_class[df.job_skill.attrs[i].type]
        if skilltype == skillclass then
            print ("Skill "..df.job_skill.attrs[i].caption.." is type: "..skillclass.." and is now Legendary for "..unit.name.first_name)
            utils.insert_or_update(unit.status.current_soul.skills, { new = true, id = i, rating = 20 }, 'id')
        end
    end
end
-- ---------------------------------------------------------------------------
function PrintSkillList()
    local count_max = count_this(df.job_skill)
    local i
    for i=0, count_max do
        print("'"..df.job_skill.attrs[i].caption.."' "..df.job_skill[i].." Type: "..df.job_skill_class[df.job_skill.attrs[i].type])
    end
    print ("Provide the UPPER CASE argument, for example: PROCESSPLANTS rather than Threshing")
end
-- ---------------------------------------------------------------------------
function PrintSkillClassList()
    local i
    local count_max = count_this(df.job_skill_class)
    for i=0, count_max do
        print(df.job_skill_class[i])
    end
    print ("Provide one of these arguments, and all skills of that type will be made Legendary")
    print ("For example: Medical will make all medical skills legendary")
end
-- ---------------------------------------------------------------------------
function adjust_all_dwarves(skillname)
    for _,v in ipairs(df.global.world.units.all) do
        if v.race == df.global.plotinfo.race_id and v.status.current_soul then
            print("Adjusting "..dfhack.df2console(dfhack.TranslateName(dfhack.units.getVisibleName(v))))
            brainwash_unit(v)
            elevate_attributes(v)
            rejuvenate(v)
            if skillname then
                if df.job_skill_class[skillname] then
                    LegendaryByClass(skillname,v)
                elseif skillname=="all" then
                    BreathOfArmok(v)
                else
                    make_legendary(skillname,v)
                end
            end
        end
    end
end
-- ---------------------------------------------------------------------------
-- main script operation starts here
-- ---------------------------------------------------------------------------
local args = {...}
local opt = args[1]
local skillname

if opt then
    if opt=="list" then
        PrintSkillList()
        return
    end
    if opt=="classes" then
        PrintSkillClassList()
        return
    end
    skillname = opt
else
    print ("No skillname supplied, no skills will be adjusted.  Pass argument 'list' to see a skill list, 'classes' to show skill classes, or use 'all' if you want all skills legendary.")
end

adjust_all_dwarves(skillname)
