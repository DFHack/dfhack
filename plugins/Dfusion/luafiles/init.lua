function err(msg) --make local maybe...
	print(msg)
	print(debug.traceback())
end
function dofile(filename) --safer dofile, with traceback (very usefull)
	f,perr=loadfile(filename)
	if f~=nil then
		return xpcall(f,err)
	else
		print(perr)
	end
end

function mainmenu(t1)
	Console.clear()
	while true do
		print("No.	Name		Desc")
		for k,v in pairs(t1) do
			print(string.format("%d %s		%s",k,v[1],v[2]))
		end
		local q=Console.lineedit("Select plugin to run (q to quit):")
		if q=='q' then return end
		q=tonumber(q)
		if q~=nil then
			if q>=1 and q<=#t1 then
				dofile("dfusion/"..t1[q][1].."/plugin.lua")
				
			end
		end
	end
end
dofile("dfusion/common.lua")
unlockDF()
plugins={}
table.insert(plugins,{"simple_embark","A simple embark dwarf count editor"})
table.insert(plugins,{"items","A collection of item hacking tools"})
table.insert(plugins,{"offsets","Find all offsets"})
mainmenu(plugins)

