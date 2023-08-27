-- Wounds editor module for gui/gm-unit.
--@ module = true

local widgets = require 'gui.widgets'
local utils = require 'utils'
local base_editor = reqscript("internal/gm-unit/base_editor")


Editor_Wounds=defclass(Editor_Wounds, base_editor.Editor)
Editor_Wounds.ATTRS{
    frame_title = "Wounds editor"
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
    for i, v in ipairs(wp.flags1) do
        if v then
            return format_flag_name(df.wound_damage_flags1[i])
        end
    end
    for i, v in ipairs(wp.flags2) do
        if v then
            return format_flag_name(df.wound_damage_flags2[i])
        end
    end
    return "<unnamed wound>"
end
function lookup_bodypart( wound_part,unit,is_singular )
    local bp=unit.body.body_plan.body_parts
    local part=bp[wound_part.body_part_id]

    if is_singular then
        return part.name_singular[0].value
    else
        return part.name_plural[0].value
    end
end
function format_wound( list_id,wound, unit)
    --TODO(warmist): what if there are more parts?
    local name="<unnamed wound>"
    local body_part=""
    if wound.flags.severed_part then
        name="severed"
        if #wound.parts>0 then
            body_part=lookup_bodypart(wound.parts[0],unit,true)
        end
    else
        if #wound.parts>0 then
            if #wound.parts[0].effect_type>0 then --try to make wound name by effect...
                name=tostring(df.wound_effect_type[wound.parts[0].effect_type[0]])
                if #wound.parts>1 then --cheap and probably incorrect...
                    name=name.."s"
                end
            elseif is_scar(wound.parts[0]) then
                name="Scar"
            else
                local wp=wound.parts[0]
                name=name_from_flags(wp)
            end
            body_part=lookup_bodypart(wound.parts[0],unit,true)
        end
    end

    return string.format("%d. %s %s(%d)",list_id,body_part,name,wound.id)
end
function Editor_Wounds:update_wounds()
    local ret={}
    for i, v in ipairs(self.trg_wounds) do
        table.insert(ret,{text=format_wound(i, v,self.target_unit),wound=v})
    end
    self.subviews.wounds:setChoices(ret)
end
function Editor_Wounds:dirty_unit()
    self.target_unit.flags2={calculated_nerves=false,calculated_bodyparts=false,calculated_insulation=false}
    --[=[
        FIXME(warmist): testing required, this might be not enough:
            * look into body.body_plan.flags
            * all the "good" flags
        worked kindof okay so maybe not?
    --]=]
end
function Editor_Wounds:get_cur_wound()
    local list_wid=self.subviews.wounds
    local _,choice=list_wid:getSelected()
    if choice==nil then
        qerror("Nothing selected")
    end
    local ret_wound=utils.binsearch(self.trg_wounds,choice.id,"id")
    return choice,ret_wound
end
function Editor_Wounds:delete_current_wound(index,choice)

    utils.erase_sorted(self.trg_wounds,choice.wound,"id")
    choice.wound:delete()
    self:dirty_unit()
    self:update_wounds()
end
function Editor_Wounds:create_new_wound()
    print("Creating")
end
function Editor_Wounds:edit_cur_wound(index,choice)

end
function Editor_Wounds:init( args )
    if self.target_unit==nil then
        qerror("invalid unit")
    end
    self.trg_wounds=self.target_unit.body.wounds

    self:addviews{
    widgets.List{
        frame = {t=0, b=2,l=0},
        view_id="wounds",
        on_submit=self:callback("edit_cur_wound"),
        on_submit2=self:callback("delete_current_wound")
    },
    widgets.HotkeyLabel{
        frame = { b=0,l=0},
        label = 'delete wound',
        key = 'STRING_A045', -- '-'
        on_activate = function()
            self:delete_current_wound(self.subviews.wounds:getSelected())
        end,
    },

    --[[ TODO(warmist): implement this and the create_new_wound
    {text=": edit wound ",
    key = "SELECT"},]]

    --[[{text=": create wound ",
    key = "CUSTOM_CTRL_I",
    on_activate= self:callback("create_new_wound")},]]
    }
    self:update_wounds()
end
