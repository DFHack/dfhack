-- a graphical mod manager for df
local gui=require 'gui'
local widgets=require 'gui.widgets'

local entity_file=dfhack.getDFPath().."/raw/objects/entity_default.txt"
local init_file=dfhack.getDFPath().."/raw/init.lua"
local mod_dir=dfhack.getDFPath().."/hack/mods"
--[[ mod format: lua script that defines:
    name - a name that is displayed in list
    author - mod author, also displayed
    description - mod description
    OPTIONAL:
    raws_list - a list (table) of file names that need to be copied over to df raws
    patch_entity - a chunk of text to patch entity TODO: add settings to which entities to add
    patch_init - a chunk of lua to add to lua init
    patch_dofile - a list (table) of files to add to lua init as "dofile"
    patch_files - a table of files to patch:
        filename - a filename (in raws folder) to patch
        patch - what to add
        after - a string after which to insert
    MORE OPTIONAL:
    guard - a token that is used in raw files to find editions and remove them on uninstall
    guard_init - a token for lua file
    [pre|post]_(un)install - callback functions. Can trigger more complicated behavior
]]

function fileExists(filename)
	local file=io.open(filename,"rb")
	if file==nil then
		return
	else
		file:close()
		return true
	end
end
if not fileExists(init_file) then
    local initFile=io.open(initFileName,"a")
    initFile:close()
end
function copyFile(from,to) --oh so primitive
    local filefrom=io.open(from,"rb")
    local fileto=io.open(to,"w+b")
    local buf=filefrom:read("*a")
    printall(buf)
    fileto:write(buf)
    filefrom:close()
    fileto:close()
end
function patchInit(initFileName,patch_guard,code)
	local initFile=io.open(initFileName,"a")
	initFile:write(string.format("\n%s\n%s\n%s",patch_guard[1],
		code,patch_guard[2]))
	initFile:close()
end
function patchDofile( initFileName,patch_guard,dofile_list )
    local initFile=io.open(initFileName,"a")
    initFile:write(patch_guard[1].."\n")
    for _,v in ipairs(dofile_list) do
        local fixed_path=mod_dir:gsub("\\","/")
        initFile:write(string.format("dofile('%s/%s')\n",fixed_path,v))
    end
    initFile:write(patch_guard[2].."\n")
    initFile:close()
end
function patchFile(file_name,patch_guard,after_string,code)
    local input_lines=patch_guard[1].."\n"..code.."\n"..patch_guard[2]
    
    local badchars="[%:%[%]]"
    local find_string=after_string:gsub(badchars,"%%%1") --escape some bad chars
	
    local entityFile=io.open(file_name,"r")
    local buf=entityFile:read("*all")
    entityFile:close()
    local entityFile=io.open(file_name,"w+")
    buf=string.gsub(buf,find_string,after_string.."\n"..input_lines)
    entityFile:write(buf)
    entityFile:close()
end
function findGuards(str,start,patch_guard)
	local pStart=string.find(str,patch_guard[1],start)
	if pStart==nil then return nil end
	local pEnd=string.find(str,patch_guard[2],pStart)
	if pEnd==nil then error("Start guard token found, but end was not found") end
	return pStart-1,pEnd+#patch_guard[2]+1
end
function findGuardsFile(filename,patch_guard)
	local file=io.open(filename,"r")
	local buf=file:read("*all")
	return findGuards(buf,1,patch_guard)
end
function unPatchFile(filename,patch_guard)
	local file=io.open(filename,"r")
	local buf=file:read("*all")
	file:close()
	
	local newBuf=""
	local pos=1
	local lastPos=1
	repeat 
		local endPos
		pos,endPos=findGuards(buf,lastPos,patch_guard)
		newBuf=newBuf..string.sub(buf,lastPos,pos)
		if endPos~=nil then
			lastPos=endPos
		end
	until pos==nil
	
	local file=io.open(filename,"w+")
	file:write(newBuf)
    file:close()
end
function checkInstalled(dfMod) --try to figure out if installed
	if dfMod.checkInstalled then
		return dfMod.checkInstalled()
	else
		if dfMod.raws_list then
			for k,v in pairs(dfMod.raws_list) do
				if fileExists(dfhack.getDFPath().."/raw/objects/"..v) then
					return true,v
				end
			end
		end
		if dfMod.patch_entity then
			if findGuardsFile(entity_file,dfMod.guard)~=nil then
				return true,"entity_default.txt"
			end
		end
        if dfMod.patch_files then
            for k,v in pairs(dfMod.patch_files) do
                if findGuardsFile(dfhack.getDFPath().."/raw/objects/"..v.filename,dfMod.guard)~=nil then
                    return true,"v.filename"
                end
            end
		end
		if dfMod.patch_init then
			if findGuardsFile(init_file,dfMod.guard_init)~=nil then
				return true,"init.lua"
			end
		end
	end
end
manager=defclass(manager,gui.FramedScreen)

