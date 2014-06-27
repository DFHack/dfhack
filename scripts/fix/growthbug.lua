--growthbug: units only grow when the current tick is 0 mod 10, so only 1/10 units will grow naturally. this script periodically sets the birth time of each unit so that it will grow
--to run periodically, use "repeat -time 2 months -command fix/growthBug now". see repeat.lua for details
-- author expwnent

local args = {...}
if args[1] ~= nil then
 print("fix/growthbug usage")
 print(" fix/growthbug")
 print("  fix the growth bug for all units on the map")
 print(" fix/growthbug help")
 print("  print this help message")
 print(" repeat -time [n] [years/months/ticks/days/etc] -command fix/growthbug")
 print("  run this script every n time units")
 print(" repeat -cancel fix/growthbug")
 print("  stop automatically running this script")
end

local count = 0
for _,unit in ipairs(df.world.units.all) do
 local offset = unit.relations.birth_time % 10;
 if offset ~= 0 then
  count = count+1
  unit.relations.birth_time = unit.relations.birth_time - offset
 end
end
print("Fixed growth bug for "..count.." units.")
