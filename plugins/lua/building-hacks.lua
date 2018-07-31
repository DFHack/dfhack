local _ENV = mkmodule('plugins.building-hacks')
--[[
    from native:
        addBuilding(custom type,impassible fix (bool), consumed power, produced power, list of connection points,
        update skip(0/nil to disable),table of frames,frame to tick ratio (-1 for machine control))
        getPower(bld) -- 2 or 0 returns, produced and consumed
        setPower(bld,produced, consumed)
    from here:
        registerBuilding{
            name -- custom workshop id e.g. SOAPMAKER << required!
            fix_impassible -- make impassible tiles impassible to liquids too
            consume -- how much machine power is needed to work
            produce -- how much machine power is produced
            needs_power -- needs power to be able to add jobs
            action -- a table of number (how much ticks to skip) and a function which gets called on shop update
            canBeRoomSubset -- room is considered in to be part of the building defined by chairs etc...
            auto_gears -- find the gears automatically and animate them
            gears -- a table or {x=?,y=?} of connection points for machines
            animate -- a table of
                frames -- a table of
                    tables of 4 numbers (tile,fore,back,bright) OR
                    empty table (tile not modified) OR
                    {x=<number> y=<number> + 4 numbers like in first case} -- this generates full frame even, usefull for animations that change little (1-2 tiles)
                frameLenght -- how many ticks does one frame take OR
                isMechanical -- a bool that says to try to match to mechanical system (i.e. how gears are turning)
            }
]]
_registeredStuff={}
local function unregall(state)
    if state==SC_WORLD_UNLOADED then
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
local function findCustomWorkshop(name)
    local raws=df.global.world.raws.buildings.all
    for k,v in ipairs(raws) do
        if v.code==name then
            return v
        end
    end
end
local function registerUpdateAction(shopId,callback)
    _registeredStuff[shopId]=callback
    onUpdateAction._library=onUpdateLocal
    dfhack.onStateChange.building_hacks=unregall
end
local function generateFrame(tiles,w,h)
    local mTiles={}
    for k,v in ipairs(tiles) do
        mTiles[v.x]=mTiles[v.x] or {}
        mTiles[v.x][v.y]=v
    end
    local ret={}
    for ty=0,h-1 do
    for tx=0,w-1 do
        if mTiles[tx] and mTiles[tx][ty] then
            table.insert(ret,mTiles[tx][ty]) -- leaves x and y in but who cares
        else
            table.insert(ret,{})
        end
    end
    end
    return ret
end
local function processFrames(shop_def,frames)
    local w,h=shop_def.dim_x,shop_def.dim_y
    for frame_id,frame in ipairs(frames) do
        if frame[1].x~=nil then
            frames[frame_id]=generateFrame(frame,w,h)
        end
    end
    return frames
end
local function findGears( shop_def ) --finds positions of all gears and inverted gears
    local w,h=shop_def.dim_x,shop_def.dim_y
    local stage=shop_def.build_stages
    local ret={}
    for x=0,w-1 do
    for y=0,h-1 do
        local tile=shop_def.tile[stage][x][y]
        if tile==42 then --gear icon
            table.insert(ret,{x=x,y=y,invert=false})
        elseif tile==15 then --inverted gear icon
            table.insert(ret,{x=x,y=y,invert=true})
        end
    end
    end
    return ret
end
local function lookup_color( shop_def,x,y,stage )
    return shop_def.tile_color[0][stage][x][y],shop_def.tile_color[1][stage][x][y],shop_def.tile_color[2][stage][x][y]
end
local function processFramesAuto( shop_def ,gears) --adds frames for all gear icons and inverted gear icons
    local w,h=shop_def.dim_x,shop_def.dim_y
    local frames={{},{}} --two frames only
    local stage=shop_def.build_stages

    for i,v in ipairs(gears) do

        local tile,tile_inv
        if v.inverted then
            tile=42
            tile_inv=15
        else
            tile=15
            tile_inv=42
        end

        table.insert(frames[1],{x=v.x,y=v.y,tile,lookup_color(shop_def,v.x,v.y,stage)})
        table.insert(frames[2],{x=v.x,y=v.y,tile_inv,lookup_color(shop_def,v.x,v.y,stage)})
    end

    for frame_id,frame in ipairs(frames) do
        frames[frame_id]=generateFrame(frame,w,h)
    end
    return frames
end
function registerBuilding(args)
    local shop_def=findCustomWorkshop(args.name)
    local shop_id=shop_def.id
    --misc
    local fix_impassible
    if args.fix_impassible then
        fix_impassible=1
    else
        fix_impassible=0
    end
    local roomSubset=args.canBeRoomSubset or -1
    --power
    local consume=args.consume or 0
    local produce=args.produce or 0
    local needs_power=args.needs_power or 1
    local auto_gears=args.auto_gears or false
    local updateSkip=0
    local action=args.action --could be nil
    if action~=nil then
        updateSkip=action[1]
        registerUpdateAction(shop_id,action[2])
    end
    --animations and connections next:
    local gears
    local frames

    local frameLength
    local animate=args.animate
    if not auto_gears then
        gears=args.gears or {}
        frameLength=1
        if animate~=nil then
            frameLength=animate.frameLength
            if animate.isMechanical then
                frameLength=-1
            end
            frames=processFrames(shop_def,animate.frames)
        end
    else
        frameLength=-1
        if animate~=nil then
            frameLength=animate.frameLength or frameLength
            if animate.isMechanical then
                frameLength=-1
            end
        end
        gears=findGears(shop_def)
        frames=processFramesAuto(shop_def,gears)
    end

    addBuilding(shop_id,fix_impassible,consume,produce,needs_power,gears,updateSkip,frames,frameLength,roomSubset)
end

return _ENV
