-- Interface powered (somewhat user friendly) unit editor.

--[[=begin

gui/gm-unit
===========
An editor for various unit attributes.

=end]]
local gui = require 'gui'
local dialog = require 'gui.dialogs'
local widgets =require 'gui.widgets'
local guiScript = require 'gui.script'
local utils = require 'utils'
local args={...}


local target
--TODO: add more ways to guess what unit you want to edit
if args[1]~= nil then
    target=df.units.find(args[1])
else
    target=dfhack.gui.getSelectedUnit(true)
end

if target==nil then
    qerror("No unit to edit") --TODO: better error message
end
local editors={}
function add_editor(editor_class)
    table.insert(editors,{text=editor_class.ATTRS.frame_title,on_submit=function ( unit )
        editor_class{target_unit=unit}:show()
    end})
end
-------------------------------various subeditors---------
--TODO set local sould or better yet skills vector to reduce long skill list access typing
editor_skills=defclass(editor_skills,gui.FramedScreen)
editor_skills.ATTRS={
    frame_style = gui.GREY_LINE_FRAME,
    frame_title = "Skill editor",
    target_unit = DEFAULT_NIL,
    learned_only= false,
}
function list_skills(unit,learned_only)
    local s_=df.job_skill
    local u_skills=unit.status.current_soul.skills
    local ret={}
    for i,v in ipairs(s_) do
        if i>0 then
            local u_skill=utils.binsearch(u_skills,i,"id")
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

                local text=string.format("%s: %s %d %d/%d",df.job_skill.attrs[i].caption,rating.caption,u_skill.rating,u_skill.experience,rating.xp_threshold)
                table.insert(ret,{text=text,id=i})
            end
        end
    end
    return ret
end
function editor_skills:update_list(no_save_place)
    local skill_list=list_skills(self.target_unit,self.learned_only)
    if no_save_place then
        self.subviews.skills:setChoices(skill_list)
    else
        self.subviews.skills:setChoices(skill_list,self.subviews.skills:getSelected())
    end
end
function editor_skills:init( args )
    if self.target_unit.status.current_soul==nil then
        qerror("Unit does not have soul, can't edit skills")
    end

    local skill_list=list_skills(self.target_unit,self.learned_only)

    self:addviews{
    widgets.FilteredList{
        choices=skill_list,
        frame = {t=0, b=1,l=1},
        view_id="skills",
    },
    widgets.Label{
                frame = { b=0,l=1},
                text ={{text= ": exit editor ",
                    key  = "LEAVESCREEN",
                    on_activate= self:callback("dismiss")
                    },
                    {text=": remove level ",
                    key = "SECONDSCROLL_UP",
                    on_activate=self:callback("level_skill",-1)},
                    {text=": add level ",
                    key = "SECONDSCROLL_DOWN",
                    on_activate=self:callback("level_skill",1)}
                    ,
                    {text=": show learned only ",
                    key = "CHANGETAB",
                    on_activate=function ()
                        self.learned_only=not self.learned_only
                        self:update_list(true)
                    end}
                    }
            },
        }
end
function editor_skills:get_cur_skill()
    local list_wid=self.subviews.skills
    local _,choice=list_wid:getSelected()
    if choice==nil then
        qerror("Nothing selected")
    end
    local u_skill=utils.binsearch(self.target_unit.status.current_soul.skills,choice.id,"id")
    return choice,u_skill
end
function editor_skills:level_skill(lvl)
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
function editor_skills:remove_rust(skill)
    --TODO
end
add_editor(editor_skills)
------- civ editor
RaceBox = defclass(RaceBox, dialog.ListBox)
RaceBox.focus_path = 'RaceBox'

RaceBox.ATTRS{
    format_name="$NAME ($TOKEN)",
    with_filter=true,
    allow_none=false,
}
function RaceBox:format_creature(creature_raw)
    local t = {NAME=creature_raw.name[0],TOKEN=creature_raw.creature_id}
    return string.gsub(self.format_name, "%$(%w+)", t)
end
function RaceBox:preinit(info)
    self.format_name=RaceBox.ATTRS.format_name or info.format_name -- preinit does not have ATTRS set yet
    local choices={}
    if RaceBox.ATTRS.allow_none or info.allow_none then
        table.insert(choices,{text="<none>",num=-1})
    end
    for i,v in ipairs(df.global.world.raws.creatures.all) do
        local text=self:format_creature(v)
        table.insert(choices,{text=text,raw=v,num=i,search_key=text:lower()})
    end
    info.choices=choices
