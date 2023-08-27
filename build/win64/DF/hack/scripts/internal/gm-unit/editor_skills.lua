-- Skill editor module for gui/gm-unit.
--@ module = true

local utils = require("utils")
local widgets = require("gui.widgets")
local base_editor = reqscript("internal/gm-unit/base_editor")

--TODO set local should or better yet skills vector to reduce long skill list access typing
Editor_Skills = defclass(Editor_Skills, base_editor.Editor)

Editor_Skills.ATTRS{
    frame_title = "Skill editor",
    learned_only = false
}

function list_skills(unit, learned_only)
    local u_skills = unit.status.current_soul.skills
    local ret = {}
    for skill,v in ipairs(df.job_skill) do
        if skill ~= df.job_skill.NONE then
            local u_skill = utils.binsearch(u_skills, skill, "id")
            if u_skill or not learned_only then
                if not u_skill then
                    u_skill={rating=-1,experience=0}
                end
                local rating
                if u_skill.rating >=0 then
                    rating=df.skill_rating.attrs[u_skill.rating]
                else
                    rating={caption="<unlearned>",xp_threshold=0}
                end
                local text=string.format("%s: %s %d %d/%d",
                    df.job_skill.attrs[skill].caption,
                    rating.caption,u_skill.rating,
                    u_skill.experience,rating.xp_threshold)
                table.insert(ret,{
                    text=text,
                    id=skill,
                    search_key=text:lower()
                })
            end
        end
    end
    return ret
end

function Editor_Skills:update_list(no_save_place)
    local skill_list=list_skills(self.target_unit,self.learned_only)
    local skills = self.subviews.skills
    local saved_filter = skills:getFilter()
    if no_save_place then
        skills:setChoices(skill_list)
    else
        skills:setChoices(skill_list,skills:getSelected())
    end
    skills:setFilter(saved_filter)
end

function Editor_Skills:init( args )
    if self.target_unit.status.current_soul==nil then
        qerror("Unit does not have soul, can't edit skills")
    end

    local skill_list=list_skills(self.target_unit,self.learned_only)

    self:addviews{
        widgets.FilteredList{
            choices=skill_list,
            frame = {t=0, b=3,l=0},
            view_id="skills",
            edit_ignore_keys={"STRING_A045", "STRING_A043", "STRING_A047"},
        },
        widgets.Label{
            frame = { b=0,l=0},
            text ={
                {text=": remove level ",
                key = "STRING_A045",
                on_activate=self:callback("level_skill",-1)},
                {text=": add level ",
                key = "STRING_A043",
                on_activate=self:callback("level_skill",1)},
                NEWLINE,
                {text=": show learned only ",
                key = "STRING_A047", -- /
                on_activate=function ()
                    self.learned_only=not self.learned_only
                    self:update_list(true)
                end}
            }
        },
    }
end
function Editor_Skills:onOpen()
    self.subviews[1].edit:setFocus(true)
end
function Editor_Skills:get_cur_skill()
    local list_wid=self.subviews.skills
    local _,choice=list_wid:getSelected()
    if choice==nil then
        qerror("Nothing selected")
    end
    local u_skill=utils.binsearch(self.target_unit.status.current_soul.skills,choice.id,"id")
    return choice,u_skill
end
function Editor_Skills:level_skill(lvl)
    local sk_en,sk=self:get_cur_skill()
    if lvl >0 then
        local rating

        if sk then
            rating=sk.rating+lvl
        else
            rating=lvl-1
        end

        utils.insert_or_update(self.target_unit.status.current_soul.skills, {new=true, id=sk_en.id, rating=rating}, 'id') --TODO set exp?
    elseif sk and sk.rating==0 and lvl<0 then
        utils.erase_sorted_key(self.target_unit.status.current_soul.skills,sk_en.id,"id")
    elseif sk and lvl<0 then
        utils.insert_or_update(self.target_unit.status.current_soul.skills, {new=true, id=sk_en.id, rating=sk.rating+lvl}, 'id') --TODO set exp?
    end
    self:update_list()
end

function Editor_Skills:remove_rust(skill)
    --TODO
end
