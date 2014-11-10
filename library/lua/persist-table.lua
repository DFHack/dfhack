
local _ENV = mkmodule('persist-table')

symbols = symbols or {}
symbolCount = symbolCount or {}

function ensure(name)
 return dfhack.persistent.save({key=name})
end

function gensym(prefix)
 if not symbols[prefix] then
  symbols[prefix] = {}
 end
 local sym = symbols[prefix] or {}
 local i = 0
 while true do
  if not sym[i] then
   symbols[prefix][i] = true
   symbolCount[prefix] = math.max(symbolCount[prefix] or 0,i)
   return prefix .. '&' .. tostring(i)
  end
  i = i+1
 end
end

function releasesym(prefix, symb)
 local sym = symbols[prefix] or {}
 local i = 0
 while true do
  if prefix .. '&' .. tostring(i) == symb then
   symbols[prefix][i] = nil
   --symbolCount[prefix] = -1 + symbolCount[prefix]
  end
  i = i+1
  if i > symbolCount[prefix] then
   return
  end
 end
end

function accessTable(prefix,...)
 local args = {...}
 local name = '__master_table'
 local previousName = nil
 local child
 for n,arg in ipairs(args) do
  child = ensure(name .. '$$' .. arg)
  local old = child.value
  if old == '' then
   child.value = gensym(prefix)
   local size = ensure(name .. '$size')
   size.value = tostring(1+(tonumber(size.value) or 0))
   size:save()
   if previousName then
    --local size = ensure(previousName .. '$size')
    --size.value = tostring(1+(tonumber(size.value) or 0))
    --size:save()
    local prev = ensure(name .. '$previous')
    prev.value = previousName
    prev:save()
   end
   child:save()
   --new child
   dfhack.persistent.save({key=name, value=arg}, true)
  end
  --print(n,arg,previousName,child.key,child.value)
  previousName = name
  name = child.value
 end
 return child
end

function deleteTable(prefix,name)
 if not name then
  do return end
 end
 local previous = ensure(name .. '$previous').value
 local children = dfhack.persistent.get_all(name) or {}
 for _,child in ipairs(children) do
  --print('delete: ', name, previous, child)
  local ptr = ensure(name .. '$$' .. child.value)
  releasesym(prefix,ptr.value)
  deleteTable(prefix,ptr.value)
  ptr:delete()
  child:delete()
 end
 ensure(name .. '$previous'):delete()
 ensure(name .. '$size'):delete()
 if previous ~= '' then
  local size = ensure(previous .. '$size')
  size.value = tostring(-1 + tonumber(size.value))
  size:save()
  local children = dfhack.persistent.get_all(previous) or {}
  for _,sibling in ipairs(children) do
   --print(sibling.value, name, previous .. '$$' .. sibling.value)
   local ptr = ensure(previous .. '$$' .. sibling.value)
   if ptr.value == name then
    ptr:delete()
    sibling:delete()
   end
  end
 end
 releasesym(prefix,name)
end

function setTable(prefix,...)
 local args = {...}
 local last = args[#args]
 table.remove(args,#args)
 --table.setn(args, #args-1)
 local entry = accessTable(prefix,table.unpack(args))
 local old = entry.value
 deleteTable(prefix,old)
 releasesym(prefix,old)
 entry.value = last
 entry:save()
 return old
end

return _ENV
