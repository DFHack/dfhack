
local _ENV = mkmodule('persist-table')

--[[
persist-table.lua
author expwnent

This module is intended to help facilitate persistent table lookups.
It is a wrapper over dfhack.persistent calls.
It supports tables of arbitrary dimension and shape.
Argument names should not include any of the following symbols: _\/&
It stores information about each table and subtable's size and children.

For convenience, all stored information is itself persistent.
It would be more efficient to cache stored information in global variables and update it on save/load but this is not yet implemented.
Ask expwnent to try this if performance is bad.
--]]

--[[
function accessTable(...)
 local args = {...}
 local name = 'mastertable'
 local previousName = nil
 local child
 for n,arg in ipairs(args) do
  child = ensure(prefix .. name .. '$$' .. arg)
  local old = child.value
  if old == '' then
   child.value = gensym()
   child.ints[1] = 0
   local size = ensure(prefix .. name .. '$size')
   size.value = tostring(1+(tonumber(size.value) or 0))
   size:save()
   if previousName then
    --local size = ensure(previousName .. '$size')
    --size.value = tostring(1+(tonumber(size.value) or 0))
    --size:save()
    local prev = ensure(prefix .. name .. '$previous')
    prev.value = previousName
    prev:save()
   end
   child:save()
   --new child
   dfhack.persistent.save({key=prefix .. name, value=arg}, true)
  end
  --print(n,arg,previousName,child.key,child.value)
  previousName = name
  name = child.value
 end
 return child
end

function deleteTable(name)
 if not name then
  do return end
 end
 local previous = ensure(prefix .. name .. '$previous').value
 local children = dfhack.persistent.get_all(prefix .. name) or {}
 for _,child in ipairs(children) do
  --print('delete: ', name, previous, child)
  local ptr = ensure(prefix .. name .. '$$' .. child.value)
  if ( ptr.ints[1] == 0 ) then
   --releasesym(ptr.value)
   deleteTable(ptr.value)
  end
  ptr:delete()
  child:delete()
 end
 ensure(prefix .. name .. '$previous'):delete()
 ensure(prefix .. name .. '$size'):delete()
 if previous ~= '' then
  local size = ensure(prefix .. previous .. '$size')
  size.value = tostring(-1 + tonumber(size.value))
  size:save()
  local children = dfhack.persistent.get_all(prefix .. previous) or {}
  for _,sibling in ipairs(children) do
   --print(sibling.value, name, previous .. '$$' .. sibling.value)
   local ptr = ensure(prefix .. previous .. '$$' .. sibling.value)
   if ptr.value == name then
    ptr:delete()
    sibling:delete()
   end
  end
 end
 releasesym(name)
end

function setTable(...)
 local args = {...}
 local last = args[#args]
 table.remove(args,#args)
 --table.setn(args, #args-1)
 local entry = accessTable(table.unpack(args))
 local old = entry.value
 if entry.ints[1] == 0 then
  deleteTable(old)
 end
 entry.ints[1] = -1
 entry.value = last
 entry:save()
 return old
end
--]]


prefix = 'persist-table'

function ensure(name)
 return dfhack.persistent.save({key=name})
end

function gensym()
 local availables = dfhack.persistent.get_all(prefix .. '$available') or {}
 local available = nil
 local smallest = nil
 for _,candidate in pairs(availables) do
  --TODO: it would be great if we could iterate over these in order but we can't
  local score = tonumber(candidate.value)
  print('gensym', candidate, score, available, smallest)
  if (score and (not available or score < smallest)) then
   smallest = score
   available = candidate
  end
 end
 if available then
  local value = available.value
  available:delete()
  print('gensym: allocate ' .. value)
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
  print('gensym: allocate ' .. result)
 return result
end

function releasesym(symb)
 local availables = dfhack.persistent.get_all(prefix .. '$available') or {}
 for _,available in pairs(availables) do
  print('releasesym: ', symb, available.value, available)
  if available.value == symb then
   print('error: persist-table.releasesym(' .. symb .. '): available.value = ' .. available.value)
   return
  end
 end
 dfhack.persistent.save({key=prefix .. '$available', value=symb}, true)
 print('releasesym: unallocate ' .. symb)
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

GlobalTable = {key = 'mastertable'}
GlobalTable.mt = {}
GlobalTable.mt.__index = function(table, key)
 print(rawget(table,'key') .. '[' .. key .. ']')
 local entry = ensure(prefix .. rawget(table,'key') .. '$$' .. key)
 if entry.ints[existIndex] == existValue and entry.ints[pointerIndex] == defaultValue then
  print('string: ' .. entry.value)
  return entry.value
 end
 if entry.ints[pointerIndex] == pointerValue then
  --pre-existing pointer
  local result = {key = entry.value}
  result.mt = rawget(GlobalTable,'mt')
  setmetatable(result,rawget(GlobalTable,'mt'))
  print('table: ' .. entry.value)
  return result
 end
 entry:delete()
 print 'table[key] does not exist.'
 return nil
end
GlobalTable.mt.__newindex = function(table, key, value)
 print(rawget(table,'key') .. '[' .. key .. '] = ' .. tostring(value))
 local entry = ensure(prefix .. rawget(table,'key') .. '$$' .. key)
 local old = entry.value
 local isNew = entry.ints[existIndex] == defaultValue
 if entry.ints[existIndex] == existValue and entry.ints[pointerIndex] == pointerValue then
  if type(value) == 'table' and rawget(value,'mt') == rawget(GlobalTable,mt) and entry.value == rawget(value,'key') then
   --if setting it to the same table it already is, then don't do anything
   return
  end
  deletePersistent(entry.value)
 end
 if not value then
  print('__newindesx: delete')
  --delete
  for i,child in ipairs(dfhack.persistent.get_all(prefix .. rawget(table,'key')) or {}) do
   if child.value == key then
    child:delete()
   end
  end
  entry:delete()
  return
 elseif type(value) == 'string' then
  print('__newindesx: string')
  entry.value = value
  entry.ints[pointerIndex] = defaultValue
  entry.ints[existIndex] = existValue
  entry:save()
  if isNew then
   print('new child!')
   dfhack.persistent.save({key=prefix .. rawget(table,'key'), value=key}, true)
  end
  return
 elseif type(value) == 'table' then
  print('__newindesx: table')
  if rawget(value,'mt') ~= rawget(GlobalTable,'mt') then
   if not isEmpty(value) then
    error('setting value to an invalid table')
   end
   print('__newindesx: empty table')
   --empty table: allocate a thing
   entry.ints[pointerIndex] = pointerValue
   entry.ints[existIndex] = existValue
   entry.value = gensym()
   entry:save()

   if isNew then
    print('new child!')
    dfhack.persistent.save({key=prefix .. rawget(table,'key'), value=key}, true)
   end
   return
  end
  print('__newindesx: table assignment')
  entry.value = rawget(value,'key')
  entry.ints[pointerIndex] = pointerValue
  entry.ints[existIndex] = existValue
  entry:save()
  if isNew then
   print('new child!')
   dfhack.persistent.save({key=prefix .. rawget(table,'key'), value=key}, true)
  end
  return
 else
  error('type(value) = ' .. type(value))
 end
end
setmetatable(GlobalTable, GlobalTable.mt)

return _ENV
