-- Check for common mistakes in raw files
--@ enable = true
--[====[

modtools/raw-lint
=================
Checks for simple issues with raw files. Can be run automatically.

]====]

local utils = require 'utils'

enabled = enabled or false

if dfhack.filesystem == nil or dfhack.filesystem.listdir_recursive == nil then
    qerror('This script requires DFHack 0.40.24-r2 or newer')
end

local perr_prefix = ''
function perr(msg, ...)
    dfhack.printerr((#perr_prefix > 0 and perr_prefix .. ': ' or '') .. tostring(msg):format(...))
end

local valid_objnames = utils.invert{
    'BODY_DETAIL_PLAN',
    'BODY',
    'BUILDING',
    'CREATURE_VARIATION',
    'CREATURE',
    'DESCRIPTOR_COLOR',
    'DESCRIPTOR_PATTERN',
    'DESCRIPTOR_SHAPE',
    'ENTITY',
    'INORGANIC',
    'INTERACTION',
    'ITEM',
    'LANGUAGE',
    'MATERIAL_TEMPLATE',
    'PLANT',
    'REACTION',
    'TISSUE_TEMPLATE',
}

local objname_overrides = {
    b_detail_plan = 'BODY_DETAIL_PLAN',
    c_variation = 'CREATURE_VARIATION',
}

function check_file(path)
    local filename = path:match('([^./]+).txt')
    if filename == nil then return perr('Unrecognized filename') end
    local f = io.open(path, 'r')
    if f == nil then return perr('Unable to open file') end
    local contents = f:read('*all')
    f:close()
    local realname = contents:sub(1, contents:find('%s') - 1)
    if realname ~= filename then
        perr('Name mismatch: expected %s, found %s', filename, realname)
    end
    local objname = filename
    local check_objnames = {} --as:string[]
    for k, v in pairs(objname_overrides) do
        if filename:sub(1, #k) == k and valid_objnames[v] ~= nil then
            table.insert(check_objnames, v)
        end
    end
    local start = filename:find('_')
    while start ~= nil do
        local part = filename:sub(1, filename:find('_', start) - 1):upper()
        if valid_objnames[part] ~= nil then
            table.insert(check_objnames, part)
        end
        start = filename:find('_', start + 1)
    end
    if #check_objnames > 0 then
        local found = false
        for i, objname in ipairs(check_objnames) do
            objname = '[OBJECT:' .. objname:upper() .. ']'
            if contents:find(objname, 1, true) ~= nil then
                found = true
            end
            check_objnames[i] = objname
        end
        if not found then
            perr('None of %s found', table.concat(check_objnames, ', '))
        end
    else
        perr('No valid object names')
    end
end

function check_folder(path)
    local files = dfhack.filesystem.listdir_recursive(path)
    if files == nil then qerror('Failed to list directory: ' .. path) end
    for i, entry in ipairs(files) do
        if entry.isdir == false and entry.path:sub(-4) == '.txt' and
                entry.path:find('/notes/') == nil and
                entry.path:find('/text/') == nil and
                entry.path:find('/examples and notes/') == nil then
            perr_prefix = entry.path
            check_file(entry.path)
            perr_prefix = ''
        end
    end
end

function check(global, save)
    if global then
        check_folder('raw/objects')
    end
    if save and dfhack.isWorldLoaded() and dfhack.getSavePath() ~= nil then
        check_folder(dfhack.getSavePath() .. '/raw/objects')
    end
end

dfhack.onStateChange.raw_lint = function(event)
    if not enabled then return end
    if event == SC_WORLD_LOADED then
        check(false, true)
    end
end

local args = {...}
if dfhack_flags and dfhack_flags.enable then
    table.insert(args, dfhack_flags.enable_state and 'enable' or 'disable')
end
if args[1] == 'enable' then
    enabled = true
    check(true, true)
elseif args[1] == 'disable' then
    enabled = false
elseif args[1] == 'help' then
    print('Usage: raw-lint.lua enable|disable|help')
else
    check(true, true)
end
