-- projectileExpansion.lua
-- author: Putnam
-- edited by expwnent
-- Adds extra functionality to projectiles. Use the argument "disable" (minus quotes) to disable.

local syndromeUtil = require("syndromeUtil")
local events=require "plugins.eventful"

local flowtypes = {
 miasma = 0,
 mist = 1,
 mist2 = 2,
 dust = 3,
 lavaMist = 4,
 smoke = 5,
 dragonFire = 6,
 fireBreath = 7,
 web = 8,
 undirectedGas = 9,
 undirectedVapor = 10,
 oceanWave = 11,
 seaFoam = 12
}

local function posIsEqual(pos1,pos2)
 if pos1.x ~= pos2.x or pos1.y ~= pos2.y or pos1.z ~= pos2.z then return false end
 return true
end

local function getMaterial(item)
 if not dfhack.matinfo.decode(item) then return nil end
 local matinfo = dfhack.matinfo.decode(item)
 return matinfo and matinfo.material
-- return dfhack.matinfo.decode(item).material
end

local function getSyndrome(material)
 if material==nil then return nil end
 if #material.syndrome>0 then return material.syndrome[0]
 else return nil end
end

local function removeItem(item)
 item.flags.garbage_collect = true
end

local function findInorganicWithName(matString)
 for inorganicID,material in ipairs(df.global.world.raws.inorganics) do
  if material.id == matString then return inorganicID end
 end
 return nil
end

local function getScriptFromMaterial(material)
 local commandStart
 local commandEnd
 local reactionClasses = material.reaction_class
 for classNumber,reactionClass in ipairs(reactionClasses) do
  if reactionClass.value == "\\COMMAND" then commandStart = classNumber end
  if reactionClass.value == "\\ENDCOMMAND" then commandEnd = classNumber break end
 end
 local script = {}
 if commandStart and commandEnd then
  for i = commandStart+1, commandEnd-1, 1 do
   table.insert(script,reactionClasses[i].value)
  end
 end
 return script
end

local function getUnitHitByProjectile(projectile)
 for uid,unit in ipairs(df.global.world.units.active) do
  if posIsEqual(unit.pos,projectile.cur_pos) then return uid,unit end
 end
 return nil
end

local function matCausesSyndrome(material)
 for _,reactionClass in ipairs(material.reaction_class) do
  if reactionClass.value == "DFHACK_CAUSES_SYNDROME" then return true end --the syndrome is the syndrome local to the projectile material
  end
 return false
end

projectileExpansionFlags = projectilExpansionFlags or {
 matWantsSpecificInorganic = 0,
 matWantsSpecificSize      = 50000,
 matCausesDragonFire       = false,
 matCausesMiasma           = false,
 matCausesMist             = false,
 matCausesMist2            = false,
 matCausesDust             = false,
 matCausesLavaMist         = false,
 matCausesSmoke            = false,
 matCausesFireBreath       = false,
 matCausesWeb              = false,
 matCausesUndirectedGas    = false,
 matCausesUndirectedVapor  = false,
 matCausesOceanWave        = false,
 matCausesSeaFoam          = false,
 matHasScriptAttached      = false,
 matCausesSyndrome         = false,
 returnLocation            = false,
 matDisappearsOnHit        = false
}

local function getProjectileExpansionFlags(material)
 local matName  = nil
 for k,reactionClass in ipairs(material.reaction_class) do
  if debugProjExp then print("checking reaction class #" .. k .. "...",reactionClass.value) end
  if string.find(reactionClass.value,"DFHACK") then 
   if debugProjExp then print("DFHack reaction class found!") end
   if reactionClass.value == "DFHACK_SPECIFIC_MAT"     then matName = material.reaction_class[k+1].value                        end
   if reactionClass.value == "DFHACK_FLOW_SIZE"        then projectileExpansionFlags.matWantsSpecificSize     = tonumber(material.reaction_class[k+1].value) end
   if reactionClass.value == "DFHACK_CAUSES_SYNDROME"  then projectileExpansionFlags.matCausesSyndrome        = true            end        
   if reactionClass.value == "DFHACK_DRAGONFIRE"       then projectileExpansionFlags.matCausesDragonFire      = true            end
   if reactionClass.value == "DFHACK_MIASMA"           then projectileExpansionFlags.matCausesMiasma          = true            end
   if reactionClass.value == "DFHACK_MIST"             then projectileExpansionFlags.matCausesMist            = true            end
   if reactionClass.value == "DFHACK_MIST2"            then projectileExpansionFlags.matCausesMist2           = true            end
   if reactionClass.value == "DFHACK_DUST"             then projectileExpansionFlags.matCausesDust            = true            end
   if reactionClass.value == "DFHACK_LAVAMIST"         then projectileExpansionFlags.matCausesLavaMist        = true            end
   if reactionClass.value == "DFHACK_SMOKE"            then projectileExpansionFlags.matCausesSmoke           = true            end
   if reactionClass.value == "DFHACK_FIREBREATH"       then projectileExpansionFlags.matCausesFireBreath      = true            end
   if reactionClass.value == "DFHACK_WEB"              then projectileExpansionFlags.matCausesWeb             = true            end
   if reactionClass.value == "DFHACK_GAS_UNDIRECTED"   then projectileExpansionFlags.matCausesUndirectedGas   = true            end
   if reactionClass.value == "DFHACK_VAPOR_UNDIRECTED" then projectileExpansionFlags.matCausesUndirectedVapor = true            end
   if reactionClass.value == "DFHACK_OCEAN_WAVE"       then projectileExpansionFlags.matCausesOceanWave       = true            end
   if reactionClass.value == "DFHACK_SEA_FOAM"         then projectileExpansionFlags.matCausesSeaFoam         = true            end
   if reactionClass.value == "DFHACK_DISAPPEARS"       then projectileExpansionFlags.matDisappearsOnHit       = true            end
  end
  if reactionClass.value == "\\COMMAND"                then projectileExpansionFlags.matHasScriptAttached     = true            end
 end
 if matName then projectileExpansionFlags.matWantsSpecificInorganic = findInorganicWithName(matName) end
 return projectileExpansionFlags
