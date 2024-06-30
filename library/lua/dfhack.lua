-- Common startup file for all dfhack scripts and plugins with lua support
-- The global dfhack table is already created by C++ init code.

-- Setup the global environment.
-- BASE_G is the original lua global environment,
-- preserved as a common denominator for all modules.
-- This file uses it instead of the new default one.

---@class dfhack
---@field BASE_G _G Original Lua global environment
---@field is_core_context boolean
---@field is_interactive fun(): boolean
local dfhack = dfhack

local base_env = dfhack.BASE_G
local _ENV = base_env

-- Types

---@alias dfhack.truthy
---| true
---| integer
---| string
---| table
---| function
---| userdata
---| fun(...): true|string|integer|table|function|userdata

CR_LINK_FAILURE = -3
CR_NEEDS_CONSOLE = -2
CR_NOT_IMPLEMENTED = -1
CR_OK = 0
CR_FAILURE = 1
CR_WRONG_USAGE = 2
CR_NOT_FOUND = 3

-- Console color constants

COLOR_RESET = -1
COLOR_BLACK = 0
COLOR_BLUE = 1
COLOR_GREEN = 2
COLOR_CYAN = 3
COLOR_RED = 4
COLOR_MAGENTA = 5
COLOR_BROWN = 6
COLOR_GREY = 7
COLOR_DARKGREY = 8
COLOR_LIGHTBLUE = 9
COLOR_LIGHTGREEN = 10
COLOR_LIGHTCYAN = 11
COLOR_LIGHTRED = 12
COLOR_LIGHTMAGENTA = 13
COLOR_YELLOW = 14
COLOR_WHITE = 15

COLOR_GRAY = COLOR_GREY
COLOR_DARKGRAY = COLOR_DARKGREY

---@alias dfhack.color
---| `COLOR_RESET`
---| `COLOR_BLACK`
---| `COLOR_BLUE`
---| `COLOR_GREEN`
---| `COLOR_CYAN`
---| `COLOR_RED`
---| `COLOR_MAGENTA`
---| `COLOR_BROWN`
---| `COLOR_GREY`
---| `COLOR_DARKGREY`
---| `COLOR_LIGHTBLUE`
---| `COLOR_LIGHTGREEN`
---| `COLOR_LIGHTCYAN`
---| `COLOR_LIGHTRED`
---| `COLOR_LIGHTMAGENTA`
---| `COLOR_YELLOW`
---| `COLOR_WHITE`
---| `COLOR_GRAY`
---| `COLOR_DARKGRAY`

-- Events

if dfhack.is_core_context then
    SC_WORLD_LOADED = 0
    SC_WORLD_UNLOADED = 1
    SC_MAP_LOADED = 2
    SC_MAP_UNLOADED = 3
    SC_VIEWSCREEN_CHANGED = 4
    SC_CORE_INITIALIZED = 5
    SC_PAUSED = 7
    SC_UNPAUSED = 8
end

-- User-changeable options

local HIDE_CONSOLE_ON_STARTUP = true
---@nodiscard
---@return boolean
function dfhack.getHideConsoleOnStartup()
    return HIDE_CONSOLE_ON_STARTUP
end
function dfhack.setHideConsoleOnStartup(value)
    HIDE_CONSOLE_ON_STARTUP = value
end

local HIDE_ARMOK_TOOLS = false
---@nodiscard
---@return boolean
function dfhack.getMortalMode()
    return HIDE_ARMOK_TOOLS
end
function dfhack.setMortalMode(value)
    HIDE_ARMOK_TOOLS = value
    dfhack.internal.setMortalMode(value)
end

-- Error handling

safecall = dfhack.safecall
curry = dfhack.curry

---@generic T
---@param f fun(...): T
---@param ... any
---@return boolean success
---@return T|string ...
function dfhack.pcall(f, ...)
    return xpcall(f, dfhack.onerror, ...)
end

---@param msg string
---@param level? integer
function qerror(msg, level)
    local name = dfhack.current_script_name()
    if name and not tostring(msg):match(name) then
        msg = name .. ': ' .. tostring(msg)
    end
    dfhack.error(msg, (level or 1) + 1, false)
