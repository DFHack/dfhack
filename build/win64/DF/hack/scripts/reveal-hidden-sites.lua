-- Expose all undiscovered sites.
-- author: Atomic Chicken

--[====[

reveal-hidden-sites
===================
This script reveals all sites in the world
that have yet to be discovered by the player
(camps, lairs, shrines, vaults, etc)
thus making them visible on the map.

Usable in both fortress and adventure mode.

See `reveal-adv-map` if you also want to expose
hidden world map tiles in adventure mode.

]====]

local count = 0
for _, site in ipairs(df.global.world.world_data.sites) do
  if site.flags.Undiscovered then
    site.flags.Undiscovered = false
    count = count + 1
  end
end

if count == 0 then
  print("No hidden sites detected!")
else
  print("Exposed " .. count .. " site" .. (count == 1 and "" or "s") .. "!")
end
