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
function dofile_silent(filename) --safer dofile, with traceback, no file not found error
	f,perr=loadfile(filename)
	if f~=nil then
		return xpcall(f,err)
	else
		if(string.sub(perr,1,11)~="cannot open") then --ugly hack
			print(perr)	
		end
	end
end
function loadall(t1) --loads all non interactive plugin parts, so that later they could be used
	for k,v in pairs(t1) do
		dofile_silent("dfusion/"..v[1].."/init.lua")
	end
end
function mainmenu(t1)
	
	while true do
		print("No.	Name           Desc")
		for k,v in pairs(t1) do
			print(string.format("%3d %15s %s",k,v[1],v[2]))
		end
		local q=dfhack.lineedit("Select plugin to run (q to quit):")
		if q=='q' then return end
		q=tonumber(q)
		if q~=nil then
			if q>=1 and q<=#t1 then
				if t1[q][3]==nil then
					dofile("dfusion/"..t1[q][1].."/plugin.lua")
				else
					t1[q][3]()
				end
			end
		end
	end
end
function RunSaved() 
	print("Locating saves...")
	local str=df.global.world.cur_savegame.save_dir
	print("Current region:"..str)
	str="data/save/"..str.."/dfusion/init.lua"
	print("Trying to run:"..str)
	dofile_silent(str)
end
dofile("dfusion/common.lua")
dofile("dfusion/utils.lua")
dofile("dfusion/offsets_misc.lua")
dofile("dfusion/editor.lua")
--dofile("dfusion/xml_struct.lua")
unlockDF()
plugins={}
table.insert(plugins,{"simple_embark","A simple embark dwarf count editor"})
table.insert(plugins,{"tools","some misc tools"})
table.insert(plugins,{"embark","Multi race embark"})
table.insert(plugins,{"friendship","Multi race fort enabler"})
--[=[table.insert(plugins,{"items","A collection of item hacking tools"})
table.insert(plugins,{"offsets","Find all offsets"})

table.insert(plugins,{"friendship_civ","Multi civ fort enabler"})


table.insert(plugins,{"triggers","a function calling plug (discontinued...)"})
table.insert(plugins,{"migrants","multi race imigrations"})
--]=]
--table.insert(plugins,{"onfunction","run lua on some df function"})
--table.insert(plugins,{"editor","edit internals of df",EditDF})
table.insert(plugins,{"saves","run current worlds's init.lua",RunSaved})
table.insert(plugins,{"adv_tools","some tools for (mainly) adventurer hacking"})
loadall(plugins)
dofile_silent("dfusion/initcustom.lua")

local args={...}


local f,err=load(table.concat(args,' '))
if f then
	f()
else
	dfhack.printerr(err)
end

if not INIT then
mainmenu(plugins)
end