end

---@generic T
---@param cleanup_fn function
---@param fn fun(...): T
---@param ... any
---@return T
function dfhack.with_finalize(cleanup_fn,fn,...)
    return dfhack.call_with_finalizer(0,true,cleanup_fn,fn,...)
end

---@generic T
---@param cleanup_fn function
---@param fn fun(...): T
---@param ... any
---@return T
function dfhack.with_onerror(cleanup_fn,fn,...)
    return dfhack.call_with_finalizer(0,false,cleanup_fn,fn,...)
end

---@param obj DFObject
local function call_delete(obj)
    if obj then obj:delete() end
end

---@generic T
---@param obj DFObject
---@param fn fun(...):T
---@param ... any
---@return T ...
function dfhack.with_temp_object(obj,fn,...)
    return dfhack.call_with_finalizer(1,true,call_delete,obj,fn,obj,...)
end

dfhack.exception.__index = dfhack.exception

-- Module loading

local function find_required_module_arg()
    -- require -> module code -> mkmodule -> find_...
    if debug.getinfo(4,'f').func == require then
        return debug.getlocal(4, 1)
    end
    -- reload -> dofile -> module code -> mkmodule -> find_...
    if debug.getinfo(5,'f').func == reload then
        return debug.getlocal(5, 1)
    end
end

---@nodiscard
---@param module string
---@param env? table|metatable
---@return _G pkg
function mkmodule(module,env)
    -- Verify that the module name is correct
    local _, rq_modname = find_required_module_arg()
    if not rq_modname then
        error('The mkmodule function must be used at the start of a module')
    end
    if rq_modname ~= module then
        error('Found module '..module..' during require '..rq_modname)
    end
    -- Reuse the already loaded module table
    local pkg = package.loaded[module]
    if pkg == nil then
        pkg = {}
    else
        if type(pkg) ~= 'table' then
            error("Not a table in package.loaded["..module.."]")
        end
    end
    -- Inject the plugin-exported functions when appropriate
    local plugname = string.match(module,'^plugins%.([%w%-]+)$')
    if plugname then
        dfhack.open_plugin(pkg,plugname)
    end
    setmetatable(pkg, { __index = (env or base_env) })
    return pkg
end

---@param module string
function reload(module)
    if type(package.loaded[module]) ~= 'table' then
        error("Module not loaded: "..module)
    end
    local path,err = package.searchpath(module,package.path)
    if not path then
        error(err)
    end
    dofile(path)
end

-- Trivial classes

function rawset_default(target,source)
    for k,v in pairs(source) do
        if rawget(target,k) == nil then
            rawset(target,k,v)
        end
    end
end

---@type any
DEFAULT_NIL = DEFAULT_NIL or {} -- Unique token

-- Create or updates a class; a class has metamethods and thus own metatable.
---@nodiscard
---@generic T: dfhack.class
---@param class? T
---@param parent? table
---@return T
function defclass(class,parent)
    return require('class').defclass(class,parent)
end

-- An instance uses the class as metatable
---@generic T: dfhack.class
---@param class T
---@param table? table
---@return T
function mkinstance(class,table)
    return require('class').mkinstance(class,table)
end

-- Misc functions

NEWLINE = "\n"
COMMA = ","
PERIOD = "."

local function _wrap_iterator(next_fn, ...)
    local wrapped_iter = function(...)
        local ret = {pcall(next_fn, ...)}
        local ok = table.remove(ret, 1)
        if ok then
            return table.unpack(ret)
        end
    end
    return wrapped_iter, ...
end

function safe_pairs(t, iterator_fn)
    iterator_fn = iterator_fn or pairs
    if (pcall(iterator_fn, t)) then
        return _wrap_iterator(iterator_fn(t))
    else
        return function() end
    end
end

-- calls elem_cb(k, v) for each element of the table
-- returns true if we iterated successfully, false if not
-- this differs from safe_pairs() above in that it only calls pcall() once per
-- full iteration and it returns whether iteration succeeded or failed.
local function safe_iterate(table, iterator_fn, elem_cb)
    local function iterate()
        for k,v in iterator_fn(table) do elem_cb(k, v) end
    end
    return pcall(iterate)
