-- Puts out fires.
-- author: Atomic Chicken
--@ module = true

local utils = require 'utils'
local validArgs = utils.invert({
  'all',
  'location',
  'help'
})
local args = utils.processArgs({...}, validArgs)

local usage = [====[

extinguish
==========
This script allows the user to put out fires affecting
map tiles, plants, units, items and buildings.

To select a target, place the cursor over it before running the script.
Alternatively, use one of the arguments below.

Arguments::

    -all
        extinguish everything on the map

    -location [ x y z ]
        extinguish only at the specified coordinates
]====]

if args.help then
  print(usage)
  return
end

if args.all and args.location then
  qerror('-all and -location cannot be specified together.')
end

function extinguishTiletype(tiletype)
  if tiletype == df.tiletype['Fire'] or tiletype == df.tiletype['Campfire'] then
    return df.tiletype['Ashes'..math.random(1,3)]
  elseif tiletype == df.tiletype['BurningTreeTrunk'] then
    return df.tiletype['TreeDeadTrunkPillar']
  elseif tiletype == df.tiletype['BurningTreeBranches'] then
    return df.tiletype['TreeDeadBranches']
  elseif tiletype == df.tiletype['BurningTreeTwigs'] then
    return df.tiletype['TreeDeadTwigs']
  elseif tiletype == df.tiletype['BurningTreeCapWall'] then
    return df.tiletype['TreeDeadCapPillar']
  elseif tiletype == df.tiletype['BurningTreeCapRamp'] then
    return df.tiletype['TreeDeadCapRamp']
  elseif tiletype == df.tiletype['BurningTreeCapFloor'] then
    return df.tiletype['TreeDeadCapFloor'..math.random(1,4)]
  else
    return tiletype
  end
end

function extinguishTile(x,y,z)
  local tileBlock = dfhack.maps.getTileBlock(x,y,z)
  tileBlock.tiletype[x%16][y%16] = extinguishTiletype(tileBlock.tiletype[x%16][y%16])
  tileBlock.temperature_1[x%16][y%16] = 10050 -- chosen as a 'standard' value; it'd be more ideal to calculate it with respect to the region, season, undergound status, etc (but DF does this for us when updating temperatures)
  tileBlock.temperature_2[x%16][y%16] = 10050
  tileBlock.flags.update_temperature = true
  tileBlock.flags.update_liquid = true
  tileBlock.flags.update_liquid_twice = true
end

function extinguishContaminant(spatter)
-- reset temperature of any contaminants to prevent them from causing reignition
-- (just in case anyone decides to play around with molten gold or whatnot)
  spatter.temperature.whole = 10050
  spatter.temperature.fraction = 0
end

function extinguishItem(item)
  local item = item --as:df.item_actual
  if item.flags.on_fire then
    item.flags.on_fire = false
    item.temperature.whole = 10050
    item.temperature.fraction = 0
    item.flags.temps_computed = false
    if item.contaminants then
      for _,spatter in ipairs(item.contaminants) do
        extinguishContaminant(spatter)
      end
    end
  end
end

function extinguishUnit(unit)
-- based on full-heal.lua
  local burning = false
  for _, status in ipairs(unit.body.components.body_part_status) do
    if not burning then
      if status.on_fire then
        burning = true
        status.on_fire = false
      end
    else
      status.on_fire = false
    end
  end
  if burning then
    for i = #unit.status2.body_part_temperature-1,0,-1 do
      unit.status2.body_part_temperature:erase(i)
    end
    unit.flags2.calculated_nerves = false
    unit.flags2.calculated_bodyparts = false
    unit.flags2.calculated_insulation = false
    unit.flags3.body_temp_in_range = false
    unit.flags3.compute_health = true
    unit.flags3.dangerous_terrain = false
    for _,spatter in ipairs(unit.body.spatters) do
      extinguishContaminant(spatter)
    end
  end
end

function extinguishAll()
  local fires = df.global.world.fires
  for i = #fires-1,0,-1 do
    extinguishTile(pos2xyz(fires[i].pos))
    fires:erase(i)
  end
  local campfires = df.global.world.campfires
  for i = #campfires-1,0,-1 do
    extinguishTile(pos2xyz(campfires[i].pos))
    campfires:erase(i)
  end
  for _,plant in ipairs(df.global.world.plants.all) do
    plant.damage_flags.is_burning = false
  end
  for _,item in ipairs(df.global.world.items.all) do
    extinguishItem(item)
  end
  for _,unit in ipairs(df.global.world.units.active) do
    extinguishUnit(unit)
  end
end

function extinguishLocation(x,y,z)
  local pos = xyz2pos(x,y,z)
  local fires = df.global.world.fires
  for i = #fires-1,0,-1 do
    if same_xyz(pos, fires[i].pos) then
      extinguishTile(x,y,z)
      fires:erase(i)
    end
  end
  local campfires = df.global.world.campfires
  for i = #campfires-1,0,-1 do
    if same_xyz(pos, campfires[i].pos) then
      extinguishTile(x,y,z)
      campfires:erase(i)
    end
  end
  local units = dfhack.units.getUnitsInBox(x,y,z,x,y,z)
  for _,unit in ipairs(units) do
    extinguishUnit(unit)
  end
  for _,item in ipairs(df.global.world.items.all) do
    if same_xyz(pos, item.pos) then
      extinguishItem(item)
    end
  end
end

if not dfhack_flags.module then
  if args.all then
    extinguishAll()
  elseif args.location then
    if dfhack.maps.isValidTilePos(args.location[1],args.location[2],args.location[3]) then
      extinguishLocation(tonumber(args.location[1]), tonumber(args.location[2]), tonumber(args.location[3]))
    else
      qerror("Invalid location coordinates!")
    end
  else
    local x,y,z = pos2xyz(df.global.cursor)
    if not x then
      qerror("Choose a target via the cursor or the -location argument, or specify -all to extinguish everything on the map.")
    end
    extinguishLocation(x,y,z)
  end
end
