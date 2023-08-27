-- Embark underground.
-- author: Atomic Chicken

--@ module = true

local utils = require 'utils'

function getFeatureID(cavernType)
  local features = df.global.world.features
  local map_features = features.map_features
  if cavernType == 'CAVERN_1' then
    for i, feature in ipairs(map_features) do
      if feature._type == df.feature_init_subterranean_from_layerst
      and feature.start_depth == 0 then
        return features.feature_global_idx[i]
      end
    end
  elseif cavernType == 'CAVERN_2' then
    for i, feature in ipairs(map_features) do
      if feature._type == df.feature_init_subterranean_from_layerst
      and feature.start_depth == 1 then
        return features.feature_global_idx[i]
      end
    end
  elseif cavernType == 'CAVERN_3' then
    for i, feature in ipairs(map_features) do
      if feature._type == df.feature_init_subterranean_from_layerst
      and feature.start_depth == 2 then
        return features.feature_global_idx[i]
      end
    end
  elseif cavernType == 'UNDERWORLD' then
    for i, feature in ipairs(map_features) do
      if feature._type == df.feature_init_underworld_from_layerst
      and feature.start_depth == 4 then
        return features.feature_global_idx[i]
      end
    end
  end
end

function getFeatureBlocks(featureID)
  local featureBlocks = {} --as:number[]
  for i,block in ipairs(df.global.world.map.map_blocks) do
    if block.global_feature == featureID and block.local_feature == -1 then
      table.insert(featureBlocks, i)
    end
  end
  return featureBlocks
end

function isValidTiletype(tiletype)
  local tiletype = df.tiletype[tiletype]
  local tiletypeAttrs = df.tiletype.attrs[tiletype]
  local material = tiletypeAttrs.material
  local forbiddenMaterials = {
    df.tiletype_material.TREE, -- so as not to embark stranded on top of a tree
    df.tiletype_material.MUSHROOM,
    df.tiletype_material.FIRE,
    df.tiletype_material.CAMPFIRE
  }
  for _,forbidden in ipairs(forbiddenMaterials) do
    if material == forbidden then
      return false
    end
  end
  local shapeAttrs = df.tiletype_shape.attrs[tiletypeAttrs.shape]
  return shapeAttrs.walkable
end

function getValidEmbarkTiles(block)
  local validTiles = {} --as:{_type:table,x:number,y:number,z:number}[]
  for xi = 0,15 do
    for yi = 0,15 do
      if block.designation[xi][yi].flow_size == 0
      and isValidTiletype(block.tiletype[xi][yi]) then
        table.insert(validTiles, {x = block.map_pos.x + xi, y = block.map_pos.y + yi, z = block.map_pos.z})
      end
    end
  end
  return validTiles
end

function blockGlowingBarrierAnnouncements(recenter)
--  temporarily disables the "glowing barrier has disappeared" announcement
--  announcement settings are restored after 1 tick
--  setting recenter to true enables recentering of game view to the announcement position
  local announcementFlags = df.global.d_init.announcements.flags.ENDGAME_EVENT_1 -- glowing barrier disappearance announcement
  local oldFlags = df.global.d_init.announcements.flags.ENDGAME_EVENT_1:new() -- backup announcement settings
  announcementFlags.DO_MEGA = false
  announcementFlags.PAUSE = false
  announcementFlags.RECENTER = recenter and true or false
  announcementFlags.A_DISPLAY = false
  announcementFlags.D_DISPLAY = recenter and true or false -- an actual announcement is required for recentering to occur
  dfhack.timeout(1,'ticks', function() -- barrier disappears after 1 tick
    announcementFlags:assign(oldFlags) -- restore announcement settings
    if recenter then
--    Remove glowing barrier notifications:
      local status = df.global.world.status
      local announcements = status.announcements
      for i = #announcements-1, 0, -1 do
        if string.find(announcements[i].text,"glowing barrier has disappeared") then
          announcements:erase(i)
          break
        end
      end
      local reports = status.reports
      for i = #reports-1, 0, -1 do
        if string.find(reports[i].text,"glowing barrier has disappeared") then
          reports:erase(i)
          break
        end
      end
      status.display_timer = 0 -- to avoid displaying other announcements
    end
  end)
end

