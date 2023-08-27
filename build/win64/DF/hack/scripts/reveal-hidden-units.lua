-- Expose all sneaking units.
-- author: Atomic Chicken

--[====[

reveal-hidden-units
===================
This script exposes all units on the map who
are currently sneaking or waiting in ambush
and thus hidden from the player's view.

Usable in both fortress and adventure mode.

]====]

local count = 0
for _, unit in ipairs(df.global.world.units.active) do
  if unit.flags1.hidden_in_ambush then
    unit.flags1.hidden_in_ambush = false
    count = count + 1
  end
end

if dfhack.is_interactive() then
  if count == 0 then
    print("No hidden units detected!")
  else
    print("Exposed " .. count .. " unit" .. (count == 1 and "" or "s") .. "!")
  end
end
