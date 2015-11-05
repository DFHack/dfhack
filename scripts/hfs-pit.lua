-- Creates a pit under the target leading straight to the Underworld.  Type '?' for help.
-- Based on script by IndigoFenix, @ https://gist.github.com/IndigoFenix/8776696
--[[=begin

hfs-pit
=======
Creates a pit to the underworld at the cursor.

Takes three arguments:  diameter of the pit in tiles, whether to wall off
the pit, and whether to insert stairs.  If no arguments are given, the default
is ``hfs-pit 1 0 0``, ie single-tile wide with no walls or stairs.::

    hfs-pit 4 0 1
    hfs-pit 2 1 0

First example is a four-across pit with stairs but no walls; second is a
two-across pit with stairs but no walls.

=end]]

args={...}

if args[1] == '?' then
    print("Example usage:  'hfs-pit 2 1 1'")
    print("First parameter is size of the pit in all directions.")
    print("Second parameter is 1 to wall off the sides of the pit on all layers except the underworld, or anything else to leave them open.")
    print("Third parameter is 1 to add stairs.  Stairs are buggy; they will not reveal the bottom until you dig somewhere, but underworld creatures will path in.")
    print("If no arguments are given, the default is 'hfs-pit 1 0 0', ie single-tile wide with no walls or stairs.")
    return
end

pos = copyall(df.global.cursor)
size = tonumber(args[1])
if size == nil or size < 1 then size = 1 end

wallOff = tonumber(args[2])
stairs = tonumber(args[3])

--Get the layer of the underworld
for index,value in ipairs(df.global.world.features.map_features) do
    local featureType=value:getType()
    if featureType==9 then --Underworld
        underworldLayer = value.layer
    end
end

if pos.x==-30000 then
    qerror("Select a location by placing the cursor")
end
local x = 0
local y = 0
for x=pos.x-size,pos.x+size,1 do
    for y=pos.y-size,pos.y+size,1 do
        z=1
        local hitAir = false
        local hitCeiling = false
        while z <= pos.z do
            local block = dfhack.maps.ensureTileBlock(x,y,z)
            if block then
                if block.tiletype[x%16][y%16] ~= 335 then
                    hitAir = true
                end
                if hitAir == true then
                    if not hitCeiling then
                        if block.global_feature ~= underworldLayer or z > 10 then hitCeiling = true end
                        if stairs == 1 and x == pos.x and y == pos.y then
                            if block.tiletype[x%16][y%16] == 32 then
                                if z == pos.z then
                                    block.tiletype[x%16][y%16] = 56
                                else
                                    block.tiletype[x%16][y%16] = 55
                                end
                            else
                                block.tiletype[x%16][y%16] = 57
                            end
                        end
                    end
                    if hitCeiling == true then
                        if block.designation[x%16][y%16].flow_size > 0 or wallOff == 1 then needsWall = true else needsWall = false end
                        if (x == pos.x-size or x == pos.x+size or y == pos.y-size or y == pos.y+size) and z==pos.z then
                            --Do nothing, this is the lip of the hole
                        elseif x == pos.x-size and y == pos.y-size then if needsWall == true then block.tiletype[x%16][y%16]=320 end
                            elseif x == pos.x-size and y == pos.y+size then if needsWall == true then block.tiletype[x%16][y%16]=321 end
                            elseif x == pos.x+size and y == pos.y+size then if needsWall == true then block.tiletype[x%16][y%16]=322 end
                            elseif x == pos.x+size and y == pos.y-size then if needsWall == true then block.tiletype[x%16][y%16]=323 end
                            elseif x == pos.x-size or x == pos.x+size then if needsWall == true then block.tiletype[x%16][y%16]=324 end
                            elseif y == pos.y-size or y == pos.y+size then if needsWall == true then block.tiletype[x%16][y%16]=325 end
                            elseif stairs == 1 and x == pos.x and y == pos.y then
                                if z == pos.z then block.tiletype[x%16][y%16]=56
                                else block.tiletype[x%16][y%16]=55 end
                            else block.tiletype[x%16][y%16]=32
                        end
                        block.designation[x%16][y%16].hidden = false
                        --block.designation[x%16][y%16].liquid_type = true -- if true, magma.  if false, water.
                        block.designation[x%16][y%16].flow_size = 0
                        dfhack.maps.enableBlockUpdates(block)
                        block.designation[x%16][y%16].flow_forbid = false
                    end
                end
                block.designation[x%16][y%16].hidden = false
            end
            z = z+1
        end
    end
end