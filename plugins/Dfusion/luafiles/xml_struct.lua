--[=[
	bld=buildinglist[1]
	bld.x1=10
	bld.flags.exists=true
	boolval=bld.flags.justice
	if boolval then
		bld.mat_type=bld.mat_type+1
	end
	
	type info:
	
--]=]
dofile("dfusion/xml_types.lua")

--[=[sometype={	
}
sometype.x1={INT32_T,0}
sometype.y1={INT32_T,4}
--...
sometype.flags={"building_flags",7*4}
--...

types.building=sometype
]=]

function parseTree(t)


	for k,v in ipairs(t) do
		if v.xarg~=nil and v.xarg["type-name"]~=nil and v.label=="ld:global-type" then
			local name=v.xarg["type-name"];
			print("Parsing:"..name)
			for kk,vv in pairs(v.xarg) do
				print("\t"..kk.." "..tostring(vv))
			end
			types[name]=makeType(v)
			
			
			print("found "..name.." or type:"..v.xarg.meta or v.xarg.base)
		end
	end
end
function findAndParse(tname)
	for k,v in ipairs(main_tree) do
	local name=v.xarg["type-name"];
		if v.xarg~=nil and v.xarg["type-name"]~=nil and v.label=="ld:global-type" then
			if(name ==tname) then
			print("Parsing:"..name)
			for kk,vv in pairs(v.xarg) do
				print("\t"..kk.." "..tostring(vv))
			end
			types[name]=makeType(v)
			end
			
			--print("found "..name.." or type:"..v.xarg.meta or v.xarg.base)
		end
	end
end


--------------------------------
types=types or {}
dofile("dfusion/patterns/xml_angavrilov.lua")
main_tree=parseXmlFile("dfusion/patterns/supplementary.xml")[1]
parseTree(main_tree)
main_tree=parseXmlFile("dfusion/patterns/codegen.out.xml")[1]
parseTree(main_tree)
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
