local _ENV = mkmodule('plugins.blueprint')

local argparse = require('argparse')
local utils = require('utils')

-- the info here is very basic and minimal, so hopefully we won't need to change
-- it when features are added and the full blueprint docs in Plugins.rst are
-- updated.
local help_text = [=[

blueprint
=========

Records the structure of a portion of your fortress in quickfort blueprints.

Usage:

    blueprint <width> <height> [<depth>] [<name> [<phases>]] [<options>]
    blueprint gui [<name> [<phases>]] [<options>]

Examples:

blueprint gui
    Runs gui/blueprint, the interactive blueprint frontend, where all
    configuration can be set visually and interactively.

blueprint 30 40 bedrooms
    Generates blueprints for an area 30 tiles wide by 40 tiles tall, starting
    from the active cursor on the current z-level. Output files are written to
    the "blueprints" directory.

See the online DFHack documentation for more examples and details.
]=]

function print_help() print(help_text) end

local valid_phase_list = {
    'dig',
    'carve',
    'build',
    'place',
    'zone',
    'query',
}
valid_phases = utils.invert(valid_phase_list)

local valid_formats_list = {
    'minimal',
    'pretty',
}
valid_formats = utils.invert(valid_formats_list)

local valid_split_strategies_list = {
    'none',
    'phase',
}
valid_split_strategies = utils.invert(valid_split_strategies_list)

local function parse_cursor(opts, arg)
    local cursor = argparse.coords(arg)
    -- be careful not to replace struct members when called from C++, but also
    -- create the table as needed when called from lua
    if not opts.start then opts.start = {} end
    utils.assign(opts.start, cursor)
end

local function parse_enum(opts, valid, name, val)
    if not valid[val] then
        qerror(('unknown %s: "%s"; expected one of: %s')
               :format(name, val, table.concat(valid, ', ')))
    end
    opts[name] = val
end

local function parse_format(opts, file_format)
    parse_enum(opts, valid_formats, 'format', file_format)
end

local function is_int(val)
    return val and val == math.floor(val)
end

local function is_positive_int(val)
    return is_int(val) and val > 0
end

local function parse_start(opts, args)
    local arg_list = argparse.stringList(args)
    local x_str, y_str = table.remove(arg_list, 1), table.remove(arg_list, 1)
    local x, y = tonumber(x_str), tonumber(y_str)
    if not is_positive_int(x) or not is_positive_int(y) then
        qerror(('playback start offsets must be positive integers: "%s", "%s"')
               :format(x_str or '', y_str or ''))
    end

    if not opts.playback_start then opts.playback_start = {} end
    opts.playback_start.x, opts.playback_start.y = x, y
    if #arg_list > 0 then
        opts.playback_start_comment = table.concat(arg_list, ', ')
    end
end

local function parse_split_strategy(opts, strategy)
    parse_enum(opts, valid_split_strategies, 'split_strategy', strategy)
end

local function parse_positionals(opts, args, start_argidx)
    local argidx = start_argidx or 1

    -- set defaults
    opts.name, opts.auto_phase = 'blueprint', true

    local name = args[argidx]
    if not name then return end
    if name == '' then
        qerror(('invalid basename: "%s"; must be a valid, non-empty pathname')
               :format(args[argidx]))
    end
    argidx = argidx + 1
    -- normalize paths and remove leading slashes
    opts.name = utils.normalizePath(name):gsub('^/', '')

    local auto_phase = true
    local phase = args[argidx]
    while phase do
        if not valid_phases[phase] then
            qerror(('unknown phase: "%s"; expected one of: %s')
                   :format(phase, table.concat(valid_phase_list, ', ')))
        end
        auto_phase = false
        opts[phase] = true
        argidx = argidx + 1
        phase = args[argidx]
    end
    opts.auto_phase = auto_phase
end

local function process_args(opts, args)
    if args[1] == 'help' then
        opts.help = true
        return
    end

    -- set defaults
    opts.format = valid_formats_list[1]
    opts.split_strategy = valid_split_strategies_list[1]

    local positionals = argparse.processArgsGetopt(args, {
            {'c', 'cursor', hasArg=true,
             handler=function(optarg) parse_cursor(opts, optarg) end},
            {'e', 'engrave', handler=function() opts.engrave = true end},
            {'f', 'format', hasArg=true,
             handler=function(optarg) parse_format(opts, optarg) end},
            {'h', 'help', handler=function() opts.help = true end},
            {'s', 'playback-start', hasArg=true,
             handler=function(optarg) parse_start(opts, optarg) end},
            {'t', 'splitby', hasArg=true,
             handler=function(optarg) parse_split_strategy(opts, optarg) end},
        })

    if opts.help then
        return
    end

    return positionals
end

-- used by the gui/blueprint script
function parse_gui_commandline(opts, args)
    local positionals = process_args(opts, args)
    if opts.help then return end
    parse_positionals(opts, positionals)
end

-- dimension must be a non-nil integer that is >= 1 (or at least non-zero if
-- negative_ok is true)
local function is_bad_dim(dim, negative_ok)
    return not is_int(dim) or
            (not negative_ok and dim < 1 or dim == 0)
end

function parse_commandline(opts, ...)
    local positionals = process_args(opts, {...})
    if opts.help then return end

    local width, height = tonumber(positionals[1]), tonumber(positionals[2])
    if is_bad_dim(width) or is_bad_dim(height) then
        qerror(('invalid width or height: "%s" "%s"; width and height must' ..
                ' be positive integers'):format(positionals[1], positionals[2]))
    end
    opts.width, opts.height, opts.depth = width, height, 1

    local depth = tonumber(positionals[3])
    if depth then
        if is_bad_dim(depth, true) then
            qerror(('invalid depth: "%s"; must be a non-zero integer')
                   :format(positionals[3]))
        end
        opts.depth = depth
    end

    if opts.playback_start and opts.playback_start.x > 0 then
        if opts.playback_start.x > width then
            qerror(('playback start x offset outside width of blueprint: %d')
                   :format(opts.playback_start.x))
        end
        if opts.playback_start.y > height then
            qerror(('playback start y offset outside height of blueprint: %d')
                   :format(opts.playback_start.y))
        end
    end

    parse_positionals(opts, positionals, depth and 4 or 3)
end

-- returns the name of the output file for the given context
function get_filename(opts, phase)
    local fullname = 'blueprints/' .. opts.name
    local _,_,basename = fullname:find('/([^/]+)/?$')
    if not basename then
        -- should not happen since opts.name should already be validated
        error(('could not parse basename out of "%s"'):format(fullname))
    end
    if fullname:endswith('/') then
        fullname = fullname .. basename
    end
    if opts.split_strategy == 'phase' then
        return ('%s-%s.csv'):format(fullname, phase)
    end
    -- no splitting
    return ('%s.csv'):format(fullname)
end

-- compatibility with old exported API.
local function do_phase(start_pos, end_pos, name, phase)
    local width = math.abs(start_pos.x - end_pos.x) + 1
    local height = math.abs(start_pos.y - end_pos.y) + 1
    local depth = math.abs(start_pos.z - end_pos.z) + 1
    if start_pos.z > end_pos.z then depth = -depth end

    local x = math.min(start_pos.x, end_pos.x)
    local y = math.min(start_pos.y, end_pos.y)
    local z = start_pos.z

    local cursor = ('--cursor=%d,%d,%d'):format(x, y, z)

    run(tostring(width), tostring(height), tostring(depth), tostring(name),
        phase, cursor)
end
for phase in pairs(valid_phases) do
    _ENV[phase] = function(s, e, n) do_phase(s, e, n, phase) end
end

return _ENV
