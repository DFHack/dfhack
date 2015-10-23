--scripts/modtools/outsideOnly.lua
--author expwnent
--enables outside only and inside only buildings
--[[=begin

modtools/outside-only
=====================
This allows you to specify certain custom buildings as outside only, or inside
only. If the player attempts to build a building in an inappropriate location,
the building will be destroyed.

=end]]
local eventful = require 'plugins.eventful'
local utils = require 'utils'

buildingType = buildingType or utils.invert({'EITHER','OUTSIDE_ONLY','INSIDE_ONLY'})
registeredBuildings = registeredBuildings or {}
checkEvery = checkEvery or 100
timeoutId = timeoutId or nil

eventful.enableEvent(eventful.eventType.UNLOAD,1)
eventful.onUnload.outsideOnly = function()
 registeredBuildings = {}
 checkEvery = 100
 timeoutId = nil
end

local function destroy(building)
 if #building.jobs > 0 and building.jobs[0] and building.jobs[0].job_type == df.job_type.DestroyBuilding then
  return
 end
 local b = dfhack.buildings.deconstruct(building)
 if b then
  --TODO: print an error message to the user so they know
  return
 end
-- building.flags.almost_deleted = 1
end

local function checkBuildings()
 local toDestroy = {}
 local function forEach(building)
  if building:getCustomType() < 0 then
   --TODO: support builtin building types if someone wants
   return
  end
  local pos = df.coord:new()
  pos.x = building.centerx
  pos.y = building.centery
  pos.z = building.z
  local outside = dfhack.maps.getTileBlock(pos).designation[pos.x%16][pos.y%16].outside
  local def = df.global.world.raws.buildings.all[building:getCustomType()]
  local btype = registeredBuildings[def.code]
  if btype then
--   print('outside: ' .. outside==true .. ', type: ' .. btype)
  end

  if not btype or btype == buildingType.EITHER then
   registeredBuildings[def.code] = nil
   return
  elseif btype == buildingType.OUTSIDE_ONLY then
   if outside then
    return
   end
  else
   if not outside then
    return
   end
  end
  table.insert(toDestroy,building)
 end
 for _,building in ipairs(df.global.world.buildings.all) do
  forEach(building)
 end
 for _,building in ipairs(toDestroy) do
  destroy(building)
 end
 if timeoutId then
  dfhack.timeout_active(timeoutId,nil)
  timeoutId = nil
 end
 timeoutId = dfhack.timeout(checkEvery, 'ticks', checkBuildings)
end

eventful.enableEvent(eventful.eventType.BUILDING, 100)
eventful.onBuildingCreatedDestroyed.outsideOnly = function(buildingId)
 checkBuildings()
end

validArgs = validArgs or utils.invert({
 'help',
 'clear',
 'checkEvery',
 'building',
 'type'
})
local args = utils.processArgs({...}, validArgs)
if args.help then
 print([[scripts/modtools/outside-only
arguments
    -help
        print this help message
    -clear
        clears the list of registered buildings
    -checkEvery n
        set how often existing buildings are checked for whether they are in the appropriate location to n ticks
    -type [EITHER, OUTSIDE_ONLY, INSIDE_ONLY]
        specify what sort of restriction to put on the building
    -building name
        specify the id of the building
]])
 return
end

if args.clear then
 registeredBuildings = {}
end

if args.checkEvery then
 if not tonumber(args.checkEvery) then
  error('Invalid checkEvery.')
 end
 checkEvery = tonumber(args.checkEvery)
end

if not args.building then
 return
end

if not args['type'] then
 print 'outside-only: please specify type'
 return
end

if not buildingType[args['type']] then
 error('Invalid building type: ' .. args['type'])
end

registeredBuildings[args.building] = buildingType[args['type']]

checkBuildings()

