
local onStrike = require 'plugins.onStrike'
local eventful = require 'plugins.eventful'
--print(onStrike)

--onStrike.triggers.onStrikeExample = function(information)
-- print(information.attacker .. ' attacks ' .. information.defender .. ': ' .. information.announcement)
onStrike.triggers.onStrikeExample = function(attacker, defender, weapon, wound)
 if weapon then
  print(attacker.id..' weapon attacks '..defender.id .. ' with ' .. weapon.id)
  --df.global.pause_state = true
 else
  print(attacker.id..' attacks '..defender.id)
 end
end

