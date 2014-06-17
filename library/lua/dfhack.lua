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

function mkmodule(module,env)
    local pkg = package.loaded[module]
    if pkg == nil then
        pkg = {}
    else
        if type(pkg) ~= 'table' then
            error("Not a table in package.loaded["..module.."]")
        end
    end
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

internal.scripts = internal.scripts or {}

local scripts = internal.scripts
local hack_path = dfhack.getHackPath()

function dfhack.run_script(name,...)
    local key = string.lower(name)
    local file = hack_path..'scripts/'..name..'.lua'
    local env = scripts[key]
    if env == nil then
        env = {}
        setmetatable(env, { __index = base_env })
    end
    local f,perr = loadfile(file, 't', env)
    if f == nil then
        error(perr)
    end
    scripts[key] = env
    return f(...)
end

function dfhack.run_command(...)
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
    result = internal.runCommand(command)
    output = ""
    for i, f in pairs(result) do
        if type(f) == 'table' then
            output = output .. f[2]
        end
    end
    return output, result.status
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
            if not string.match(perr, 'No such file or directory') then
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
