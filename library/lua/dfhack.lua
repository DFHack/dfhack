-- Common startup file for all dfhack plugins with lua support
-- The global dfhack table is already created by C++ init code.

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

function printall(table)
    if table == nil then return end
    for k,v in pairs(table) do
        print(k,"   = "..tostring(v))
    end
end

function dfhack.persistent:__tostring()
    return "<persistent "..self.entry_id..":"..self.key.."=\""
           ..self.value.."\":"..table.concat(self.ints,",")..">"
end

-- Feed the table back to the require() mechanism.
return dfhack
