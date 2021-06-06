local _ENV = mkmodule('plugins.dig-now')

local utils = require('utils')
local guidm = require('gui.dwarfmode')

local short_help_text = [=[

dig-now
=======

Instantly completes dig designations, modifying map tiles and creating boulders,
ores, and gems as if a miner were doing the mining or engraving. By default, all
dig designations on the map are completed and boulder generation follows
standard game rules, but the behavior is configurable.

Usage:

    dig-now [<pos> <pos>] [<options>]

Examples:

dig-now
    Dig designated tiles according to standard game rules.

dig-now --clean
    Dig designated tiles, but don't generate any boulders, ores, or gems.

dig-now --dump here
    Dig tiles and dump all generated boulders, ores, and gems at the tile under
    the game cursor.

See the online DFHack documentation for details on all options.
]=]

function print_help() print(short_help_text) end

local function parse_coords(opts, configname, arg)
    local x, y, z
    if arg == 'here' then
        local cursor = guidm.getCursorPos()
        if not cursor then
            qerror('"here" specified for position, but no game cursor found!')
        end
        x, y, z = cursor.x, cursor.y, cursor.z
    else
        _, _, x, y, z = arg:find('^%s*(%d+)%s*,%s*(%d+)%s*,%s*(%d+)%s*$')
        if not x then
            qerror(('invalid position: "%s"; expected "here" or' ..
                    ' "<x>,<y>,<z>" triple (e.g. "30,60,150")'):format(arg))
        end
    end
    opts[configname].x = tonumber(x)
    opts[configname].y = tonumber(y)
    opts[configname].z = tonumber(z)
end

local function out_of_bounds(percentage)
    percentage = tonumber(percentage)
    return percentage < 0 or percentage > 100
end

local function parse_percentages(opts, arg)
    _, _, layer, vein, small, deep =
            arg:find('^%s*(%d+)%s*,%s*(%d+)%s*,%s*(%d+)%s*,%s*(%d+)%s*$')
    if not layer or out_of_bounds(layer) or out_of_bounds(vein)
            or out_of_bounds(small) or out_of_bounds(deep) then
        qerror(('invalid percentages: "%s"; expected format is "<layer>,' ..
                '<vein>,<small cluster>,<deep>", where each number is between'..
                ' 0 and 100, inclusive (e.g. "0,33,100,100")'):format(arg))
    end
    local config = opts.boulder_percents
    config.layer, config.vein, config.small_cluster, config.deep =
            tonumber(layer), tonumber(vein), tonumber(small), tonumber(deep)
end

local function min_to_max(...)
    local args = {...}
    table.sort(args, function(a, b) return a < b end)
    return table.unpack(args)
end

function parse_commandline(opts, ...)
    local use_zlevel = false
    local positionals = utils.processArgsGetopt({...}, {
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
    elseif #positionals >= 2 then
        parse_coords(opts, 'start', positionals[1])
        parse_coords(opts, 'end', positionals[2])
        opts.start.x, opts['end'].x = min_to_max(opts.start.x, opts['end'].x)
        opts.start.y, opts['end'].y = min_to_max(opts.start.y, opts['end'].y)
        opts.start.z, opts['end'].z = min_to_max(opts.start.z, opts['end'].z)
    end
end

return _ENV
