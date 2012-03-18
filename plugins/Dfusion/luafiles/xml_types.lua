--<angavrilov> otherwise you just maintain alignment granularity in addition to size for all fields,
--  round up current offset to field alignment, 
--	assign structs the max alignment of any field, and round up struct size to its alignment
function type_read(valtype,address)
	if valtype.issimple then
		--print("Simple read:"..tostring(valtype.ctype))
		return engine.peek(address,valtype.ctype)
	else
		return valtype:makewrap(address)
	end
end
function type_write(valtype,address,val)
	if valtype.issimple then
		engine.poke(address,valtype.ctype,val)
	else
		engine.poke(address,DWORD,rawget(val,"ptr"))
	end
end
function first_of_type(node,labelname)
	for k,v in ipairs(node) do
		if type(v)=="table" and v.label==labelname then
			return v
		end
	end
end
xtypes={} -- list of all types prototypes (e.g. enum-type -> announcement_type)
-- type must have size, new and makewrap (that makes a wrapper around ptr)
if WINDOWS then
	dofile("dfusion/xml_types_windows.lua")
elseif LINUX then
	dofile("dfusion/xml_types_linux.lua")
end

function padAddress(curoff,typetoadd) --return new offset to place things... Maybe linux is different?
	--windows -> sizeof(x)==alignof(x)
	--[=[
	
	if sizetoadd>8 then sizetoadd=8 end
	if(math.mod(curoff,sizetoadd)==0) then 
		return curoff
	else
		return curoff+(sizetoadd-math.mod(curoff,sizetoadd))
	end
	--]=]
	if typetoadd==nil or typetoadd.__align==nil then
		return curoff
	else
		
		if(math.mod(curoff,typetoadd.__align)==0) then 
			return curoff
		else
			if PRINT_PADS then
				print("padding off:"..curoff.." with align:"..typetoadd.__align.." pad="..(typetoadd.__align-math.mod(curoff,typetoadd.__align)))
			end
			return curoff+(typetoadd.__align-math.mod(curoff,typetoadd.__align))
		end
	end
end


local sarr={} 
sarr.__index=sarr
function sarr.new(node,obj)
	local o=obj or {}
	setmetatable(o,sarr)
	--print("Making array.")
	o.count=tonumber(node.xarg.count)
	--print("got count:"..o.count)
	o.item_type=makeType(first_of_type(node,"ld:item"))
	o.__align=o.item_type.__align or 4
	o.size=o.count*o.item_type.size
	--print("got subtypesize:"..o.item_type.size)
	return o
end
function sarr:makewrap(address)
	local o={}
	o.mtype=self
	o.ptr=address
	setmetatable(o,self.wrap)
	return o
end
sarr.wrap={}
function sarr.wrap:__index(key)
	if key=="size" then
		return rawget(self,"mtype").count
	end
	local num=tonumber(key)
	local mtype=rawget(self,"mtype")
	if num~=nil and num<mtype.count then
		return type_read(mtype.item_type,num*mtype.item_type.size+rawget(self,"ptr"))
	else
		error("invalid key to static-array")
	end
end
function sarr.wrap:__newindex(key,val)
	local num=tonumber(key)
	if num~=nil and num<rawget(self,"mtype").count then
		return type_write(mtype.item_type,num*mtype.item_type.size+rawget(self,"ptr"),val)
	else
		error("invalid key to static-array")
	end
end
--]=]
xtypes["static-array"]=sarr


simpletypes={}

simpletypes["s-float"]={FLOAT,4,4}
simpletypes.int64_t={QWORD,8,8}
simpletypes.uint32_t={DWORD,4,4}
simpletypes.uint16_t={WORD,2,2}
simpletypes.uint8_t={BYTE,1,1}
simpletypes.int32_t={DWORD,4,4}
simpletypes.int16_t={WORD,2,2}
simpletypes.int8_t={BYTE,1,1}
simpletypes.bool={BYTE,1,1}
simpletypes["stl-string"]={STD_STRING,28,4}
function getSimpleType(typename,obj)
	if simpletypes[typename] == nil then return end
	local o=obj or {}
	o.name=typename
	o.ctype=simpletypes[typename][1]
	o.size=simpletypes[typename][2]
	o.__align=simpletypes[typename][3]
	o.issimple=true
	return o
