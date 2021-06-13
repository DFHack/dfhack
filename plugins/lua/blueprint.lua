local _ENV = mkmodule('plugins.blueprint')

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
    'build',
    'place',
    'query',
}

valid_phases = utils.invert(valid_phase_list)

local function parse_cursor(opts, arg)
    local _, _, x, y, z = arg:find('^(%d+),(%d+),(%d+)$')
    if not x then
        qerror(('invalid argument for --cursor option: "%s"; expected format' ..
                ' is "<x>,<y>,<z>", for example: "30,60,150"'):format(arg))
    end
    -- be careful not to replace struct members when called from C++, but also
    -- create the table as needed when called from lua
    if not opts.start then opts.start = {} end
    opts.start.x = tonumber(x)
    opts.start.y = tonumber(y)
    opts.start.z = tonumber(z)
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
    -- normalize paths to forward slashes
    opts.name = name:gsub(package.config:sub(1,1), "/")

    local auto_phase = true
    local phase = args[argidx]
    while phase do
        if not valid_phases[phase] then
            qerror(('unknown phase: "%s"; expected one of: %s'):
                   format(phase, table.concat(valid_phase_list, ', ')))
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

    return utils.processArgsGetopt(args, {
            {'c', 'cursor', hasArg=true,
             handler=function(optarg) parse_cursor(opts, optarg) end},
            {'h', 'help', handler=function() opts.help = true end},
        })
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
    return not dim or
            (not negative_ok and dim < 1 or dim == 0) or
            dim ~= math.floor(dim)
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

    parse_positionals(opts, positionals, depth and 4 or 3)
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
