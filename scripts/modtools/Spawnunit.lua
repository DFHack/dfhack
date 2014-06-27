--Spawnunit.lua
--create unit at pointer. Usage e.g. "spawnunit DWARF 0 Dwarfy"
--author Warmist, Runrusher?
--edited by expwnent

function findCasteIndex(race_id,casteName)
 local cr=df.creature_raw.find(race_id)
 for casteIndex,caste in ipairs(cr.caste) do
  if caste.caste_id == casteName then
   return casteIndex
  end
 end
 return nil
end
function getCaste(race_id,caste_id)
 local cr=df.creature_raw.find(race_id)
 return cr.caste[caste_id]
end
function genBodyModifier(body_app_mod)
 local a=math.random(0,#body_app_mod.ranges-2)
 return math.random(body_app_mod.ranges[a],body_app_mod.ranges[a+1])
end
function getBodySize(caste,time)
 --todo real body size...
 return caste.body_size_1[#caste.body_size_1-1] --returns last body size
end
function genAttribute(array)
 local a=math.random(0,#array-2)
 return math.random(array[a],array[a+1])
end
function norm()
 return math.sqrt((-2)*math.log(math.random()))*math.cos(2*math.pi*math.random())
end
function normalDistributed(mean,sigma)
 return mean+sigma*norm()
end
function clampedNormal(min,median,max)
 local val=normalDistributed(median,math.sqrt(max-min))
 if val<min then return min end
 if val>max then return max end
 return val
end
function makeSoul(unit,caste)
 local tmp_soul=df.unit_soul:new()
 tmp_soul.unit_id=unit.id
 tmp_soul.name:assign(unit.name)
 tmp_soul.race=unit.race
 tmp_soul.sex=unit.sex
 tmp_soul.caste=unit.caste
 --todo skills,preferences,traits.
 local attrs=caste.attributes
 for k,v in pairs(attrs.ment_att_range) do
  local max_percent=attrs.ment_att_cap_perc[k]/100
  local cvalue=genAttribute(v)
  tmp_soul.mental_attrs[k]={value=cvalue,max_value=cvalue*max_percent}
 end
 for k,v in pairs(tmp_soul.traits) do
  local min,mean,max
  min=caste.personality.a[k]
  mean=caste.personality.b[k]
  max=caste.personality.c[k]
  tmp_soul.traits[k]=clampedNormal(min,mean,max)
 end
 unit.status.souls:insert("#",tmp_soul)
 unit.status.current_soul=tmp_soul
end
function CreateUnit(race_id,caste_id)
 local race=df.creature_raw.find(race_id)
 if race==nil then error("Invalid race_id") end
 local caste=getCaste(race_id,caste_id)
 local unit=df.unit:new()
 unit.race=race_id
 unit.caste=caste_id
 unit.id=df.global.unit_next_id
	df.global.unit_next_id=df.global.unit_next_id+1
	if caste.misc.maxage_max==-1 then
		unit.relations.old_year=-1
	else
		unit.relations.old_year=math.random(caste.misc.maxage_min,caste.misc.maxage_max)
	end
	unit.sex=caste.gender
 local body=unit.body
 body.body_plan=caste.body_info
 local body_part_count=#body.body_plan.body_parts
 local layer_count=#body.body_plan.layer_part
 --components
 unit.relations.birth_year=df.global.cur_year
 --unit.relations.birth_time=??

 --unit.relations.old_time=?? --TODO add normal age
 local cp=body.components
 cp.body_part_status:resize(body_part_count)
 cp.numbered_masks:resize(#body.body_plan.numbered_masks)
 for num,v in ipairs(body.body_plan.numbered_masks) do
  cp.numbered_masks[num]=v
 end

 cp.layer_status:resize(layer_count)
 cp.layer_wound_area:resize(layer_count)
 cp.layer_cut_fraction:resize(layer_count)
 cp.layer_dent_fraction:resize(layer_count)
 cp.layer_effect_fraction:resize(layer_count)
 local attrs=caste.attributes
 for k,v in pairs(attrs.phys_att_range) do
  local max_percent=attrs.phys_att_cap_perc[k]/100
  local cvalue=genAttribute(v)
  unit.body.physical_attrs[k]={value=cvalue,max_value=cvalue*max_percent}
  --unit.body.physical_attrs:insert(k,{new=true,max_value=genMaxAttribute(v),value=genAttribute(v)})
 end

 body.blood_max=getBodySize(caste,0) --TODO normal values
 body.blood_count=body.blood_max
 body.infection_level=0
 unit.status2.body_part_temperature:resize(body_part_count)
 for k,v in pairs(unit.status2.body_part_temperature) do
  unit.status2.body_part_temperature[k]={new=true,whole=10067,fraction=0}
 end
 --------------------
 local stuff=unit.enemy
 stuff.body_part_878:resize(body_part_count) -- all = 3
 stuff.body_part_888:resize(body_part_count) -- all = 3
 stuff.body_part_relsize:resize(body_part_count) -- all =0
 
 --TODO add correct sizes. (calculate from age)
	local size=caste.body_size_2[#caste.body_size_2-1]
	body.size_info.size_cur=size
	body.size_info.size_base=size
	body.size_info.area_cur=math.pow(size,0.666)
	body.size_info.area_base=math.pow(size,0.666)
 body.size_info.area_cur=math.pow(size*10000,0.333)
	body.size_info.area_base=math.pow(size*10000,0.333)
	
 stuff.were_race=race_id
 stuff.were_caste=caste_id
 stuff.normal_race=race_id
 stuff.normal_caste=caste_id
 stuff.body_part_8a8:resize(body_part_count) -- all = 1
 stuff.body_part_base_ins:resize(body_part_count) 
 stuff.body_part_clothing_ins:resize(body_part_count) 
 stuff.body_part_8d8:resize(body_part_count) 
 unit.recuperation.healing_rate:resize(layer_count) 
 --appearance
  
 local app=unit.appearance
 app.body_modifiers:resize(#caste.body_appearance_modifiers) --3
 for k,v in pairs(app.body_modifiers) do
  app.body_modifiers[k]=genBodyModifier(caste.body_appearance_modifiers[k])
 end
 app.bp_modifiers:resize(#caste.bp_appearance.modifier_idx) --0
 for k,v in pairs(app.bp_modifiers) do
  app.bp_modifiers[k]=genBodyModifier(caste.bp_appearance.modifiers[caste.bp_appearance.modifier_idx[k]])
 end
 --app.unk_4c8:resize(33)--33
 app.tissue_style:resize(#caste.bp_appearance.style_part_idx)
 app.tissue_style_civ_id:resize(#caste.bp_appearance.style_part_idx)
 app.tissue_style_id:resize(#caste.bp_appearance.style_part_idx)
 app.tissue_style_type:resize(#caste.bp_appearance.style_part_idx)
 app.tissue_length:resize(#caste.bp_appearance.style_part_idx)
 app.genes.appearance:resize(#caste.body_appearance_modifiers+#caste.bp_appearance.modifiers) --3
 app.genes.colors:resize(#caste.color_modifiers*2) --???
 app.colors:resize(#caste.color_modifiers)--3
  
 makeSoul(unit,caste)
  
 df.global.world.units.all:insert("#",unit)
 df.global.world.units.active:insert("#",unit)
 --todo set weapon bodypart
	
	local num_inter=#caste.body_info.interactions
 --used to be anon_5 and anon_6: I guessed at what those were before the df-structures update. It seems to work at least a bit. ~expwnent
	unit.curse.own_interaction:resize(num_inter)
	unit.curse.own_interaction_delay:resize(num_inter)
 return unit
end
function findRace(name)
	for k,v in pairs(df.global.world.raws.creatures.all) do
		if v.creature_id==name then
			return k
		end
	end
	qerror("Race:"..name.." not found!")
end
function PlaceUnit(raceName,casteName,name,position)
 local pos
 if position.x==-30000 then
  pos = copyall(df.global.cursor)
 else
  pos = position
 end
 if pos.x==-30000 then
  qerror("Spawnunit: specify location or place the cursor where you want the unit to be created.")
 end
	local race=findRace(raceName)
 local caste=findCasteIndex(race,casteName)
	local u=CreateUnit(race,tonumber(caste) or 0)
	u.pos:assign(pos)
	if name then
		u.name.first_name=name
		u.name.has_name=true
	end
	u.civ_id=df.global.ui.civ_id

	local desig,ocupan=dfhack.maps.getTileFlags(pos)
	ocupan.unit=true
	--createNemesis(u)
end
function createFigure(trgunit)
 local hf=df.historical_figure:new()
 hf.id=df.global.hist_figure_next_id
 hf.race=trgunit.race
 hf.caste=trgunit.caste
 df.global.hist_figure_next_id=df.global.hist_figure_next_id+1
 hf.name.first_name=trgunit.name.first_name
 hf.name.has_name=true
 df.global.world.history.figures:insert("#",hf)
 return hf
end
function createNemesis(trgunit)
 local id=df.global.nemesis_next_id
 local nem=df.nemesis_record:new()
 nem.id=id
 nem.unit_id=trgunit.id
 nem.unit=trgunit
 nem.flags:resize(1)
 nem.flags[4]=true
 nem.flags[5]=true
 nem.flags[6]=true
 nem.flags[7]=true
 nem.flags[8]=true
 nem.flags[9]=true
 --[[for k=4,8 do
  nem.flags[k]=true
 end]]
 df.global.world.nemesis.all:insert("#",nem)
 df.global.nemesis_next_id=id+1
 trgunit.general_refs:insert("#",{new=df.general_ref_is_nemesisst,nemesis_id=id})
 trgunit.flags1.important_historical_figure=true
 local gen=df.global.world.worldgen
 nem.save_file_id=gen.next_unit_chunk_id;
 gen.next_unit_chunk_id=gen.next_unit_chunk_id+1
 gen.next_unit_chunk_offset=gen.next_unit_chunk_offset+1
  
 --[[ local gen=df.global.world.worldgen
 gen.next_unit_chunk_id
 gen.next_unit_chunk_offset
 ]]
 nem.figure=createFigure(trgunit)
end

args={...}

pos = df.new(df.coord)
if #args > 3 then
 pos.x = tonumber(args[4]) or -30000
 pos.y = tonumber(args[5]) or -30000
 pos.z = tonumber(args[6]) or -30000
end

PlaceUnit(args[1],args[2],args[3],pos)

