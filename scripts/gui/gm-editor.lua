-- Interface powered item editor.
local gui = require 'gui'
local dialog = require 'gui.dialogs'
local widgets =require 'gui.widgets'
local guiScript = require 'gui.script'
local args={...}

local keybindings={
    offset={key="CUSTOM_ALT_O",desc="Show current items offset"},
    find={key="CUSTOM_F",desc="Find a value by entering a predicate"},
    lua_set={key="CUSTOM_ALT_S",desc="Set by using a lua function"},
    insert={key="CUSTOM_ALT_I",desc="Insert a new value to the vector"},
    delete={key="CUSTOM_ALT_D",desc="Delete selected entry"},
    reinterpret={key="CUSTOM_ALT_R",desc="Open selected entry as something else"},
    help={key="HELP",desc="Show this help"},
    NOT_USED={key="SEC_SELECT",desc="Choose an enum value from a list"}, --not a binding...
}
function getTargetFromScreens()
    local my_trg
    if dfhack.gui.getCurFocus() == 'item' then
        my_trg=dfhack.gui.getCurViewscreen().item
    elseif dfhack.gui.getCurFocus() == 'joblist' then
        local t_screen=dfhack.gui.getCurViewscreen()
        my_trg=t_screen.jobs[t_screen.cursor_pos]
    elseif dfhack.gui.getCurFocus() == 'createquota' then
        local t_screen=dfhack.gui.getCurViewscreen()
        my_trg=t_screen.orders[t_screen.sel_idx]
    elseif dfhack.gui.getCurFocus() == 'dwarfmode/LookAround/Flow' then
        local t_look=df.global.ui_look_list.items[df.global.ui_look_cursor]
        my_trg=t_look.flow

    elseif dfhack.gui.getSelectedUnit(true) then
        my_trg=dfhack.gui.getSelectedUnit(true)
    elseif dfhack.gui.getSelectedItem(true) then
        my_trg=dfhack.gui.getSelectedItem(true)
    elseif dfhack.gui.getSelectedJob(true) then
        my_trg=dfhack.gui.getSelectedJob(true)
    else
        qerror("No valid target found")
    end
    return my_trg
end

GmEditorUi = defclass(GmEditorUi, gui.FramedScreen)
GmEditorUi.ATTRS={
    frame_style = gui.GREY_LINE_FRAME,
    frame_title = "GameMaster's editor",
	}
function GmEditorUi:onHelp()
    self.subviews.pages:setSelected(2)
end
function burning_red(input) -- todo does not work! bug angavrilov that so that he would add this, very important!!
    local col=COLOR_LIGHTRED
    return {text=input,pen=dfhack.pen.parse{fg=COLOR_LIGHTRED,bg=0}}
end
function Disclaimer(tlb)
    local dsc={"Association Of ",{text="Psychic ",pen=dfhack.pen.parse{fg=COLOR_YELLOW,bg=0}},
        "Dwarves (AOPD) is not responsible for all the damage",NEWLINE,"that this tool can (and will) cause to you and your loved dwarves",NEWLINE,"and/or saves.Please use with caution.",NEWLINE,{text="Magma not included.",pen=dfhack.pen.parse{fg=COLOR_LIGHTRED,bg=0}}}
    if tlb then
        for _,v in ipairs(dsc) do
            table.insert(tlb,v)
        end
    end
    return dsc
end
function GmEditorUi:init(args)
    self.stack={}
    self.item_count=0
    self.keys={}
    local helptext={{text="Help"},NEWLINE,NEWLINE}
    for k,v in pairs(keybindings) do
        table.insert(helptext,{text=v.desc,key=v.key,key_sep=':'})
        table.insert(helptext,NEWLINE)
    end
    table.insert(helptext,NEWLINE)
    Disclaimer(helptext)
    
    local helpPage=widgets.Panel{
        subviews={widgets.Label{text=helptext,frame = {l=1,t=1,yalign=0}}}}
    local mainList=widgets.List{view_id="list_main",choices={},frame = {l=1,t=3,yalign=0},on_submit=self:callback("editSelected"),
        on_submit2=self:callback("editSelectedEnum"),
        text_pen=dfhack.pen.parse{fg=COLOR_DARKGRAY,bg=0},cursor_pen=dfhack.pen.parse{fg=COLOR_YELLOW,bg=0}}
    local mainPage=widgets.Panel{
        subviews={
            mainList,
            widgets.Label{text={{text="<no item>",id="name"},{gap=1,text="Help",key="HELP",key_sep = '()'}}, view_id = 'lbl_current_item',frame = {l=1,t=1,yalign=0}},
            --widgets.Label{text="BLAH2"}
                }
        ,view_id='page_main'}
    
    local pages=widgets.Pages{subviews={mainPage,helpPage},view_id="pages"}
    self:addviews{
        pages
    }
    self:pushTarget(args.target)
