-- teleport.lua
-- teleports a unit to a location
-- author Putnam
-- edited by expwnent

local function teleport(unit,pos)
    local unitoccupancy = dfhack.maps.getTileBlock(unit.pos).occupancy[unit.pos.x%16][unit.pos.y%16]
    unit.pos.x = pos.x
    unit.pos.y = pos.y
    unit.pos.z = pos.z
    if not unit.flags1.on_ground then
        unitoccupancy.unit = false
    else
        unitoccupancy.unit_grounded = false
    end
end

local function usage()
    print([[
Usage:
  teleport showunitid
    Displays the id of of the selected unit
  teleport showpos
    Displays the x,y,z position of the selected tile
  teleport unit <id> x <x val> y <y val> z <z val>
    Moves the specified unit to the given coordinates 
]])
end

local function getArgsTogether(args)
    local settings = {pos={}}
    local skipNextArg = false;
    settings.doMove = true
    for k,v in ipairs(args) do
        if true == skipNextArg then
            skipNextArg = false
        else
            v = string.lower(v)
            if v == "unit" then
                settings.unitID=tonumber(args[k+1])
                skipNextArg = true
            elseif v == "x" then
                settings.pos['x']=tonumber(args[k+1])
                skipNextArg = true
            elseif v == "y" then
                settings.pos['y']=tonumber(args[k+1])
                skipNextArg = true
            elseif v == "z" then
                settings.pos['z']=tonumber(args[k+1])
                skipNextArg = true
            elseif v == "showunitid" then
                print(dfhack.gui.getSelectedUnit(true).id)
                settings.doMove = false
            elseif v == "showpos" then
                printall(df.global.cursor)
                settings.doMove = false
            else
                print("Invalid argument: "..v)
                usage();
                settings.doMove = false
            end
        end
    end
    if true == settings.doMove then
        if not settings.pos.x or not settings.pos.y or not settings.pos.z then
            settings.pos=nil
        end
        if not settings.unitID and not settings.pos.x then
            qerror("Needs a position, a unit ID or both, but not neither!")
        end
    end
    return settings
end

local args = {...}
if not args[1] or args[1] == "help" then
    usage()
else
    local teleportSettings=getArgsTogether(args)
    if true == teleportSettings.doMove then
        local unit = teleportSettings.unitID and df.unit.find(teleportSettings.unitID) or dfhack.gui.getSelectedUnit(true)
        local pos = teleportSettings.pos and teleportSettings.pos or df.global.cursor

        teleport(unit,pos)
    end
end

