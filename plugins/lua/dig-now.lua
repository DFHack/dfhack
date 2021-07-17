local _ENV = mkmodule('plugins.dig-now')

local argparse = require('argparse')
local guidm = require('gui.dwarfmode')
local utils = require('utils')

local short_help_text = [=[

dig-now
=======

Instantly completes dig designations, modifying map tiles and creating boulders,
ores, and gems as if a miner were doing the mining or engraving. By default, all
dig designations on the map are completed and boulder generation follows
standard game rules, but the behavior is configurable.

Usage:

    dig-now [<pos> [<pos>]] [<options>]

Examples:

dig-now
    Dig all designated tiles according to standard game rules.

dig-now --clean
    Dig designated tiles, but don't generate any boulders, ores, or gems.

dig-now --dump here
    Dig tiles and dump all generated boulders, ores, and gems at the tile under
    the game cursor.

See the online DFHack documentation for details on all options.
]=]

function print_help() print(short_help_text) end

local function parse_coords(opts, configname, arg)
    local cursor = argparse.coords(arg, configname)
    utils.assign(opts[configname], cursor)
end

local function parse_percentages(opts, arg)
    local nums = argparse.numberList(arg, 'percentages', 4)
    for _,percentage in ipairs(nums) do
        if percentage < 0 or percentage > 100 then
            qerror(('invalid percentages: "%s"; expected format is "<layer>,' ..
                    '<vein>,<small cluster>,<deep>", where each number is'..
                    ' between 0 and 100, inclusive (e.g. "0,33,100,100")')
                   :format(arg))
        end
    end
    local config = opts.boulder_percents
    config.layer, config.vein, config.small_cluster, config.deep =
            nums[1], nums[2], nums[3], nums[4]
end

local function min_to_max(...)
    local args = {...}
    table.sort(args, function(a, b) return a < b end)
    return table.unpack(args)
end

function parse_commandline(opts, ...)
    local use_zlevel = false
    local positionals = argparse.processArgsGetopt({...}, {
            {'c', 'clean',
             handler=function() parse_percentages(opts, '0,0,0,0') end},
            {'d', 'dump', hasArg=true,
             handler=function(arg) parse_coords(opts, 'dump_pos', arg) end},
            {'e', 'everywhere',
             handler=function() parse_percentages(opts, '100,100,100,100') end},
            {'h', 'help', handler=function() opts.help = true end},
            {'p', 'percentages', hasArg=true,
             handler=function(arg) parse_percentages(opts, arg) end},
            {'z', 'cur-zlevel', handler=function() use_zlevel = true end},
        })

    if positionals[1] == 'help' then opts.help = true end
    if opts.help then return end

    if use_zlevel then
        local x, y, z = df.global.world.map.x_count - 1,
                df.global.world.map.y_count - 1,
                df.global.window_z
        parse_coords(opts, 'start', ('0,0,%d'):format(z))
        parse_coords(opts, 'end', ('%d,%d,%d'):format(x, y, z))
    elseif #positionals >= 1 then
        parse_coords(opts, 'start', positionals[1])
        if #positionals >= 2 then
            parse_coords(opts, 'end', positionals[2])
            opts.start.x, opts['end'].x = min_to_max(opts.start.x,opts['end'].x)
            opts.start.y, opts['end'].y = min_to_max(opts.start.y,opts['end'].y)
            opts.start.z, opts['end'].z = min_to_max(opts.start.z,opts['end'].z)
        else
            utils.assign(opts['end'], opts.start)
        end
    end
end

return _ENV
