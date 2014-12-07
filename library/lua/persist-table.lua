
local _ENV = mkmodule('persist-table')

--[[
persist-table.lua
author expwnent

This module is intended to help facilitate persistent table lookups.
It is a wrapper over dfhack.persistent calls.
It supports tables of arbitrary dimension and shape.
It stores information about each table and subtable's size and children.

For convenience, all stored information is itself persistent.
It would be more efficient to cache stored information in global variables and update it on save/load but this is not yet implemented.
Ask expwnent to try this if performance is bad.

Usage:
local persistTable = require 'persist-table'
persistTable.GlobalTable.doomitude = 'doom!' -- will be stored persistently
print(persistTable.GlobalTable.doomitude) --doom!
persistTable.GlobalTable.doomitude = nil --delete the persistent record
print(persistTable.GlobalTable.doomitude) --nil

persistTable.GlobalTable.mana = {} --allocate a subtable for mana
local mana = persistTable.GlobalTable.mana --setting elements in this table will be persistent too
mana['1'] = '3' --slightly faster than persistTable.GlobalTable.mana['1'] = '3'
--be aware that if you don't change the local variable mana when the game exits and reloads a different save you will run into mysterious problems so don't do that
mana['2'] = '100'
mana.maximum = '1000' --tables can be any arbitrary shape
local globalTable = persistTable.GlobalTable --this is safe too
globalTable.mana = nil --this is safe: it will deallocate all subtables cleanly
globalTable.revengeDesire = {}
--globalTable.revengeDseire.foo = {} -- error: tables must be allocated first with parentTable.subtableName = {}
--you can check if it exists with globalTable.tableName != nil
local revengeTable = globalTable.revengeDesire
revengeTable['2'] = revengeTable['2'] or {} --this is fine too
revengeTable['2']['3'] = 'totally a lot, man'
revengeTable['3'] = {}
revengeTable['3']['2'] = 'maybe a little bit'
revengeTable['2'] = nil
-------------
And so on.

Be careful not to name your tables in a way that will conflict with other scripts! The easiest way is to just put all your tables in one giant table named based on your script.

All stored values MUST be strings. Numbers can be supported later but are not yet supported. Keep in mind there is a significant overhead for each table element so don't go totally crazy storing massive amounts of information or the game will run out of RAM.

--table._children returns a list of child keys
for _,childKey in ipairs(table._children) do
 local child = table[childKey]
 --blah
end

--]]

local prefix = 'persist-table'

local function ensure(name)
 return dfhack.persistent.save({key=name})
end

local function gensym()
 local availables = dfhack.persistent.get_all(prefix .. '$available') or {}
 local available = nil
 local smallest = nil
 for _,candidate in pairs(availables) do
  --TODO: it would be great if we could iterate over these in order but we can't
  local score = tonumber(candidate.value)
  --print('gensym', candidate, score, available, smallest)
  if (score and (not available or score < smallest)) then
   smallest = score
   available = candidate
  end
 end
 if available then
  local value = available.value
  available:delete()
  --print('gensym: allocate ' .. value)
  return value
 end
 --none explicitly available, so smallest unused is the next available number
 local smallestUnused = ensure(prefix .. '$smallest_unused')
 if smallestUnused.value == '' then
  smallestUnused.value = '0'
 end
 local result = smallestUnused.value
 smallestUnused.value = tostring(1+tonumber(result))
 smallestUnused:save()
  --print('gensym: allocate ' .. result)
 return result
end

local function releasesym(symb)
 local availables = dfhack.persistent.get_all(prefix .. '$available') or {}
 for _,available in pairs(availables) do
  --print('releasesym: ', symb, available.value, available)
  if available.value == symb then
   print('error: persist-table.releasesym(' .. symb .. '): available.value = ' .. available.value)
   return
  end
 end
 dfhack.persistent.save({key=prefix .. '$available', value=symb}, true)
 --print('releasesym: unallocate ' .. symb)
end
local intCount = 7
local existIndex = intCount-0
local existValue = 1
local pointerIndex = intCount-1
local pointerValue = 1
local defaultValue = -1

local function isEmpty(table)
 return next(table) == nil
end

