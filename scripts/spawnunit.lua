-- create unit at pointer or given location
-- wraps modtools/create-unit.lua
--[[=begin

spawnunit
=========
``spawnunit RACE CASTE`` creates a unit of the given race and caste at the cursor, by
wrapping `modtools/create-unit`.  Run ``spawnunit help`` for more options.

=end]]

usage = [[Usage:
- spawnunit RACE CASTE
- spawnunit RACE CASTE NAME
- spawnunit RACE CASTE NAME x y z
- spawnunit RACE CASTE [NAME] [additional arguments]
    Create a unit
- spawnunit -help
    Display this help message
- spawnunit -command ARGUMENTS
    Display the command used to invoke modtools/create-unit

additional arguments are passed to modtools/create-unit -
see "modtools/create-unit -help" for more information.
]]

function extend(tbl, tbl2)
    for _, v in pairs(tbl2) do
        table.insert(tbl, v)
    end
end

local show_command = false
local args = {...}
local first_arg = (args[1] or ''):gsub('^-*', '')
if first_arg == 'help' or #args < 2 then
    print(usage)
    return
elseif first_arg == 'command' then
    show_command = true
    table.remove(args, 1)
end

local new_args = {'-race', args[1], '-caste', args[2]}
if #args == 3 then
    extend(new_args, {'-nick', args[3]})
elseif #args == 6 then
    if tonumber(args[4]) and tonumber(args[5]) and tonumber(args[6]) then
        extend(new_args, {'-nick', args[3], '-location', '[', args[4], args[5], args[6], ']'})
    end
else
    local start = 3
    if #args >= 3 and args[3]:sub(1, 1) ~= '-' then
        extend(new_args, {'-nick', args[3]})
        start = 4
    end
    for i = start, #args do
        table.insert(new_args, args[i])
    end
end
if show_command then
    print('modtools/create-unit ' .. table.concat(new_args, ' '))
    return
end
dfhack.run_script('modtools/create-unit', table.unpack(new_args))
