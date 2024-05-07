local _ENV = mkmodule('plugins.aquifer')

local argparse = require('argparse')
local guidm = require('gui.dwarfmode')
local utils = require('utils')

local actions = utils.invert{'list', 'add', 'drain', 'convert'}
local aq_types = utils.invert{'light', 'heavy'}

local function parse_positionals(positionals)
    local action, aq_type, pos1, pos2
    local idx = 1

    if actions[positionals[idx]] then
        action = positionals[idx]
        idx = idx + 1
    else
        action = 'list'
    end

    if aq_types[positionals[idx]] then
        aq_type = positionals[idx]
        idx = idx + 1
    end

    if positionals[idx] then
        pos1 = argparse.coords(positionals[idx])
        idx = idx + 1
    end

    if positionals[idx] then
        pos2 = argparse.coords(positionals[idx])
        idx = idx + 1
    end

    return action, aq_type, pos1, pos2
end

local function get_bounds(opts, def_all)
    local map = df.global.world.map
    local curz = df.global.window_z
    local origin = xyz2pos(0, 0, 0)
    local max = xyz2pos(map.x_count-1, map.y_count-1, map.z_count-1)

    if opts.all then
        return origin, max
    elseif opts.zdown then
        return origin, xyz2pos(max.x, max.y, curz)
    elseif opts.zup then
        return xyz2pos(origin.x, origin.y, curz), max
    elseif opts.curz then
        return xyz2pos(origin.x, origin.y, curz), xyz2pos(max.x, max.y, curz)
    end

    if def_all then
        return origin, max
    end

    qerror('Please specify a target range')
end

function parse_commandline(args)
    local opts = {
        all=false,
        zdown=false,
        zup=false,
        curz=false,
        help=false,
        skip_top=0,
        levels=nil,
        leaky=false,
        quiet=false,
    }

    local positionals = argparse.processArgsGetopt(args, {
        {'a', 'all', handler=function() opts.all = true end},
        {'d', 'zdown', handler=function() opts.zdown = true end},
        {'h', 'help', handler=function() opts.help = true end},
        {'l', 'levels', hasArg=true, handler=function(optarg) opts.levels = argparse.positiveInt(optarg, 'levels') end},
        {nil, 'leaky', handler=function() opts.leaky = true end},
        {'q', 'quiet', handler=function() opts.quiet = true end},
        {'s', 'skip-top', hasArg=true, handler=function(optarg) opts.skip_top = argparse.nonnegativeInt(optarg, 'skip-top') end},
        {'u', 'zup', handler=function() opts.zup = true end},
        {'z', 'cur-zlevel', handler=function() opts.curz = true end},
    })

    if help or positionals[1] == 'help' then
        print(dfhack.script_help())
        return false
    end

    local action, aq_type, pos1, pos2 = parse_positionals(positionals)

    if pos1 and not pos2 then
        local cursor = guidm.getCursorPos()
        if cursor then
            pos2 = cursor
        else
            pos2 = pos1
        end
        opts.all, opts.zdown, opts.zup, opts.curz = false, false, false, false
    end

    if not pos1 then
        pos1, pos2 = get_bounds(opts, action == 'list')
    end

    if not pos1 then
        qerror('must specify a target range (--all, --zdown, --zup, --cur-zlevel, or coordinates)')
    end

    pos1.x, pos2.x = math.min(pos1.x, pos2.x), math.max(pos1.x, pos2.x)
    pos1.y, pos2.y = math.min(pos1.y, pos2.y), math.max(pos1.y, pos2.y)
    pos1.z, pos2.z = math.min(pos1.z, pos2.z), math.max(pos1.z, pos2.z)

    if not opts.levels then
        opts.levels = pos2.z - pos1.z + 1
    elseif opts.zdown then
        pos1.z = math.max(pos1.z, pos2.z - opts.levels + 1)
        opts.levels = pos2.z - pos1.z + 1
    elseif opts.zup then
        pos2.z = math.min(pos2.z, pos1.z + opts.levels - 1)
        opts.levels = pos2.z - pos1.z + 1
    end

    if action == 'list' then
        aquifer_list(pos1, pos2, opts.levels, opts.leaky)
        return true
    end

    if action ~= 'drain' and not aq_type then
        qerror('must specify an aquifer type (heavy or light)')
    end

    local modified = 0

    if action == 'drain' then
        modified = aquifer_drain(aq_type or 'all', pos1, pos2, opts.skip_top, opts.levels, opts.leaky)
    elseif action == 'convert' then
        modified = aquifer_convert(aq_type, pos1, pos2, opts.skip_top, opts.levels, opts.leaky)
    elseif action == 'add' then
        modified = aquifer_add(aq_type, pos1, pos2, opts.skip_top, opts.levels, opts.leaky)
    else
        qerror(('invalid action: %s'):format(action))
    end

    if not opts.quiet then
        print(('%d tile(s) modified'):format(modified))
    end

    return true
end

return _ENV