local function deletePersistent(name)
  if name == '' then
   return
  end
  local children = dfhack.persistent.get_all(prefix .. name) or {}
  for _,childKey in ipairs(children) do
   local childEntry = ensure(prefix .. name .. '$$' .. childKey.value)
   if childEntry.ints[existIndex] == existValue and childEntry.ints[pointerIndex] == pointerValue then
    deletePersistent(childEntry.value)
   end
   childEntry:delete()
   childKey:delete()
  end
  releasesym(name)
end

GlobalTable = GlobalTable or {key = 'mastertable'}
GlobalTable.mt = GlobalTable.mt or {}
GlobalTable.mt.__index = GlobalTable.mt.__index or function(theTable, key)
 if key == '_children' then
  return rawget(theTable,key)
 end
 --print(rawget(theTable,'key') .. '[' .. key .. ']')
 local entry = ensure(prefix .. rawget(theTable,'key') .. '$$' .. key)
 if entry.ints[existIndex] == existValue and entry.ints[pointerIndex] == defaultValue then
  --print('string: ' .. entry.value)
  return entry.value
 end
 if entry.ints[pointerIndex] == pointerValue then
  --pre-existing pointer
  local result = {key = entry.value}
  result.mt = rawget(GlobalTable,'mt')
  local childArgs = dfhack.persistent.get_all(prefix .. entry.value)
  result._children = {}
  for _,childArg in ipairs(childArgs or {}) do
   --print(result._children, childArg.value)
   table.insert(result._children, childArg.value)
  end
  setmetatable(result,rawget(GlobalTable,'mt'))
  --print('theTable: ' .. entry.value)
  return result
 end
 entry:delete()
 --print 'theTable[key] does not exist.'
 return nil
end
GlobalTable.mt.__newindex = GlobalTable.mt.__newindex or function(theTable, key, value)
 --print(rawget(theTable,'key') .. '[' .. key .. '] = ' .. tostring(value))
 local entry = ensure(prefix .. rawget(theTable,'key') .. '$$' .. key)
 local old = entry.value
 local isNew = entry.ints[existIndex] == defaultValue
 if entry.ints[existIndex] == existValue and entry.ints[pointerIndex] == pointerValue then
  if type(value) == 'table' and rawget(value,'mt') == rawget(GlobalTable,'mt') and entry.value == rawget(value,'key') then
   --if setting it to the same table it already is, then don't do anything
   return
  end
  deletePersistent(entry.value)
 end
 if not value then
  --print('__newindesx: delete')
  --delete
  for i,child in ipairs(dfhack.persistent.get_all(prefix .. rawget(theTable,'key')) or {}) do
   if child.value == key then
    child:delete()
   end
  end
  entry:delete()
  return
 elseif type(value) == 'string' then
  --print('__newindesx: string')
  entry.value = value
  entry.ints[pointerIndex] = defaultValue
  entry.ints[existIndex] = existValue
  entry:save()
  if isNew then
   --print('new child!')
   dfhack.persistent.save({key=prefix .. rawget(theTable,'key'), value=key}, true)
  end
  return
 elseif type(value) == 'table' then
  --print('__newindesx: table')
  if rawget(value,'mt') ~= rawget(GlobalTable,'mt') then
   if not isEmpty(value) then
    error('setting value to an invalid table')
   end
   --print('__newindesx: empty table')
   --empty table: allocate a thing
   entry.ints[pointerIndex] = pointerValue
   entry.ints[existIndex] = existValue
   entry.value = gensym()
   entry:save()

   if isNew then
    --print('new child!')
    dfhack.persistent.save({key=prefix .. rawget(theTable,'key'), value=key}, true)
   end
   return
  end
  --print('__newindesx: table assignment')
  entry.value = rawget(value,'key')
  entry.ints[pointerIndex] = pointerValue
  entry.ints[existIndex] = existValue
  entry:save()
  if isNew then
   --print('new child!')
   dfhack.persistent.save({key=prefix .. rawget(theTable,'key'), value=key}, true)
  end
  return
 else
  error('type(value) = ' .. type(value))
 end
end
setmetatable(GlobalTable, GlobalTable.mt)

return _ENV
