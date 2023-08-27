-- create unit at pointer or given location
-- wraps modtools/create-unit.lua

local guidm = require('gui.dwarfmode')
local utils = require('utils')

function extend(tbl, tbl2)
    for _, v in pairs(tbl2) do
        table.insert(tbl, v)
    end
end

local show_command = false
local args = {...}
local first_arg = (args[1] or ''):gsub('^-*', '')
if first_arg == 'help' or #args < 2 then
    print(dfhack.script_help())
    return
elseif first_arg == 'command' then
    show_command = true
    table.remove(args, 1)
end

local new_args = {'-race', args[1], '-caste', args[2]}
if #args == 3 then
    extend(new_args, {'-nick', args[3]})
elseif #args == 6 and tonumber(args[4]) and tonumber(args[5]) and tonumber(args[6]) then
    extend(new_args, {'-nick', args[3], '-location', '[', args[4], args[5], args[6], ']'})
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

if not utils.invert(new_args)['-location'] then
    local cursor = guidm.getCursorPos() or qerror('This script requires an active cursor.')
    extend(new_args, {'-location', '[', tostring(cursor.x), tostring(cursor.y), tostring(cursor.z), ']'})
end

if show_command then
    print('modtools/create-unit ' .. table.concat(new_args, ' '))
    return
end
dfhack.run_script('modtools/create-unit', table.unpack(new_args))
