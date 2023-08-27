-- Displays the raw memory of a structure, separated by field.
--author BenLubar
--luacheck:skip-entirely
--
--[====[

devel/visualize-structure
=========================
Displays the raw memory of a structure, separated by field.
Useful for checking if structures are aligned.

]====]

local args = {...}

if #args ~= 1 then
    qerror([[Usage: devel/visualize-structure [path]

Displays the raw memory of a structure, separated by field.
Useful for checking if structures are aligned.]])
end

local utils = require('utils')

local ok, ref = pcall(function() return utils.df_expr_to_ref(args[1]) end)
if not ok then
    qerror(ref)
end
if ref._kind == 'primitive' and type(ref._type) == 'string' and ref._type:endswith('*') and ref.value ~= nil then
    -- dereference primitive pointer fields, since we usually care more about their contents
    ref = ref.value
end

local size, baseaddr = ref:sizeof()
local actual_size = -1

if ref._type == 'void*' then
    baseaddr = df.reinterpret_cast('uintptr_t', ref).value
    size = 0
end

local ptrsz = dfhack.getArchitecture() / 8

if baseaddr % 32 == 16 then
    local intptr = df.reinterpret_cast('uint64_t', baseaddr)
    if intptr:_displace(-1).value == 0xdfdf4ac8 then
        actual_size = intptr:_displace(-2).value
        if size < actual_size then
            size = actual_size
        end
    end
end

print(ref._type)

local expected_padding_byte = 0xd2

for _, g in pairs(df.global) do
    if type(g) == 'userdata' then
        local global_size, global_start = g:sizeof()
        if baseaddr >= global_start and baseaddr < global_start + global_size then
            expected_padding_byte = 0
            break
        end
    end
end

local byteptr = df.reinterpret_cast('uint8_t', baseaddr)
local offset = 0
local function bytes_until(target, expect)
    while offset < target do
        if expect == 'padding' then
            if byteptr:_displace(offset).value == expected_padding_byte then
                dfhack.color(COLOR_DARKGREY)
            else
                dfhack.color(COLOR_LIGHTRED)
            end
        elseif expect == 'vtable' then
            dfhack.color(COLOR_DARKGREY)
        end
        dfhack.print(string.format('%02x', byteptr:_displace(offset).value))
        dfhack.color(COLOR_RESET)
        offset = offset + 1
        if offset % 4 == 0 then
            dfhack.print(' ')
        end
        if offset % 8 == 0 then
            dfhack.print(' ')
        end
    end
    print()
end

-- adapted from devel/query
local function key_pairs(item)
    local mt = debug.getmetatable(item)
    if mt and mt._index_table then
        local idx = 0
        return function()
            idx = idx + 1
            if mt._index_table[idx] then
                return mt._index_table[idx]
            end
        end
    end
end
if ref._field then
    for k in key_pairs(ref) do
        local field = ref:_field(k)
        local fsize, faddr = field:sizeof()
        local foff = faddr - baseaddr
        if offset < foff then
            print()
            if offset == 0 and foff == ptrsz then
                print('(vtable)')
                bytes_until(foff, 'vtable')
            else
                print('(padding)')
                bytes_until(foff, 'padding')
            end
        end

        print()
        print(tostring(field._type) .. ' ' .. k)
        bytes_until(foff + fsize)
        if field._kind == 'container' then
            print('length: ' .. #field)
        end
    end
end

if offset < size then
    print()
    print('(padding)')
    bytes_until(size, 'padding')
end
