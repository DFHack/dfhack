-- Color schemes manager.
--[====[

color-schemes
=============
A script to manage color schemes.

Current features are :
    * :Registration:
    * :Loading:
    * :Listing:
    * :Default: Setting/Loading

For detailed information and usage, type ``color-schemes`` in console.

Loaded as a module, this script will export those methods :
    * register(path, force) : Register colors schemes by path (file or directory), relative to DF main directory
    * load(name)            : Load a registered color scheme by name
    * list()                : Return a list of registered color schemes
    * set_default(name)     : Set the default color scheme
    * load_default()        : Load the default color scheme

For more information about arguments and return values, see ``hack/scripts/color-schemes.lua``.

Related scripts:
    * `gui/color-schemes` is the in-game GUI for this script.
    * `season-palette` swaps color schemes when the season changes.

]====]

--[[
Copyright (c) 2020 Nicolas Ayala `https://github.com/nicolasayala`

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
]]

--@ module = true

--+================+
--|    IMPORTS     |
--+================+

local json = require "json"

--+================+
--|    GLOBALS     |
--+================+

-- `color-schemes` version
VERSION = "0.1.0"
-- Config file (default color-scheme)
CONFIG_FILE = "dfhack-config/color-scheme.json"
-- Table of registered color schemes (see Scheme class for template)
_registered_schemes = _registered_schemes or {}
-- The current loaded color scheme instance (loaded_scheme.loaded = true)
_loaded_scheme = _loaded_scheme or nil

--+================+
--|    LOGGING     |
--+================+

-- Log levels
local SUCCESS = 1
local INFO = 2
local WARN = 3
local ERR = 4
-- Log level threshold
local verbosity = 4

local log_colors = {COLOR_LIGHTGREEN, COLOR_LIGHTBLUE, COLOR_YELLOW, COLOR_LIGHTRED}
local function log(msg, level)
    level = level or 1
    if verbosity - level >= 0 then
        dfhack.color(log_colors[level])
        print("color-schemes : " .. msg)
        dfhack.color()
    end
end

--+==================+
--|    FILESYSTEM    |
--+==================+

--[[
    Define filesystem methods working with DF's root directory
]]--
local ROOT = dfhack.getDFPath() .. "/"

local function open(path, mode)
    return io.open(ROOT .. path, mode)
end

local function exists(path)
    return dfhack.filesystem.exists(ROOT .. path)
end

local function get_mtime(path)
    return dfhack.filesystem.mtime(ROOT .. path)
end

