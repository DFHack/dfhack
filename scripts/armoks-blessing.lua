-- Adjust all attributes, personality, age and skills of all dwarves in play
-- without arguments, all attributes, age & personalities are adjusted
-- arguments allow for skills to be adjusted as well
-- WARNING: USING THIS SCRIPT WILL ADJUST ALL DWARVES IN PLAY!
-- by vjek
--[[=begin

armoks-blessing
===============
Runs the equivalent of `rejuvenate`, `elevate-physical`, `elevate-mental`, and
`brainwash` on all dwarves currently on the map.  This is an extreme change,
which sets every stat to an ideal - legendary skills, great traits, and
easy-to-satisfy preferences.

=end]]
function rejuvenate(unit)
    if unit==nil then
        print ("No unit available!  Aborting with extreme prejudice.")
        return
    end

    local current_year=df.global.cur_year
    local newbirthyear=current_year - 20
    if unit.relations.birth_year < newbirthyear then
        unit.relations.birth_year=newbirthyear
    end
    if unit.relations.old_year < current_year+100 then
        unit.relations.old_year=current_year+100
    end

end
-- ---------------------------------------------------------------------------
function brainwash_unit(unit)
    if unit==nil then
        print ("No unit available!  Aborting with extreme prejudice.")
        return
    end

    local profile ={75,25,25,75,25,25,25,99,25,25,25,50,75,50,25,75,75,50,75,75,25,75,75,50,75,25,50,25,75,75,75,25,75,75,25,75,25,25,75,75,25,75,75,75,25,75,75,25,25,50}
    local i

    for i=1, #profile do
        unit.status.current_soul.personality.traits[i-1]=profile[i]
    end

end
-- ---------------------------------------------------------------------------
function elevate_attributes(unit)
    if unit==nil then
        print ("No unit available!  Aborting with extreme prejudice.")
        return
    end

    local ok,f,t,k = pcall(pairs,unit.status.current_soul.mental_attrs)
    if ok then
        for k,v in f,t,k do
            v.value=v.max_value
        end
    end

    local ok,f,t,k = pcall(pairs,unit.body.physical_attrs)
    if ok then
        for k,v in f,t,k do
            v.value=v.max_value
        end
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
        utils = require 'utils'
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
    utils = require 'utils'
    for i=0, count_max do
        utils.insert_or_update(unit.status.current_soul.skills, { new = true, id = i, rating = 20 }, 'id')
    end
    print ("The breath of Armok has engulfed "..unit.name.first_name)
end
-- ---------------------------------------------------------------------------
function LegendaryByClass(skilltype,v)
    unit=v
    if unit==nil then
        print ("No unit available!  Aborting with extreme prejudice.")
        return
    end

    utils = require 'utils'
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
        if v.race == df.global.ui.race_id then
            print("Adjusting "..dfhack.TranslateName(dfhack.units.getVisibleName(v)))
            brainwash_unit(v)
            elevate_attributes(v)
            rejuvenate(v)
            if skillname then
                if skillname=="Normal" or skillname=="Medical" or skillname=="Personal" or skillname=="Social" or skillname=="Cultural" or skillname=="MilitaryWeapon" or skillname=="MilitaryAttack" or skillname=="MilitaryDefense" or skillname=="MilitaryMisc" then
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
local opt = ...
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
