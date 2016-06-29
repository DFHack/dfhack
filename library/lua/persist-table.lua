local _ENV = mkmodule('persist-table')

if not GlobalTable then
  GlobalTable = {}
  setmetatable(GlobalTable, {
    -- It's tempting to store the result of .getPersistTable(),
    -- but we must not keep references when the world is unloaded.
    __index = function(table, key)
      local mastertable = dfhack.persistent.getPersistTable()
      return mastertable[key]
    end,
    __newindex = function(table, key, value)
      local mastertable = dfhack.persistent.getPersistTable()
      mastertable[key] = value
    end,
    __tostring = function(table)
      local mastertable = dfhack.persistent.getPersistTable()
      return tostring(mastertable)
    end,
  })
end

return _ENV