end
function GmEditorUi:find(test)
    local trg=self:currentTarget() 
    
    if test== nil then
        dialog.showInputPrompt("Test function","Input function that tests(k,v as argument):",COLOR_WHITE,"",dfhack.curry(self.find,self))
        return
    end
    
    local e,what=load("return function(k,v) return "..test.." end")
    if e==nil then
        dialog.showMessage("Error!","function failed to compile\n"..what,COLOR_RED)
    end
        
    if trg.target and trg.target._kind and trg.target._kind=="container" then
        
        for k,v in pairs(trg.target) do
            if e()(k,v)==true then
                self:pushTarget(v)
                return
            end
        end
    else
        local i=1
        for k,v in pairs(trg.target) do
            if e()(k,v)==true then
                self.subviews.list_main:setSelected(i)
                return
            end
            i=i+1
        end
    end
end
function GmEditorUi:insertNew(typename)
    local tp=typename
    if typename== nil then
        dialog.showInputPrompt("Class type","Input class type:",COLOR_WHITE,"",self:callback("insertNew"))
        return
    end
    local ntype=df[tp]
    if ntype== nil then
        dialog.showMessage("Error!","Type '"..tp.." not found",COLOR_RED)
        return
    end
    
    local trg=self:currentTarget() 
    if trg.target and trg.target._kind and trg.target._kind=="container" then
        local thing=ntype:new()
        dfhack.call_with_finalizer(1,false,df.delete,thing,function (tscreen,target,to_insert)
            target:insert("#",to_insert); tscreen:updateTarget(true,true);end,self,trg.target,thing)
        
    end
end
function GmEditorUi:deleteSelected(key)
    local trg=self:currentTarget()
    if trg.target and trg.target._kind and trg.target._kind=="container" then
        trg.target:erase(key)
        self:updateTarget(true,true)
    end
end
function GmEditorUi:getSelectedKey()
    return self:currentTarget().keys[self.subviews.list_main:getSelected()]