end

local function print_element(k, v)
    dfhack.println(string.format("%-23s\t = %s", tostring(k), tostring(v)))
end

---@param table table
function printall(table)
    safe_iterate(table, pairs, print_element)
end

---@param table table
function printall_ipairs(table)
    safe_iterate(table, ipairs, print_element)
end

local do_print_recurse

local function print_string(printfn, v, seen, indent)
    local str = tostring(v)
    printfn(str)
    return #str;
end

local fill_chars = {
    __index = function(table, key, value)
        local rv = string.rep(' ', 23 - key) .. ' = '
        rawset(table, key, rv)
        return rv
    end,
}

setmetatable(fill_chars, fill_chars)

local function print_fields(value, seen, indent, prefix)
    local prev_value = "not a value"
    local repeated = 0
    local print_field = function(k, v)
        -- Only show set values of bitfields
        if value._kind ~= "bitfield" or v then
            local continue = false
            if type(k) == "number" then
                if prev_value == v then
                    repeated = repeated + 1
                    continue = true
                else
                    prev_value = v
                end
            else
                prev_value = "not a value"
            end
            if not continue then
                if repeated > 0 then
                    dfhack.println(prefix .. "<Repeated " .. repeated .. " times>")
                    repeated = 0
                end
                dfhack.print(prefix)
                local len = do_print_recurse(dfhack.print, k, seen, indent + 1)
                dfhack.print(fill_chars[len <= 23 and len or 23])
                do_print_recurse(dfhack.println, v, seen, indent + 1)
            end
        end
    end
    if not safe_iterate(value, pairs, print_field) then
        dfhack.print(prefix)
        dfhack.println('<Type doesn\'t support iteration with pairs>')
    elseif repeated > 0 then
        dfhack.println(prefix .. "<Repeated " .. repeated .. " times>")
    end
    return 0
end

-- This should be same as print_array but userdata doesn't compare equal even if
-- they hold same pointer.
local function print_userdata(printfn, value, seen, indent)
    local prefix = string.rep('    ', indent)
    local strvalue = tostring(value)
    dfhack.println(strvalue)
    if seen[strvalue] then
        dfhack.print(prefix)
        dfhack.println('<Cyclic reference! Skipping fields>\n')
        return 0
    end
    seen[strvalue] = true
    return print_fields(value, seen, indent, prefix)
end

local function print_array(printfn, value, seen, indent)
    local prefix = string.rep('    ', indent)
    dfhack.println(tostring(value))
    if seen[value] then
        dfhack.print(prefix)
        dfhack.println('<Cyclic reference! skipping fields>\n')
        return 0
    end
    seen[value] = true
    return print_fields(value, seen, indent, prefix)
end

local recurse_type_map = {
    number = print_string,
    string = print_string,
    boolean = print_string,
    ['function'] = print_string,
    ['nil'] = print_string,
    userdata = print_userdata,
    table = print_array,
}

do_print_recurse = function(printfn, value, seen, indent)
    local t = type(value)
    if not recurse_type_map[t] then
        printfn("Unknown type " .. t .. " " .. tostring(value))
        return
    end
    return recurse_type_map[t](printfn, value, seen, indent)
end

function printall_recurse(value, seen)
    local seen = seen or {}
    do_print_recurse(dfhack.println, value, seen, 0)
end

---@generic T
---@param table T
---@return T
function copyall(table)
    local rv = {}
    for k,v in pairs(table) do rv[k] = v end
    return rv
end

---@param pos df.coord
---@return number? x
---@return number? y
---@return number? z
function pos2xyz(pos)
    if pos then
        local x = pos.x
        if x and x >= 0 then
            return x, pos.y, pos.z
        end
    end
end

---@nodiscard
---@param x number
---@param y number
---@param z number
---@return df.coord
function xyz2pos(x,y,z)
    if x then
        return {x=x,y=y,z=z}
    else
        return {x=-30000,y=-30000,z=-30000}
    end
end

---@nodiscard
---@param a df.coord
---@param b df.coord
---@return boolean
function same_xyz(a,b)
    return a and b and a.x == b.x and a.y == b.y and a.z == b.z