local function listdir(path)
    local paths = dfhack.filesystem.listdir_recursive(ROOT .. path)
    for _,v in ipairs(paths) do
        if string.startswith(v.path, ROOT) then
            v.path = string.sub(v.path, #ROOT + 1)
        end
    end
    return paths
end

local function isdir(path)
    return dfhack.filesystem.isdir(ROOT .. path)
end

local function read_file(path)
    if not exists(path) then
        return nil, path .. " does not exist"
    end
    local file = open(path, "rb")
    if not file then
        return nil, "Could not open " .. path
    end
    local data = file:read("*a")
    file:close()
    return data
end

local function decode_file(path, ...)
    return json.decode_file(ROOT .. path, ...)
end

local function encode_file(data, path, ...)
    json.encode_file(data, ROOT .. path, ...)
end

local function sanitize_path(path)
    path = string.gsub(path, "/+", "/")
    path = string.gsub(path, "^/", "")
    return path
end

--+==========================+
--|   COLOR SCHEME PARSING   |
--+==========================+

-- Returns the table flipped (keys and values flipped)
local function flip(table)
    local T = {}
    for k,v in pairs(table) do
        T[v] = k
    end
    return T
end

local COLORS = {
    "BLACK", "BLUE", "GREEN", "CYAN", "RED", "MAGENTA",
    "BROWN", "LGRAY", "DGRAY", "LBLUE", "LGREEN",
    "LCYAN", "LRED", "LMAGENTA", "YELLOW", "WHITE"
}

local CHANNELS = {"R", "G", "B"}

--[[
    Parse colors' values from string with format `[<color>_<channel>:<value>]...`
    <color> in `COLORS`, <channel> in `CHANNELS`, <value> in range [0,255]
    Return
        valid : <boolean> Parsing was successful
        values :
        {
            "<color>" = {
                "R" = <number>,
                "G" = <number>,
                "B" = <number>
            }
            ...
        }
        (<number> = -1 when unspecified)
]]
local function parse_color_scheme(string)
    local values = {}
    local total = 0
    local colors = flip(COLORS)
    local channels = flip(CHANNELS)
    -- Initialize values table
    for color,_ in pairs(colors) do
        values[color] = {R = -1, G = -1, B = -1}
    end
    -- Parse color values
    for color, channel, value in string.gmatch(string, "%[([A-Z]+)_([RGB]):([0-9]+)%]") do
        value = tonumber(value)
        if colors[color] and channels[channel] and value ~= nil and values[color][channel] == -1 then
            values[color][channel] = math.max(0, math.min(255, value)) -- Clamp value in range [0,255]
            total = total + 1 -- Keep count of parsed colors channels
        end
    end
    local valid = (total == 48) -- #COLORS * #CHANNELS = 16 * 3 = 48
    return valid, values
end

--+==================+
--|   SCHEME CLASS   |
--+==================+

--[[
    Color scheme class, used to store colors' values, file path, modification time, loaded state
    template :
    {
        name  : <string>
        file  : <string>    Relative to DF main directory
        mtime : <number>    File modification time, used for reloading
        valid : <boolean>   Specifies if that color scheme is valid (all values are defined)
        values: <table>     Table of colors' values (see `parse_color_scheme` for template)
    }
]]
Scheme = defclass(Scheme)
Scheme.ATTRS {
    name = "",
    file = "",
    mtime = 0,
    valid = false,
    values = DEFAULT_NIL
}

-- Create a color scheme from file
function Scheme.new(file)
    file = sanitize_path(file)
    -- Parse name from file path, /<dir>/<name>.<ext> (Replace `_` and `-` with spaces)
    local name = string.gsub(string.match(file, "([^/]+)%.[^%.]*$"), "[_-]", " ")
    -- Read file, get modification time and parse color values
    local data, err = read_file(file)
    if not data then qerror(err) end
    local mtime = get_mtime(file)
    local valid, values = parse_color_scheme(data)
    return Scheme {
        name = name,
        file = file,
        mtime = mtime,
        valid = valid,
        values = values
    }
end

-- Load a color scheme
function Scheme:load()
    for c,color in ipairs(COLORS) do
        for l,channel in ipairs(CHANNELS) do
            -- Use `math.max` for -1 values (not parsed correctly)
            local value = math.max(0, self.values[color][channel]) / 255
            df.global.enabler.ccolor[c - 1][l - 1] = value
        end
    end
    df.global.gps.force_full_display_count = 1
end

--+===================+
--|   SCRIPT CONFIG   |
--+===================+

local config = nil

local function get_config()
    if not exists(CONFIG_FILE) then
        return {}
    end
    local success, config = pcall(decode_file, CONFIG_FILE)
    if not success then qerror(string.format("Unable to parse config file `%s`", CONFIG_FILE)) end
    return config
end

local function config_set(params)
    for param, value in pairs(params) do
        config[param] = value
    end
    encode_file(config, CONFIG_FILE)
end

config = get_config()

--+========================+
--|   LOCAL CORE METHODS   |
--+========================+

--[[
    Register color scheme from file (relative to DF main directory)
    `force` argument forces registering invalid or incomplete color schemes
    Return color scheme instance if success, nil otherwise
]]
local function register_file(file, force)
    local new = Scheme.new(file)
    local mtime = get_mtime(file)
    local status
    local log_level = INFO
    local suffix = ""
    local save = true
    -- Check if that color scheme was already registered
    local old = _registered_schemes[new.name]
    if not old then
        -- That color scheme isn't already registered
        status = "Registered"
    elseif new.file ~= old.file then
        -- Duplicate color scheme name
        status = "Couldn't register"
        suffix = "duplicate name"
        log_level = ERR
        save = false
    elseif old.mtime ~= new.mtime then
        -- That color scheme was updated
        status = "Updated"
    else
        -- Color scheme already up to date
        status = "Up to date"
        save = false
    end

    if save then
        -- Check color scheme validity
        if new.valid then
            _registered_schemes[new.name] = new
            log_level = INFO
        else
            suffix = "invalid"
            if force then
                _registered_schemes[new.name] = new
                log_level = WARN
            else
                status = "Couldn't register"
                log_level = ERR
            end
        end
    end
    local action = string.format("%s `%s`", status, new.name)
    local file = string.format("[%s]", new.file)
    log(string.format("%-30s %-40s %s", action, file, suffix), log_level)
    return save and new or nil;
end

-- Register color schemes in directory via `register_file`
local function register_dir(dir, force)
    for _,value in ipairs(listdir(dir)) do
        if not value.isdir then
            if string.match(value.path, ".txt$") then
                register_file(value.path, force)
            end
        end
    end
end

--+======================+
--|   EXPORTED METHODS   |
--+======================+

--[[
    Register color schemes by path (file or directory)
    Registering color schemes by directory is recursive
    `force` argument forces registering invalid or incomplete color schemes
]]
function register(path, force)
    if not exists(path) then
        log("Couldn't register file or directory `" .. path .. "` (does not exist)", ERR)
    else
        if (isdir(path)) then
            register_dir(path, force)
        else
            register_file(path, force)
        end
    end
end

--[[
    Load a registered color scheme by name
    The color scheme must be first registered with the `register` method
    Return the color scheme instance
]]
function load(name)
    local scheme = _registered_schemes[name]
    if not scheme then
        log("Unregistered color scheme `" .. name .. "`", ERR)
        return nil
    end
    if scheme ~= _loaded_scheme then
        _loaded_scheme = scheme
        scheme:load()
        log("Loaded `" .. scheme.name .. "`")
    else
        log("Color scheme `" .. scheme.name .. "` already loaded !", WARN)
    end
    return scheme
end

--[[
    Return the table of registered color schemes
    template :
    {
        {
            name    : <string>
            file    : <string>
            valid   : <boolean>
            loaded  : <boolean>
        }
        ...
    }
]]
function list()
    local schemes = {}
    for name,scheme in pairs(_registered_schemes) do
        table.insert(schemes, {
            name = name,
            file = scheme.file,
            valid = scheme.valid,
            loaded = (scheme == _loaded_scheme),
            default = (scheme.name == config.default)
        })
    end
    return schemes
end

--[[
    Set the default color scheme name, stored in `CONFIG_FILE`, persistent between restarts.
]]
function set_default(name)
    if name then
        log(string.format("Default set to `%s`", name), SUCCESS)
        config_set({default = name})
    end
end

--[[
    Load the default color scheme, however it must be registered first like any other color scheme.
    (see `dfhack*.init` files to register color schemes on start)
]]
function load_default()
    local default = config.default
    if not default then
        log("No default color scheme", ERR)
        return nil
    end
    return load(default)
end

if moduleMode then return _ENV end

--+==================+
--|   ARGS PARSING   |
--+==================+

--[[
    Print a formated table with colors
    <-----spacing_0----> <-----spacing_1-----> ...
    <column_0>          |<columns_1>          |...
    ====================|=====================|...
    <entry_0>.column_0  |<entry_0>.column_1   |...
    <entry_1>.column_0  |<entry_1>.column_1   |...

    Args
        T           : <table>
        on_print    : <func(item)> Function called before entry is printed
        ...         : <strings>    Format "<column>:<spacing>" (column name and spacing)
]]
local function printSchemes(T, on_print, ...)
    local format = {...}
    local columns = {}
    local spacings = {}
    local entry_format = ""
    local sep = "|"
    local bar = ""
    -- Parse columns' names and build entry format
    for i,c in ipairs(format) do
        if i == #format then sep = "" end
        local name, spacing = string.match(c, "(%a+):(%d+)")
        entry_format = entry_format .. "%-" .. spacing .. "s" .. sep
        bar = bar .. string.rep("=", tonumber(spacing)) .. sep
        table.insert(columns, name)
        table.insert(spacings, tonumber(spacing))
    end
    print(string.format(entry_format, table.unpack(columns)))
    print(bar)
    -- Build entry with specified columns
    for name,item in pairs(T) do
        local entry = {}
        for i,c in ipairs(columns) do
            -- Clamp string fields to match spacing
            if type(item[c]) == "string" and #item[c] > spacings[i] then
                table.insert(entry, string.sub(item[c], 0, spacings[i]-3) .. "...")
            else
                table.insert(entry, item[c])
            end
        end
        if on_print then on_print(item) end
        print(string.format(entry_format, table.unpack(entry)))
        dfhack.color()
    end
end

local function map(f, T, i, j)
    local M = {}
    for k = (i or 1),(j or #T) do
        M[k] = f(k, T[k])
    end
    return M
end

local function extract(T, i, j)
    return {table.unpack(T, i, j)}
end

local function find(v, T)
    for key,value in ipairs(T) do
        if v == value then return key, value end
    end
    return nil
end

local function concat(T, f, sep, i, j)
    return table.concat(map(function(k, v) return f(v) end, T, i, j), sep)
end

local function get(self, key)
    if type(self[key]) == "function" then
        return self[key](self)
    else
        return self[key]
    end
end

local class = function(class, parent, attrs)
    local class = defclass(class, parent)
    local ATTRS = {}
    for attr,default in pairs(attrs or {}) do
        local internal = string.format("_%s", attr)
        ATTRS[internal] = default
        class[attr] = function(self, v) self[internal] = v return self end
    end
    class.ATTRS(ATTRS)
    return class
end

--+=====================+
--|   PARSING CLASSES   |
--+=====================+

--[[ PItem ]]--

PItem = class(PItem, nil,
    {
        name        = DEFAULT_NIL, -- string
        description = "",          -- string
        action      = DEFAULT_NIL, -- boolean func(self, result, target, args)
    }
)

local actions = {
    store       = function(self, result, target, args) result[target] = args end,
    store_true  = function(self, result, target, args) result[target] = true end,
    store_false = function(self, result, target, args) result[target] = false end,
    count       = function(self, result, target, args) result[target] = 1 or result[target] + 1 end
}

function PItem:init(args)
    local class = getmetatable(self)
    class.__tostring = class.__tostring or class.super.__tostring
end

function PItem:error(format, ...)
    return qerror(string.format(format, ...))
end

function PItem:__tostring()
    return string.format("%-20s %s", self._name, self._description)
end

--[[ Argument ]]--

Argument = class(Argument, PItem,
    {
        action      = actions.store, -- boolean func(self, result, target, args)
        args        = 1,             -- number
        choices     = DEFAULT_NIL,   -- table<string|number>
        convert     = DEFAULT_NIL,   -- function(arg)
    }
)

function Argument:usage()
    return string.rep(string.format("<%s>", self._name), self._args, " ")
end

function Argument:consume(args, result)
    local A = {}
    local consumed = 0
    for i = 1,self._args do
        local arg = args[i]
        if not arg then
            self:error("argument `%s` requires %d argument(s)", self._name, self._args)
        end
        if self._choices then
            if not find(arg, self._choices) then
                local C = concat(self._choices, curry(string.format, "`%s`"), ", ")
                self:error("argument `%s` must be one of %s", self._name, C)
            end
        end
        consumed = consumed + 1
        if self._convert then
            local res = self._convert(arg)
            if not res then self:error("argument `%s` could not be converted", arg) end
            table.insert(A, res)
        else
            table.insert(A, arg)
        end
    end
    local sub_args = extract(args, consumed + 1)
    if self:_action(result, self._target, (consumed == 1) and A[1] or A) then
        return sub_args, true
    end
    return sub_args
end

function Argument:name(name)
    self._name = name
    self._target = name
    return self
end

--[[ Option ]]--

Option = class(Option, Argument)

function Option:name(name)
    self._aliases = self._aliases or {}
    for _,alias in ipairs(string.split(name, " ")) do
        local short = alias:match("^-%a$")
        local long  = alias:match("^%-%-%a+$")
        if short then self._short = short end
        self._target = string.gsub(short or long, "%-", "")
        table.insert(self._aliases, short or long)
    end
    self._name = name
    return self
end

function Option:usage()
    if self._args == 0 then
        return string.format("[%s]", self._short)
    else
        return string.format("[%s %s]", self._short, Argument.usage(self))
    end
end

function Option:__tostring()
    if self._args == 0 then
        return string.format("%-20s %s", table.concat(self._aliases, ", "), self._description)
    else
        local arg = string.format("<%s>", self._target)
        local args = string.rep(arg, self._args, " ")
        local lines = map(function(k,v) return string.format("%s %s", v, args) end, self._names)
        local width = 0
        for _,l in ipairs(lines) do width = math.max(width, #l) end
        local help = concat(lines, curry(string.format, "%" .. width .. "s"), ",\n   ")
        return string.format("%s\n   %20s %s", help, "", self._description)
    end
end

--[[ Parser ]]--

Parser = class(Parser, PItem,
    {
        action      = actions.store, -- boolean func(self, result, target, args)
        summary     = DEFAULT_NIL,   -- string
        notes       = DEFAULT_NIL,   -- string
        req_command = true,          -- boolean
    }
)

Parser.help_items = {
    {key = "_name",          format = "%s\n"},
    {key = "_description",   format = "\n%s\n"},
    {key = "usage",         format = "\nUsage: %s\n"},
    {key = "_arguments",     format = "\nArguments:\n%s\n"},
    {key = "_options",       format = "\nOptions:\n%s\n"},
    {key = "_commands",      format = "\nCommands:\n%s\n"},
    {key = "_notes",         format = "\nNotes:\n%s\n"},
}

Parser.usage_items = {
    {key = "_name",      usage = function(value) return value end},
    {key = "_options",   usage = function(value) return concat(value, Option.usage, " ") end},
    {key = "_arguments", usage = function(value) return concat(value, Argument.usage, " ") end},
    {key = "_commands",  usage = function(value) return "<command>..." end},
}

function Parser:command()
    local command = Parser()
    self._commands = self._commands or {}
    table.insert(self._commands, command)
    return command
end

function Parser:argument()
    local argument = Argument()
    self._arguments = self._arguments or {}
    table.insert(self._arguments, argument)
    return argument
end

function Parser:option()
    local option = Option()
    self._options = self._options or {}
    table.insert(self._options, option)
    return option
end

function Parser:flag()
    return self
            :option()
            :args(0)
            :action(actions.store_true)
end

function Parser:add_help()
    self:flag()
        :name("-h --help")
        :description("Show this help message and exit")
        :action(function() print(self:help()) return true end)
    return self
end

function Parser:__tostring()
    return string.format("%-20s %s", self._name, self._summary or self._description)
end

function Parser:usage()
    local parts = {}
    for _,item in ipairs(Parser.usage_items) do
        local value = self[item.key]
        if value then
            table.insert(parts, item.usage(value))
        end
    end
    return table.concat(parts, " ")
end

function Parser:help()
    local help_format = ""
    local help_args = {}
    for _,item in ipairs(Parser.help_items) do
        local value = get(self, item.key)
        if value then
            help_format = help_format .. item.format
            local value_str
            if type(value) == "table" then
                value_str = "   " .. concat(value, tostring, "\n   ")
            elseif type(value) == "string" then
                value_str = value
            end
            table.insert(help_args, value_str)
        end
    end
    return help_format:format(table.unpack(help_args))
end

function Parser:parse(args, result)
    local result = result or {}
    local arg = args[1]
    if not arg then
        if self._commands and self._req_command then
            self:error("a command is required")
        elseif self._arguments then
            for _,argument in ipairs(self._arguments) do
                if not result[argument._name] then
                    self:error("missing argument `%s`", argument._name)
                end
            end
        end
        self:_action(result, self._name)
        return result
    end

    -- Options
    if arg:sub(1, 1) == "-" and self._options then
        local o = arg
        for _,option in ipairs(self._options) do
            if find(o, option._aliases) then
                local sub_args, exit = option:consume(extract(args, 2), result)
                if exit then return result end
                return self:parse(sub_args, result)
            end
        end
        self:error("unknown option `%s`", o)
    end

    -- Arguments
    if self._arguments then
        for _,argument in ipairs(self._arguments) do
            if not result[argument._name] then
                local sub_args, exit = argument:consume(args, result)
                if exit then return result end
                return self:parse(sub_args, result)
            end
        end
    end

    -- Commands
    local c = arg
    if self._commands then
        for _,command in ipairs(self._commands) do
            if c == command._name then
                result.command = command:parse(extract(args, 2))
                return result
            end
        end
        self:error("unknown command `%s`", c)
    end
    self:error("too many arguments")
end

--[[ Parser Config ]]--

local parser = Parser()
parser
    :name("color-schemes"):description("Manage color schemes")
    :add_help()
    :notes("   Type `--help` after a command to get further information")
parser:flag()
    :name("-V --version"):description("Show version number and exit")
    :action(function() print(VERSION) return true end)
parser:flag()
    :name("-q --quiet"):description("Do not write anything to output")
    :action(function() verbosity = 0 end)

local load_cmd = parser:command()
load_cmd
    :name("load"):description("Load a registered color scheme")
    :add_help()
    :action(function(self, result) load(result.name) end)
load_cmd:argument()
    :name("name")
    :description("Color scheme name")

local list_cmd = parser:command()
list_cmd
    :name("list"):description("List registered color schemes")
    :add_help()
    :action(
        function()
            printSchemes(
                list(),
                function(item)
                    if not item.valid then return dfhack.color(COLOR_LIGHTRED)
                    elseif item.loaded then return dfhack.color(COLOR_LIGHTGREEN) end
                end,
                "name:30", "file:50"
            )
        end
    )

local register_cmd = parser:command()
register_cmd
    :name("register"):description("Register color schemes")
    :add_help()
    :action(function(self, result) register(result.path, result.force) end)
register_cmd:flag()
    :name("-f --force"):description("Also register syntaxically incorrect or partial color schemes")
register_cmd:argument()
    :name("path"):description("Path to color scheme file or directory")

local default_cmd = parser:command()
default_cmd
    :name("default"):description("Load or set the default color scheme")
    :add_help()
default_cmd:command()
    :name("load"):description("Load the default color scheme")
    :add_help()
    :action(curry(load_default))
default_cmd:command()
    :name("set"):description("Set the default color scheme")
    :add_help()
    :action(function(self, result) set_default(result.name) end)
    :argument()
        :name("name"):description("Color scheme name")

local args = {...}
if #args == 0 then print(parser:help()) return _ENV end
local result = parser:parse(args)
-- result table isn't used here since logic is inside parsing actions

return _ENV
