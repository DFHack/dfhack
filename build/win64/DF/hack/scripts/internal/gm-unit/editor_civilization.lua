-- Civilization editor module for gui/gm-unit.
--@ module = true

local dialog = require 'gui.dialogs'
local widgets = require 'gui.widgets'
local base_editor = reqscript("internal/gm-unit/base_editor")


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
    for i, v in ipairs(df.global.world.raws.creatures.all) do
        local text=self:format_creature(v)
        table.insert(choices,{text=text,raw=v,num=i,search_key=text:lower()})
    end
    info.choices=choices
end
function showRacePrompt(title, text, tcolor, on_select, on_cancel, min_width,allow_none)
    print('showing race prompt')
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

CivBox.ATTRS{
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
        return string.gsub(format_no_name or "<unnamed> ($ID)", "%$(%w+)", t)
    end
    return string.gsub(format_name or "$NAME ($ENGLISH) ($ID)", "%$(%w+)", t)
end
function CivBox:update_choices()
    local choices={}
    if self.allow_other then
        table.insert(choices,{text=self.name_other,num=-1})
    end

    for i, v in ipairs(df.global.world.entities.all) do
        if not self.race_filter or (v.race==self.race_filter) then --TODO filter type
            local text=civ_name(v,self.format_name,self.format_no_name,self.name_other,self.name_invalid)
            table.insert(choices,{text=text,raw=v,num=i})
        end
    end
    if self.subviews.list then
        self.subviews.list:setChoices(choices)
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
    self.subviews.list.edit.ignore_keys={"STRING_A047"},
    self:addviews{
        widgets.Label{frame={t=1,l=0},text={
        {text="Filter race ",key="STRING_A047",key_sep="()",on_activate=self:callback("choose_race")},
        }},
        widgets.Label{frame={t=1,l=16},view_id="race_label",
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

Editor_Civ=defclass(Editor_Civ, base_editor.Editor)
Editor_Civ.ATTRS{
    frame_title = "Civilization editor"
}

function Editor_Civ:update_curren_civ()
    self.subviews.civ_name:setText("Currently: "..civ_name(self.target_unit.civ_id))
end
function Editor_Civ:init( args )
    if self.target_unit==nil then
        qerror("invalid unit")
    end

    self:addviews{
    widgets.Label{view_id="civ_name",frame = { t=0,l=0}, text="Currently: "..civ_name(self.target_unit.civ_id)},
    widgets.Label{frame = { t=2,l=0}, text={{text=": set to other (-1, usually enemy)",key="CUSTOM_N",
        on_activate= function() self.target_unit.civ_id=-1;self:update_curren_civ() end}}},
    widgets.Label{frame = { t=3,l=0}, text={{text=": set to current civ ("..df.global.plotinfo.civ_id..")",key="CUSTOM_C",
        on_activate= function() self.target_unit.civ_id=df.global.plotinfo.civ_id;self:update_curren_civ() end}}},
    widgets.Label{frame = { t=4,l=0}, text={{text=": manually enter",key="CUSTOM_E",
        on_activate=function ()
         dialog.showInputPrompt("Civ id","Enter new civ id:",COLOR_WHITE,
            tostring(self.target_unit.civ_id),function(new_value)
                self.target_unit.civ_id=new_value
                self:update_curren_civ()
            end)
        end}}
        },
    widgets.Label{frame= {t=5,l=0}, text={{text=": select from list",key="CUSTOM_L",
        on_activate=function (  )
            showCivPrompt("Choose civilization", "Select units civilization",nil,function ( id,choice )
                self.target_unit.civ_id=choice.num
                self:update_curren_civ()
            end,nil,nil,true)
        end
        }}},
    }
end
