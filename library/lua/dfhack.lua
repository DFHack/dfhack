-- Common startup file for all dfhack plugins with lua support
-- The global dfhack table is already created by C++ init code.

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

-- Error handling

safecall = dfhack.safecall

function dfhack.pcall(f, ...)
    return xpcall(f, dfhack.onerror, ...)
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
    setmetatable(pkg, { __index = (env or _G) })
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

-- Misc functions

function printall(table)
    if type(table) == 'table' or df.isvalid(table) == 'ref' then
        for k,v in pairs(table) do
            print(string.format("%-23s\t = %s",tostring(k),tostring(v)))
        end
    end
end

function dfhack.persistent:__tostring()
    return "<persistent "..self.entry_id..":"..self.key.."=\""
           ..self.value.."\":"..table.concat(self.ints,",")..">"
end

function dfhack.matinfo:__tostring()
    return "<material "..self.type..":"..self.index.." "..self:getToken()..">"
end

-- Feed the table back to the require() mechanism.
return dfhack