end

---@nodiscard
---@param path df.coord_path
---@param i number
---@return number x
---@return number y
---@return number z
function get_path_xyz(path,i)
    return path.x[i], path.y[i], path.z[i]
end

---@nodiscard
---@param pos df.coord|df.coord2d
---@return number? x
---@return number? y
function pos2xy(pos)
    if pos then
        local x = pos.x
        if x and x >= 0 then
            return x, pos.y
        end
    end
end

---@nodiscard
---@param x number
---@param y number
---@return df.coord2d
function xy2pos(x,y)
    if x then
        return {x=x,y=y}
    else
        return {x=-30000,y=-30000}
    end
end

---@nodiscard
---@param a df.coord|df.coord2d
---@param b df.coord|df.coord2d
---@return boolean
function same_xy(a,b)
    return a and b and a.x == b.x and a.y == b.y
end

---@nodiscard
---@param path df.coord_path|df.coord2d_path
---@param i number
---@return integer x
---@return integer y
function get_path_xy(path,i)
    return path.x[i], path.y[i]
end

-- Walks a sequence of dereferences, which may be represented by numbers or
-- strings. Returns nil if any of obj or indices is nil, or a numeric index is
-- out of array bounds.
---@param obj table
---@param idx number|string
---@param ... number|string
---@return any obj
function safe_index(obj,idx,...)
    local obj_type = type(obj)
    if obj == nil or idx == nil or (obj_type ~= "table" and obj_type ~= "userdata") then
        return nil
    end
    if type(idx) == 'number' and
            obj_type == 'userdata' and -- this check is only relevant for c++
            (idx < 0 or idx >= #obj)
    then
        return nil
    end
    obj = obj[idx]
    if select('#',...) > 0 then
        return safe_index(obj,...)
    else
        return obj
    end
end

---@param t table
---@param key integer|string
---@param default_value? any
---@return any
function ensure_key(t, key, default_value)
    if t[key] == nil then
        t[key] = (default_value ~= nil) and default_value or {}
    end
    return t[key]
end

---@param t table
---@param key integer|string
---@param ... integer|string
---@return table
function ensure_keys(t, key, ...)
    t = ensure_key(t, key)
    if select('#', ...) > 0 then
        return ensure_keys(t, ...)
    end
    return t
end

-- String class extentions

-- prefix is a literal string, not a pattern
---@nodiscard
---@param self string
---@param prefix string
---@return boolean
function string:startswith(prefix)
    return self:sub(1, #prefix) == prefix
end

-- suffix is a literal string, not a pattern
---@nodiscrd
---@param self string
---@param suffix string
---@return boolean
function string:endswith(suffix)
    return self:sub(-#suffix) == suffix or #suffix == 0
end

-- Split a string by the given delimiter. If no delimiter is specified, space
-- (' ') is used. The delimter is treated as a pattern unless a <plain> is
-- specified and set to true. To treat multiple successive delimiter characters
-- as a single delimiter, e.g. to avoid getting empty string elements, pass a
-- pattern like ' +'. Be aware that passing patterns that match empty strings
-- (like ' *') will result in improper string splits.
---@nodiscard
---@param self string
---@param delimiter? string
---@param plain? boolean
---@return string[]
function string:split(delimiter, plain)
    delimiter = delimiter or ' '
    local result = {}
    local from = 1
    local delim_from, delim_to = self:find(delimiter, from, plain)
    -- delim_from will be greater than delim_to when the delimiter matches the
    -- empty string, which would lead to an infinite loop if we didn't check it
    while delim_from and delim_from <= delim_to do
        table.insert(result, self:sub(from, delim_from - 1))
        from = delim_to + 1
        delim_from, delim_to = self:find(delimiter, from, plain)
    end
    table.insert(result, self:sub(from))
    return result
end

-- Removes spaces (i.e. everything that matches '%s') from the start and end of
-- a string. Spaces between non-space characters are left untouched.
---@nodiscard
---@param self string
---@return string
function string:trim()
    return self:match('^%s*(.-)%s*$')
end

local function wrap_word(word, width, wrapped_text, words, cur_line_len, opts)
    local word_len = #word
    -- word fits within the current line
    if cur_line_len + word_len <= width then
        table.insert(words, word)
        cur_line_len = cur_line_len + word_len
        return words, cur_line_len
    end
    local trimmed_word = word
    if not opts.keep_trailing_spaces then
        trimmed_word = word:trim()
        -- trimmed word fits on the current line and ends it
        if cur_line_len + #trimmed_word <= width then
            table.insert(words, trimmed_word)
            table.insert(wrapped_text, table.concat(words, ''))
            return {}, 0
        end
    end
    -- word needs to go on the next line, but is not itself longer
    -- than the specified width
    if word_len <= width then
        table.insert(wrapped_text, table.concat(words, ''))
        return {word}, word_len
    end
    -- word is too long to fit on one line and needs to be split up
    local emitted_chars, trimmed_word_len = 0, #trimmed_word
    repeat
        if #words > 0 then
            table.insert(wrapped_text, table.concat(words, ''))
        end
        local word_frag = word:sub(emitted_chars + 1, emitted_chars + width)
        words, cur_line_len = {word_frag}, #word_frag
        emitted_chars = emitted_chars + cur_line_len
    until emitted_chars >= trimmed_word_len
    return words, cur_line_len
end

-- Inserts newlines into a string so no individual line exceeds the given width.
---@nodiscard
---@param self string
---@param width number
---@param opts {return_as_table:boolean, keep_trailing_spaces:boolean}
---@return string|string[]
function string:wrap(width, opts)
    width, opts = width or 72, opts or {}
    if width <= 0 then error('expected width > 0; got: '..tostring(width)) end
    local wrapped_text = {}
    for line in self:gmatch('[^\n]*'..(opts.keep_original_newlines and '\n?' or '')) do
        local prespace = line:match('^(%s*)')
        local words, cur_line_len = {}, 0
        if #prespace > 0 then
            words, cur_line_len = wrap_word(prespace, width, wrapped_text, words, cur_line_len, opts)
        end
        for word in line:gmatch('%S+%s*') do
            words, cur_line_len = wrap_word(word, width, wrapped_text, words, cur_line_len, opts)
        end
        table.insert(wrapped_text, table.concat(words, ''))
    end
    return opts.return_as_table and wrapped_text or table.concat(wrapped_text, '\n')
end

-- Escapes regex special chars in a string. E.g. "a+b" -> "a%+b"
local regex_chars_pattern = '(['..('%^$()[].*+-?'):gsub('(.)', '%%%1')..'])'
---@nodiscard
---@param self string
---@return string
function string:escape_pattern()
    return self:gsub(regex_chars_pattern, '%%%1')
end

-- String conversions

---@nodiscard
---@param self self
---@return string
function dfhack.matinfo:__tostring()
    return "<material "..self.type..":"..self.index.." "..self:getToken()..">"
end

dfhack.random.__index = dfhack.random

---@nodiscard
---@param self self
---@return string
function dfhack.random:__tostring()
    return "<random generator>"
end

dfhack.penarray.__index = dfhack.penarray

---@nodiscard
---@return string
function dfhack.penarray.__tostring()
    return "<penarray>"
end

---@nodiscard
---@return number x
---@return number y
---@return number z
function dfhack.maps.getSize()
    local map = df.global.world.map
    return map.x_count_block, map.y_count_block, map.z_count_block
end

---@nodiscard
---@return number x
---@return number y
---@return number z
function dfhack.maps.getTileSize()
    local map = df.global.world.map
    return map.x_count, map.y_count, map.z_count
end

---@param bld building
---@return number width
---@return number height
---@return number centerx
---@return number centery
function dfhack.buildings.getSize(bld)
    local x, y = bld.x1, bld.y1
    return bld.x2+1-x, bld.y2+1-y, bld.centerx-x, bld.centery-y
end

---@nodiscard
---@param scr_type _viewscreen
---@param n? number
---@return viewscreen|nil
function dfhack.gui.getViewscreenByType(scr_type, n)
    -- translated from modules/Gui.cpp
    if n == nil then
        n = 1
    end
    local limit = (n > 0)
    local scr = dfhack.gui.getCurViewscreen()
    while scr do
        if limit then
            n = n - 1
            if n < 0 then
                return nil
            end
        end
        if scr_type:is_instance(scr) then
            return scr
        end
        scr = scr.parent
    end
end

---@nodiscard
---@return world_site|nil
function dfhack.world.getCurrentSite()
    return df.world_site.find(dfhack.world.GetCurrentSiteId())
end

---@nodiscard
---@param which string
---@param key string
---@param default? any
---@return any
local function persistent_getData(which, key, default)
    local serialized = dfhack.persistent['get'..which..'DataString'](key)
    if not serialized then return default end
    return require('json').decode(serialized) or default
end

---@param which string
---@param key string
---@param data any
local function persistent_saveData(which, key, data)
    local serialized = require('json').encode(data)
    dfhack.persistent['save'..which..'DataString'](key, serialized)
end

---@nodiscard
---@param key string
---@param default? any
---@return any
function dfhack.persistent.getSiteData(key, default)
    return persistent_getData('Site', key, default)
end

---@param key string
---@param data any
function dfhack.persistent.saveSiteData(key, data)
    persistent_saveData('Site', key, data)
end

---@param key string
---@param default? any
---@return any
function dfhack.persistent.getWorldData(key, default)
    return persistent_getData('World', key, default)
end

---@param key string
---@param data any
function dfhack.persistent.saveWorldData(key, data)
    persistent_saveData('World', key, data)
end

-- Interactive

local print_banner = true

---@param prompt? string
---@param hfile? string
---@param env? table|metatable
---@return boolean|nil
---@return string|nil
function dfhack.interpreter(prompt,hfile,env)
    if not dfhack.is_interactive() then
        return nil, 'not interactive'
    end

    print("Type quit to exit interactive lua interpreter.")

    if print_banner then
        print("Shortcuts:\n"..
              " '= foo' => '_1,_2,... = foo'\n"..
              " '! foo' => 'print(foo)'\n"..
              " '~ foo' => 'printall(foo)'\n"..
              " '^ foo' => 'printall_recurse(foo)'\n"..
              " '@ foo' => 'printall_ipairs(foo)'\n"..
              "All of these save the first result as '_'.")
        print_banner = false
    end

    local prompt_str = "["..(prompt or 'lua').."]# "
    local prompt_cont = string.rep(' ',#prompt_str-4)..">>> "
    local prompt_env = {}
    local cmdlinelist = {}
    local t_prompt = nil
    local vcnt = 1

    local pfix_handlers = {
        ['!'] = function(data)
            print(table.unpack(data,2,data.n))
        end,
        ['~'] = function(data)
            print(table.unpack(data,2,data.n))
            printall(data[2])
        end,
        ['@'] = function(data)
            print(table.unpack(data,2,data.n))
            printall_ipairs(data[2])
        end,
        ['^'] = function(data)
            printall_recurse(data[2])
        end,
        ['='] = function(data)
            for i=2,data.n do
                local varname = '_'..vcnt
                prompt_env[varname] = data[i]
                dfhack.print(varname..' = ')
                safecall(print, data[i])
                vcnt = vcnt + 1
            end
        end
    }

    setmetatable(prompt_env, { __index = env or _G })

    while true do
        local cmdline = dfhack.lineedit(t_prompt or prompt_str, hfile)

        if cmdline == nil or cmdline == 'quit' then
            break
        elseif cmdline ~= '' then
            local pfix = string.sub(cmdline,1,1)

            if not t_prompt and pfix_handlers[pfix] then
                cmdline = 'return '..string.sub(cmdline,2)
            else
                pfix = nil
            end

            table.insert(cmdlinelist,cmdline)
            cmdline = table.concat(cmdlinelist,'\n')

            local code,err = load(cmdline, '=(interactive)', 't', prompt_env)

            if code == nil then
                if not pfix and err:sub(-5)=="<eof>" then
                    t_prompt=prompt_cont
                else
                    dfhack.printerr(err)
                    cmdlinelist={}
                    t_prompt=nil
                end
            else
                cmdlinelist={}
                t_prompt=nil

                local data = table.pack(safecall(code))

                if data[1] and data.n > 1 then
                    prompt_env._ = data[2]
                    safecall(pfix_handlers[pfix], data)
                end
            end
        end
    end

    return true
end

-- Command scripts

local internal = dfhack.internal

Script = defclass(Script)
function Script:init(path)
    self.path = path
    self.mtime = dfhack.filesystem.mtime(path)
    self._flags = {}
end
function Script:needs_update()
    return (not self.env) or self.mtime ~= dfhack.filesystem.mtime(self.path)
end
function Script:get_flags()
    local mtime = dfhack.filesystem.mtime(self.path)
    if self.flags_mtime ~= mtime then
        self.flags_mtime = mtime
        self._flags = {}
        local f = io.open(self.path)
        for line in f:lines() do
            local at_tag = line:match('^%-%-@(.+)')
            if not at_tag then goto continue end
            local chunk = load(at_tag, self.path, 't', self._flags)
            if chunk then
                chunk()
            else
                dfhack.printerr('Parse error: ' .. line)
            end
            ::continue::
        end
        f:close()
    end
    return self._flags
end

internal.scripts = internal.scripts or {}

function dfhack.findScript(name)
    return dfhack.internal.findScript(name .. '.lua')
end

local valid_script_flags = {
    enable = {required = true, error = 'Does not recognize enable/disable commands'},
    enable_state = {required = false},
    module = {
        required = function(flags)
            if flags.module_strict == false then return false end
            return true
        end,
        error = 'Cannot be used as a module'
    },
    module_strict = {required = false},
    alias = {required = false},
    alias_count = {required = false},
    scripts = {required = false},
}

local warned_scripts = {}

function dfhack.run_script(name,...)
    if not warned_scripts[name] then
        local helpdb = require('helpdb')
        if helpdb.has_tag(name, 'unavailable') then
            warned_scripts[name] = true
            dfhack.printerr(('UNTESTED WARNING: the "%s" script has not been validated to work well with this version of DF.'):format(name))
            dfhack.printerr('It may not work as expected, or it may corrupt your game.')
            qerror('Please run the command again to ignore this warning and proceed.')
        end
    end

    return dfhack.run_script_with_env(nil, name, nil, ...)
end

function dfhack.enable_script(name, state)
    local res, err = dfhack.pcall(dfhack.run_script_with_env, nil, name, {enable=true, enable_state=state})
    if not res then
        dfhack.printerr(err.message)
        qerror(('Cannot %s Lua script: %s'):format(state and 'enable' or 'disable', name))
    end
end

function dfhack.reqscript(name)
    return dfhack.script_environment(name, true)
end
reqscript = dfhack.reqscript

function dfhack.script_environment(name, strict)
    local scripts = internal.scripts
    local path = dfhack.findScript(name)
    if not scripts[path] or scripts[path]:needs_update() then
        local _, env = dfhack.run_script_with_env(nil, name, {
            module=true,
            module_strict=(strict and true or false)  -- ensure that this key is present if 'strict' is nil
        })
        return env
    else
        if strict and not scripts[path]:get_flags().module then
            error(('%s: %s'):format(name, valid_script_flags.module.error))
        end
        return scripts[path].env
    end
end

function dfhack.run_script_with_env(envVars, name, flags, ...)
    if type(flags) ~= 'table' then flags = {} end
    local file = dfhack.findScript(name)
    if not file then
        error('Could not find script ' .. name)
    end

    local scripts = flags.scripts or internal.scripts
    if scripts[file] == nil then
        scripts[file] = Script(file)
    end
    local script_flags = scripts[file]:get_flags()
    if script_flags.alias then
        flags.alias_count = (flags.alias_count or 0) + 1
        if flags.alias_count > 10 then
            error('Too many script aliases: ' .. flags.alias_count)
        end
        return dfhack.run_script_with_env(envVars, script_flags.alias, flags, ...)
    end
    for flag, value in pairs(flags) do
        if value then
            local v = valid_script_flags[flag]
            if not v then
                error('Invalid flag: ' .. flag)
            elseif ((type(v.required) == 'boolean' and v.required) or
                    (type(v.required) == 'function' and v.required(flags))) then
                if not script_flags[flag] then
                    local msg = v.error or ('Flag "' .. flag .. '" not recognized')
                    error(name .. ': ' .. msg)
                end
            end
        end
    end

    local env = scripts[file].env
    if env == nil then
        env = {}
        setmetatable(env, { __index = base_env })
    end
    for x,y in pairs(envVars or {}) do
        env[x] = y
    end
    env.dfhack_flags = flags
    env.moduleMode = flags.module
    local script_code
    local perr
    local time = dfhack.filesystem.mtime(file)
    if time == scripts[file].mtime and scripts[file].run then
        script_code = scripts[file].run
    else
        --reload
        script_code, perr = loadfile(file, 't', env)
        if not script_code then
            error(perr)
        end
        -- avoid updating mtime if the script failed to load
        scripts[file].mtime = time
    end
    scripts[file].env = env
    scripts[file].run = script_code
    local args = {...}
    for i,v in ipairs(args) do
        args[i] = tostring(v) -- ensure passed parameters are strings
    end
    return script_code(table.unpack(args)), env
end

function dfhack.current_script_name()
    local frame = 1
    while true do
        local info = debug.getinfo(frame, 'f')
        if not info then break end
        if info.func == dfhack.run_script_with_env then
            local i = 1
            while true do
                local name, value = debug.getlocal(frame, i)
                if not name then break end
                if name == 'name' then
                    return value
                end
                i = i + 1
            end
            break
        end
        frame = frame + 1
    end
end

function dfhack.script_help(script_name, extension)
    script_name = script_name or dfhack.current_script_name()
    return require('helpdb').get_entry_long_help(script_name)
end

local function _run_command(args, use_console)
    if type(args[1]) == 'table' then
        command = args[1]
    elseif #args > 1 and type(args[2]) == 'table' then
        -- {args[1]} + args[2]
        command = args[2]
        table.insert(command, 1, args[1])
    elseif #args == 1 and type(args[1]) == 'string' then
        command = args[1]
    elseif #args > 1 and type(args[1]) == 'string' then
        command = args
    else
        error('Invalid arguments')
    end
    return internal.runCommand(command, use_console)
end

function dfhack.run_command_silent(...)
    local result = _run_command({...})
    local output = ""
    for i, f in pairs(result) do
        if type(f) == 'table' then
            output = output .. f[2]
        end
    end
    return output, result.status
end

function dfhack.run_command(...)
    local result = _run_command({...}, true)
    return result.status
end

-- Per-save init file

function dfhack.getSavePath()
    if dfhack.isWorldLoaded() then
        return dfhack.getDFPath() .. '/save/' .. df.global.world.cur_savegame.save_dir
    end
end

if dfhack.is_core_context then
    local function loadInitFile(path, name)
        local env = setmetatable({ SAVE_PATH = path }, { __index = base_env })
        local f,perr = loadfile(name, 't', env)
        if f == nil then
            if dfhack.filesystem.exists(name) then
                dfhack.printerr(perr)
            end
        elseif safecall(f) then
            if not internal.save_init then
                internal.save_init = {}
            end
            table.insert(internal.save_init, env)
        end
    end

    dfhack.onStateChange.DFHACK_PER_SAVE = function(op)
        if op == SC_WORLD_LOADED or op == SC_WORLD_UNLOADED then
            if internal.save_init then
                for k,v in ipairs(internal.save_init) do
                    if v.onUnload then
                        safecall(v.onUnload)
                    end
                end
                internal.save_init = nil
            end

            local path = dfhack.getSavePath()

            if path and op == SC_WORLD_LOADED then
                loadInitFile(path, path..'/init.lua')

                local dirlist = dfhack.internal.getDir(path..'/init.d/')
                if dirlist then
                    table.sort(dirlist)
                    for i,name in ipairs(dirlist) do
                        if string.match(name,'%.lua$') then
                            loadInitFile(path, path..'/init.d/'..name)
                        end
                    end
                end
            end
        elseif internal.save_init then
            for k,v in ipairs(internal.save_init) do
                if v.onStateChange then
                    safecall(v.onStateChange, op)
                end
            end
        end
    end
end

-- Feed the table back to the require() mechanism.
return dfhack