end
function showRacePrompt(title, text, tcolor, on_select, on_cancel, min_width,allow_none)
    RaceBox{
        frame_title = title,
        text = text,
        text_pen = tcolor,
        on_select = on_select,
        on_cancel = on_cancel,
        frame_width = min_width,
        allow_none = allow_none,
    }:show()
end
CivBox = defclass(CivBox,dialog.ListBox)
CivBox.focus_path = "CivBox"

CivBox.ATTRS={
    format_name="$NAME ($ENGLISH):$ID",
    format_no_name="<unnamed>:$ID",
    name_other="<other(-1)>",
    with_filter=true,
    allow_other=false,
}

function civ_name(id,format_name,format_no_name,name_other,name_invalid)
    if id==-1 then
        return name_other or "<other (-1)>"
    end
    local civ
    if type(id)=='userdata' then
        civ=id
    else
        civ=df.historical_entity.find(id)
        if civ==nil then
            return name_invalid or "<invalid>"
        end
    end
    local t={NAME=dfhack.TranslateName(civ.name),ENGLISH=dfhack.TranslateName(civ.name,true),ID=civ.id} --TODO race?, maybe something from raws?
    if t.NAME=="" then
        return string.gsub(format_no_name or "<unnamed>:$ID", "%$(%w+)", t)
    end
    return string.gsub(format_name or "$NAME ($ENGLISH):$ID", "%$(%w+)", t)
end
function CivBox:update_choices()
    local choices={}
    if self.allow_other then
        table.insert(choices,{text=self.name_other,num=-1})
    end

    for i,v in ipairs(df.global.world.entities.all) do
        if not self.race_filter or (v.race==self.race_filter) then --TODO filter type
            local text=civ_name(v,self.format_name,self.format_no_name,self.name_other,self.name_invalid)
            table.insert(choices,{text=text,raw=v,num=i})
        end
    end
    self.choices=choices
    if self.subviews.list then
        self.subviews.list:setChoices(self.choices)
    end
end
function CivBox:update_race_filter(id)
    local raw=df.creature_raw.find(id)
    if raw then
        self.subviews.race_label:setText(": "..raw.name[0])
        self.race_filter=id
    else
        self.subviews.race_label:setText(": <none>")
        self.race_filter=nil
    end

    self:update_choices()
end
function CivBox:choose_race()
    showRacePrompt("Choose race","Select new race:",nil,function (id,choice)
        self:update_race_filter(choice.num)
    end,nil,nil,true)
end
function CivBox:init(info)
    self.subviews.list.frame={t=3,r=0,l=0}
    self:addviews{
        widgets.Label{frame={t=1,l=0},text={
        {text="Filter race ",key="CUSTOM_CTRL_A",key_sep="()",on_activate=self:callback("choose_race")},
        }},
        widgets.Label{frame={t=1,l=21},view_id="race_label",
        text=": <none>",
        }
    }
    self:update_choices()
end
function showCivPrompt(title, text, tcolor, on_select, on_cancel, min_width,allow_other)
    CivBox{
        frame_title = title,
        text = text,
        text_pen = tcolor,
        on_select = on_select,
        on_cancel = on_cancel,
        frame_width = min_width,
        allow_other = allow_other,
    }:show()
end

editor_civ=defclass(editor_civ,gui.FramedScreen)
editor_civ.ATTRS={
    frame_style = gui.GREY_LINE_FRAME,
    frame_title = "Civilization editor",
    target_unit = DEFAULT_NIL,
    }

function editor_civ:update_curren_civ()
    self.subviews.civ_name:setText("Currently: "..civ_name(self.target_unit.civ_id))
end
function editor_civ:init( args )
    if self.target_unit==nil then
        qerror("invalid unit")
    end

    self:addviews{
    widgets.Label{view_id="civ_name",frame = { t=1,l=1}, text="Currently: "..civ_name(self.target_unit.civ_id)},
    widgets.Label{frame = { t=2,l=1}, text={{text=": set to other (-1, usually enemy)",key="CUSTOM_N",
        on_activate= function() self.target_unit.civ_id=-1;self:update_curren_civ() end}}},
    widgets.Label{frame = { t=3,l=1}, text={{text=": set to current civ("..df.global.ui.civ_id..")",key="CUSTOM_C",
        on_activate= function() self.target_unit.civ_id=df.global.ui.civ_id;self:update_curren_civ() end}}},
    widgets.Label{frame = { t=4,l=1}, text={{text=": manually enter",key="CUSTOM_E",
        on_activate=function ()
         dialog.showInputPrompt("Civ id","Enter new civ id:",COLOR_WHITE,
            tostring(self.target_unit.civ_id),function(new_value)
                self.target_unit.civ_id=new_value
                self:update_curren_civ()
            end)
        end}}
        },
    widgets.Label{frame= {t=5,l=1}, text={{text=": select from list",key="CUSTOM_L",
        on_activate=function (  )
            showCivPrompt("Choose civilization", "Select units civilization",nil,function ( id,choice )
                self.target_unit.civ_id=choice.num
                self:update_curren_civ()
            end,nil,nil,true)
        end
        }}},
    widgets.Label{
                frame = { b=0,l=1},
                text ={{text= ": exit editor ",
                    key  = "LEAVESCREEN",
                    on_activate= self:callback("dismiss")
                    },
                    }
            },
        }
