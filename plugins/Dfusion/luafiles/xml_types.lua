function type_read(valtype,address)
	if type(valtype)~= "table" then
		return engine.peek(valtype,address)
	else
		return valtype:makewrap(address)
	end
end
function type_write(valtype,address,val)
	if type(valtype)~= "table" then
		engine.poke(valtype,address,val)
	else
		engine.poke(DWORD,address,rawget(val,"ptr"))
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
-- type must have new and makewrap (that makes a wrapper around ptr)
local sarr={} 
sarr.__index=sarr
function sarr.new(node)
	local o={}
	setmetatable(o,sarr)
	print("Making array.")
	o.count=tonumber(node.xarg.count)
	print("got cound:"..o.count)
	o.ctype=makeType(first_of_type(node,"ld:item"))
	
	o.size=o.count*o.ctype.size
	print("got subtypesize:"..o.ctype.size)
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
	local num=tonumber(key)
	local mtype=rawget(self,"mtype")
	if num~=nil and num<mtype.count then
		return type_read(mtype.ctype,num*mtype.ctype.size+rawget(self,"ptr"))
	else
		error("invalid key to static-array")
	end
end
function sarr.wrap:__newindex(key,val)
	local num=tonumber(key)
	if num~=nil and num<rawget(self,"mtype").count then
		return type_write(mtype.ctype,num*mtype.ctype.size+rawget(self,"ptr"),val)
	else
		error("invalid key to static-array")
	end
end
--]=]
xtypes["static-array"]=sarr


simpletypes={}
simpletypes.uint32_t={DWORD,4}
simpletypes.uint16_t={WORD,2}
simpletypes.uint8_t={BYTE,1}
simpletypes.int32_t={DWORD,4}
simpletypes.int16_t={WORD,2}
simpletypes.int8_t={BYTE,1}
simpletypes.bool={BYTE,1}
simpletypes["stl-string"]={STD_STRING,24}
function getSimpleType(typename)
	if simpletypes[typename] == nil then return end
	local o={}
	o.ctype=simpletypes[typename][1]
	o.size=simpletypes[typename][2]
	return o
end
local type_enum={}
type_enum.__index=type_enum
function type_enum.new(node)
	local o={}
	setmetatable(o,type_enum)
	for k,v in pairs(node) do
		if type(v)=="table" and v.xarg~=nil then
			--print("\t"..k.." "..v.xarg.name)
			o[k-1]=v.xarg.name
		end
	end
	local btype=node.xarg["base-type"] or "uint32_t"
	--print(btype.." t="..convertType(btype))
	o.type=getSimpleType(btype) -- should be simple type
	o.size=o.type.size
	return o
end
xtypes["enum-type"]=type_enum

local type_bitfield={}
type_bitfield.__index=type_bitfield
function type_bitfield.new(node)
	local o={}
	setmetatable(o,type_bitfield)
	o.size=0
	for k,v in pairs(node) do
		if type(v)=="table" and v.xarg~=nil then
			--print("\t"..k.." "..v.xarg.name)
			o[k-1]=v.xarg.name
			o.size=o.size+1
		end
	end
	o.size=o.size/8 -- size in bytes, not bits.
	return o
end
xtypes["bitfield-type"]=type_bitfield


local type_class={}
type_class.__index=type_class
function type_class.new(node)
	local o={}
	setmetatable(o,type_class)
	o.types={}
	o.size=0
	for k,v in pairs(node) do
		if type(v)=="table" and v.label=="ld:field" and v.xarg~=nil then
			local t_name=""
			local name=v.xarg.name or v.xarg["anon-name"] or ("unk_"..k)
				
			print("\t"..k.." "..name.."->"..v.xarg.meta.." ttype:"..v.label)
			local ttype=makeType(v)
			
			--for k,v in pairs(ttype) do
			--	print(k..tostring(v))
			--end
			o.types[name]={ttype,o.size}
			o.size=o.size+ttype.size
		end
	end
	return o
end

type_class.wrap={}
function type_class.wrap:__index(key)
	local myptr=rawget(self,"ptr")
	local mytype=rawget(self,"ptype")
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
function type_pointer.new(node)
	local o={}
	setmetatable(o,type_pointer)
	o.ptype=makeType(first_of_type(node,"ld:item"))
	o.size=4
	return o
end
type_pointer.wrap={}
function type_pointer.wrap:tonumber()
	local myptr=rawget(self,"ptr")
	return type_read(myptr,DWORD)
end
function type_pointer.wrap:fromnumber(num)
	local myptr=rawget(self,"ptr")
	return type_write(myptr,DWORD,num)
end
function type_pointer.wrap:deref()
	local myptr=rawget(self,"ptr")
	local mytype=rawget(self,"ptype")
	return type_read(myptr,mytype)
