--print-args.lua
--author expwnent
--[[=begin

devel/print-args
================
Prints all the arguments you supply to the script on their own line.
Useful for debugging other scripts.

=end]]

local args = {...}
for _,arg in ipairs(args) do
 print(arg)
end
