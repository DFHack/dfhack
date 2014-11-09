--print-args.lua
--author expwnent
--prints all the arguments on their own line. useful for debugging

local args = {...}
for _,arg in ipairs(args) do
 print(arg)
end
