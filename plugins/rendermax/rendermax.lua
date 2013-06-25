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
-- supported flags:
--		flicker
-- 		sizeModifiesPower
--		sizeModifiesRange
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
function addBuilding(id,transparency,emitance,radius,flags)
	--stuff
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
--special things
special.LAVA=makeMaterialDef({0.8,0.2,0.2},{0.8,0.2,0.2},5)
special.WATER=makeMaterialDef({0.5,0.5,0.8})
special.FROZEN_LIQUID=makeMaterialDef({0.2,0.7,0.9}) -- ice
special.AMBIENT=makeMaterialDef({0.85,0.85,0.85}) --ambient fog
special.CURSOR=makeMaterialDef({1,1,1},{0.96,0.84,0.03},11, {"flicker"})
special.CITIZEN=makeMaterialDef(nil,{0.80f,0.80f,0.90f},6)
special.LevelDim=0.2 -- darkness. do not set to 0
--TODO dragonfire
--TODO daylight
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
-- TODO gems
--buildings
