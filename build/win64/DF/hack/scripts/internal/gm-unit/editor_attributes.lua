-- Attributes editor module for gui/gm-unit.
--@ module = true

local dialog = require 'gui.dialogs'
local widgets = require 'gui.widgets'
local base_editor = reqscript("internal/gm-unit/base_editor")


Editor_Attrs = defclass(Editor_Attrs, base_editor.Editor)
Editor_Attrs.ATTRS{
    frame_title = "Attributes editor"
}
function format_attr( name ,max_len)
    local n=name
    n=n:gsub("_"," "):lower() --"_" to " " and lower case
    n=n .. string.rep(" ", max_len - #n+1) --pad to max_len+1 for nice columns
    n=n:gsub("^%l",string.upper) --uppercase first character
    return n
end
function list_attrs(unit)
    local m_attrs=unit.status.current_soul.mental_attrs
    local b_attrs=unit.body.physical_attrs
    local ret = {}
    local max_len=0
    for i,v in ipairs(df.mental_attribute_type) do
        if(max_len<#v) then
            max_len=#v
        end
    end
    for i,v in ipairs(df.physical_attribute_type) do
        if(max_len<#v) then
            max_len=#v
        end
    end
    for i,v in ipairs(m_attrs) do
        local attr_name=format_attr(df.mental_attribute_type[i],max_len)
        local text=string.format("%s: %d/%d",
            attr_name,v.value,v.max_value)
        table.insert(ret,{
            text=text,
            attr=v,
            attr_name=attr_name,
            search_key=text:lower()
        })
    end
    for i,v in ipairs(b_attrs) do
        local attr_name=format_attr(df.physical_attribute_type[i],max_len)
        local text=string.format("%s: %d/%d",
            attr_name,v.value,v.max_value)
        table.insert(ret,{
            text=text,
            attr=v,
            attr_name=attr_name,
            search_key=text:lower()
        })
    end
    return ret
end
function Editor_Attrs:update_list(no_save_place)
    local attr_list=list_attrs(self.target_unit)
    if no_save_place then
        self.subviews.attributes:setChoices(attr_list)
    else
        self.subviews.attributes:setChoices(attr_list,self.subviews.attributes:getSelected())
    end
end

function Editor_Attrs:init( args )
    if self.target_unit.status.current_soul==nil then
        qerror("Unit does not have soul, can't edit mental attributes")
    end

    local attr_list=list_attrs(self.target_unit)

    self:addviews{
        widgets.FilteredList{
            choices=attr_list,
            frame = {t=0, b=2,l=0},
            view_id="attributes",
            edit_ignore_keys={'STRING_A047'}
        },
        widgets.Label{
            frame = { b=0,l=0},
            text = {
                {text=": set attribute ",
                key = "SELECT",
                on_activate= function (  )
                    local a,a_name=self:get_cur_attr()
                    dialog.showInputPrompt(a_name,"Enter new value:",COLOR_WHITE,
                    tostring(a.value),function(new_value)
                        a.value=new_value
                        self:update_list()
                    end)
                end
                },
                {text=": set max attribute ",
                key = "STRING_A047",
                on_activate= function (  )
                    local a,a_name=self:get_cur_attr()
                    dialog.showInputPrompt(a_name,"Enter new max value:",COLOR_WHITE,
                    tostring(a.max_value),function(new_value)
                        a.max_value=new_value
                        self:update_list()
                    end)
                end
                },
            }
        },
    }
end
function Editor_Attrs:onOpen()
    self.subviews[1].edit:setFocus(true)
end
function Editor_Attrs:get_cur_attr()
    local list_wid=self.subviews.attributes
    local _,choice=list_wid:getSelected()
    if choice==nil then
        qerror("Nothing selected")
    end
    return choice.attr,choice.attr_name
end
function Editor_Attrs:remove_rust(attr)
    --TODO
    attr.unused_counter=0;
    attr.soft_demotion =0;
    attr.rust_counter=0;
    attr.demotion_counter=0;
end
