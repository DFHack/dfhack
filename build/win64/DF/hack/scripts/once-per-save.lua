-- runs dfhack commands unless ran already in this save

local HELP = [====[

once-per-save
=============
Runs commands like `multicmd`, but only unless
not already ran once in current save. You may actually
want `on-new-fortress`.

Only successfully ran commands are saved.

Parameters:

--help            display this help
--rerun commands  ignore saved commands
--reset           deletes saved commands

]====]

local STORAGEKEY_PREFIX = 'once-per-save'
local storagekey = STORAGEKEY_PREFIX .. ':' .. tostring(df.global.plotinfo.site_id)

local args = {...}
local rerun = false

local utils = require 'utils'
local arg_help = utils.invert{"?", "-?", "-help", "--help"}
local arg_rerun = utils.invert{"-rerun", "--rerun"}
local arg_reset = utils.invert{"-reset", "--reset"}
if arg_help[args[1]] then
    print(HELP)
    return
elseif arg_rerun[args[1]] then
    rerun = true
    table.remove(args, 1)
elseif arg_reset[args[1]] then
    while dfhack.persistent.delete(storagekey) do end
    table.remove(args, 1)
end
if #args == 0 then return end

local age = df.global.plotinfo.fortress_age
local year = df.global.cur_year
local year_tick = df.global.cur_year_tick
local year_tick_advmode = df.global.cur_year_tick_advmode

local function is_later(a, b)
    for i, v in ipairs(a) do
        if v < b[i] then
            return true
        elseif v > b[i] then
            return false
        --else: v == b[i] so keep iterating
        end
    end
    return false
end

local once_run = {}
if not rerun then
    local entries = dfhack.persistent.get_all(storagekey) or {}
    for i, entry in ipairs(entries) do
        local ints = entry.ints
        if ints[1] > age
        or age == 0 and is_later({ints[2], ints[3], ints[4]}, {year, year_tick, year_tick_advmode})
        then
            print (dfhack.current_script_name() .. ': unretired fortress, deleting `' .. entry.value .. '`')
            --printall_recurse(entry) -- debug
            entry:delete()
        else
            once_run[entry.value]=entry
        end
    end
end

local save = dfhack.persistent.save
for cmd in table.concat(args, ' '):gmatch("%s*([^;]+);?%s*") do
    if not once_run[cmd] then
        local ok = dfhack.run_command(cmd) == 0
        if ok then
            once_run[cmd] = save({key = storagekey,
                                  value = cmd,
                                  ints = { age, year, year_tick, year_tick_advmode }},
                                 true)
        elseif rerun and once_run[cmd] then
            once_run[cmd]:delete()
        end
    end
end
