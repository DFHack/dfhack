-- Interface powered item editor.
local gui = require 'gui'
local dialog = require 'gui.dialogs'
local args={...}
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


local MODE_BROWSE=0
local MODE_EDIT=1
GmEditorUi = defclass(GmEditorUi, gui.FramedScreen)
GmEditorUi.ATTRS={
    frame_style = gui.GREY_LINE_FRAME,
    frame_title = "GameMaster's editor",
	}
function GmEditorUi:init(args)
    self.stack={}
    self.item_count=0
    self.mode=MODE_BROWSE
    self.keys={}
    self:pushTarget(args.target)
    
    return self
end
function GmEditorUi:find(test)
    local trg=self:currentTarget() 
    if trg.target and trg.target._kind and trg.target._kind=="container" then
        if test== nil then
            dialog.showInputPrompt("Test function","Input function that tests(k,v as argument):",COLOR_WHITE,"",dfhack.curry(self.find,self))
            return
        end
        local e,what=load("return function(k,v) return "..test.." end")
        if e==nil then
            dialog.showMessage("Error!","function failed to compile\n"..what,COLOR_RED)
        end
        for k,v in pairs(trg.target) do
            if e()(k,v)==true then
                self:pushTarget(v)
                return
            end
        end
    end
end
function GmEditorUi:insertNew(typename)
    local tp=typename
    if typename== nil then
        dialog.showInputPrompt("Class type","Input class type:",COLOR_WHITE,"",dfhack.curry(self.insertNew,self))
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
        dfhack.call_with_finalizer(1,false,df.delete,thing,trg.target.insert,trg.target,'#',thing)
        
    end
end
function GmEditorUi:deleteSelected()
    local trg=self:currentTarget()
    if trg.target and trg.target._kind and trg.target._kind=="container" then
        trg.target:erase(trg.keys[trg.selected])
    end
end
function GmEditorUi:currentTarget()
    return self.stack[#self.stack]
end
function GmEditorUi:changeSelected(delta)
    local trg=self:currentTarget()
    if trg.item_count <= 1 then return end
    trg.selected = 1 + (trg.selected + delta - 1) % trg.item_count
end
function GmEditorUi:editSelected()
    local trg=self:currentTarget()
    if trg.target and trg.target._kind and trg.target._kind=="bitfield" then
        trg.target[trg.keys[trg.selected]]= not trg.target[trg.keys[trg.selected]]
    else
        --print(type(trg.target[trg.keys[trg.selected]]),trg.target[trg.keys[trg.selected]]._kind or "")
        local trg_type=type(trg.target[trg.keys[trg.selected]])
        if trg_type=='number' or trg_type=='string' then --ugly TODO: add metatable get selected
            self.mode=MODE_EDIT
            self.input=tostring(trg.target[trg.keys[trg.selected]])
        elseif trg_type=='boolean' then
            trg.target[trg.keys[trg.selected]]= not trg.target[trg.keys[trg.selected]]
        elseif trg_type=='userdata' then
            self:pushTarget(trg.target[trg.keys[trg.selected]])
            --local screen = mkinstance(gui.FramedScreen,GmEditorUi):init(trg.target[trg.keys[trg.selected]]) -- does not work
            --screen:show()
        else
            print("Unknow type:"..trg_type)
            print("Subtype:"..tostring(trg.target[trg.keys[trg.selected]]._kind))
        end
    end
end
function GmEditorUi:cancelEdit()
    self.mode=MODE_BROWSE
    self.input=""
end
function GmEditorUi:commitEdit()
    local trg=self:currentTarget()
    self.mode=MODE_BROWSE
    if type(trg.target[trg.keys[trg.selected]])=='number' then
        trg.target[trg.keys[trg.selected]]=tonumber(self.input)
    elseif type(trg.target[trg.keys[trg.selected]])=='string' then
        trg.target[trg.keys[trg.selected]]=self.input
    end
end
function GmEditorUi:onRenderBody( dc)
    local trg=self:currentTarget()
    dc:seek(2,1):string(tostring(trg.target), COLOR_RED)
    local offset=2
    local page_offset=0
    local current_item=1
    local t_col
    local width,height=self:getWindowSize()
    local window_height=height-offset-2
    local cursor_window=math.floor(trg.selected / window_height)
    if  cursor_window>0 then
        page_offset=cursor_window*window_height-1
    end
    for k,v in pairs(trg.target) do
        
        if current_item==trg.selected then
            t_col=COLOR_LIGHTGREEN
        else
            t_col=COLOR_GRAY
        end
        
        if current_item-page_offset > 0 then
            local y_pos=current_item-page_offset+offset
            dc:seek(2,y_pos):string(tostring(k),t_col)
            
            if self.mode==MODE_EDIT and current_item==trg.selected then
                dc:seek(20,y_pos):string(self.input..'_',COLOR_GREEN)
            else
                dc:seek(20,y_pos):string(tostring(v),t_col)
            end
            if y_pos+3>height then
                break
            end
        end
        current_item=current_item+1
        
    end
end
 function GmEditorUi:onInput(keys)
    if self.mode==MODE_BROWSE then
        if keys.LEAVESCREEN  then
            self:popTarget()
        elseif keys.CURSOR_UP then
            self:changeSelected(-1)
        elseif keys.CURSOR_DOWN then
            self:changeSelected(1)
        elseif keys.CURSOR_UP_FAST then
            self:changeSelected(-10)
        elseif keys.CURSOR_DOWN_FAST then
            self:changeSelected(10)
        elseif keys.SELECT then
            self:editSelected()
        elseif keys.CUSTOM_ALT_F then
            self:find()
        elseif keys.CUSTOM_ALT_E then
            --self:specialEditor()
        elseif keys.CUSTOM_ALT_I then --insert
            self:insertNew()
        elseif keys.CUSTOM_ALT_D then --delete
            self:deleteSelected()
        end
    elseif self.mode==MODE_EDIT then
        if keys.LEAVESCREEN  then
            self:cancelEdit()
        elseif keys.SELECT then
            self:commitEdit()
        elseif keys._STRING then
            if keys._STRING==0 then
                self.input=string.sub(self.input,1,-2)
            else
                self.input=self.input.. string.char(keys._STRING)
            end
        end
    end
end
function GmEditorUi:pushTarget(target_to_push)
    local new_tbl={}
    new_tbl.target=target_to_push
    new_tbl.keys={}
    new_tbl.selected=1
    for k,v in pairs(target_to_push) do
        table.insert(new_tbl.keys,k)
    end
    new_tbl.item_count=#new_tbl.keys
    table.insert(self.stack,new_tbl)
end
function GmEditorUi:popTarget()
    table.remove(self.stack) --removes last element
    if #self.stack==0 then
        self:dismiss()
    end
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
    else
        local t=load("return "..args[1])()
        show_editor(t)
    end
else
    show_editor(getTargetFromScreens())
end

