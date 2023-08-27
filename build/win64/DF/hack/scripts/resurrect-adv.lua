-- Bring your adventurer back to life.
-- author: Atomic Chicken
-- essentially a wrapper for "full-heal.lua"

--[====[

resurrect-adv
=============
Brings a dead adventurer back to life, fully healing them
in the process.

This script only targets the current player character in
your party, and should be run after being presented with
the "You are deceased" message. It is not possible to
resurrect the adventurer after the game has been ended.

]====]

local fullHeal = reqscript('full-heal')

if df.global.gamemode ~= df.game_mode.ADVENTURE then
  qerror("This script can only be used in adventure mode!")
end

local adventurer = df.nemesis_record.find(df.global.adventure.player_id).unit
if not adventurer or not adventurer.flags2.killed then
  qerror("Your adventurer hasn't died yet!")
end

fullHeal.heal(adventurer, true)
df.global.adventure.player_control_state = 1 -- this ensures that the player will be able to regain control of their unit after resurrection if the script is run before hitting DONE at the "You are deceased" message
