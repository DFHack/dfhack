
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

return _ENV
