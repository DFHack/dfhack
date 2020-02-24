-- Common startup file for all dfhack plugins with lua support
-- The global dfhack table is already created by C++ init code.

-- Setup the global environment.
-- BASE_G is the original lua global environment,
-- preserved as a common denominator for all modules.
-- This file uses it instead of the new default one.

local dfhack = dfhack
local base_env = dfhack.BASE_G
local _ENV = base_env

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

-- Error handling

safecall = dfhack.safecall
curry = dfhack.curry

function dfhack.pcall(f, ...)
    return xpcall(f, dfhack.onerror, ...)
end

function qerror(msg, level)
    local name = dfhack.current_script_name()
    if name and not tostring(msg):match(name) then
        msg = name .. ': ' .. tostring(msg)
    end
    dfhack.error(msg, (level or 1) + 1, false)
end

function dfhack.with_finalize(...)
    return dfhack.call_with_finalizer(0,true,...)
end
function dfhack.with_onerror(...)
    return dfhack.call_with_finalizer(0,false,...)
end

local function call_delete(obj)
    if obj then obj:delete() end
end

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

DEFAULT_NIL = DEFAULT_NIL or {} -- Unique token

function defclass(...)
    return require('class').defclass(...)
end

function mkinstance(...)
    return require('class').mkinstance(...)
end

-- Misc functions

NEWLINE = "\n"
COMMA = ","
PERIOD = "."

function printall(table)
    local ok,f,t,k = pcall(pairs,table)
    if ok then
        for k,v in f,t,k do
            print(string.format("%-23s\t = %s",tostring(k),tostring(v)))
        end
    end
end

function printall_ipairs(table)
    local ok,f,t,k = pcall(ipairs,table)
    if ok then
        for k,v in f,t,k do
            print(string.format("%-23s\t = %s",tostring(k),tostring(v)))
        end
    end
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
    local ok,f,t,k = pcall(pairs,value)
    if not ok then
        dfhack.print(prefix)
        dfhack.println('<Type doesn\'t support iteration with pairs>')
        return 0
    end
    local prev_value = "not a value"
    local repeated = 0
    for k, v in f,t,k do
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
    if repeated > 0 then
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

function copyall(table)
    local rv = {}
    for k,v in pairs(table) do rv[k] = v end
    return rv
end

function pos2xyz(pos)
    if pos then
        local x = pos.x
        if x and x ~= -30000 then
            return x, pos.y, pos.z
        end
    end
end

function xyz2pos(x,y,z)
    if x then
        return {x=x,y=y,z=z}
    else
        return {x=-30000,y=-30000,z=-30000}
    end
end

function same_xyz(a,b)
    return a and b and a.x == b.x and a.y == b.y and a.z == b.z
end

function get_path_xyz(path,i)
    return path.x[i], path.y[i], path.z[i]
end

function pos2xy(pos)
    if pos then
        local x = pos.x
        if x and x ~= -30000 then
            return x, pos.y
        end
    end
end

function xy2pos(x,y)
    if x then
        return {x=x,y=y}
    else
        return {x=-30000,y=-30000}
    end
end

function same_xy(a,b)
    return a and b and a.x == b.x and a.y == b.y
end

function get_path_xy(path,i)
    return path.x[i], path.y[i]
end

function safe_index(obj,idx,...)
    if obj == nil or idx == nil then
        return nil
    end
    if type(idx) == 'number' and (idx < 0 or idx >= #obj) then
        return nil
    end
    obj = obj[idx]
    if select('#',...) > 0 then
        return safe_index(obj,...)
    else
        return obj
    end
end

function string:startswith(prefix)
    return self:sub(1, #prefix) == prefix
end

function string:endswith(suffix)
    return self:sub(-#suffix) == suffix or #suffix == 0
end

-- String conversions

function dfhack.persistent:__tostring()
    return "<persistent "..self.entry_id..":"..self.key.."=\""
           ..self.value.."\":"..table.concat(self.ints,",")..">"
end

function dfhack.matinfo:__tostring()
    return "<material "..self.type..":"..self.index.." "..self:getToken()..">"
end

dfhack.random.__index = dfhack.random

function dfhack.random:__tostring()
    return "<random generator>"
end

dfhack.penarray.__index = dfhack.penarray

function dfhack.penarray.__tostring()
    return "<penarray>"
end

function dfhack.maps.getSize()
    local map = df.global.world.map
    return map.x_count_block, map.y_count_block, map.z_count_block
end

function dfhack.maps.getTileSize()
    local map = df.global.world.map
    return map.x_count, map.y_count, map.z_count
end

function dfhack.buildings.getSize(bld)
    local x, y = bld.x1, bld.y1
    return bld.x2+1-x, bld.y2+1-y, bld.centerx-x, bld.centery-y
end

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

-- Interactive

local print_banner = true

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
    if self.flags_mtime ~= dfhack.filesystem.mtime(self.path) then
        self.flags_mtime = dfhack.filesystem.mtime(self.path)
        self._flags = {}
        local f = io.open(self.path)
        local contents = f:read('*all')
        f:close()
        for line in contents:gmatch('%-%-@([^\n]+)') do
            local chunk = load(line, self.path, 't', self._flags)
            if chunk then
                chunk()
            else
                dfhack.printerr('Parse error: ' .. line)
            end
        end
    end
    return self._flags
end

internal.scripts = internal.scripts or {}
local scripts = internal.scripts

local hack_path = dfhack.getHackPath()

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
}

function dfhack.run_script(name,...)
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
                    local msg = v.error or 'Flag "' .. flag .. '" not recognized'
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
    return script_code(...), env
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
    extension = extension or 'lua'
    local full_name = script_name .. '.' .. extension
    local path = dfhack.internal.findScript(script_name .. '.' .. extension)
        or error("Could not find script: " .. full_name)
    local begin_seq, end_seq
    if extension == 'rb' then
        begin_seq = '=begin'
        end_seq = '=end'
    else
        begin_seq = '[====['
        end_seq = ']====]'
    end
    local f = io.open(path) or error("Could not open " .. path)
    local in_help = false
    local help = ''
    for line in f:lines() do
        if line:endswith(begin_seq) then
            in_help = true
        elseif in_help then
            if line:endswith(end_seq) then
                break
            end
            if line ~= script_name and line ~= ('='):rep(#script_name) then
                help = help .. line .. '\n'
            end
        end
    end
    f:close()
    help = help:gsub('^\n+', ''):gsub('\n+$', '')
    return help
end

local function _run_command(...)
    args = {...}
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
    return internal.runCommand(command)
end

function dfhack.run_command_silent(...)
    local result = _run_command(...)
    local output = ""
    for i, f in pairs(result) do
        if type(f) == 'table' then
            output = output .. f[2]
        end
    end
    return output, result.status
end

function dfhack.run_command(...)
    local result = _run_command(...)
    for i, f in pairs(result) do
        if type(f) == 'table' then
            dfhack.color(f[1])
            dfhack.print(f[2])
        end
    end
    dfhack.color(COLOR_RESET)
    return result.status
end

-- Per-save init file

function dfhack.getSavePath()
    if dfhack.isWorldLoaded() then
        return dfhack.getDFPath() .. '/data/save/' .. df.global.world.cur_savegame.save_dir
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
                loadInitFile(path, path..'/raw/init.lua')

                local dirlist = dfhack.internal.getDir(path..'/raw/init.d/')
                if dirlist then
                    table.sort(dirlist)
                    for i,name in ipairs(dirlist) do
                        if string.match(name,'%.lua$') then
                            loadInitFile(path, path..'/raw/init.d/'..name)
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