end
local type_enum={}
type_enum.__index=type_enum
function type_enum.new(node,obj)
	local o=obj or {}
	setmetatable(o,type_enum)
	o.names={}
	for k,v in pairs(node) do
		if v.label=="enum-item" then
			if  v.xarg~=nil and v.xarg.name then
				--print("\t"..k.." "..v.xarg.name)
				o.names[k-1]=v.xarg.name
			else
				o.names[k-1]=string.format("unk_%d",k-1)
			end
		end
	end
	local btype=node.xarg["base-type"] or "uint32_t"
	--print(btype.." t="..convertType(btype))
	o:setbtype(btype)
	return o
end
function type_enum:setbtype(btype)
	self.etype=getSimpleType(btype) -- should be simple type
	self.size=self.etype.size
	self.__align=self.etype.__align
end
function type_enum:makewrap(address)
	local o={}
	o.mtype=self
	o.ptr=address
	setmetatable(o,self.wrap)
	return o
end
type_enum.wrap={}
type_enum.wrap.__index=type_enum.wrap
type_enum.wrap.set=function (tbl,val)
	local mtype=rawget(tbl,"mtype")
	local ptr=rawget(tbl,"ptr")
	type_write(mtype.etype,ptr,val)
end
type_enum.wrap.get=function (tbl)
	local mtype=rawget(tbl,"mtype")
	local ptr=rawget(tbl,"ptr")
	return type_read(mtype.etype,ptr)
end
xtypes["enum-type"]=type_enum

local type_bitfield={} --bitfield can be accessed by number (bf[1]=true) or by name bf.DO_MEGA=true
type_bitfield.__index=type_bitfield
function type_bitfield.new(node,obj)
	local o=obj or {}
	setmetatable(o,type_bitfield)
	o.size=0
	o.fields_named={}
	o.fields_numed={}
	o.__align=1
	for k,v in pairs(node) do
		if type(v)=="table" and v.xarg~=nil then
			
			local name=v.xarg.name
			if name==nil then
				name="anon_"..tostring(k)
			end
			--print("\t"..k.." "..name)
			o.fields_numed[k]=name
			o.fields_named[name]=k
			o.size=o.size+1
		end
	end
	--o.size=o.size/8 -- size in bytes, not bits.
	o.size=4
	o.size=math.ceil(o.size)
	--[=[if math.mod(o.size,o.__align) ~= 0 then
		o.size=o.size+ (o.__align-math.mod(o.size,o.__align))
	end]=]
	return o
end
function type_bitfield:bitread(addr,nbit)
	local byte=engine.peekb(addr+nbit/8)
	if bit.band(byte,bit.lshift(1,nbit%8))~=0 then
		return true
	else
		return false
	end
end
function type_bitfield:bitwrite(addr,nbit,value)
	local byte=engine.peekb(addr+nbit/8)
	if self:bitread(addr,nbit)~= value then
		local byte=bit.bxor(byte,bit.lshift(1,nbit%8))
		engine.pokeb(addr+nbit/8,byte)
	end
end
type_bitfield.wrap={}

function type_bitfield:makewrap(address)
	local o={}
	o.mtype=self
	o.ptr=address
	setmetatable(o,self.wrap)
	return o
end
function type_bitfield.wrap.__next(tbl,key)
	
	local kk,vv=next(rawget(tbl,"mtype").fields_named,key)
	if kk ~= nil then
		return kk,tbl[kk]
	else
		return nil
	end
end
function type_bitfield.wrap:__index(key)
	local myptr=rawget(self,"ptr")
	local mytype=rawget(self,"mtype")
	local num=tonumber(key)
	if num~=nil then
		if mytype.fields_numed[num]~=nil then
			return mytype:bitread(myptr,num-1)
		else
			error("No bit with index:"..tostring(num))
		end
	elseif mytype.fields_named[key]~= nil then
		return mytype:bitread(myptr,mytype.fields_named[key]-1)
	else
		error("No such field exists")
	end
end
function type_bitfield.wrap:__newindex(key,value)
	local myptr=rawget(self,"ptr")
	local mytype=rawget(self,"mtype")
	local num=tonumber(key)
	if num~=nil then
		if mytype.fields_numed[num]~=nil then
			return mytype:bitwrite(myptr,num-1,value)
		else
			error("No bit with index:"..tostring(num))
		end
	elseif mytype.fields_named[key]~= nil then
		return mytype:bitwrite(myptr,mytype.fields_named[key]-1,value)
	else
		error("No such field exists")
	end
end
xtypes["bitfield-type"]=type_bitfield


