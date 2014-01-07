local _ENV = mkmodule('plugins.building-hacks')
--[[
	from native:
		addBuilding(custom type,impassible fix (bool), consumed power, produced power, list of connection points, 
		update skip(0/nil to disable),table of frames,frame to tick ratio (-1 for machine control))
	from here:
		registerBuilding{
			name -- custom workshop id e.g. SOAPMAKER << required!
			fix_impassible -- make impassible tiles impassible to liquids too
			consume -- how much machine power is needed to work
			produce -- how much machine power is produced
			gears -- a table or {x=?,y=?} of connection points for machines
			action -- a table of number (how much ticks to skip) and a function which gets called on shop update
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
function registerBuilding(args)
	local shop_def=findCustomWorkshop(args.name)
	local shop_id=shop_def.id
	local fix_impassible
	if args.fix_impassible then
		fix_impassible=1
	else
		fix_impassible=0
	end
	local consume=args.consume or 0
	local produce=args.produce or 0
	local gears=args.gears or {}
	local action=args.action --could be nil
	local updateSkip=0
	if action~=nil then
		updateSkip=action[1]
		registerUpdateAction(shop_id,action[2])
	end
	local animate=args.animate
	local frameLength=1
	local frames
	if animate~=nil then
		frameLength=animate.frameLength
		if animate.isMechanical then
			frameLength=-1
		end
		frames=processFrames(shop_def,animate.frames)
	end
	
	addBuilding(shop_id,fix_impassible,consume,produce,gears,updateSkip,frames,frameLength)
end

return _ENV