-- Stuff used by dfusion
local _ENV = mkmodule('plugins.dfusion')

local ms=require("memscan")

local marker={0xDE,0xAD,0xBE,0xEF}
patches={}
-- A reversable binary patch
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
    self.max_val=0
    for k,v in pairs(args.pre_data) do
        if type(k)~="number" then
            error("non number key in pre_data")
        end
        if self.max_val<k then
            self.max_val=k
        end
    end
    for k,v in pairs(args.data) do
        if type(k)~="number" then
            error("non number key in data")
        end
        if args.pre_data[k]==nil then
            error("can't edit without revert data")
        end
    end
end
function BinaryPatch:postinit(args)
    patches[args.name]=self
end
function BinaryPatch:apply()
    local arr=ms.CheckedArray.new('uint8_t',self.address,self.address+self.max_val)
    for k,v in pairs(self.pre_data) do
        if arr[k-1]~=v then
            error(string.format("pre-data does not match expected:%x got:%x",v,arr[k-1]))
        end
    end
    
    local post_buf=df.new('uint8_t',self.max_val)
    for k,v in pairs(self.pre_data) do
        if self.data[k]==nil then
            post_buf[k-1]=v
        else
            post_buf[k-1]=self.data[k]
        end
    end
    local ret=dfhack.with_finalize(function() post_buf:delete() end,dfhack.internal.patchMemory,self.address,post_buf,self.max_val)
    if not ret then
        error("Patch application failed!")
    end
    self.is_applied=true
    --[[
    for k,v in pairs(self.data) do
        arr[k]=v
    end
    ]]--
end
function BinaryPatch:remove(delete)
    if delete==nil then
        delete=true
    end
    if not self.is_applied then
        error("can't remove BinaryPatch, not applied.")
    end
    local arr=ms.CheckedArray.new('uint8_t',self.address,self.address+self.max_val)
    
    local post_buf=df.new('uint8_t',self.max_val)
    for k,v in pairs(self.pre_data) do
            post_buf[k-1]=v
    end
    local ret=dfhack.with_finalize(function() post_buf:delete() end,dfhack.internal.patchMemory,self.address,post_buf,self.max_val)
    if not ret then
        error("Patch remove failed!")
    end
    self.is_applied=false
    if delete then
        patches[self.name]=nil
    end
    
    --[[
    for k,v in pairs(self.data) do
        arr[k]=self.pre_data[k]
    end --]]
end

function BinaryPatch:__gc()
    if self.is_applied then
        self:remove()
    end
end
-- A binary hack (obj file) loader/manager
-- has to have: a way to get offsets for marked areas (for post load modification) or some way to do that pre-load
-- page managing (including excecute/write flags for DEP and the like)
plugins=plugins or {}
BinaryPlugin=defclass(BinaryPlugin)
BinaryPlugin.ATTRS {filename=DEFAULT_NIL,reloc_table={},name=DEFAULT_NIL}
function BinaryPlugin:init(args)
    if args.name==nil then error("Not a valid plugin name!") end
    if plugins[args.name]==nil then
        plugins[args.name]=true
    else
        error("Trying to create a same plugin")
    end
    self.allocated_object={}
    
end
function BinaryPlugin:postinit(args)
    self:load()
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
return _ENV