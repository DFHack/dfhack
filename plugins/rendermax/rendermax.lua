--scroll down to the end for configuration
ret={...}
ret=ret[1]
ret.materials={}
ret.buildings={}
ret.special={}
ret.items={}
ret.creatures={}
for k,v in pairs(ret) do
  _ENV[k]=v
end
-- add material by id (index,mat pair or token string or a type number), flags is a table of strings
-- supported flags (but not implemented):
--        flicker
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
        SiegeEngine=df.siegeengine_type}
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
                        return ret
                    end
                end
            elseif ret.type==df.building_type.Furnace then
                for k,v in pairs(df.global.world.raws.buildings.furnaces) do
                    if v.code==tokens[3] then
                        ret.custom=v.id
                        return ret
                    end
                end
            end
        end
        qerror("Invalid custom building:"..tokens[3])
    end
    return ret
end
function itemLookup(id)
    local ret={}
    local tokens={}
    for i in string.gmatch(id, "[^:]+") do
        table.insert(tokens,i)
    end
    ret.type=df.item_type[tokens[1]]
    ret.subtype=-1
    if tokens[2] then
        for k,v in ipairs(df.global.world.raws.itemdefs.all) do --todo lookup correct itemdef
            if v.id==tokens[2] then
                ret.subtype=v.subtype
                return ret
            end
        end
        qerror("Failed item subtype lookup:"..tokens[2])
    end
    return ret
end
function creatureLookup(id)
    local ret={}
    local tokens={}
    for i in string.gmatch(id, "[^:]+") do
        table.insert(tokens,i)
    end
    for k,v in ipairs(df.global.world.raws.creatures.all) do
        if v.creature_id==tokens[1] then
            ret.type=k
            if tokens[2] then
                for k,v in ipairs(v.caste) do
                    if v.caste_id==tokens[2] then
                        ret.subtype=k
                        break
                    end
                end
                if ret.subtype==nil then
                    qerror("caste "..tokens[2].." for "..tokens[1].." not found")
                end
            end
            return ret
        end
    end
    qerror("Failed to find race:"..tokens[1])
end
-- add creature by id ("DWARF" or "DWARF:MALE")
-- supported flags:
function addCreature(id,transparency,emitance,radius,flags)
    local crId=creatureLookup(id)
    local mat=makeMaterialDef(transparency,emitance,radius,flags)
    table.insert(creatures,{race=crId.type,caste=crId.subtype or -1, light=mat})
end
-- add item by id ( "TOTEM" or "WEAPON:PICK" or "WEAPON" for all the weapon types)
-- supported flags:
--        hauling     --active when hauled    TODO::currently all mean same thing...
--        equiped        --active while equiped    TODO::currently all mean same thing...
--        inBuilding    --active in building    TODO::currently all mean same thing...
--        contained    --active in container    TODO::currently all mean same thing...
--        onGround    --active on ground
--        useMaterial --uses material, but the defined things overwrite
function addItem(id,transparency,emitance,radius,flags)
    local itemId=itemLookup(id)
    local mat=makeMaterialDef(transparency,emitance,radius,flags)
    table.insert(items,{["type"]=itemId.type,subtype=itemId.subtype,light=mat})
end
-- add building by id (string e.g. "Statue" or "Workshop:Masons", flags is a table of strings
-- supported flags:
--        useMaterial --uses material, but the defined things overwrite
--        poweredOnly --glow only when powered
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
function colorFrom16(col16)
    local col=df.global.enabler.ccolor[col16]
    return {col[0],col[1],col[2]}
end
function addGems()
    for k,v in pairs(df.global.world.raws.inorganics) do
        if v.material.flags.IS_GEM then
            addMaterial("INORGANIC:"..v.id,colorFrom16(v.material.tile_color[0]+v.material.tile_color[2]*8))
        end
    end
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
special.diffusionCount=1 -- split beam max 1 times to mimic diffuse lights
special.advMode=0 -- 1 or 0 different modes for adv mode. 0-> use df vision system,
                  -- 1(does not work)->everything visible, let rendermax light do the work
--TODO dragonfire
--materials


--        glasses
addMaterial("GLASS_GREEN",{0.1,0.9,0.5})
addMaterial("GLASS_CLEAR",{0.5,0.95,0.9})
addMaterial("GLASS_CRYSTAL",{0.75,0.95,0.95})
--        Plants
addMaterial("PLANT:TOWER_CAP",nil,{0.65,0.65,0.65},6)
addMaterial("PLANT:MUSHROOM_CUP_DIMPLE",nil,{0.03,0.03,0.5},3)
addMaterial("PLANT:CAVE MOSS",nil,{0.1,0.1,0.4},2)
addMaterial("PLANT:MUSHROOM_HELMET_PLUMP",nil,{0.2,0.1,0.6},2)
--        inorganics
addMaterial("INORGANIC:ADAMANTINE",{0.1,0.3,0.3},{0.1,0.3,0.3},4)
--        creature stuff
addMaterial("CREATURE:DRAGON:BLOOD",nil,{0.6,0.1,0.1},4)
addGems()
--buildings
addBuilding("Statue",{1,1,1},{0.9,0.75,0.3},8)
addBuilding("Bed",{1,1,1},{0.3,0.2,0.0},2)
addBuilding("WindowGlass",nil,nil,0,{"useMaterial"})
addBuilding("WindowGem",nil,nil,0,{"useMaterial"})
addBuilding("Door",nil,nil,0,{"useMaterial"}) -- special case, only closed door obstruct/emit light
addBuilding("Floodgate",nil,nil,0,{"useMaterial"}) -- special case, only closed door obstruct/emit light
--creatures
addCreature("ELEMENTMAN_MAGMA",{0.8,0.2,0.2},{0.8,0.2,0.2},5)
--items
addItem("GEM",nil,nil,{"useMaterial","onGround"})
addItem("ROUGH",nil,nil,{"useMaterial","onGround"})
addItem("SMALLGEM",nil,nil,{"useMaterial","onGround"})
