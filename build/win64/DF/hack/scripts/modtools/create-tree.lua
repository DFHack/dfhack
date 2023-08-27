-- Create a specific tree.
-- Author: Atomic Chicken

--@ module = true

local usage = [====[

modtools/create-tree
====================
Spawns a tree.

Usage::

    -tree treeName
        specify the tree to be created
        examples:
            OAK
            NETHER_CAP

    -age howOld
        set the age of the tree in years (integers only)
        defaults to 1 if omitted

    -location [ x y z ]
        create the tree at the specified coordinates

    example:
        modtools/create-tree -tree OAK -age 100 -location [ 33 145 137 ]
]====]

local utils = require 'utils'

function createTree(...)
  local old_gametype = df.global.gametype
  local old_mode = df.global.plotinfo.main.mode
  local old_popups = {} --as:df.popup_message[]
  for _, popup in pairs(df.global.world.status.popups) do
    table.insert(old_popups, popup)
  end
  df.global.world.status.popups:resize(0)

  local ok, ret = dfhack.pcall(createTreeInner, ...)

  df.global.gametype = old_gametype
  df.global.plotinfo.main.mode = old_mode
  for _, popup in pairs(old_popups) do
    df.global.world.status.popups:insert('#', popup)
  end

  if not ok then
    error(ret)
  end

  return ret
end

function createTreeInner(treeRaw, treeAge, treePos)
  local gui = require 'gui'

  if not dfhack.maps.isValidTilePos(treePos[1], treePos[2], treePos[3]) then
    qerror("Invalid location coordinates!")
  end

  local view_x = df.global.window_x
  local view_y = df.global.window_y
  local view_z = df.global.window_z

  local curViewscreen = dfhack.gui.getCurViewscreen()
  local dwarfmodeScreen = df.viewscreen_dwarfmodest:new()
  curViewscreen.child = dwarfmodeScreen
  dwarfmodeScreen.parent = curViewscreen
  df.global.plotinfo.main.mode = df.ui_sidebar_mode.LookAround

  local arenaSettings = df.global.world.arena_settings
  local oldTreeCursor = arenaSettings.tree_cursor
  local oldTreeFilter = arenaSettings.tree_filter
  local oldTreeAge = arenaSettings.tree_age

  arenaSettings.tree_cursor = 0
  arenaSettings.tree_filter = ""
  arenaSettings.tree_age = treeAge

  df.global.gametype = df.game_type.DWARF_ARENA
  gui.simulateInput(dwarfmodeScreen, 'D_LOOK_ARENA_TREE')

  df.global.cursor.x = tonumber(treePos[1])
  df.global.cursor.y = tonumber(treePos[2])
  df.global.cursor.z = tonumber(treePos[3])

  local spawnScreen = dfhack.gui.getCurViewscreen()

  arenaSettings.tree_types:insert(0, treeRaw)
  gui.simulateInput(spawnScreen,'SELECT')
  arenaSettings.tree_types:erase(0)

  curViewscreen.child = nil
  dwarfmodeScreen:delete()

  arenaSettings.tree_cursor = oldTreeCursor
  arenaSettings.tree_filter = oldTreeFilter
  arenaSettings.tree_age = oldTreeAge

  df.global.window_x = view_x
  df.global.window_y = view_y
  df.global.window_z = view_z
end

local validArgs = utils.invert({
  'help',
  'tree',
  'age',
  'location'
})

if moduleMode then
  return
end

local args = utils.processArgs({...}, validArgs)

if args.help then
  print(usage)
  return
end

if not args.tree then
  qerror("Tree name not specified!")
end

local treeRaw
for _, tree in ipairs(df.global.world.raws.plants.trees) do
  if tree.id == args.tree then
    treeRaw = tree
    break
  end
end

if not treeRaw then
  qerror("Invalid tree name: " .. args.tree)
end

local treeAge
if args.age then
  treeAge = tonumber(args.age)
  if not treeAge or treeAge < 0 then
    qerror("Invalid age: " .. args.age)
  end
else
  treeAge = 1
end

local treePos
if args.location then
  treePos = args.location
else
  qerror("Location not specified!")
end

createTree(treeRaw, treeAge, treePos)
