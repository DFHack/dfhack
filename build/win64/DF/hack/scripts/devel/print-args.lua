--print-args.lua
--author expwnent
--[====[

devel/print-args
================
Prints all the arguments you supply to the script on their own line.
Useful for debugging other scripts.

]====]

local args = {...}
for _,arg in ipairs(args) do
 print(arg)
end
