--growth-bug.lua: units only grow when the current tick is 0 mod 10, so only 1/10 units will grow naturally. this script periodically sets the birth time of each unit so that it will grow
--to run periodically, use "repeat -time 2 months -command fix/growth-bug -now". see repeat.lua for details
--author expwnent

local utils = require 'utils'

validArgs = validArgs or utils.invert({
 'help',
 'now'
})

local args = utils.processArgs({...}, validArgs)

if args.help or not next(args) then
 print("fix/growth-bug usage")
 print(" fix/growth-bug")
 print("  fix the growth bug for all units on the map")
 print(" fix/growth-bug -help")
 print("  print this help message")
 print(" repeat -time [n] [years/months/ticks/days/etc] -command fix/growth-bug now")
 print("  run this script every n time units")
 print(" repeat -cancel fix/growth-bug")
 print("  stop automatically running this script")
end

local count = 0
for _,unit in ipairs(df.global.world.units.all) do
 local offset = unit.relations.birth_time % 10;
 if offset ~= 0 then
  unit.relations.birth_time = unit.relations.birth_time - offset
  count = count+1
 end
end
print("Fixed growth bug for "..count.." units.")