function manager:init(args)
    self.mods={}
    local mods=self.mods
	local mlist=dfhack.internal.getDir(mod_dir)

    if #mlist==0 then
        qerror("Mod directory not found! Are you sure it is in:"..mod_dir)
    end
	for k,v in ipairs(mlist) do
		if v~="." and v~=".." then
			local f,modData=pcall(dofile,mod_dir.."/".. v .. "/init.lua")
            if f then
                mods[modData.name]=modData
                modData.guard=modData.guard or {">>"..modData.name.." patch","<<End "..modData.name.." patch"}
                modData.guard_init={"--"..modData.guard[1],"--"..modData.guard[2]}
                modData.path=mod_dir.."/"..v..'/'
            end
		end
	end
    ---show thingy
    local modList={}
    for k,v in pairs(self.mods) do 
        table.insert(modList,{text=k,data=v}) 
    end
    
    self:addviews{
        
        
        widgets.Panel{subviews={
            widgets.Label{
                text="Info:",
                frame={t=1,l=1}
            },
            widgets.Label{
                text="<no-info>",
                --text={text=self:callback("formDescription")},
                view_id='info',
                frame={t=2,l=1},
            },
            widgets.Label{
                text={"Author:",{text=self:callback("formAuthor")}},
                view_id='author',
                frame={b=5,l=1}
            },
            widgets.Label{
                text={
                {text="Install",key="CUSTOM_I",key_sep="()",disabled=self:callback("curModInstalled"),
                    on_activate=self:callback("installCurrent")},NEWLINE,
                {text="Uninstall",key="CUSTOM_U",key_sep="()",enabled=self:callback("curModInstalled"),
                    on_activate=self:callback("uninstallCurrent")},NEWLINE,
                {text="Settings",key="CUSTOM_S",key_sep="()",enabled=self:callback("hasSettings")},NEWLINE,
                {text="Exit",key="LEAVESCREEN",key_sep="()",},NEWLINE
                },
                frame={l=1,b=0}
            },
        },
        frame={l=21,t=1,b=1}
        },
        widgets.Panel{subviews={
            widgets.Label{
                text="Mods:",
                frame={t=1,l=1}
            },
            widgets.List{
                choices=modList,
                frame={t=2,l=1},
                on_select=self:callback("selectMod")
            },
            },
            frame={w=20,t=1,l=1,b=1}
        },
    }
    self:updateState()
    
end
function manager:postinit(args)
    self:selectMod(1,{data=self.selected})-- workaround for first call, now the subviews are constructed
end
function manager:curModInstalled()
    return self.selected.installed
end
function manager:hasSettings()
    return self.selected.settings -- somehow add the entity selection as a default, if it mods entities
end
function manager:formDescription()
    local ret={}
    if self.selected.description then
        return self.selected.description
        --[[
        local str=require('utils').split_string(self.selected.description,"\n")
        for _,s in ipairs(str) do
            table.insert(ret,{text=s})
            table.insert(ret,NEWLINE)
        end
        return ret]]
    else
        return "<no-info>"
    end
end
function manager:formAuthor()
    return self.selected.author or "<no-info>"
end
function manager:selectMod(idx,choice)
    self.selected=choice.data
    if self.subviews.info then
        self.subviews.info:setText(self:formDescription())
        self:updateLayout()
    end
end
function manager:updateState()
    for k,v in pairs(self.mods) do
        v.installed=checkInstalled(v)
    end
end
function manager:installCurrent()
    self:install(self.selected)
end
function manager:uninstallCurrent()
    self:uninstall(self.selected)
end
function manager:install(trgMod,force)
    
    if trgMod==nil then
        qerror 'Mod does not exist'
    end
    if not force then
        local isInstalled,file=checkInstalled(trgMod) -- maybe load from .installed?
        if isInstalled then
            qerror("Mod already installed. File:"..file)
		end
    end
    print("installing:"..trgMod.name)
    if trgMod.pre_install then
        trgMod.pre_install(args)
    end
	if trgMod.raws_list then
		for k,v in pairs(trgMod.raws_list) do
			copyFile(trgMod.path..v,dfhack.getDFPath().."/raw/objects/"..v)
		end
	end
	if trgMod.patch_entity then
		local entity_target="[ENTITY:MOUNTAIN]" --TODO configure
		patchFile(entity_file,trgMod.guard,entity_target,trgMod.patch_entity)
	end
    if trgMod.patch_files then
        for k,v in pairs(trgMod.patch_files) do
            patchFile(dfhack.getDFPath().."/raw/objects/"..v.filename,trgMod.guard,v.after,v.patch)
        end
    end
	if trgMod.patch_init then
		patchInit(init_file,trgMod.guard_init,trgMod.patch_init)
	end
    if trgMod.patch_dofile then
        patchDofile(init_file,trgMod.guard_init,trgMod.patch_dofile)
    end
    trgMod.installed=true
    
    if trgMod.post_install then
        trgMod.post_install(self)
    end
    print("done")
end
function manager:uninstall(trgMod)
    print("Uninstalling:"..trgMod.name)
	if trgMod.pre_uninstall then
        trgMod.pre_uninstall(args)
    end
    
	if trgMod.raws_list then
		for k,v in pairs(trgMod.raws_list) do
			os.remove(dfhack.getDFPath().."/raw/objects/"..v)
		end
	end
	if trgMod.patch_entity then
		unPatchFile(entity_file,trgMod.guard)
	end
    if trgMod.patch_files then
        for k,v in pairs(trgMod.patch_files) do
            unPatchFile(dfhack.getDFPath().."/raw/objects/"..v.filename,trgMod.guard)
        end
    end
	if trgMod.patch_init or trgMod.patch_dofile then
		unPatchFile(init_file,trgMod.guard_init)
	end

    trgMod.installed=false
	if trgMod.post_uninstall then
        trgMod.post_uninstall(args)
    end
    print("done")
end
function manager:onInput(keys)

    if keys.LEAVESCREEN  then
        self:dismiss()
    else
        self:inputToSubviews(keys)
    end
    
end
if dfhack.gui.getCurFocus()~='title' then
    qerror("Can only be used in title screen")
end
local m=manager{}
m:show()
