-- Find some TWBT-related offsets
--luacheck:skip-entirely
--[====[

devel/find-twbt
===============

Finds some TWBT-related offsets - currently just ``twbt_render_map``.

]====]
local ms = require('memscan')
local cs = ms.get_code_segment()

function get_ptr_size()
    local v = df.new('uintptr_t')
    local ret = df.sizeof(v)
    df.delete(v)
    return ret
end

function print_off(name, off)
    print(string.format("<global-address name='%s' value='0x%x'/>", name, off - dfhack.internal.getRebaseDelta()))
end

local ptr_size = get_ptr_size()

local vtoff = dfhack.internal.getVTable('viewscreen_dwarfmodest')
-- print_off("Vtable:", vtoff)

local vtable = ms.CheckedArray.new('uintptr_t', vtoff, vtoff + 3 * ptr_size)

local render_method = vtable[2] --third method aka render
-- print_off("render", render_method)

function list_all_possible_calls(start, len)
    local func = ms.CheckedArray.new('uint8_t', start, start + len) -- should be near
    local possible_calls = {}

    for i = 0, #func - 1 do
        if func[i] == 0xe8 then
            table.insert(possible_calls, i)
        end
    end
    return possible_calls
end
function get_call_target(offset)
    local call_offset = df.reinterpret_cast('int32_t', offset + 1)
    local ret = call_offset.value + offset + 5
    if cs:contains_range(ret, 1) then
        return ret
    else
        return nil
    end
end
function list_all_valid_calls(start, len)
    local all_calls = list_all_possible_calls(start, len)
    local ret = {}
    for i, v in ipairs(all_calls) do
        local ct = get_call_target(start + v)
        if ct then
            table.insert(ret, ct)
        end
    end
    return ret
end
-- TODO(warmist): this is probably stupid, but without disassembler ( sad face ) i can't say for sure
-- if it's part of instruction or an arg, so we just hope to find only one?

local possible_calls = list_all_valid_calls(render_method, 50)

if #possible_calls == 0 then
    qerror("Failed to find call instruction in render vmethod")
elseif #possible_calls > 1 then
    qerror("Found multiple 0xe8 (call) in render vmethod start")
end

local dwarfmode_render_main = possible_calls[1]
print_off("twbt_render_map", dwarfmode_render_main)

--[[ other not used offset(s)
local dfrender = list_all_valid_calls(dwarfmode_render_main, 200)[2] --this could be further
print_off("A_RENDER_MAP", dfrender)
]]