function reveal(pos)
-- creates an unbound glowing barrier at the target location
-- so as to trigger tile revelation when it disappears 1 tick later (fortress mode only)
-- should be run in conjunction with blockGlowingBarrierAnnouncements()
  local x,y,z = pos2xyz(pos)
  local block = dfhack.maps.getTileBlock(x,y,z)
  local tiletype = block.tiletype[x%16][y%16]
  if tiletype ~= df.tiletype.GlowingBarrier then -- to avoid multiple instances
    block.tiletype[x%16][y%16] = df.tiletype.GlowingBarrier
    local barriers = df.global.world.glowing_barriers
    local barrier = df.glowing_barrier:new()
    barrier.buildings:insert('#',-1) -- being unbound to a building makes the barrier disappear immediately
    barrier.pos:assign(pos)
    barriers:insert('#',barrier)
    local hfs = df.glowing_barrier:new()
    hfs.triggered = true -- this prevents HFS events (which can otherwise be triggered by the barrier disappearing)
    barriers:insert('#',hfs)
    dfhack.timeout(1,'ticks', function() -- barrier tiletype disappears after 1 tick
      block.tiletype[x%16][y%16] = tiletype -- restore old tiletype
      barriers:erase(#barriers-1) -- remove hfs blocker
      barriers:erase(#barriers-1) -- remove revelation barrier
    end)
  end
end

function moveEmbarkStuff(selectedBlock, embarkTiles)
  local spawnPosCentre
  for _, hotkey in ipairs(df.global.plotinfo.main.hotkeys) do
    if hotkey.name == "Wagon arrival location" then -- the preset hotkey is centred around the spawn point
      spawnPosCentre = xyz2pos(hotkey.x, hotkey.y, hotkey.z)
      hotkey:assign(embarkTiles[math.random(1, #embarkTiles)]) -- set the hotkey to the new spawn point
      break
    end
  end

-- only target things within this zone to help avoid teleporting non-embark stuff:
-- the following values might need to be modified
  local x1 = spawnPosCentre.x - 15
  local x2 = spawnPosCentre.x + 15
  local y1 = spawnPosCentre.y - 15
  local y2 = spawnPosCentre.y + 15
  local z1 = spawnPosCentre.z - 3 -- units can be spread across multiple z-levels when embarking on a mountain
  local z2 = spawnPosCentre.z + 3

-- Move citizens and pets:
  local unitsAtSpawn = dfhack.units.getUnitsInBox(x1,y1,z1,x2,y2,z2)
  local movedUnit = false
  for i, unit in ipairs(unitsAtSpawn) do
    if unit.civ_id == df.global.plotinfo.civ_id and not unit.flags1.inactive and not unit.flags2.killed then
      local pos = embarkTiles[math.random(1, #embarkTiles)]
      dfhack.units.teleport(unit, pos)
      reveal(pos)
      movedUnit = true
    end
  end
  if movedUnit then
    blockGlowingBarrierAnnouncements(true) -- this is separate from the reveal() function as it only needs to be called once per tick, regardless of how many times reveal() has been run
  end

-- Move wagon contents:
  local wagonFound = false
  for _, wagon in ipairs(df.global.world.buildings.other.WAGON) do --as:df.building_wagonst
    if wagon.age == 0 then -- just in case there's an older wagon present for some reason
      local contained = wagon.contained_items
      for i = #contained-1, 0, -1 do
        if contained[i].use_mode == 0 then -- actual contents (as opposed to building components)
          local item = contained[i].item
--        dfhack.items.moveToGround() does not handle items within buildings, so do this manually:
          contained:erase(i)
          for k = #item.general_refs-1, 0, -1 do
            if item.general_refs[k]._type == df.general_ref_building_holderst then
              item.general_refs:erase(k)
            end
          end
          item.flags.in_building = false
          item.flags.on_ground = true
          local pos = embarkTiles[math.random(1, #embarkTiles)]
          item.pos:assign(pos)
          selectedBlock.items:insert('#', item.id)
          selectedBlock.occupancy[pos.x%16][pos.y%16].item = true
        end
      end
      dfhack.buildings.deconstruct(wagon)
      wagon.flags.almost_deleted = true -- wagon vanishes a tick later
      wagonFound = true
      break
    end
  end

-- Move items scattered around the spawn point if there's no wagon:
  if not wagonFound then
    for _, item in ipairs(df.global.world.items.other.IN_PLAY) do
      local flags = item.flags
      if item.age == 0 -- embark equipment consists of newly created items
      and item.pos.x >= x1 and item.pos.x <= x2
      and item.pos.y >= y1 and item.pos.y <= y2
      and item.pos.z >= z1 and item.pos.z <= z2
      and flags.on_ground
      and not flags.in_inventory
      and not flags.in_building
      and not flags.in_chest
      and not flags.construction
      and not flags.spider_web
      and not flags.encased then
        dfhack.items.moveToGround(item, embarkTiles[math.random(1, #embarkTiles)])
      end
    end
  end
end

function deepEmbark(cavernType, blockDemons)
  if not cavernType then
    qerror('Cavern type not specified!')
  end

  local cavernBlocks = getFeatureBlocks(getFeatureID(cavernType))
  if #cavernBlocks == 0 then
    qerror(cavernType .. " not found!")
  end

  local moved = false
  for n = 1, #cavernBlocks do
    local i = math.random(1, #cavernBlocks)
    local selectedBlock = df.global.world.map.map_blocks[cavernBlocks[i]]
    local embarkTiles = getValidEmbarkTiles(selectedBlock)
    if #embarkTiles >= 20 then -- value chosen arbitrarily; might want to increase/decrease (determines how cramped the embark spot is allowed to be)
      moveEmbarkStuff(selectedBlock, embarkTiles)
      moved = true
      break
    end
    table.remove(cavernBlocks, i)
  end
  if not moved then
    qerror('Insufficient space at ' .. cavernType)
  end

  if blockDemons then
    disableSpireDemons()
  end
end

function disableSpireDemons()
--  marks underworld spires on the map as having been breached already, preventing HFS events
  for _, spire in ipairs(df.global.world.deep_vein_hollows) do
    spire.triggered = true
  end
end

function inEmbarkMode()
  if df.global.gametype ~= df.game_type.DWARF_MAIN then -- is always set at fortress mode setup
    return false
  end
  local embarkViewScreens = {
    df.viewscreen_adopt_regionst, -- onLoad.init kicks in early; this is the viewscreen present at this stage (the 'loading world' viewscreen is also present at adventure mode setup and legends mode, hence the game_type check above)
    df.viewscreen_choose_start_sitest,
    df.viewscreen_setupdwarfgamest
  }
  local view = dfhack.gui.getCurViewscreen()
  for _, valid in ipairs(embarkViewScreens) do
    if view._type == valid or view.parent._type == valid and view._type ~= df.viewscreen_textviewerst then -- df.viewscreen_textviewerst is present right after embarking (displays the embark message) and has .parent._type == df.viewscreen_setupdwarfgamest
      return true
    end
  end
  return false
end

local validArgs = utils.invert({
  'depth',
  'atReclaim',
  'blockDemons',
  'clear',
  'help'
})
local args = utils.processArgs({...}, validArgs)

if moduleMode then
  return
end

if args.help then
  print(dfhack.script_help())
  return
end

if args.clear then
  dfhack.onStateChange.DeepEmbarkMonitor = nil
  print("Cleared settings; now embarking normally.")
  return
end

if not args.depth then
  qerror('Depth not specified! Enter "help deep-embark" for more information.')
end

local validDepths = {
  ["CAVERN_1"] = true,
  ["CAVERN_2"] = true,
  ["CAVERN_3"] = true,
  ["UNDERWORLD"] = true
}

if not validDepths[args.depth] then
  qerror("Invalid depth: " .. args.depth)
end

local consoleMode = dfhack.is_interactive() -- true if the script has been called directly from the DFHack console, false if called from onLoad.init

if consoleMode and not inEmbarkMode() then
  -- if running from the console (not onLoad.init), abort if not currently in an embark viewscreen.
  qerror('When run from the command line, this script should be run during the embark setup screens. Enter "help deep-embark" for more information.')
end

if consoleMode then
  print("Embarking at: " .. tostring(args.depth))
end

dfhack.onStateChange.DeepEmbarkMonitor = function(event)
  if event == SC_VIEWSCREEN_CHANGED then -- I initially tried using SC_MAP_LOADED, but the map appears to be loaded too early when reclaiming sites
    local view = dfhack.gui.getCurViewscreen()
    if not consoleMode and not args.atReclaim and df.global.gametype == df.game_type.DWARF_RECLAIM then -- it's assumed that a player who chooses to run the script from console whilst reclaiming knows what they're doing, so there's no need to check for -atReclaim in this scenario
      dfhack.onStateChange.DeepEmbarkMonitor = nil -- stop monitoring
      return -- don't deepEmbark if running from onLoad.init and in reclaim mode without -atReclaim
    elseif view._type == df.viewscreen_choose_start_sitest then -- on embark screen
      if view.choosing_embark or view.choosing_reclaim then -- on a fresh embark, or on a reclaim
        deepEmbark(args.depth, args.blockDemons)
        dfhack.onStateChange.DeepEmbarkMonitor = nil
      end
    elseif view._type == df.viewscreen_dwarfmodest then -- we're in game. If we got here then we never got an embark screen, so this is loading a save and we abort.
      dfhack.onStateChange.DeepEmbarkMonitor = nil
    end
  elseif event == SC_WORLD_UNLOADED then -- embark aborted
    dfhack.onStateChange.DeepEmbarkMonitor = nil
  end
end
