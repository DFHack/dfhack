--scroll down to the end for configuration
ret={...}
ret=ret[1]
ret.materials={}
ret.buildings={}
ret.special={}
for k,v in pairs(ret) do
  _ENV[k]=v
end
-- add material by id (index,mat pair or token string or a type number), flags is a table of strings
-- supported flags (but not implemented):
--		flicker
function addMaterial(id,transparency,emitance,radius,flags)
	local matinfo
	if type(id)=="string" then
		matinfo=dfhack.matinfo.find(id)
	elseif type(id)=="table" then
		matinfo=dfhack.matinfo.decode(id[1],id[2])
	else
		matinfo=dfhack.matinfo.decode(id,0)
	end
	if matinfo==nil then
		error("Material not found")
	end
	materials[matinfo.type]=materials[matinfo.type] or {}
	materials[matinfo.type][matinfo.index]=makeMaterialDef(transparency,emitance,radius,flags)
end
function buildingLookUp(id)
	local tokens={}
	local lookup={ Workshop=df.workshop_type,Furnace=df.furnace_type,Trap=df.trap_type,
		SiegeEngine=siegeengine_type}
	for i in string.gmatch(id, "[^:]+") do
		table.insert(tokens,i)
	end
	local ret={}
	ret.type=df.building_type[tokens[1]]
	if tokens[2] then
		local type_array=lookup[tokens[1]]
		if type_array then
			ret.subtype=type_array[tokens[2]]
		end
		if tokens[2]=="Custom"  and tokens[3] then --TODO cache for faster lookup
			if ret.type==df.building_type.Workshop then
				for k,v in pairs(df.global.world.raws.buildings.workshops) do
					if v.code==tokens[3] then
						ret.custom=v.id
						break
					end
				end
			elseif ret.type==df.building_type.Furnace then
				for k,v in pairs(df.global.world.raws.buildings.furnaces) do
					if v.code==tokens[3] then
						ret.custom=v.id
						break
					end
				end
			end
		end
	end
	return ret
end
-- add building by id (string e.g. "Statue" or "Workshop:Masons", flags is a table of strings
-- supported flags:
--		useMaterial --uses material, but the defined things overwrite
--		poweredOnly --glow only when powered
function addBuilding(id,transparency,emitance,radius,flags,size,thickness)
	size=size or 1
	thickness=thickness or 1
	local bld=buildingLookUp(id)
	local mat=makeMaterialDef(transparency,emitance,radius,flags)
	mat.size=size
	mat.thickness=thickness
	buildings[bld.type]=buildings[bld.type] or {}
	if bld.subtype then
		if bld.custom then
			buildings[bld.type][bld.subtype]=buildings[bld.type][bld.subtype] or {}
			buildings[bld.type][bld.subtype][bld.custom]=mat
		else
			buildings[bld.type][bld.subtype]={[-1]=mat}
		end
	else
		buildings[bld.type][-1]={[-1]=mat}
	end
end
function makeMaterialDef(transparency,emitance,radius,flags)
	local flg
	if flags then
		flg={}
		for k,v in ipairs(flags) do
			flg[v]=true
		end
	end
	return {tr=transparency,em=emitance,rad=radius,flags=flg}
end
------------------------------------------------------------------------
----------------   Configuration Starts Here   -------------------------
------------------------------------------------------------------------
is_computer_quantum=false -- will enable more costly parts in the future
--special things
special.LAVA=makeMaterialDef({0.8,0.2,0.2},{0.8,0.2,0.2},5)
special.WATER=makeMaterialDef({0.5,0.5,0.8})
special.FROZEN_LIQUID=makeMaterialDef({0.2,0.7,0.9}) -- ice
special.AMBIENT=makeMaterialDef({0.85,0.85,0.85}) --ambient fog
special.CURSOR=makeMaterialDef({1,1,1},{0.96,0.84,0.03},11, {"flicker"})
special.CITIZEN=makeMaterialDef(nil,{0.80,0.80,0.90},6)
special.LevelDim=0.2 -- darkness. Do not set to 0
special.dayHour=-1 -- <0 cycle, else hour of the day
special.dayColors={ {0,0,0}, --dark at 0 hours 
					{0.6,0.5,0.5}, --reddish twilight
				    {1,1,1}, --fullbright at 12 hours 
					{0.5,0.5,0.5}, 
					{0,0,0}} --dark at 24 hours 
special.daySpeed=1 -- 1->1200 cur_year_ticks per day. 2->600 ticks
--TODO dragonfire
--materials


--		glasses
addMaterial("GLASS_GREEN",{0.1,0.9,0.5})
addMaterial("GLASS_CLEAR",{0.5,0.95,0.9})
addMaterial("GLASS_CRYSTAL",{0.75,0.95,0.95})
--		Plants
addMaterial("PLANT:TOWER_CAP",nil,{0.65,0.65,0.65},6)
addMaterial("PLANT:MUSHROOM_CUP_DIMPLE",nil,{0.03,0.03,0.5},3)
addMaterial("PLANT:CAVE MOSS",nil,{0.1,0.1,0.4},2)
addMaterial("PLANT:MUSHROOM_HELMET_PLUMP",nil,{0.2,0.1,0.6},2)
--		inorganics
addMaterial("INORGANIC:ADAMANTINE",{0.1,0.3,0.3},{0.1,0.3,0.3},4)
--		creature stuff
addMaterial("CREATURE:DRAGON:BLOOD",nil,{0.6,0.1,0.1},4)
-- TODO gems
--buildings
addBuilding("Statue",{1,1,1},{0.9,0.75,0.3},8)
addBuilding("Bed",{1,1,1},{0.3,0.2,0.0},2)
addBuilding("WindowGlass",nil,nil,0,{"useMaterial"})
addBuilding("WindowGem",nil,nil,0,{"useMaterial"})
addBuilding("Door",nil,nil,0,{"useMaterial"}) -- special case, only closed door obstruct/emit light