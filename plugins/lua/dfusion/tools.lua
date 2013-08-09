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
			local entry=dfhack.lineedit()
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
			id=dfhack.lineedit()
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
	local u_nem=dfhack.units.getNemesis(unit)
	local t_nem=dfhack.units.getNemesis(trgunit)
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
	unit.general_refs:insert(#unit.general_refs,proj_ref)
	unit.flags1.projectile=true
end
function empregnate(unit)
	if unit==nil then
		unit=dfhack.gui.getSelectedUnit()
	end
	if unit==nil then
		error("Failed to empregnate. Unit not selected/valid")
	end
	if unit.curse then
		unit.curse.add_tags2.STERILE=false
	end
	local genes = unit.appearance.genes
	if unit.relations.pregnancy_genes == nil then
		print("creating preg ptr.")
		if false then
			print(string.format("%x %x",df.sizeof(unit.relations:_field("pregnancy_genes"))))
			return
		end
		unit.relations.pregnancy_genes = { new = true, assign = genes }
	end
	local ngenes = unit.relations.pregnancy_genes
	if #ngenes.appearance ~= #genes.appearance or #ngenes.colors ~= #genes.colors then
		print("Array sizes incorrect, fixing.")
		ngenes:assign(genes);
	end
	print("Setting preg timer.")
	unit.relations.pregnancy_timer=10
	unit.relations.pregnancy_caste=1
end
menu:add("Empregnate",empregnate)
function healunit(unit)
    if unit==nil then
		unit=dfhack.gui.getSelectedUnit()
	end

	if unit==nil then
		error("Failed to Heal unit. Unit not selected/valid")
	end
    
    unit.body.wounds:resize(0) -- memory leak here :/
	unit.body.blood_count=unit.body.blood_max
	--set flags for standing and grasping...
	unit.status2.limbs_stand_max=4
	unit.status2.limbs_stand_count=4
	unit.status2.limbs_grasp_max=4
	unit.status2.limbs_grasp_count=4
	--should also set temperatures, and flags for breath etc...
	unit.flags1.dead=false
	unit.flags2.calculated_bodyparts=false
	unit.flags2.calculated_nerves=false
	unit.flags2.circulatory_spray=false
	unit.flags2.vision_good=true
	unit.flags2.vision_damaged=false
	unit.flags2.vision_missing=false
	unit.counters.winded=0
	unit.counters.unconscious=0
	for k,v in pairs(unit.body.components) do
		for kk,vv in pairs(v) do
			if k == 'body_part_status' then v[kk].whole = 0  else v[kk] = 0 end
		end
	end
end
menu:add("Heal unit",healunit)
function powerup(unit,labor_rating,military_rating,skills)
    if unit==nil then
		unit=dfhack.gui.getSelectedUnit()
	end
	if unit==nil then
		error("Failed to power up unit. Unit not selected/valid")
	end
    
    if unit.status.current_soul== nil then
        error("Failed to power up unit. Unit has no soul")
    end
    local utils = require 'utils'
    labor_rating = labor_rating or 15
    military_rating = military_rating or 70

    skill =skill or { 0,2,3,4,5,6,7,8,9,10,11,12,13,14,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,41,42,43,44,45,46,47,48,49,54,55,57,58,59,60,61,62,63,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,95,96,97,98,99,100,101,102,103,104,105,109,110,111,112,113,114,115 }
    local military = { 38,39,41,42,43,44,45,46,54,99,100,101,102,103,104,105 }

    for sk,sv in ipairs(skill) do
    local new_rating = labor_rating
    for _,v in ipairs(military) do
      if v == sv then
        local new_rating = military_rating
      end
    end
    utils.insert_or_update(unit.status.current_soul.skills, { new = true, id = sv, rating = new_rating, experience = (new_rating * 500) + (new_rating * (new_rating - 1)) * 50}, 'id')
    end
  
end
menu:add("Power up",powerup)
return _ENV
