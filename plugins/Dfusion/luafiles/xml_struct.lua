if types ~= nil then
	return
end
dofile("dfusion/xml_types.lua")

function parseTree(t)
	for k,v in ipairs(t) do
		if v.xarg~=nil and v.xarg["type-name"]~=nil and v.label=="ld:global-type" then
			local name=v.xarg["type-name"];
			if(types[name]==nil) then
				
				--for kk,vv in pairs(v.xarg) do
				--	print("\t"..kk.." "..tostring(vv))
				--end
				types[name]=makeType(v)
				--print("found "..name.." or type:"..v.xarg.meta or v.xarg.base)
			
			else
				types[name]=makeType(v,types[name])
			end
		end
	end
end
function parseTreeGlobals(t)
	local glob={}
	--print("Parsing global-objects")
	for k,v in ipairs(t) do
		if v.xarg~=nil and v.label=="ld:global-object" then
			local name=v.xarg["name"];
			--print("Parsing:"..name)
			local subitem=v[1]
			if subitem==nil then 
				error("Global-object subitem is nil:"..name)
			end
			local ret=makeType(subitem)
			if ret==nil then
				error("Make global returned nil!")
			end
			glob[name]=ret
		end
	end
	--print("Printing globals:")
	--for k,v in pairs(glob) do
	--	print(k)
	--end
	return glob
end
function findAndParse(tname)
	--if types[tname]==nil then types[tname]={} end
	-- [=[
	for k,v in ipairs(main_tree) do
		local name=v.xarg["type-name"];
		if v.xarg~=nil and v.xarg["type-name"]~=nil and v.label=="ld:global-type" then
			
			if(name ==tname) then
			--print("Parsing:"..name)
			--for kk,vv in pairs(v.xarg) do
			--	print("\t"..kk.." "..tostring(vv))
			--end
			types[name]=makeType(v,types[name])
			end
			--print("found "..name.." or type:"..v.xarg.meta or v.xarg.base)
		end
	end
	--]=]
end
df={}
df.types=rawget(df,"types") or {} --temporary measure for debug
local df_meta={}
function df_meta:__index(key)
	local addr=VersionInfo.getAddress(key)
	local vartype=rawget(df,"types")[key];
	if addr==0 then
		error("No such global address exist")
	elseif vartype==nil then
		error("No such global type exist")
	else
		return type_read(vartype,addr)
	end
end
function df_meta:__newindex(key,val)
	local addr=VersionInfo.getAddress(key)
	local vartype=rawget(df,"types")[key];
	if addr==0 then
		error("No such global address exist")
	elseif vartype==nil then
		error("No such global type exist")
	else
		return type_write(vartype,addr,val)
	end
end
setmetatable(df,df_meta)
--------------------------------
types=types or {}
dofile("dfusion/patterns/xml_angavrilov.lua")
-- [=[
main_tree=parseXmlFile("dfusion/patterns/supplementary.xml")[1]
parseTree(main_tree)
main_tree=parseXmlFile("dfusion/patterns/codegen.out.xml")[1]
parseTree(main_tree)
rawset(df,"types",parseTreeGlobals(main_tree))
--]=]
--[=[labels={}
for k,v in ipairs(t) do
	labels[v.label]=labels[v.label] or {meta={}}
	if v.label=="ld:global-type" and v.xarg~=nil and v.xarg.meta ~=nil then
		labels[v.label].meta[v.xarg.meta]=1
	end
end
for k,v in pairs(labels) do
	print(k)
	if v.meta~=nil then
		for kk,vv in pairs(v.meta) do
			print("=="..kk)
		end
	end
end--]=]
function addressOf(var,key)
	if key== nil then
		local addr=rawget(var,"ptr")
		return addr
	else
		local meta=getmetatable(var)
		if meta== nil then
			error("Failed to get address, no metatable")
		end
		if meta.__address == nil then
			error("Failed to get address, no __address function")
		end
		return meta.__address(var,key)
	end
end
function printGlobals()
	print("Globals:")
	for k,v in pairs(rawget(df,"types")) do
		print(k)
	end
end
function printFields(object)
	local tbl
	if getmetatable(object)==xtypes["struct-type"].wrap then
		tbl=rawget(object,"mtype")
	elseif getmetatable(object)==xtypes["struct-type"] then
		tbl=object
	else
		error("Not an class_type or a class_object")
	end
	for k,v in pairs(tbl.types) do
		print(string.format("%s %x",k,v[2]))
	end
end