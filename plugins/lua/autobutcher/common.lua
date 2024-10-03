local _ENV = mkmodule('plugins.autobutcher.common')

verbose = verbose or nil
prefix = prefix or ''

function printLocal(text)
  print(prefix .. ': ' .. text)
end

function handleError(text)
  qerror(prefix .. ': ' .. text)
end

function printDetails(text)
  if verbose then
    printLocal(text)
  end
end

function dumpToString(o)
  if type(o) == 'table' then
    local s = '{ '
    for k, v in pairs(o) do
      if type(k) ~= 'number' then
        k = '"' .. k .. '"'
      end
      s = s .. '[' .. k .. '] = ' .. dumpToString(v) .. ','
    end
    return s .. '} '
  else
    return tostring(o)
  end
end
return _ENV
