-- Stuff used by dfusion
local _ENV = mkmodule('plugins.dfusion')

local ms=require("memscan")

local marker={0xDE,0xAD,0xBE,0xEF}
--utility functions
function dwordToTable(dword)
    local b={bit32.extract(dword,0,8),bit32.extract(dword,8,8),bit32.extract(dword,16,8),bit32.extract(dword,24,8)}
    return b
end
function concatTables(t1,t2)
    for k,v in pairs(t2) do
        table.insert(t1,v)
    end
end
function makeCall(from,to)
    local ret={0xe8}
    concatTables(ret,dwordToTable(to-from-5))
    return ret
end
-- A reversable binary patch
patches={}
BinaryPatch=defclass(BinaryPatch)
BinaryPatch.ATTRS {pre_data=DEFAULT_NIL,data=DEFAULT_NIL,address=DEFAULT_NIL,name=DEFAULT_NIL}
function BinaryPatch:init(args)
    self.is_applied=false
    if args.pre_data==nil or args.data==nil or args.address==nil or args.name==nil then
        error("Invalid parameters to binary patch")
    end
    if patches[self.name]~=nil then
        error("Patch already exist")
    end

    for k,v in ipairs(args.data) do
        if args.pre_data[k]==nil then
            error("can't edit without revert data")
        end
    end
end
function BinaryPatch:postinit(args)
    patches[args.name]=self
end
function BinaryPatch:test()
    local arr=ms.CheckedArray.new('uint8_t',self.address,self.address+#self.pre_data)
    for k,v in ipairs(self.pre_data) do
        if arr[k-1]~=v then
            return false
        end
    end
    return true
end
function BinaryPatch:apply()
    if not self:test() then
        error(string.format("pre-data for binary patch does not match expected"))
    end
    
    local post_buf=df.new('uint8_t',#self.pre_data)
    for k,v in ipairs(self.pre_data) do
        if self.data[k]==nil then
            post_buf[k-1]=v
        else
            post_buf[k-1]=self.data[k]
        end
    end
    local ret=dfhack.with_finalize(function() post_buf:delete() end,dfhack.internal.patchMemory,self.address,post_buf,#self.pre_data)
    if not ret then
        error("Patch application failed!")
    end
    self.is_applied=true
end
function BinaryPatch:repatch(newdata)
    if newdata==nil then newdata=self.data end
    self:remove()
    self.data=newdata
    self:apply()
end
function BinaryPatch:remove(delete)
    if delete==nil then
        delete=true
    end
    if not self.is_applied then
        error("can't remove BinaryPatch, not applied.")
    end
    local arr=ms.CheckedArray.new('uint8_t',self.address,self.address+#self.pre_data)
    
    local post_buf=df.new('uint8_t',#self.pre_data)
    for k,v in pairs(self.pre_data) do
            post_buf[k-1]=v
    end
    local ret=dfhack.with_finalize(function() post_buf:delete() end,dfhack.internal.patchMemory,self.address,post_buf,#self.pre_data)
    if not ret then
        error("Patch remove failed!")
    end
    self.is_applied=false
    if delete then
        patches[self.name]=nil
    end

end

function BinaryPatch:__gc()
    if self.is_applied then
        self:remove()
    end
end
-- A binary hack (obj file) loader/manager
-- has to have: a way to get offsets for marked areas (for post load modification) or some way to do that pre-load
-- page managing (including excecute/write flags for DEP and the like)
-- TODO plugin state enum, a way to modify post install (could include repatching code...)
plugins=plugins or {}
BinaryPlugin=defclass(BinaryPlugin)
BinaryPlugin.ATTRS {filename=DEFAULT_NIL,reloc_table={},name=DEFAULT_NIL}
function BinaryPlugin:init(args)
    
end
function BinaryPlugin:postinit(args)
    if self.name==nil then error("Not a valid plugin name!") end
    if plugins[args.name]==nil then
        plugins[self.name]=self
    else
        error("Trying to create a same plugin")
    end
    self.allocated_object={}
    self:load()
end
function BinaryPlugin:get_or_alloc(name,typename,arrsize)
    if self.allocated_object[name]~=nil then
        return self.allocated_object[name]
    else
        return self:allocate(name,typename,arrsize)
    end
end
function BinaryPlugin:allocate(name,typename,arrsize)
    local trg
    if df[typename]==nil then
        trg=df.new(typename,arrsize)
        self.allocated_object[name]=trg
    else
        trg=df[typename]:new(arrsize)
        self.allocated_object[name]=trg
    end
    return trg
end
function BinaryPlugin:load()
    local obj=loadObjectFile(self.filename)
    self.data=df.reinterpret_cast("uint8_t",obj.data)
    self.size=obj.data_size
    for _,v in pairs(obj.symbols) do
        if string.sub(v.name,1,5)=="mark_" then
            local new_pos=self:find_marker(v.pos)
            self.reloc_table[string.sub(v.name,6)]=new_pos
        end
    end
end
function BinaryPlugin:find_marker(start_pos)
    local matched=0
    for i=start_pos,self.size do
        if self.data[i]==marker[4-matched] then
            matched=matched+1
            if matched == 4 then
                return i-4
            end
        end
    end
end

function BinaryPlugin:set_marker_dword(marker,dword) -- i hope Toady does not make a 64bit version...
    if self.reloc_table[marker]==nil then
        error("marker ".. marker.. " not found")
    end
    local b=dwordToTable(dword)
    local off=self.reloc_table[marker]
    for k,v in ipairs(b) do
        self.data[off+k]=b[k]
    end
end
function BinaryPlugin:move_to_df()
    local _,addr=df.sizeof(self.data)
    markAsExecutable(addr)
    return addr
end
function BinaryPlugin:print_data()
    local out=""
    for i=0,self.size do
        out=out..string.format(" %02x",self.data[i])
        if math.modf(i,16)==15 then
            print(out)
            out=""
        end
    end
    print(out)
end
function BinaryPlugin:status()
    return "invalid, base class only!"
end
function BinaryPlugin:__gc()
    for k,v in pairs(self.allocated_object) do
        df.delete(v)
    end
    if self.unload then
        self:unload()
    end
    self.data:delete()
end
-- a Menu for some stuff. Maybe add a posibility of it working as a gui, or a gui adaptor?
-- Todo add hints, and parse them to make a "smart" choice of parameters to pass
SimpleMenu=defclass(SimpleMenu)
SimpleMenu.ATTRS{title=DEFAULT_NIL}
function SimpleMenu:init(args)
    self.items={}
end
function SimpleMenu:add(name,entry,hints)
	table.insert(self.items,{entry,name,hints})
end
function SimpleMenu:display()
	print("Select choice (q exits):")
	for p,c in pairs(self.items) do
		print(string.format("%3d).%s",p,c[2]))
	end
	local ans
	repeat
		local r
		r=dfhack.lineedit()
		if r==nil then return end
		if r=='q' then return end
		ans=tonumber(r)
		
		if ans==nil or not(ans<=#self.items and ans>0) then
			print("Invalid choice.")
		end
		
	until ans~=nil and (ans<=#self.items and ans>0)
    if type(self.items[ans][1])=="function" then
        self.items[ans][1]()
    else
        self.items[ans][1]:display()
    end
end

return _ENV