local type_class={}
type_class.__index=type_class
function type_class.new(node,obj)
	local o=obj or {}
	setmetatable(o,type_class)
	o.types={}
	o.base={}
	o.size=0
	local isunion=false
	if node.xarg["is-union"]=="true" then
		--error("unions not supported!")
		isunion=true
	end
	local firsttype;
	--o.baseoffset=0
	if node.xarg["inherits-from"]~=nil then
		table.insert(o.base,getGlobal(node.xarg["inherits-from"]))
		--error("Base class:"..node.xarg["inherits-from"])
	end
	
	for k,v in ipairs(o.base) do
		for kk,vv in pairs(v.types) do
			o.types[kk]={vv[1],vv[2]+o.size}
		end
		o.size=o.size+v.size
	end
	--o.baseoffset=o.size;
	--o.size=0
	for k,v in pairs(node) do
		if type(v)=="table" and v.label=="ld:field" and v.xarg~=nil then
			local t_name=""
			local name=v.xarg.name or v.xarg["anon-name"] or ("unk_"..k)
				
			--print("\t"..k.." "..name.."->"..v.xarg.meta.." ttype:"..v.label)
			local ttype=makeType(v)
			if ttype.size==0 then error("Field with 0 size! type="..node.xarg["type-name"].."."..name) end
			if ttype.size-math.ceil(ttype.size) ~= 0 then error("Field with real offset! type="..node.xarg["type-name"].."."..name) end
			--for k,v in pairs(ttype) do
			--	print(k..tostring(v))
			--end
			
			local off=padAddress(o.size,ttype)
			--[=[if PRINT_PADS then
				
				if ttype.__align then
					print(name.." "..ttype.__align .. " off:"..off.." "..math.mod(off,ttype.__align))
					
				end
				if off~=o.size then
					print("var:"..name.." size:"..ttype.size)
				end
			end]=]
			--print("var:"..name.." ->"..tostring(off).. " :"..ttype.size)
			if isunion then
				if ttype.size > o.size then
					o.size=ttype.size
				end
				o.types[name]={ttype,0}
			else
			o.size=off
			o.types[name]={ttype,o.size}--+o.baseoffset
			o.size=o.size+ttype.size
			end
			if firsttype== nil then
				firsttype=o.types[name][1]
			end
			
		end
	end
	if isunion then
		o.__align=0
		for k,v in pairs(o.types) do
			if v[1].__align~= nil and v[1].__align>o.__align then
				o.__align=v[1].__align
			end
		end
	else
		if o.base[1]~= nil then
			o.__align=o.base[1].__align
		elseif firsttype~= nil then
			o.__align=firsttype.__align
			--if o.__align~=nil then
				--print("\t\t setting align to:"..(o.__align or ""))
			--else
				--o.__align=4
				--print("\t\t NIL ALIGNMENT!")
			--end
		end
	end

	return o
end

type_class.wrap={}
function type_class.wrap:__address(key)
	local myptr=rawget(self,"ptr")
	local mytype=rawget(self,"mtype")
	if mytype.types[key] ~= nil then
		return myptr+mytype.types[key][2]
	else
		error("No such field exists")
	end
end
function type_class.wrap:__index(key)
	local myptr=rawget(self,"ptr")
	local mytype=rawget(self,"mtype")
	if mytype.types[key] ~= nil then
		return type_read(mytype.types[key][1],myptr+mytype.types[key][2])
	else
		error("No such field exists")
	end
end
function type_class.wrap:__newindex(key,value)
	local myptr=rawget(self,"ptr")
	local mytype=rawget(self,"mtype")
	if mytype.types[key] ~= nil then
		return type_write(mytype.types[key][1],myptr+mytype.types[key][2],value)
	else
		error("No such field exists")
	end
end
function type_class:makewrap(ptr)
	local o={}
	o.ptr=ptr
	o.mtype=self
	setmetatable(o,self.wrap)
	return o
end
xtypes["struct-type"]=type_class
xtypes["class-type"]=type_class
local type_pointer={}
type_pointer.__index=type_pointer
function type_pointer.new(node,obj)
	local o=obj or {}
	setmetatable(o,type_pointer)
	local subnode=first_of_type(node,"ld:item")
	if subnode~=nil then
	o.ptype=makeType(subnode,nil,true)
	end
	
	o.size=4
	o.__align=4
	return o
end
type_pointer.wrap={}
type_pointer.wrap.__index=type_pointer.wrap
function type_pointer.wrap:tonumber()
	local myptr=rawget(self,"ptr")
	return engine.peekd(myptr)--type_read(DWORD,myptr)
end
function type_pointer.wrap:__setup(trg)
	if trg~= nil then
		self:fromnumber(trg)
	else
		self:fromnumber(0)
	end
