local _ENV = mkmodule('plugins.building-hacks')
--[[
    from native:
        setOwnableBuilding(workshop_type)
        setAnimationInfo(workshop_type,table frames,int frameskip=-1)
        setUpdateSkip(workshop_type,int=0)
        setMachineInfo(workshop_type,bool need_power,int consume=0,int produce=0,table connection_points)
        fixImpassible(workshop_type)
        getPower(building) -- 2 or 0 returns, produced and consumed
        setPower(building,produced, consumed)
    from here:
        setMachineInfoAuto(name,int consume,int produce,bool need_power)
        setAnimationInfoAuto(name,bool make_graphics_too)
        setOnUpdate(name,int interval,callback)
]]

_registeredStuff={}
--cache graphics tiles for mechanical gears
local graphics_cache
function reload_graphics_cache(  )
    graphics_cache={}
    graphics_cache[1]=dfhack.screen.findGraphicsTile('AXLES_GEARS',0,2)
    graphics_cache[2]=dfhack.screen.findGraphicsTile('AXLES_GEARS',1,2)
end

--on world unload unreg callbacks and invalidate cache
local function unregall(state)
    if state==SC_WORLD_UNLOADED then
        graphics_cache=nil
        onUpdateAction._library=nil
        dfhack.onStateChange.building_hacks= nil
        _registeredStuff={}
    end
end

local function onUpdateLocal(workshop)
    local f=_registeredStuff[workshop:getCustomType()]
    if f then
        f(workshop)
    end
end
local function findCustomWorkshop(name_or_id)
    if type(name_or_id) == "string" then
        local raws=df.global.world.raws.buildings.all
        for k,v in ipairs(raws) do
            if v.code==name_or_id then
                return v
            end
        end
        error("Building def:"..name_or_id.." not found")
    elseif type(name_or_id)=="number" then
        return df.building_def.find(name_or_id)
    else
        error("Expected string or integer id for workshop definition")
    end
end
local function registerUpdateAction(shopId,callback)
    _registeredStuff[shopId]=callback
    onUpdateAction._library=onUpdateLocal
    dfhack.onStateChange.building_hacks=unregall
end
--take in tiles with {x=?, y=? ,...} and output a table flat sparse 31x31 table
local function generateFrame(tiles,w,h)
    local mTiles={}
    local ret={}
    for k,v in ipairs(tiles) do
        ret[v.x+v.y*31]=v
    end
    return ret
end
--convert frames to flat arrays if needed
local function processFrames(shop_def,frames)
    local w,h=shop_def.dim_x,shop_def.dim_y
    for frame_id,frame in ipairs(frames) do
        if frame[1].x~=nil then
            frames[frame_id]=generateFrame(frame,w,h)
        end
    end
    return frames
end
--locate gears on the workshop from the raws definition
local function findGears( shop_def ,gear_tiles) --finds positions of all gears and inverted gears
    gear_tiles=gear_tiles or {42,15}
    local w,h=shop_def.dim_x,shop_def.dim_y
    local stage=shop_def.build_stages
    local ret={}
    for x=0,w-1 do
    for y=0,h-1 do
        local tile=shop_def.tile[stage][x][y]
        if tile==gear_tiles[1] then --gear icon
            table.insert(ret,{x=x,y=y,invert=false})
        elseif tile==gear_tiles[2] then --inverted gear icon
            table.insert(ret,{x=x,y=y,invert=true})
        end
    end
    end
    if #ret==0 then
        error(string.format("Could not find gears in a workshop (%s) that was marked for auto-gear finding",shop_def.code))
    end
    return ret
end
--helper for reading tile color info from raws
local function lookup_color( shop_def,x,y,stage )
    return shop_def.tile_color[0][stage][x][y],shop_def.tile_color[1][stage][x][y],shop_def.tile_color[2][stage][x][y]
end
--adds frames for all gear icons and inverted gear icons
local function processFramesAuto( shop_def ,gears,auto_graphics,gear_tiles)
    gear_tiles=gear_tiles or {42,15,graphics_cache[1],graphics_cache[2]}
    local w,h=shop_def.dim_x,shop_def.dim_y
    local frames={{},{}} --two frames only
    local stage=shop_def.build_stages

    for i,v in ipairs(gears) do

        local tile,tile_inv
        if v.inverted then
            tile=gear_tiles[1]
            tile_inv=gear_tiles[2]
        else
            tile=gear_tiles[2]
            tile_inv=gear_tiles[1]
        end

        table.insert(frames[1],{x=v.x,y=v.y,tile,lookup_color(shop_def,v.x,v.y,stage)})
        table.insert(frames[2],{x=v.x,y=v.y,tile_inv,lookup_color(shop_def,v.x,v.y,stage)})

        --insert default gear graphics if auto graphics is on
        if auto_graphics then
            frames[1][#frames[1]][5]=gear_tiles[3]
            frames[2][#frames[2]][5]=gear_tiles[4]
        end
    end

    for frame_id,frame in ipairs(frames) do
        frames[frame_id]=generateFrame(frame,w,h)
    end
    return frames
end
function setMachineInfoAuto( name,need_power,consume,produce,gear_tiles)
    local shop_def=findCustomWorkshop(name)
    local gears=findGears(shop_def,gear_tiles)
    setMachineInfo(name,need_power,consume,produce,gears)
end
function setAnimationInfoAuto( name,make_graphics_too,frame_length,gear_tiles )
    if graphics_cache==nil then
        reload_graphics_cache()
    end
    local shop_def=findCustomWorkshop(name)
    local gears=findGears(shop_def,gear_tiles)
    local frames=processFramesAuto(shop_def,gears,make_graphics_too,gear_tiles)
    setAnimationInfo(name,frames,frame_length)
end
function setOnUpdate(name,interval,callback)
    local shop_def=findCustomWorkshop(name)
    local shop_id=shop_def.id
    setUpdateSkip(name,interval)
    registerUpdateAction(shop_id,callback)
end

return _ENV
