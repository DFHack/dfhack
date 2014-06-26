--printArgs.lua
--author expwnent
--prints all the arguments on their own line with quotes around them. useful for debugging

local args = {...}
print("printArgs")
for _,arg in ipairs(args) do
 print("'"..arg.."'")
end
