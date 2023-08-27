-- Changes the first name of all units to "Bob"
--author expwnent
--
--[====[

devel/all-bob
=============
Changes the first name of all units to "Bob".
Useful for testing `modtools/interaction-trigger` events.

]====]

for _,v in ipairs(df.global.world.units.all) do
 v.name.first_name = "Bob"
end