end
function type_pointer.wrap:fromnumber(num)
	local myptr=rawget(self,"ptr")
	return engine.poked(myptr,num)--type_write(DWORD,myptr,num)
end
function type_pointer.wrap:deref()
	local myptr=rawget(self,"ptr")
	local mytype=rawget(self,"mtype")
	return type_read(mytype.ptype,engine.peekd(myptr))
end
function type_pointer.wrap:setref(val)
	local myptr=rawget(self,"ptr")
	local mytype=rawget(self,"mtype")
	return type_write(mytype.ptype,engine.peekd(myptr),val)
end
function type_pointer.wrap:newref(val)
	local myptr=rawget(self,"ptr")
	local mytype=rawget(self,"mtype")
	local ptr=engine.alloc(mytype.ptype.size)
	self:fromnumber(ptr)
	return ptr
end
function type_pointer:makewrap(ptr)
	local o={}
	o.ptr=ptr
	o.mtype=self
	setmetatable(o,self.wrap)
	return o
end
xtypes["pointer"]=type_pointer
--------------------------------------------
--stl-vector (beginptr,endptr,allocptr)
--df-flagarray (ptr,size)
xtypes.containers=xtypes.containers or {}
local dfarr={} 
dfarr.__index=dfarr
function dfarr.new(node,obj)
	local o=obj or {}
	setmetatable(o,dfarr)
	o.size=8
	o.__align=4
	o.item_type=makeType(first_of_type(node,"ld:item"))
	return o
end
function dfarr:makewrap(address)
	local o={}
	o.mtype=self
	o.ptr=address
	setmetatable(o,self.wrap)
	return o
end
dfarr.wrap={}
function dfarr.wrap:__setup(size)
	local mtype=rawget(self,"mtype")
	engine.pokew(rawget(self,"ptr")+4,size)
	local newptr=engine.alloc(size*mtype.item_type.size)
	engine.poked(rawget(self,"ptr"),newptr)
end
function dfarr.wrap:__index(key)
	local num=tonumber(key)
	local mtype=rawget(self,"mtype")
	local size=engine.peekw(rawget(self,"ptr")+4)
	if key=="size" then
		return size
	end
	local item_start=engine.peekd(rawget(self,"ptr"))
	if num~=nil and num<sizethen then
		return type_read(mtype.item_type,num*mtype.item_type.size+item_start)
	else
		error("invalid key to df-array")
	end
end
function dfarr.wrap:__newindex(key,val)
	local num=tonumber(key)
	local size=engine.peekw(rawget(self,"ptr")+4)
	local item_start=engine.peekd(rawget(self,"ptr"))
	if num~=nil and num<size then
		return type_write(mtype.item_type,num*mtype.item_type.size+item_start,val)
	else
		error("invalid key to df-array")
	end
end

xtypes.containers["df-array"]=dfarr
local farr={} 
farr.__index=farr
function farr.new(node,obj)
	local o=obj or {}
	setmetatable(o,farr)
	o.size=8
	o.__align=4
	if node.xarg["index-enum"]~= nil then
		o.index=getGlobal(node.xarg["index-enum"],true)
	end
	return o
end
function farr:makewrap(address)
	local o={}
	o.mtype=self
	o.ptr=address
	setmetatable(o,self.wrap)
	return o
end
function farr:bitread(addr,nbit)
	local byte=engine.peekb(addr+nbit/8)
	if bit.band(byte,bit.lshift(1,nbit%8))~=0 then
		return true
	else
		return false
	end
end
function farr:bitwrite(addr,nbit,value)
	local byte=engine.peekb(addr+nbit/8)
	if self:bitread(addr,nbit)~= value then
		local byte=bit.bxor(byte,bit.lshift(1,nbit%8))
		engine.pokeb(addr+nbit/8,byte)
	end
end
type_bitfield.wrap={}

function type_bitfield:makewrap(address)
	local o={}
	o.mtype=self
	o.ptr=address
	setmetatable(o,self.wrap)
	return o
end

farr.wrap={}
function farr.wrap.__next(tbl,key)
	error("TODO")
end
function farr.wrap:__index(key)
	
	local num=tonumber(key)
	local mtype=rawget(self,"mtype")
	local size=engine.peekd(rawget(self,"ptr")+4)*8
	if key=="size" then
		return size;
	end
	if mtype.index~=nil and num==nil then
		--print("Requested:"..key)
		for k,v in pairs(mtype.index.names) do
			if v==key then
				num=k
				break
			end
		end
			--print("is key:"..num)

	end
	if num~=nil and num<size then
		return mtype:bitread(rawget(self,"ptr"),num)
	else
		error("invalid key to df-flagarray")
	end