end
function type_pointer:makewrap(ptr)
	local o={}
	o.ptr=ptr
	o.mtype=self
	setmetatable(o,self.wrap)
	return o
end
xtypes["pointer"]=type_class
--------------------------------------------
--stl-vector (beginptr,endptr,allocptr)
--df-flagarray (ptr,size)
xtypes.containers={}
local farr={} 
farr.__index=farr
function farr.new(node)
	local o={}
	setmetatable(o,sarr)
	o.size=8
	return o
end
function farr:makewrap(address)
	local o={}
	o.mtype=self
	o.ptr=address
	setmetatable(o,self.wrap)
	return o
end
farr.wrap={}
function farr.wrap:__index(key)
	local num=tonumber(key)
	local mtype=rawget(self,"mtype")
	local size=type_read(rawget(self,"ptr")+4,DWORD)
	error("TODO make __index for df-flagarray")
	if num~=nil and num<sizethen then
		return type_read(mtype.ctype,num*mtype.ctype.size+rawget(self,"ptr"))
	else
		error("invalid key to df-flagarray")
	end
end
function farr.wrap:__newindex(key,val)
	local num=tonumber(key)
	error("TODO make __index for df-flagarray")
	if num~=nil and num<rawget(self,"mtype").count then
		return type_write(mtype.ctype,num*mtype.ctype.size+rawget(self,"ptr"),val)
	else
		error("invalid key to static-array")
	end
end

xtypes.containers["df-flagarray"]=farr

local stl_vec={} 
stl_vec.__index=farr
function stl_vec.new(node)
	local o={}
	setmetatable(o,sarr)
	o.size=12
	local titem=first_of_type(node,"ld:item")
	if titem~=nil then
		o.item_type=makeType(titem)
	else
		o.item_type=getSimpleType("uint32_t")
	end
	return o
end
function stl_vec:makewrap(address)
	local o={}
	o.mtype=self
	o.ptr=address
	setmetatable(o,self.wrap)
	return o
end
stl_vec.wrap={}
function stl_vec.wrap:__index(key)
	local num=tonumber(key)
	local mtype=rawget(self,"item_type")
	local ptr=rawget(self,"ptr")
	local p_begin=type_read(ptr,DWORD)
	local p_end=type_read(ptr+4,DWORD)
	--allocend=type_read(ptr+8,DWORD)
	error("TODO make __index for stl_vec")
	if num~=nil and num<sizethen then
		return type_read(mtype.ctype,num*mtype.ctype.size+rawget(self,"ptr"))
	else
		error("invalid key to df-flagarray")
	end
end
function stl_vec.wrap:__newindex(key,val)
	local num=tonumber(key)
	error("TODO make __index for stl_vec")
	if num~=nil and num<rawget(self,"mtype").count then
		return type_write(mtype.ctype,num*mtype.ctype.size+rawget(self,"ptr"),val)
	else
		error("invalid key to static-array")
	end
end

xtypes.containers["stl-vector"]=stl_vec
--------------------------------------------
parser={}
parser["ld:global-type"]=function  (node)
	return xtypes[node.xarg.meta].new(node)
end
parser["ld:global-object"]=function  (node)
	
end
parser["ld:field"]=function (node)
	local meta=node.xarg.meta
	if meta=="number" or (meta=="primitive" and node.xarg.subtype=="stl-string") then
		return getSimpleType(node.xarg.subtype)
	elseif meta=="static-array" then
		return xtypes["static-array"].new(node)
	elseif meta=="global" then
		if types[node.xarg["type-name"]]== nil then
			findAndParse(node.xarg["type-name"])
			if types[node.xarg["type-name"]]== nil then
				error("type:"..node.xarg["type-name"].." failed find-and-parse")
			end
			--error("type:"..node.xarg["type-name"].." should already be ready")
		end
		return types[node.xarg["type-name"]]
	elseif meta=="compound" then
		if node.xarg.subtype==nil then
			return xtypes["struct-type"].new(node)
		else
			return xtypes[node.xarg.subtype.."-type"].new(node)
		end
	elseif meta=="container" then
		local subtype=node.xarg.subtype
		
		if xtypes.containers[subtype]==nil then
			error(subtype.." not implemented... (container)")
		else
			return xtypes.containers[subtype].new(node)
		end
	elseif meta=="pointer" then
		return xtypes["pointer"].new(node)
	elseif meta=="bytes" then
		error("TODO make bytes")
	else
		error("Unknown meta:"..meta)
	end
end
parser["ld:item"]=parser["ld:field"]
function makeType(node,overwrite)
	local label=overwrite or node.label
	if parser[label] ~=nil then
		print("Make Type with:"..label)
		local ret=parser[label](node)
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
		return xtypes[node.xarg.meta].new(node)
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