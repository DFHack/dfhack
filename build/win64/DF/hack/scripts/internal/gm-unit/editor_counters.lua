-- Counters editor module for gui/gm-unit.
--@ module = true

local dialog = require 'gui.dialogs'
local widgets = require 'gui.widgets'
local base_editor = reqscript("internal/gm-unit/base_editor")

Editor_Counters=defclass(Editor_Counters, base_editor.Editor)
Editor_Counters.ATTRS{
    frame_title = "Counters editor",
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
function Editor_Counters:fill_counters()
    local ret = {}
    local u = self.target_unit
    for i, v in ipairs(self.counters1) do
        table.insert(ret, {f=u.counters:_field(v),name=v})
    end
    for i, v in ipairs(self.counters2) do
        table.insert(ret, {f=u.counters2:_field(v),name=v})
    end
    return ret
end
function Editor_Counters:update_counters()
    for i, v in ipairs(self.counter_list) do
        v.text=string.format("%s: %d", v.name, v.f.value)
    end
    self.subviews.counters:setChoices(self.counter_list)
end
function Editor_Counters:set_cur_counter(value,_,choice)
    choice.f.value = value
    self:update_counters()
end
function Editor_Counters:choose_cur_counter(index,choice)
    dialog.showInputPrompt(choice.name,"Enter new value:",COLOR_WHITE,
            tostring(choice.f.value),function(new_value)
                self:set_cur_counter(new_value,index,choice)
            end)
end
function Editor_Counters:init( args )
    if self.target_unit==nil then
        qerror("invalid unit")
    end

    self.counter_list=self:fill_counters()


    self:addviews{
        widgets.FilteredList{
            choices=self.counter_list,
            frame = {t=0, b=2,l=0},
            view_id="counters",
            edit_ignore_keys={"STRING_A047"},
            on_submit=self:callback("choose_cur_counter"),
        },
        widgets.Label{
            frame = {b=0,l=0},
            text = {
                {text=": set counter ", key = "SELECT"},
                --TODO some things need to be set to different defaults
                {text=": reset counter ", key = "STRING_A047",
                 on_activate=function()
                    self:set_cur_counter(0, self.subviews.counters:getSelected())
                 end},
            }
        },
    }
    self:update_counters()
end
function Editor_Counters:onOpen()
    self.subviews[1].edit:setFocus(true)
end
