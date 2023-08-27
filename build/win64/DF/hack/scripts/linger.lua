-- Take over your killer in adventure mode.
-- author: Atomic Chicken
-- Meant as a substitute for the long gone "reincarnate" dfusion feature.
-- calls "bodyswap.lua" to carry out the shift in unit control.

--[====[

linger
======
Enables the player to take control of their adventurer's killer.
Run this script after being presented with "You are deceased."

The killer is identified by examining the historical event
generated when the adventurer died. If this is unsuccessful,
the killer is assumed to be the last unit to have attacked the
adventurer prior to their death.

This will fail if the unit in question is no longer present
on the local map.

(Adventure mode only!)

]====]

local bodyswap = reqscript('bodyswap')

if df.global.gamemode ~= df.game_mode.ADVENTURE then
  qerror("This script can only be used in adventure mode!")
end

local adventurer = df.nemesis_record.find(df.global.adventure.player_id).unit
if not adventurer.flags2.killed then
  qerror("Your adventurer hasn't died yet!")
end

function getHistoricalSlayer(unit)
  local histFig = unit.hist_figure_id ~= -1 and df.historical_figure.find(unit.hist_figure_id)
  if not histFig then
    return
  end

  local deathEvents = df.global.world.history.events_death
  for i = #deathEvents-1,0,-1 do
    local event = deathEvents[i] --as:df.history_event_hist_figure_diedst
    if event.victim_hf == unit.hist_figure_id then
      return df.historical_figure.find(event.slayer_hf)
    end
  end
end

local slayerHistFig = getHistoricalSlayer(adventurer)
local slayer = slayerHistFig and df.unit.find(slayerHistFig.unit_id)
if not slayer then
  slayer = df.unit.find(adventurer.relationship_ids.LastAttacker)
end
if not slayer then
  qerror("Killer not found!")
elseif slayer.flags2.killed then
  local slayerName = ""
  if slayer.name.has_name then
    slayerName = ", "..dfhack.TranslateName(slayer.name)..","
  end
  qerror("Your slayer"..slayerName.." is dead!")
end

bodyswap.swapAdvUnit(slayer)