end
function GmEditorUi:currentTarget()
    return self.stack[#self.stack]
end
function GmEditorUi:editSelectedEnum(index,choice)
    local trg=self:currentTarget()
    local trg_key=trg.keys[index]
    if trg.target._field==nil then qerror("not an enum") end
    local enum=trg.target:_field(trg_key)._type

    if enum._kind=="enum-type" then
        local list={}
        for i=enum._first_item, enum._last_item do
            table.insert(list,{text=tostring(enum[i]),value=i})
        end
        guiScript.start(function()
            local ret,idx,choice=guiScript.showListPrompt("Choose item:",nil,3,list)
            if ret then
                trg.target[trg_key]=choice.value
                self:updateTarget(true)
            end
        end)
        
    else
        qerror("not an enum")
    end
end
function GmEditorUi:openReinterpret(key)
    local trg=self:currentTarget()
    dialog.showInputPrompt(tostring(trg_key),"Enter new type:",COLOR_WHITE,
                "",function(choice)
                    local ntype=df[tp]
                    self:pushTarget(df.reinterpret_cast(ntype,trg.target[key]))
                end)
end
function GmEditorUi:editSelected(index,choice)
    local trg=self:currentTarget()
    local trg_key=trg.keys[index]
    if trg.target and trg.target._kind and trg.target._kind=="bitfield" then
        trg.target[trg_key]= not trg.target[trg_key]
        self:updateTarget(true)
    else
        --print(type(trg.target[trg.keys[trg.selected]]),trg.target[trg.keys[trg.selected]]._kind or "")
        local trg_type=type(trg.target[trg_key])
        if trg_type=='number' or trg_type=='string' then --ugly TODO: add metatable get selected
            dialog.showInputPrompt(tostring(trg_key),"Enter new value:",COLOR_WHITE,
                tostring(trg.target[trg_key]),self:callback("commitEdit",trg_key))
            
        elseif trg_type=='boolean' then
            trg.target[trg_key]= not trg.target[trg_key]
            self:updateTarget(true)
        elseif trg_type=='userdata' then
            self:pushTarget(trg.target[trg_key])
        else
            print("Unknow type:"..trg_type)
            print("Subtype:"..tostring(trg.target[trg_key]._kind))
        end
    end
end

function GmEditorUi:commitEdit(key,value)
    local trg=self:currentTarget()
    if type(trg.target[key])=='number' then
        trg.target[key]=tonumber(value)
    elseif type(trg.target[key])=='string' then
        trg.target[key]=value
    end
    self:updateTarget(true)
end

function GmEditorUi:set(key,input)
    local trg=self:currentTarget() 
    
    if input== nil then
        dialog.showInputPrompt("Set to what?","Lua code to set to (v cur target):",COLOR_WHITE,"",self:callback("set",key))
        return
    end
    local e,what=load("return function(v) return "..input.." end")
    if e==nil then
        dialog.showMessage("Error!","function failed to compile\n"..what,COLOR_RED)
        return
    end
    trg.target[key]=e()(trg)
    self:updateTarget(true)
end
function GmEditorUi:onInput(keys)
    if keys.LEAVESCREEN  then
        if self.subviews.pages:getSelected()==2 then
            self.subviews.pages:setSelected(1)
        else
            self:popTarget()
        end
    elseif keys[keybindings.offset.key] then
        local trg=self:currentTarget()
        local _,stoff=df.sizeof(trg.target)
        local size,off=df.sizeof(trg.target:_field(self:getSelectedKey()))
        dialog.showMessage("Offset",string.format("Size hex=%x,%x dec=%d,%d\nRelative hex=%x dec=%d",size,off,size,off,off-stoff,off-stoff),COLOR_WHITE)

    elseif keys[keybindings.find.key] then
        self:find()
    elseif keys[keybindings.lua_set.key] then
        self:set(self:getSelectedKey())
    elseif keys[keybindings.insert.key] then --insert
        self:insertNew()
    elseif keys[keybindings.delete.key] then --delete
        self:deleteSelected(self:getSelectedKey())
    elseif keys[keybindings.reinterpret.key] then 
        self:openReinterpret(self:getSelectedKey())
    end

    self.super.onInput(self,keys)
end
function getStringValue(trg,field)
    local obj=trg.target
    
    local text=tostring(obj[field])
    pcall(function()
    if obj._field ~= nil then
        local enum=obj:_field(field)._type
        if enum._kind=="enum-type" then
            text=text.."("..tostring(enum[obj[field]])..")"
        end
    end
    end)
    return text
end
function GmEditorUi:updateTarget(preserve_pos,reindex)
    local trg=self:currentTarget()
    if reindex then
        trg.keys={}
        for k,v in pairs(trg.target) do
            table.insert(trg.keys,k)
        end
    end
    self.subviews.lbl_current_item:itemById('name').text=tostring(trg.target)
    local t={}
    for k,v in pairs(trg.keys) do
			table.insert(t,{text={{text=string.format("%-25s",tostring(v))},{gap=1,text=getStringValue(trg,v)}}})
    end
    local last_pos
    if preserve_pos then
        last_pos=self.subviews.list_main:getSelected()
    end
    self.subviews.list_main:setChoices(t)
    if last_pos then
        self.subviews.list_main:setSelected(last_pos)
    else
        self.subviews.list_main:setSelected(trg.selected)
    end
end
function GmEditorUi:pushTarget(target_to_push)
    local new_tbl={}
    new_tbl.target=target_to_push
    new_tbl.keys={}
    new_tbl.selected=1
    if self:currentTarget()~=nil then
        self:currentTarget().selected=self.subviews.list_main:getSelected()
    end
    for k,v in pairs(target_to_push) do
        table.insert(new_tbl.keys,k)
    end
    new_tbl.item_count=#new_tbl.keys
    table.insert(self.stack,new_tbl)
    
    self:updateTarget()
end
function GmEditorUi:popTarget()
    table.remove(self.stack) --removes last element
    if #self.stack==0 then
        self:dismiss()
        return
    end
    self:updateTarget()
end
function show_editor(trg)
    local screen = GmEditorUi{target=trg}
    screen:show()
end
if #args~=0 then
    if args[1]=="dialog" then
        function thunk(entry)
            local t=load("return "..entry)()
            show_editor(t)
        end
        dialog.showInputPrompt("Gm Editor", "Object to edit:", COLOR_GRAY, "",thunk)
    elseif args[1]=="free" then
        show_editor(df.reinterpret_cast(df[args[2]],args[3]))
    else
        local t=load("return "..args[1])()
        show_editor(t)
    end
else
    show_editor(getTargetFromScreens())
end

