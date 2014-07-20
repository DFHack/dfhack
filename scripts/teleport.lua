-- teleport.lua
-- teleports a unit to a location
-- author Putnam
-- edited by expwnent

local function teleport(unit,pos)
 local unitoccupancy = dfhack.maps.getTileBlock(unit.pos).occupancy[unit.pos.x%16][unit.pos.y%16]
 unit.pos.x = pos.x
 unit.pos.y = pos.y
 unit.pos.z = pos.z
 if not unit.flags1.on_ground then unitoccupancy.unit = false else unitoccupancy.unit_grounded = false end
end

local function getArgsTogether(args)
 local settings={pos={}}
 for k,v in ipairs(args) do
  v=string.lower(v)
  if v=="unit" then settings.unitID=tonumber(args[k+1]) end
  if v=="x" then settings.pos['x']=tonumber(args[k+1]) end
  if v=="y" then settings.pos['y']=tonumber(args[k+1]) end
  if v=="z" then settings.pos['z']=tonumber(args[k+1]) end
  if v=="showunitid" then print(dfhack.gui.getSelectedUnit(true).id) end
  if v=="showpos" then printall(df.global.cursor) end
 end
 if not settings.pos.x or not settings.pos.y or not settings.pos.z then settings.pos=nil end
 if not settings.unitID and not settings.pos.x then qerror("Needs a position, a unit ID or both, but not neither!") end
 return settings
end

local args = {...}
local teleportSettings=getArgsTogether(args)
local unit = teleportSettings.unitID and df.unit.find(teleportSettings.unitID) or dfhack.gui.getSelectedUnit(true)
local pos = teleportSettings.pos and teleportSettings.pos or df.global.cursor

teleport(unit,pos)

