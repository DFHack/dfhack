-- This script will modify a skill or the skills of a single unit
-- the skill will be increased to 20 (Legendary +5)
-- by vjek
--[[=begin

make-legendary
==============
Makes the selected dwarf legendary in one skill, a group of skills, or all
skills.  View groups with ``make-legendary classes``, or all skills with
``make-legendary list``.  Use ``make-legendary MINING`` when you need something
dug up, or ``make-legendary all`` when only perfection will do.

=end]]

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

function getName(unit)
    return dfhack.df2console(dfhack.TranslateName(dfhack.units.getVisibleName(unit)))
end

function make_legendary(skillname)
    local skillnamenoun,skillnum
    unit=dfhack.gui.getSelectedUnit()

    if unit==nil then
        print ("No unit under cursor!  Aborting with extreme prejudice.")
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
        print (getName(unit) .. " is now a Legendary "..skillnamenoun)
    else
        print ("Empty skill name noun, bailing out!")
        return
    end
end

function PrintSkillList()
    local count_max = count_this(df.job_skill)
    local i
    for i=0, count_max do
        print("'"..df.job_skill.attrs[i].caption.."' "..df.job_skill[i].." Type: "..df.job_skill_class[df.job_skill.attrs[i].type])
    end
    print ("Provide the UPPER CASE argument, for example: PROCESSPLANTS rather than Threshing")
end

function BreathOfArmok()
    unit=dfhack.gui.getSelectedUnit()
    if unit==nil then
        print ("No unit under cursor!  Aborting with extreme prejudice.")
        return
    end
    local i
    local count_max = count_this(df.job_skill)
    utils = require 'utils'
    for i=0, count_max do
        utils.insert_or_update(unit.status.current_soul.skills, { new = true, id = i, rating = 20 }, 'id')
    end
    print ("The breath of Armok has engulfed "..getName(unit))
end

function LegendaryByClass(skilltype)
    unit=dfhack.gui.getSelectedUnit()
    if unit==nil then
        print ("No unit under cursor!  Aborting with extreme prejudice.")
        return
    end

    utils = require 'utils'
    local i
    local skillclass
    local count_max = count_this(df.job_skill)
    for i=0, count_max do
        skillclass = df.job_skill_class[df.job_skill.attrs[i].type]
        if skilltype == skillclass then
            print ("Skill "..df.job_skill.attrs[i].caption.." is type: "..skillclass.." and is now Legendary for "..getName(unit))
            utils.insert_or_update(unit.status.current_soul.skills, { new = true, id = i, rating = 20 }, 'id')
        end
    end
end

function PrintSkillClassList()
    local i
    local count_max = count_this(df.job_skill_class)
    for i=0, count_max do
        print(df.job_skill_class[i])
    end
    print ("Provide one of these arguments, and all skills of that type will be made Legendary")
    print ("For example: Medical will make all medical skills legendary")
end

--main script operation starts here
----
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
    if opt=="all" then
        BreathOfArmok()
        return
    end
    if opt=="Normal" or opt=="Medical" or opt=="Personal" or opt=="Social" or opt=="Cultural" or opt=="MilitaryWeapon" or opt=="MilitaryAttack" or opt=="MilitaryDefense" or opt=="MilitaryMisc" then
        LegendaryByClass(opt)
        return
    end
    skillname = opt
else
    print ("No skillname supplied.\nUse argument 'list' to see a list, 'classes' to show skill classes, or use 'all' if you want it all!")
    print ("Example:  To make a legendary miner, use make_legendary MINING")
    return
end

make_legendary(skillname)
