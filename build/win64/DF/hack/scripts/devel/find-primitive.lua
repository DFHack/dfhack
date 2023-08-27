-- Finds a primitive variable in memory

--[====[

devel/find-primitive
====================

Finds a primitive variable in DF's data section, relying on the user to change
its value. This is similar to `devel/find-offsets`, but useful for new variables
whose locations are unknown (i.e. they could be part of an existing global).

Usage::

    devel/find-primitive data-type val1 val2 [val3...]

where ``data-type`` is a primitive type (int32_t, uint8_t, long, etc.) and each
``val`` is a valid value for that type.

Use ``devel/find-primitive help`` for a list of valid data types.

]====]

local ms = require('memscan')

local searcher = ms.DiffSearcher.new(ms.get_data_segment())
local values = {...}
local data_type = table.remove(values, 1)

--luacheck: out=bool skip
function is_valid_data_type(t)
    return getmetatable(searcher.area[t]) == ms.CheckedArray
end

if data_type == 'help' or not is_valid_data_type(data_type) then
    print('Valid data types:')
    for t in pairs(searcher.area) do
        if is_valid_data_type(t) then
            print('  ' .. t)
        end
    end
    if data_type ~= 'help' then
        qerror('Invalid data type')
    end
    return
end

local addr = searcher:find_menu_cursor(
    'Adjust the value of the variable so that it matches the message given, then press enter.',
    data_type,
    values
)
if addr then
    print(('0x%x'):format(addr))
else
    qerror('Unable to find variable')
end
