-- scripts/remove-wear.lua
-- Resets all items in your fort to 0 wear
-- original author: Laggy
-- edited by expwnent
--[[=begin

remove-wear
===========
Sets the wear on all items in your fort to zero.

=end]]

local args = {...}

if args[1] == 'help' then
 print([[remove-wear - this script removes wear from all items, or from individual ones

remove-wear all
 remove wear from all items
remove-wear n1 n2 n3 ...
 remove wear from items with the given ids. order does not matter
repeat -time 2 months -command remove-wear all
 remove wear from all items every 2 months. see repeat.lua for details
]])
 do return end
elseif args[1] == 'all' then
 local count = 0;
 for _,item in ipairs(df.global.world.items.all) do
  if (item.wear > 0) then
   item:setWear(0)
   count = count+1
  end
 end
 print('remove-wear removed wear from '..count..' objects')
else
 local argIndex = 1
 local isCompleted = {}
 for i,x in ipairs(args) do
  args[i] = tonumber(x)
 end
 table.sort(args)
 for _,item in ipairs(df.global.world.items.all) do
  local function loop()
   if argIndex > #args then
    return
   elseif item.id > args[argIndex] then
    argIndex = argIndex+1
    loop()
    return
   elseif item.id == args[argIndex] then
    --print('removing wear from item with id ' .. args[argIndex])
    item:setWear(0)
    isCompleted[args[argIndex]] = true
    argIndex = argIndex+1
   end
  end
  loop()
 end
 for _,arg in ipairs(args) do
  if isCompleted[arg] ~= true then
   print('failed to remove wear from item ' .. arg .. ': could not find item with that id')
  end
 end
end
