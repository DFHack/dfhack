--print-args2.lua
--author expwnent
--[====[

devel/print-args2
=================
Prints all the arguments you supply to the script on their own line
with quotes around them.

]====]

local args = {...}
print("print-args")
for _,arg in ipairs(args) do
 print("'"..arg.."'")
end
