-- Interface powered memory object editor.
--@module=true

local gui = require 'gui'
local json = require 'json'
local dialog = require 'gui.dialogs'
local widgets = require 'gui.widgets'
local guiScript = require 'gui.script'
local utils = require 'utils'

config = config or json.open('dfhack-config/gm-editor.json')

local REFRESH_MS = 100

function save_config(data)
    utils.assign(config.data, data)
    config:write()
end

find_funcs = find_funcs or (function()
    local t = {}
    for k in pairs(df) do
        pcall(function()
            t[k] = df[k].find
        end)
    end
    return t
end)()

local keybindings_raw = {
    {name='toggle_ro', key="CUSTOM_CTRL_D",desc="Toggle between read-only and read-write"},
    {name='autoupdate', key="CUSTOM_ALT_A",desc="See live updates of changing values"},
    {name='offset', key="CUSTOM_ALT_O",desc="Show current items offset"},
    {name='find', key="CUSTOM_F",desc="Find a value by entering a predicate"},
    {name='find_id', key="CUSTOM_I",desc="Find object with this ID, using ref-target if available"},
    {name='find_id_raw', key="CUSTOM_SHIFT_I",desc="Find object with this ID, forcing dialog box"},
    {name='lua_set', key="CUSTOM_ALT_S",desc="Set by using a lua function"},
    {name='insert', key="CUSTOM_ALT_I",desc="Insert a new value to the vector"},
    {name='delete', key="CUSTOM_ALT_D",desc="Delete selected entry"},
    {name='reinterpret', key="CUSTOM_ALT_R",desc="Open selected entry as something else"},
    {name='start_filter', key="CUSTOM_S",desc="Start typing filter, Enter to finish"},
    {name='gotopos', key="CUSTOM_G",desc="Move map view to location of target"},
    {name='help', key="STRING_A063",desc="Show this help"},
    {name='displace', key="STRING_A093",desc="Open reference offseted by index"},
    --{name='NOT_USED', key="SEC_SELECT",desc="Edit selected entry as a number (for enums)"}, --not a binding...
}

local keybindings = {}
for _, v in ipairs(keybindings_raw) do
    keybindings[v.name] = v
end

function getTypeName(type)
    return tostring(type):gmatch('<type: (.+)>')() or '<unknown type>'
end
function getTargetFromScreens()
    local my_trg = dfhack.gui.getSelectedUnit(true) or dfhack.gui.getSelectedItem(true)
            or dfhack.gui.getSelectedJob(true) or dfhack.gui.getSelectedBuilding(true)
            or dfhack.gui.getSelectedStockpile(true) or dfhack.gui.getSelectedCivZone(true)
    if not my_trg then
        qerror("No valid target found")
    end
    return my_trg