end
add_editor(editor_civ)
------- counters editor
editor_counters=defclass(editor_counters,gui.FramedScreen)
editor_counters.ATTRS={
    frame_style = gui.GREY_LINE_FRAME,
    frame_title = "Counters editor",
    target_unit = DEFAULT_NIL,
    counters1={
    "think_counter",
    "job_counter",
    "swap_counter",
    "winded",
    "stunned",
    "unconscious",
    "suffocation",
    "webbed",
    "soldier_mood_countdown",
    "soldier_mood", --todo enum,
    "pain",
    "nausea",
    "dizziness",
    },
    counters2={
    "paralysis",
    "numbness",
    "fever",
    "exhaustion",
    "hunger_timer",
    "thirst_timer",
    "sleepiness_timer",
    "stomach_content",
    "stomach_food",
    "vomit_timeout",
    "stored_fat" --TODO what to reset to?
    }
}
function editor_counters:fill_counters()
    local ret={}
    local u=self.target_unit
    for i,v in ipairs(self.counters1) do
        table.insert(ret,{f=u.counters:_field(v),name=v})
    end
    for i,v in ipairs(self.counters2) do
        table.insert(ret,{f=u.counters2:_field(v),name=v})
    end
    return ret
end
function editor_counters:update_counters()
    for i,v in ipairs(self.counter_list) do
        v.text=string.format("%s:%d",v.name,v.f.value)
    end
    self.subviews.counters:setChoices(self.counter_list)
end
function editor_counters:set_cur_counter(value,index,choice)
    choice.f.value=value
    self:update_counters()
end
function editor_counters:choose_cur_counter(index,choice)
    dialog.showInputPrompt(choice.name,"Enter new value:",COLOR_WHITE,
            tostring(choice.f.value),function(new_value)
                self:set_cur_counter(new_value,index,choice)
            end)
end
function editor_counters:init( args )
    if self.target_unit==nil then
        qerror("invalid unit")
    end

    self.counter_list=self:fill_counters()


    self:addviews{
    widgets.FilteredList{
        choices=self.counter_list,
        frame = {t=0, b=1,l=1},
        view_id="counters",
        on_submit=self:callback("choose_cur_counter"),
        on_submit2=self:callback("set_cur_counter",0),--TODO some things need to be set to different defaults
    },
    widgets.Label{
                frame = { b=0,l=1},
                text ={{text= ": exit editor ",
                    key  = "LEAVESCREEN",
                    on_activate= self:callback("dismiss")
                    },
                    {text=": reset counter ",
                    key = "SEC_SELECT",
                    },
                    {text=": set counter ",
                    key = "SELECT",
                    }
                    
                    }
            },
        }
    self:update_counters()
end
add_editor(editor_counters)

wound_creator=defclass(wound_creator,gui.FramedScreen)
wound_creator.ATTRS={
    frame_style = gui.GREY_LINE_FRAME,
    frame_title = "Wound creator",
    target_wound = DEFAULT_NIL,
    --filter
}
function wound_creator:init( args )
    if self.target_wound==nil then
        qerror("invalid wound")
    end
    

    self:addviews{
    widgets.List{
        
        frame = {t=0, b=1,l=1},
        view_id="fields",
        on_submit=self:callback("edit_cur_wound"),
        on_submit2=self:callback("delete_current_wound")
    },
    widgets.Label{
                frame = { b=0,l=1},
                text ={{text= ": exit editor ",
                    key  = "LEAVESCREEN",
                    on_activate= self:callback("dismiss")},

                    {text=": edit wound ",
                    key = "SELECT"},

                    {text=": delete wound ",
                    key = "SEC_SELECT"},
                    {text=": create wound ",
                    key = "CUSTOM_CTRL_I",
                    on_activate= self:callback("create_new_wound")},

                    }
            },
        }
    self:update_wounds()
