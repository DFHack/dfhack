-- Exposes/hides the entire world map in adventure mode.
-- author: Atomic Chicken

--@ module = true

local usage = [====[

reveal-adv-map
==============

This script can be used to either reveal or hide all tiles on the
world map in adventure mode (visible when viewing the quest log
or fast travelling, for example).

Note that the script does not reveal hidden lairs, camps, etc.
See `reveal-hidden-sites` for this functionality.

Arguments::

    -hide
        Include this if you want to hide all world tiles instead
        of revealing them

]====]

local utils = require 'utils'

function revealAdvMap(hide)
  local world = df.global.world.world_data
  for world_x = 0, world.world_width-1, 1 do
    for world_y = 0, world.world_height-1, 1 do
      df.global.world.world_data.region_map[world_x]:_displace(world_y).flags.discovered = not hide
    end
  end
-- update the quest log configuration if it is already open (restricts map cursor movement):
  local view = dfhack.gui.getCurViewscreen()
  if view._type == df.viewscreen_adventure_logst then
    local player = view.player_region
    if hide then
      view.cursor.x = player.x
      view.cursor.y = player.y
    end
    view.min_discovered.x = (hide and player.x) or 0
    view.min_discovered.y = (hide and player.y) or 0
    view.max_discovered.x = (hide and player.x) or world.world_width-1
    view.max_discovered.y = (hide and player.y) or world.world_height-1
  end
end

local validArgs = utils.invert({
  'hide',
  'help'
})
local args = utils.processArgs({...}, validArgs)

if dfhack_flags.module then
  return
end

if args.help then
  print(usage)
  return
end

if df.global.gamemode ~= df.game_mode.ADVENTURE then
  qerror("This script can only be used in adventure mode!")
end

revealAdvMap(args.hide and true)
