--all-bob.lua
--author expwnent
--names all units bob. Useful for testing interaction trigger events

for _,v in ipairs(df.global.world.units.all) do
 v.name.first_name = "Bob"
end