end
function search_relevance(search, candidate)
    local function clean(str)
        return ' ' .. str:lower():gsub('[^a-z0-9]','') .. ' '
    end
    search = clean(search)
    candidate = clean(candidate)
    local ret = 0
    while #search > 0 do
        local pos = candidate:find(search:sub(1, 1), 1, true)
        if pos then
            ret = ret + (#search - pos)
            candidate = candidate:sub(pos + 1)
        end
        search = search:sub(2)
    end
    return ret
end


GmEditorUi = defclass(GmEditorUi, widgets.Window)
GmEditorUi.ATTRS{
    frame=copyall(config.data.frame or {}),
    frame_title="GameMaster's editor",
    frame_inset=0,
    resizable=true,
    resize_min={w=30, h=20},
    read_only=(config.data.read_only or false)
}

function burning_red(input) -- todo does not work! bug angavrilov that so that he would add this, very important!!
    local col=COLOR_LIGHTRED
    return {text=input,pen=dfhack.pen.parse{fg=COLOR_LIGHTRED,bg=0}}
end
function Disclaimer(tlb)
    local dsc={
        "Association Of ", {text="Psychic ",pen=COLOR_YELLOW}, "Dwarves (AOPD) is not responsible for all the damage", NEWLINE,
        "that this tool can (and will) cause to you and your loved dwarves", NEWLINE,
        "and/or saves. Please use with caution.", NEWLINE,
        {text="Magma not included.", pen=COLOR_LIGHTRED,bg=0}
    }
    if tlb then
        for _,v in ipairs(dsc) do
            table.insert(tlb,v)
        end
    end
    return dsc
end

function GmEditorUi:init(args)
    if not next(self.frame) then
        self.frame = {w=80, h=50}
    end

    -- don't appear directly over the current window
    if next(views) then
        if self.frame.l then self.frame.l = self.frame.l + 1 end
        if self.frame.r then self.frame.r = self.frame.r - 1 end
        if self.frame.t then self.frame.t = self.frame.t + 1 end
        if self.frame.b then self.frame.b = self.frame.b - 1 end
    end

    self.stack={}
    self.item_count=0
    self.keys={}
    local helptext={{text="Help"},NEWLINE,NEWLINE}
    for _,v in ipairs(keybindings_raw) do
        table.insert(helptext,{text=v.desc,key=v.key,key_sep=': '})
        table.insert(helptext,NEWLINE)
    end
    table.insert(helptext,NEWLINE)
    Disclaimer(helptext)

    local helpPage=widgets.Panel{
        subviews={widgets.Label{text=helptext,frame = {l=1,t=1,yalign=0}}}}
    local mainList=widgets.List{view_id="list_main",choices={},frame = {l=1,t=3,yalign=0},on_submit=self:callback("editSelected"),
        on_submit2=self:callback("editSelectedRaw"),
        text_pen=COLOR_GREY, cursor_pen=COLOR_YELLOW}
    local mainPage=widgets.Panel{
        subviews={
            mainList,
            widgets.Label{text={{text="<no item>",id="name"},{gap=1,text="Help",key=keybindings.help.key,key_sep = '()'}}, view_id = 'lbl_current_item',frame = {l=1,t=1,yalign=0}},
            widgets.EditField{frame={l=1,t=2,h=1},label_text="Search",key=keybindings.start_filter.key,key_sep='(): ',on_change=self:callback('text_input'),view_id="filter_input"}}
        ,view_id='page_main'}

    self:addviews{widgets.Pages{subviews={mainPage,helpPage},view_id="pages"}}
    self:pushTarget(args.target)
end
function GmEditorUi:verifyStack()
    local failure = false

    local last_good_level = nil

    for i, level in pairs(self.stack) do
        local obj=level.target
        if obj._kind == "bitfield" or obj._kind == "struct" then goto continue end

        local keys = level.keys
        local selection = level.selected
        local sel_key = keys[selection]
        if not sel_key then goto continue end
        local next_by_ref
        local status, _ = pcall(
        function()
            next_by_ref = obj[sel_key]
            end
        )
        if not status then
            failure = true
            last_good_level = i - 1
            break
        end
        if self.stack[i+1] and not self.stack[i+1] == next_by_ref then
            failure = true
            break
        end
        ::continue::
    end
    if failure then
        self.stack = {table.unpack(self.stack, 1, last_good_level)}
        return false
    end
    return true
end
function GmEditorUi:text_input(new_text)
    self:updateTarget(true,true)
end
function GmEditorUi:find(test)
    local trg=self:currentTarget()

    if test== nil then
        dialog.showInputPrompt("Test function","Input function that tests(k,v as argument):",COLOR_WHITE,"",dfhack.curry(self.find,self))
        return
    end

    local e,what=load("return function(k,v) return "..test.." end")
    if e==nil then
        dialog.showMessage("Error!","function failed to compile\n"..what,COLOR_LIGHTRED)
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
function GmEditorUi:find_id(force_dialog)
    local key = tostring(self:getSelectedKey())
    local id = tonumber(self:getSelectedValue())
    local field = self:getSelectedField()
    local ref_target = nil
    if field and field.ref_target then
        ref_target = field.ref_target
    end
    if ref_target and not force_dialog then
        local obj
        if not ref_target.find then
            if key == 'mat_type' then
                local ok, mi = pcall(function()
                    return self:currentTarget().target['mat_index']
                end)
                if ok then
                    obj = dfhack.matinfo.decode(id, mi)
                end
            end
            if not obj then
                dialog.showMessage("Error!", ("Cannot look up %s by ID"):format(getmetatable(ref_target)), COLOR_LIGHTRED)
                return
            end
        end
        obj = obj or ref_target.find(id)
        if obj then
            self:pushTarget(obj)
        else
            dialog.showMessage("Error!", ("%s with ID %d not found"):format(getmetatable(ref_target), id), COLOR_LIGHTRED)
        end
        return
    end
    if not id then return end
    local raw_message
    local search_key = key
    if ref_target then
        search_key = getmetatable(ref_target)
        raw_message = 'This field has a ref-target of ' .. search_key
        if not ref_target.get_vector then
            raw_message = raw_message .. '\nbut this type does not have an instance vector'
        end
    else
        raw_message = 'This field has no ref-target specified. If you\n' ..
                      'know what it should be, please report it!'
    end
    local opts = {}
    for name, func in pairs(find_funcs) do
        table.insert(opts, {text=name, callback=func, weight=search_relevance(search_key, name)})
    end
    table.sort(opts, function(a, b)
        return a.weight > b.weight
    end)
    local message = {{pen=COLOR_LIGHTRED, text="Note: "}}
    for _, line in ipairs(raw_message:split('\n')) do
        table.insert(message, line)
        table.insert(message, NEWLINE)
    end
    guiScript.start(function()
        local ret,idx,choice=guiScript.showListPrompt("Choose type:",message,COLOR_WHITE,opts,nil,true)
        if ret then
            local obj = choice.callback(id)
            if obj then
                self:pushTarget(obj)
            else
                dialog.showMessage("Error!", ('%s with ID %d not found'):format(choice.text, id), COLOR_LIGHTRED)
            end
        end
    end)
end
function GmEditorUi:insertNew(typename)
    if self.read_only then return end
    local tp=typename
    if typename == nil then
        dialog.showInputPrompt("Class type","You can:\n * Enter type name (without 'df.')\n * Leave empty for default type and 'nil' value\n * Enter '*' for default type and 'new' constructed pointer value",COLOR_WHITE,"",self:callback("insertNew"))
        return
    end

    local trg=self:currentTarget()
    if trg.target and trg.target._kind and trg.target._kind=="container" then
        if tp == "" then
            trg.target:resize(#trg.target+1)
        elseif tp== "*" then
            trg.target:insert("#",{new=true})
        else
            local ntype=df[tp]
            if ntype== nil then
                dialog.showMessage("Error!","Type '"..tp.." not found",COLOR_RED)
                return
            end
            trg.target:insert("#",{new=ntype})
        end
        self:updateTarget(true,true)
    end
end
function GmEditorUi:deleteSelected(key)
    if self.read_only then return end
    local trg=self:currentTarget()
    if trg.target and trg.target._kind and trg.target._kind=="container" then
        trg.target:erase(key)
        self:updateTarget(true,true)
    end
end
function GmEditorUi:getSelectedKey()
    return self:currentTarget().keys[self.subviews.list_main:getSelected()]
end
function GmEditorUi:getSelectedValue()
    return self:currentTarget().target[self:getSelectedKey()]
end
function GmEditorUi:getSelectedField()
    local ok, ret = pcall(function()
        return self:currentTarget().target:_field(self:getSelectedKey())
    end)
    if ok then
        return ret
    end
end
function GmEditorUi:currentTarget()
    return self.stack[#self.stack]
end
function GmEditorUi:getSelectedEnumType()
    local trg=self:currentTarget()
    local trg_key=trg.keys[self.subviews.list_main:getSelected()]

    local ok,ret=pcall(function () --super safe way to check if the field has enum
        return trg.target._field==nil or trg.target:_field(trg_key)==nil
    end)
    if not ok or ret==true then
        return nil
    end

    local enum=trg.target:_field(trg_key)._type
    if enum._kind=="enum-type" then
        return enum
    else
        return nil
    end
end
function GmEditorUi:editSelectedEnum(index,choice)
    local enum=self:getSelectedEnumType()
    if enum then
        local trg=self:currentTarget()
        local trg_key=self:getSelectedKey()
        local list={}
        for i=enum._first_item, enum._last_item do
            table.insert(list,{text=('%s (%i)'):format(tostring(enum[i]), i),value=i})
        end
        guiScript.start(function()
            local ret,idx,choice=guiScript.showListPrompt("Choose "..getTypeName(enum).." item:",nil,3,list,nil,true)
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
    dialog.showInputPrompt(tostring(self:getSelectedKey()),"Enter new type:",COLOR_WHITE,
                "",function(choice)
                    local ntype=df[choice]
                    self:pushTarget(df.reinterpret_cast(ntype,trg.target[key]))
                end)
end
function GmEditorUi:openOffseted(index,choice)
    local trg=self:currentTarget()
    local trg_key=trg.keys[index]
    dialog.showInputPrompt(tostring(trg_key),"Enter offset:",COLOR_WHITE,"",
        function(choice)
            self:pushTarget(trg.target[trg_key]:_displace(tonumber(choice)))
        end)
end
function GmEditorUi:locate(t)
    if getmetatable(t) == 'coord' then return t end
    if df.unit:is_instance(t) then return xyz2pos(dfhack.units.getPosition(t)) end
    if df.item:is_instance(t) then return xyz2pos(dfhack.items.getPosition(t)) end
    if df.building:is_instance(t) then return xyz2pos(t.centerx, t.centery, t.z) end
    -- anything else - look for pos or x/y/z fields
    local ok, pos = pcall(function() return t.pos end)
    if getmetatable(pos) == 'coord' then return pos end
    local x,y,z
    ok, x = pcall(function() return t.x end)
    if type(x)~='number' then ok, x = pcall(function() return t.centerx end) end
    if type(x)~='number' then ok, x = pcall(function() return t.x1 end) end
    ok, y = pcall(function() return t.y end)
    if type(y)~='number' then ok, y = pcall(function() return t.centery end) end
    if type(y)~='number' then ok, y = pcall(function() return t.y1 end) end
    ok, z = pcall(function() return t.z end)
    if type(x)=='number' and type(y)=='number' and type(z)=='number' then
        return xyz2pos(x,y,z)
    end
    return nil
end
function GmEditorUi:gotoPos()
    if not dfhack.isMapLoaded() then return end
    -- if the selected value is a coord then use it
    local pos = self:getSelectedValue()
    if getmetatable(pos) ~= 'coord' then
        -- otherwise locate the current target
        pos = GmEditorUi:locate(self:currentTarget().target)
        if not pos then
            -- otherwise locate the current selected value
            pos = GmEditorUi:locate(self:getSelectedValue())
        end
    end
    if pos then
        dfhack.gui.revealInDwarfmodeMap(pos,true)
        df.global.game.main_interface.recenter_indicator_m.x = pos.x
        df.global.game.main_interface.recenter_indicator_m.y = pos.y
        df.global.game.main_interface.recenter_indicator_m.z = pos.z
    end
end
function GmEditorUi:editSelectedRaw(index,choice)
    self:editSelected(index, choice, {raw=true})
end
function GmEditorUi:editSelected(index,choice,opts)
    if not self:verifyStack() then
        self:updateTarget()
        return
    end
    opts = opts or {}
    local trg=self:currentTarget()
    local trg_key=trg.keys[index]
    if trg.target and trg.target._kind and trg.target._kind=="bitfield" then
        if self.read_only then return end
        trg.target[trg_key]= not trg.target[trg_key]
        self:updateTarget(true)
    else
        local trg_type=type(trg.target[trg_key])
        if self:getSelectedEnumType() and not opts.raw then
            if self.read_only then return end
            self:editSelectedEnum()
        elseif trg_type=='number' or trg_type=='string' then --ugly TODO: add metatable get selected
            if self.read_only then return end
            local prompt = "Enter new value:"
            if self:getSelectedEnumType() then
                prompt = "Enter new " .. getTypeName(trg.target:_field(trg_key)._type) .. " value"
            end
            dialog.showInputPrompt(tostring(trg_key), prompt, COLOR_WHITE,
                tostring(trg.target[trg_key]), self:callback("commitEdit",trg_key))

        elseif trg_type == 'boolean' then
            if self.read_only then return end
            trg.target[trg_key] = not trg.target[trg_key]
            self:updateTarget(true)
        elseif trg_type == 'userdata' or trg_type == 'table' then
            self:pushTarget(trg.target[trg_key])
        elseif trg_type == 'nil' or trg_type == 'function' then
            -- ignore
        else
            print("Unknown type:"..trg_type)
            pcall(function() print("Subtype:"..tostring(trg.target[trg_key]._kind)) end)
        end
    end
end

function GmEditorUi:commitEdit(key,value)
    if self.read_only then return end
    local trg=self:currentTarget()
    if type(trg.target[key])=='number' then
        trg.target[key]=tonumber(value)
    elseif type(trg.target[key])=='string' then
        trg.target[key]=value
    end
    self:updateTarget(true)
end

function GmEditorUi:set(key,input)
    if self.read_only then return end
    local trg=self:currentTarget()

    if input== nil then
        dialog.showInputPrompt("Set to what?","Lua code to set to (v cur target):",COLOR_WHITE,"",self:callback("set",key))
        return
    end
    local e,what=load("return function(v) return "..input.." end")
    if e==nil then
        dialog.showMessage("Error!","function failed to compile\n"..what,COLOR_LIGHTRED)
        return
    end
    trg.target[key]=e()(trg)
    self:updateTarget(true)
end
function GmEditorUi:onInput(keys)
    if GmEditorUi.super.onInput(self, keys) then return true end

    if keys.LEAVESCREEN or keys._MOUSE_R_DOWN then
        if self.subviews.pages:getSelected()==2 then
            self.subviews.pages:setSelected(1)
        else
            self:popTarget()
        end
        return true
    end

    if self.subviews.pages:getSelected() == 2 then
        return false
    end

    if keys[keybindings.toggle_ro.key] then
        self.read_only = not self.read_only
        self:updateTitles()
        return true
    elseif keys[keybindings.autoupdate.key] then
        self.autoupdate = not self.autoupdate
        return true
    elseif keys[keybindings.offset.key] then
        local trg=self:currentTarget()
        local _,stoff=df.sizeof(trg.target)
        local size,off=df.sizeof(trg.target:_field(self:getSelectedKey()))
        dialog.showMessage("Offset",string.format("Size hex=%x,%x dec=%d,%d\nRelative hex=%x dec=%d",size,off,size,off,off-stoff,off-stoff),COLOR_WHITE)
        return true
    elseif keys[keybindings.displace.key] then
        self:openOffseted(self.subviews.list_main:getSelected())
        return true
    elseif keys[keybindings.find.key] then
        self:find()
        return true
    elseif keys[keybindings.find_id.key] then
        self:find_id()
        return true
    elseif keys[keybindings.find_id_raw.key] then
        self:find_id(true)
        return true
    elseif keys[keybindings.lua_set.key] then
        self:set(self:getSelectedKey())
        return true
    elseif keys[keybindings.insert.key] then --insert
        self:insertNew()
        return true
    elseif keys[keybindings.delete.key] then --delete
        self:deleteSelected(self:getSelectedKey())
        return true
    elseif keys[keybindings.reinterpret.key] then
        self:openReinterpret(self:getSelectedKey())
        return true
    elseif keys[keybindings.gotopos.key] then
        self:gotoPos()
        return true
    elseif keys[keybindings.help.key] then
        self.subviews.pages:setSelected(2)
        return true
    end
end

function getStringValue(trg,field)
    local obj=trg.target

    local text=tostring(obj[field])
    pcall(function()
    if obj._field ~= nil then
        local f = obj:_field(field)
        if df.coord:is_instance(f) then
            text=('(%d, %d, %d) '):format(f.x, f.y, f.z) .. text
        elseif df.coord2d:is_instance(f) then
            text=('(%d, %d) '):format(f.x, f.y) .. text
        end
        local enum=f._type
        if enum._kind=="enum-type" then
            text=text.." ("..tostring(enum[obj[field]])..")"
        end
        local ref_target=f.ref_target
        if ref_target then
            text=text.. " (ref-target: "..getmetatable(ref_target)..")"
        end
    end
    end)
    return text
end

function GmEditorUi:updateTitles()
    local title = "GameMaster's Editor"
    if self.read_only then
        title = title.." (Read Only)"
    end
    for view,_ in pairs(views) do
        view.subviews[1].frame_title = title
    end
    self.frame_title = title
    save_config({read_only = self.read_only})
end
function GmEditorUi:updateTarget(preserve_pos,reindex)
    self:verifyStack()
    local trg=self:currentTarget()
    local filter=self.subviews.filter_input.text:lower()

    if reindex then
        trg.keys={}
        trg.kw=10
        for k,v in pairs(trg.target) do
            if #tostring(k)>trg.kw then trg.kw=#tostring(k) end
            if filter~= "" then
                local ok,ret=dfhack.pcall(string.match,tostring(k):lower(),filter)
                if not ok then
                    table.insert(trg.keys,k)
                elseif ret then
                    table.insert(trg.keys,k)
                end
            else
                table.insert(trg.keys,k)
            end
        end
    end
    self.subviews.lbl_current_item:itemById('name').text=tostring(trg.target)
    local t={}
    for k,v in pairs(trg.keys) do
        table.insert(t,{text={{text=string.format("%-"..trg.kw.."s",tostring(v))},{gap=2,text=getStringValue(trg,v)}}})
    end
    local last_selected, last_top
    if preserve_pos then
        last_selected=self.subviews.list_main:getSelected()
        last_top=self.subviews.list_main.page_top
    end
    self.subviews.list_main:setChoices(t)
    if last_selected then
        self.subviews.list_main:setSelected(last_selected)
        self.subviews.list_main:on_scrollbar(last_top)
    else
        self.subviews.list_main:setSelected(trg.selected)
    end
    self:updateTitles()
    self.next_refresh_ms = dfhack.getTickCount() + REFRESH_MS
end
function GmEditorUi:pushTarget(target_to_push)
    local new_tbl={}
    new_tbl.target=target_to_push
    new_tbl.keys={}
    new_tbl.kw=10
    new_tbl.selected=1
    new_tbl.filter=""
    if self:currentTarget()~=nil then
        self:currentTarget().selected=self.subviews.list_main:getSelected()
        self.stack[#self.stack].filter=self.subviews.filter_input.text
    end
    for k,v in pairs(target_to_push) do
        if #tostring(k)>new_tbl.kw then new_tbl.kw=#tostring(k) end
        table.insert(new_tbl.keys,k)
    end
    new_tbl.item_count=#new_tbl.keys
    table.insert(self.stack,new_tbl)
    self.subviews.filter_input:setText("")
    self:updateTarget()
end
function GmEditorUi:popTarget()
    table.remove(self.stack) --removes last element
    if #self.stack==0 then
        self.parent_view:dismiss()
        return
    end
    self.subviews.filter_input:setText(self.stack[#self.stack].filter) --restore filter
    self:updateTarget()
end
eval_env = utils.df_shortcut_env()
function eval(s)
    local f, err = load("return " .. s, "expression", "t", eval_env)
    if err then qerror(err) end
    return f()
end
function GmEditorUi:postUpdateLayout()
    save_config({frame = self.frame})
end
function GmEditorUi:onRenderFrame(dc, rect)
    GmEditorUi.super.onRenderFrame(self, dc, rect)
    if self.parent_view.freeze then
        dc:seek(rect.x1+2, rect.y2):string(' GAME SUSPENDED ', COLOR_RED)
    end
    if self.autoupdate and self.next_refresh_ms <= dfhack.getTickCount() then
        self:updateTarget(true, true)
    end
end

FreezeScreen = defclass(FreezeScreen, gui.Screen)
FreezeScreen.ATTRS{
    focus_path='gm-editor/freeze',
}

function FreezeScreen:init()
    self:addviews{
        widgets.Panel{
            frame_background=gui.CLEAR_PEN,
            subviews={
                widgets.Label{
                    frame={t=0, l=1},
                    auto_width=true,
                    text='Dwarf Fortress is currently suspended by gui/gm-editor',
                },
                widgets.Label{
                    frame={t=0, r=1},
                    auto_width=true,
                    text='Dwarf Fortress is currently suspended by gui/gm-editor',
                },
                widgets.Label{
                    frame={},
                    auto_width=true,
                    text='Dwarf Fortress is currently suspended by gui/gm-editor',
                },
                widgets.Label{
                    frame={b=0, l=1},
                    auto_width=true,
                    text='Dwarf Fortress is currently suspended by gui/gm-editor',
                },
                widgets.Label{
                    frame={b=0, r=1},
                    auto_width=true,
                    text='Dwarf Fortress is currently suspended by gui/gm-editor',
                },
            },
        },
    }
    freeze_screen = self
end

function FreezeScreen:onDismiss()
    freeze_screen = nil
end

GmScreen = defclass(GmScreen, gui.ZScreen)
GmScreen.ATTRS {
    focus_path='gm-editor',
    freeze=false,
}

local function has_frozen_view()
    for view in pairs(views) do
        if view.freeze then
            return true
        end
    end
    return false
end

function GmScreen:init(args)
    local target = args.target
    if not target then
        qerror('Target not found')
    end
    if self.freeze then
        self.force_pause = true
        if not has_frozen_view() then
            FreezeScreen{}:show()
            -- raise existing views above the freeze screen
            for view in pairs(views) do
                view:raise()
            end
        end
    end
    self:addviews{GmEditorUi{target=target}}
    views[self] = true
end

function GmScreen:onDismiss()
    views[self] = nil
    if freeze_screen then
        if not has_frozen_view() then
            freeze_screen:dismiss()
        end
    end
end

local function get_editor(args)
    local freeze = false
    if args[1] == '-f' or args[1] == '--freeze' then
        freeze = true
        table.remove(args, 1)
    end
    if #args~=0 then
        if args[1]=="dialog" then
            dialog.showInputPrompt("Gm Editor", "Object to edit:", COLOR_GRAY,
                    "", function(entry)
                        GmScreen{freeze=freeze, target=eval(entry)}:show()
                    end)
        elseif args[1]=="free" then
            GmScreen{freeze=freeze, target=df.reinterpret_cast(df[args[2]],args[3])}:show()
        elseif args[1]=="scr" then
            -- this will not work for more complicated expressions, like scr.fieldname, but
            -- it should capture the most common case
            GmScreen{freeze=freeze, target=dfhack.gui.getDFViewscreen(true)}:show()
        else
            GmScreen{freeze=freeze, target=eval(args[1])}:show()
        end
    else
        GmScreen{freeze=freeze, target=getTargetFromScreens()}:show()
    end
end

views = views or {}
freeze_screen = freeze_screen or nil

if dfhack_flags.module then
    return
end

get_editor{...}