end
function farr.wrap:__newindex(key,val)
	local num=tonumber(key)
	local size=engine.peekd(rawget(self,"ptr")+4)*8
	local mtype=rawget(self,"mtype")
	if mtype.index~=nil and num==nil then
		--print("Requested:"..key)
		for k,v in pairs(mtype.index.names) do
			if v==key then
				num=k
				break
			end
		end
			--print("is key:"..num)

	end
	if num~=nil and num<size then
		return mtype:bitwrite(rawget(self,"ptr"),num,val)
	else
		error("invalid key to df-flagarray")
	end
end

xtypes.containers["df-flagarray"]=farr


--------------------------------------------
local bytes_pad={} 
bytes_pad.__index=bytes_pad
function bytes_pad.new(node,obj)
	local o=obj or {}
	setmetatable(o,bytes_pad)
	o.size=tonumber(node.xarg.size)
	if node.xarg.alignment~=nil then
		--print("Aligned bytes!")
		o.__align=tonumber(node.xarg.alignment)
	end
	return o
end
xtypes["bytes"]=bytes_pad
--------------------------------------------
function getGlobal(name,canDelay)
	
	if types[name]== nil then
		if canDelay then
			types[name]={}	
			return types[name]
		end
		findAndParse(name)
		if types[name]== nil then
			error("type:"..name.." failed find-and-parse")
		end
		--error("type:"..node.xarg["type-name"].." should already be ready")
	end
	if types[name].size==nil and canDelay==nil then --was delayed, now need real type
		findAndParse(name)
		if types[name]== nil then
			error("type:"..name.." failed find-and-parse")
		end
	end
	return types[name]
end
parser={}
parser["ld:global-type"]=function  (node,obj)
	return xtypes[node.xarg.meta].new(node,obj)
end
parser["ld:global-object"]=function  (node)
	
end

parser["ld:field"]=function (node,obj,canDelay)
	local meta=node.xarg.meta
	if meta=="number" or (meta=="primitive" and node.xarg.subtype=="stl-string") then
		return getSimpleType(node.xarg.subtype,obj)
	elseif meta=="static-array" then
		return xtypes["static-array"].new(node,obj)
	elseif meta=="global" then
		local ltype=getGlobal(node.xarg["type-name"],canDelay)
		if node.xarg["base-type"]~=nil then
			ltype:setbtype(node.xarg["base-type"])
		end
		return ltype
	elseif meta=="compound" then
		if node.xarg.subtype==nil then
			return xtypes["struct-type"].new(node,obj)
		else
			return xtypes[node.xarg.subtype.."-type"].new(node,obj)
		end
	elseif meta=="container" then
		local subtype=node.xarg.subtype
		
		if xtypes.containers[subtype]==nil then
			error(subtype.." not implemented... (container)")
		else
			return xtypes.containers[subtype].new(node,obj)
		end
	elseif meta=="pointer" then
		return xtypes["pointer"].new(node,obj)
	elseif meta=="bytes" then
		return xtypes["bytes"].new(node,obj)
	else
		error("Unknown meta:"..meta)
	end
end
parser["ld:item"]=parser["ld:field"]
function makeType(node,obj,canDelay)
	local label=node.label
	if parser[label] ~=nil then
		--print("Make Type with:"..label)
		local ret=parser[label](node,obj,canDelay)
		if ret==nil then
			error("Error parsing:"..label.." nil returned!")
		else
			return ret
		end
	else
		for k,v in pairs(node) do
			print(k.."->"..tostring(v))
		end
		error("Node parser not found: "..label)
	end
	--[=[
	if getSimpleType(node)~=nil then
		return getSimpleType(node)
	end
	print("Trying to make:"..node.xarg.meta)
	if xtypes[node.xarg.meta]~=nil then
		return xtypes[node.xarg.meta].new(node,obj)
	end
	
	if node.xarg.meta=="global" then
		--print(node.xarg["type-name"])
		
		if types[node.xarg["type-name"]]== nil then
			error("type:"..node.xarg["type-name"].." should already be ready")
		end
		return types[node.xarg["type-name"]]
	end
	]=]
	--[=[for k,v in pairs(node) do
		print(k.."=>"..tostring(v))
		if type(v)=="table" then
			for kk,vv in pairs(v) do 
				print("\t"..kk.."=>"..tostring(vv))
			end
		end
	end]=]
	
end