end

debugProjExp=false

events.onProjItemCheckImpact.expansion=function(projectile)
 if debugProjExp then print("Thwack! Projectile item hit. Running projectileExpansion.") end
 if projectile then
  if debugProjExp then print("Found the item. Working on it.") end
  local material = getMaterial(projectile.item)
  if not material then return nil end
  local projectileExpansionFlags=getProjectileExpansionFlags(material)
  if debugProjExp then print(projectileExpansionFlags) printall(projectileExpansionFlags) end
  local syndrome = getSyndrome(material)
  local emissionMat = projectileExpansionFlags.matWantsSpecificInorganic --defaults to iron
  local flowSize = projectileExpansionFlags.matWantsSpecificSize         --defaults to 50000
  if projectileExpansionFlags.matCausesDragonFire      then dfhack.maps.spawnFlow(projectile.cur_pos,flowtypes.dragonFire,0,0,flowSize) end
  if projectileExpansionFlags.matCausesMiasma          then dfhack.maps.spawnFlow(projectile.cur_pos,flowtypes.miasma,0,0,flowSize) end
  if projectileExpansionFlags.matCausesMist            then dfhack.maps.spawnFlow(projectile.cur_pos,flowtypes.mist,0,0,flowSize) end
  if projectileExpansionFlags.matCausesMist2           then dfhack.maps.spawnFlow(projectile.cur_pos,flowtypes.mist2,0,0,flowSize) end
  if projectileExpansionFlags.matCausesDust            then dfhack.maps.spawnFlow(projectile.cur_pos,flowtypes.dust,0,emissionMat,flowSize) end
  if projectileExpansionFlags.matCausesLavaMist        then dfhack.maps.spawnFlow(projectile.cur_pos,flowtypes.lavaMist,0,emissionMat,flowSize) end
  if projectileExpansionFlags.matCausesSmoke           then dfhack.maps.spawnFlow(projectile.cur_pos,flowtypes.smoke,0,0,flowSize) end
  if projectileExpansionFlags.matCausesFireBreath      then dfhack.maps.spawnFlow(projectile.cur_pos,flowtypes.fireBreath,0,0,flowSize) end
  if projectileExpansionFlags.matCausesWeb             then dfhack.maps.spawnFlow(projectile.cur_pos,flowtypes.web,0,emissionMat,flowSize) end
  if projectileExpansionFlags.matCausesUndirectedGas   then dfhack.maps.spawnFlow(projectile.cur_pos,flowtypes.undirectedGas,0,emissionMat,flowSize) end
  if projectileExpansionFlags.matCausesUndirectedVapor then dfhack.maps.spawnFlow(projectile.cur_pos,flowtypes.undirectedVapor,0,emissionMat,flowSize) end
  if projectileExpansionFlags.matCausesOceanWave       then dfhack.maps.spawnFlow(projectile.cur_pos,flowtypes.oceanWave,0,0,flowSize) end
  if projectileExpansionFlags.matCausesSeaFoam         then dfhack.maps.spawnFlow(projectile.cur_pos,flowtypes.seaFoam,0,0,flowSize) end
  if projectileExpansionFlags.matHasScriptAttached or projectileExpansionFlags.matCausesSyndrome then
   local uid,unit = getUnitHitByProjectile(projectile)        
   if projectileExpansionFlags.matHasScriptAttached then
    local script = getScriptFromMaterial(material)
    for k,v in ipairs(script) do
     if script[k] == "\\UNIT_HIT_ID" then script[k] = unit.id end
     if script[k] == "\\LOCATION" then
      script[k] = projectile.cur_pos.x
      table.insert(script,projectile.cur_pos.y,k+1)
      table.insert(script,projectile.cur_pos.z,k+2)
     end
    end
    dfhack.run_script(table.unpack(script))
   end
   if projectileExpansionFlags.matCausesSyndrome then syndromeUtil.infectWithSyndrome(unit,syndrome.id) end
  end
  --if projectileExpansionFlags.matDisappearsOnHit then dfhack.items.remove(projectile) end
 end
 return true
end

if ... ~= "disable" then 
 print("Enabled projectileExpansion.")
else
 events.onProjItemCheckImpact.expansion = nil
 print("Disabled projectileExpansion.")
end

