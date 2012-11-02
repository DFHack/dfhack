local _ENV = mkmodule('plugins.dfusion.tools')
local dfu=require("plugins.dfusion")
local ms=require "memscan"
menu=dfu.SimpleMenu()
RaceNames={}
function build_race_names()
    if #RaceNames~=0 then
        return RaceNames
    else
        for k,v in pairs(df.global.world.raws.creatures.all) do
            RaceNames[v.creature_id]=k
        end
        dfhack.onStateChange.invalidate_races=function(change_id) --todo does this work?
            if change_id==SC_WORLD_UNLOADED then
                dfhack.onStateChange.invalidate_races=nil
                RaceNames={}
            end
        end
        return RaceNames
    end
end
function setrace(name) 
	local RaceTable=build_race_names()
	print("Your current race is:"..df.global.world.raws.creatures.all[df.global.ui.race_id].creature_id)
	local id
	if name == nil then
		print("Type new race's token name in full caps (q to quit):")
		repeat
			local entry=io.stdin:read()
			if entry=="q" then
				return
			end
			id=RaceTable[entry]
		until id~=nil
	else
		id=RaceTable[name]
		if id==nil then
			error("Name not found!")
		end
	end
	df.global.ui.race_id=id
end
menu:add("Set current race",setrace)
function GiveSentience(names) 
	local RaceTable=build_race_names() --slow.If loaded don't load again
    local id,ids
	if names ==nil then
		ids={}
		print("Type race's  token name in full caps to give sentience to:")
		repeat
			id=io.stdin:read()
			id=RaceTable[entry]
            if id~=nil then
                table.insert(ids,id)
            end
		until id==nil
		
	else
		ids={}
		for _,name in pairs(names) do
			id=RaceTable[name]
			table.insert(ids,id)
		end
	end
	for _,id in pairs(ids) do
		local races=df.global.world.raws.creatures.all

		local castes=races[id].caste
		print(string.format("Caste count:%i",#castes))
		for i =0,#castes-1 do
			
			print("Caste name:"..castes[i].caste_id.."...")
			
			local flags=castes[i].flags
			--print(string.format("%x",flagoffset))
			if flags.CAN_SPEAK then
				print("\tis sentient.")
			else
				print("\tnon sentient. Allocating IQ...")
				flags.CAN_SPEAK=true
			end
		end
	end
end
menu:add("Give Sentience",GiveSentience)
function MakeFollow(unit,trgunit)
	if unit == nil then
		unit=dfhack.gui.getSelectedUnit()
	end
	if unit== nil then
		error("Invalid creature")
	end
	if trgunit==nil then
		trgunit=df.global.world.units.active[0]
	end
	unit.relations.group_leader_id=trgunit.id
	local u_nem=getNemesis(unit)
	local t_nem=getNemesis(trgunit)
	if u_nem then
		u_nem.group_leader_id=t_nem.id
	end
	if t_nem and u_nem then
		t_nem.companions:insert(#t_nem.companions,u_nem.id)
	end
end
menu:add("Make creature follow",MakeFollow)
function project(unit,trg) --TODO add to menu?
	if unit==nil then
		unit=getCreatureAtPointer()
	end
	
	if unit==nil then
		error("Failed to project unit. Unit not selected/valid")
	end
	-- todo: add projectile to world, point to unit, add flag to unit, add gen-ref to projectile.
	local p=df.proj_unitst:new()
	local startpos={x=unit.pos.x,y=unit.pos.y,z=unit.pos.z}
	p.origin_pos=startpos
	p.target_pos=trg
	p.cur_pos=startpos
	p.prev_pos=startpos
	p.unit=unit
	--- wtf stuff
	p.unk14=100
	p.unk16=-1
	p.unk23=-1
	p.fall_delay=5
	p.fall_counter=5
	p.collided=true
	-- end wtf
	local citem=df.global.world.proj_list
	local maxid=1
	local newlink=df.proj_list_link:new()
	newlink.item=p
	while citem.item~= nil do
		if citem.item.id>maxid then maxid=citem.item.id end
		if citem.next ~= nil then 
			citem=citem.next
		else
			break
		end
	end
	p.id=maxid+1
	newlink.prev=citem
	citem.next=newlink
	local proj_ref=df.general_ref_projectile:new()
	proj_ref.projectile_id=p.id
	unit.refs:insert(#unit.refs,proj_ref)
	unit.flags1.projectile=true
end
function empregnate(unit)
	if unit==nil then
		unit=getSelectedUnit()
	end
	
	if unit==nil then
		unit=getCreatureAtPos(getxyz())
	end
	
	if unit==nil then
		error("Failed to empregnate. Unit not selected/valid")
	end
	if unit.curse then
		unit.curse.add_tags2.STERILE=false
	end
	local genes = unit.appearance.genes
	if unit.relations.pregnancy_ptr == nil then
		print("creating preg ptr.")
		if false then
			print(string.format("%x %x",df.sizeof(unit.relations:_field("pregnancy_ptr"))))
			return
		end
		unit.relations.pregnancy_ptr = { new = true, assign = genes }
	end
	local ngenes = unit.relations.pregnancy_ptr
	if #ngenes.appearance ~= #genes.appearance or #ngenes.colors ~= #genes.colors then
		print("Array sizes incorrect, fixing.")
		ngenes:assign(genes);
	end
	print("Setting preg timer.")
	unit.relations.pregnancy_timer=10
	unit.relations.pregnancy_mystery=1
end
menu:add("Empregnate",empregnate)
return _ENV