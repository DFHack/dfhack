-- Dump all global addresses

--[====[

devel/dump-offsets
==================

.. warning::

    THIS SCRIPT IS STRICTLY FOR DFHACK DEVELOPERS.

    Running this script on a new DF version will NOT
    MAKE IT RUN CORRECTLY if any data structures
    changed, thus possibly leading to CRASHES AND/OR
    PERMANENT SAVE CORRUPTION.

This dumps the contents of the table of global addresses (new in 0.44.01).

Passing global names as arguments calls setAddress() to set those globals'
addresses in-game. Passing "all" does this for all globals.

]====]

GLOBALS = {}
for k, v in pairs(df.global._fields) do
    GLOBALS[v.original_name] = k
end

function read_cstr(addr)
    local s = ''
    local p = df.reinterpret_cast('char', addr)
    local i = 0
    while p[i] ~= 0 do
        s = s .. string.char(p[i])
        i = i + 1
    end
    return s
end

local utils = require 'utils'
local iargs = utils.invert{...}

local ms = require 'memscan'
local data = ms.get_data_segment() or qerror('Could not find data segment')

local search
if dfhack.getArchitecture() == 64 then
    search = {0x1234567812345678, 0x8765432187654321, 0x89abcdef89abcdef}
else
    search = {0x12345678, 0x87654321, 0x89abcdef}
end

local addrs = {}
function save_addr(name, addr)
    print(("<global-address name='%s' value='0x%x'/>"):format(name, addr))
    if iargs[name] or iargs.all then
        ms.found_offset(name, addr)
    end
    addrs[name] = addr
end

local extended = false
local start, start_addr = data.intptr_t:find_one(search)
if start then
    extended = true
else
    -- try searching for a non-extended table
    table.remove(search, #search)
    start = data.intptr_t:find_one(search)
end
if not start then
    qerror('Could not find global table header')
end

if extended then
    -- structures only has types for an extended global table
    save_addr('global_table', start_addr + (#search * data.intptr_t.esize))
end

local index = 1
local entry_size = (extended and 3 or 2)
while true do
    local p_name = data.intptr_t[start + (index * entry_size)]
    if p_name == 0 then
        break
    end
    local g_addr = data.intptr_t[start + (index * entry_size) + 1]
    local df_name = read_cstr(p_name)
    local g_name = GLOBALS[df_name]
    if df_name:find('^index[12]_') then
        -- skip
    elseif not g_name then
        if iargs.warn then
            dfhack.printerr(('Unknown DF global: %s 0x%x'):format(df_name, g_addr))
        end
    else
        save_addr(g_name, g_addr)
    end
    index = index + 1
end

print('global table length:', index)