end
-------------------
editor_wounds=defclass(editor_wounds,gui.FramedScreen)
editor_wounds.ATTRS={
    frame_style = gui.GREY_LINE_FRAME,
    frame_title = "Wound editor",
    target_unit = DEFAULT_NIL,
    --filter
}
function is_scar( wound_part )
    return wound_part.flags1.scar_cut or wound_part.flags1.scar_smashed or
        wound_part.flags1.scar_edged_shake1 or wound_part.flags1.scar_blunt_shake1
end
function format_flag_name( fname )
    return fname:sub(1,1):upper()..fname:sub(2):gsub("_"," ")
end
function name_from_flags( wp )
    for i,v in ipairs(wp.flags1) do
        if v then
            return format_flag_name(df.wound_damage_flags1[i])
        end
    end
    for i,v in ipairs(wp.flags2) do
        if v then
            return format_flag_name(df.wound_damage_flags2[i])
        end
    end
    return "<unnamed wound>"
end
function format_wound( list_id,wound, unit)

    local name="<unnamed wound>"
    if #wound.parts>0 and #wound.parts[0].effect_type>0 then --try to make wound name by effect...
        name=tostring(df.wound_effect_type[wound.parts[0].effect_type[0]])
        if #wound.parts>1 then --cheap and probably incorrect...
            name=name.."s"
        end
    elseif #wound.parts>0 and is_scar(wound.parts[0]) then
        name="Scar"
    elseif #wound.parts>0 then
        local wp=wound.parts[0]
        name=name_from_flags(wp)
    end

    return string.format("%d. %s id=%d",list_id,name,wound.id)
end
function editor_wounds:update_wounds()
    local ret={}
    for i,v in ipairs(self.trg_wounds) do
        table.insert(ret,{text=format_wound(i,v,self.target_unit),wound=v})
    end
    self.subviews.wounds:setChoices(ret)
    self.wound_list=ret
end
function editor_wounds:dirty_unit()
    print("todo: implement unit status recalculation")
end
function editor_wounds:get_cur_wound()
    local list_wid=self.subviews.wounds
    local _,choice=list_wid:getSelected()
    if choice==nil then
        qerror("Nothing selected")
    end
    local ret_wound=utils.binsearch(self.trg_wounds,choice.id,"id")
    return choice,ret_wound
end
function editor_wounds:delete_current_wound(index,choice)
    
    utils.erase_sorted(self.trg_wounds,choice.wound,"id")
    choice.wound:delete()
    self:dirty_unit()
    self:update_wounds()
end
function editor_wounds:create_new_wound()
    print("Creating")
end
function editor_wounds:edit_cur_wound(index,choice)
    
end
function editor_wounds:init( args )
    if self.target_unit==nil then
        qerror("invalid unit")
    end
    self.trg_wounds=self.target_unit.body.wounds

    self:addviews{
    widgets.List{
        
        frame = {t=0, b=1,l=1},
        view_id="wounds",
        on_submit=self:callback("edit_cur_wound"),
        on_submit2=self:callback("delete_current_wound")
    },
    widgets.Label{
                frame = { b=0,l=1},
                text ={{text= ": exit editor ",
                    key  = "LEAVESCREEN",
                    on_activate= self:callback("dismiss")},

                    {text=": edit wound ",
                    key = "SELECT"},

                    {text=": delete wound ",
                    key = "SEC_SELECT"},
                    {text=": create wound ",
                    key = "CUSTOM_CTRL_I",
                    on_activate= self:callback("create_new_wound")},

                    }
            },
        }
    self:update_wounds()
end
add_editor(editor_wounds)

-------------------------------main window----------------
unit_editor = defclass(unit_editor, gui.FramedScreen)
unit_editor.ATTRS={
    frame_style = gui.GREY_LINE_FRAME,
    frame_title = "GameMaster's unit editor",
    target_unit = DEFAULT_NIL,
    }


function unit_editor:init(args)

    self:addviews{
    widgets.FilteredList{
        choices=editors,
        on_submit=function (idx,choice)
            if choice.on_submit then
                choice.on_submit(self.target_unit)
            end
        end
    },
    widgets.Label{
                frame = { b=0,l=1},
                text ={{text= ": exit editor",
                    key  = "LEAVESCREEN",
                    on_activate= self:callback("dismiss")
                    },
                    }
            },
        }
end


unit_editor{target_unit=target}:show()
