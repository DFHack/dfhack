xtypes.containers=xtypes.containers or {}
local stl_vec={}
--[=[
   (make-instance 'pointer :name $start)
   (make-instance 'pointer :name $end)
   (make-instance 'pointer :name $block-end)
   (make-instance 'padding :name $pad :size 4 :alignment 4)
--]=] 
stl_vec.__index=stl_vec

function stl_vec.new(node,obj)
	local o=obj or {}
	
	o.size=16
	o.__align=4
	local titem=first_of_type(node,"ld:item")
	if titem~=nil then
		o.item_type=makeType(titem)
	else
		o.item_type=getSimpleType("uint32_t")
	end
	setmetatable(o,stl_vec)
	return o
end
function stl_vec:makewrap(address)
	local o=obj or {}
	o.mtype=self
	o.ptr=address
	setmetatable(o,self.wrap)
	return o
end
stl_vec.wrap={}
function stl_vec.wrap:__index(key)
	local num=tonumber(key)
	local mtype=rawget(self,"mtype")
	local ptr=rawget(self,"ptr")
	local p_begin=engine.peek(ptr,DWORD)
	local p_end=engine.peek(ptr+4,DWORD)
	local size=(p_end-p_begin)/mtype.item_type.size
	if key=="size" then
		return size
	end
	
	--allocend=type_read(ptr+8,DWORD)
	if num~=nil and num<size then
		return type_read(mtype.item_type,num*mtype.item_type.size+p_begin)
	else
		error(string.format("Invalid key: %s max: %d",key,size))
	end
end
function stl_vec.wrap:__newindex(key,val)
	local num=tonumber(key)
	error("TODO make __newindex for stl_vec")
	if num~=nil and num<rawget(self,"mtype").count then
		return type_write(mtype.item_type,num*mtype.item_type.size+rawget(self,"ptr"),val)
	else
		error("invalid key to stl vector")
	end
end
function stl_vec.wrap.__next(tbl,key)
	--print("next with:"..tostring(key))
	if key==nil then
		return 0,tbl[0]
	else
		if key<tbl.size-1 then
			return	key+1,tbl[key+1]
		else
			return nil
		end
	end
end
xtypes.containers["stl-vector"]=stl_vec

local stl_vec_bit={} 
--[=[
	(make-instance 'pointer :name $start)
	(make-instance 'pointer :name $end)
	(make-instance 'pointer :name $block-end)
	(make-instance 'padding :name $pad :size 4)
	(make-instance 'int32_t :name $size)
--]=]
stl_vec_bit.__index=stl_vec_bit
function stl_vec_bit.new(node,obj)
	local o=obj or {}
	setmetatable(o,stl_vec_bit)
	o.size=20
	o.align=4
	return o
end
function stl_vec_bit:makewrap(address)
	local o=obj or {}
	o.mtype=self
	o.ptr=address
	setmetatable(o,self.wrap)
	return o
end
stl_vec_bit.wrap={}
function stl_vec_bit.wrap:__index(key)
	local num=tonumber(key)
	local mtype=rawget(self,"item_type")
	local ptr=rawget(self,"ptr")
	local p_begin=type_read(ptr,DWORD)
	local p_end=type_read(ptr+4,DWORD)
	--allocend=type_read(ptr+8,DWORD)
	error("TODO make __index for stl_vec_bit")
	if num~=nil and num<sizethen then
		return type_read(mtype.ctype,num*mtype.ctype.size+rawget(self,"ptr"))
	else
		error("invalid key to df-flagarray")
	end
end
function stl_vec_bit.wrap:__newindex(key,val)
	local num=tonumber(key)
	error("TODO make __index for stl_vec_bit")
	if num~=nil and num<rawget(self,"mtype").count then
		return type_write(mtype.ctype,num*mtype.ctype.size+rawget(self,"ptr"),val)
	else
		error("invalid key to static-array")
	end
end
function stl_vec_bit.wrap:__next(tbl,key)
	if key==nil then
		return 0,self[0]
	else
		if key+1<self.size then
			return	key+1,self[key+1]
		else
			return nil
		end
	end
end
xtypes.containers["stl-bit-vector"]=stl_vec_bit

local stl_deque={}
--[=[
	(make-instance 'pointer :name $proxy)
	(make-instance 'pointer :name $map)
	(make-instance 'int32_t :name $map-size)
	(make-instance 'int32_t :name $off)
	(make-instance 'int32_t :name $size)
	(make-instance 'padding :size 4 :alignment 4)
--]=]
stl_deque.__index=stl_deque
function stl_deque.new(node,obj)
	local o=obj or {}
	setmetatable(o,stl_deque)
	o.size=24
	o.__align=4
	return o
end
xtypes.containers["stl-deque"]=stl_deque
local dfllist={} 

function dfllist.new(node,obj)
	
	local realtype=makeType(first_of_type(node,"ld:item"))
	return realtype --.new(node,obj)
end
xtypes.containers["df-linked-list"]=dfllist