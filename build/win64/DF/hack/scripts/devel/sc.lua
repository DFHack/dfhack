-- Size Check
--author mifki
--luacheck:skip-entirely
--
--[====[

devel/sc
========
Size Check: scans structures for invalid vectors, misaligned structures,
and unidentified enum values.

.. note::

    This script can take a very long time to complete, and DF may be
    unresponsive while it is running. You can use `kill-lua` to interrupt
    this script.

Examples:

* scan world::

    devel/sc

* scan all globals::

    devel/sc -all

* scan result of expression::

    devel/sc [expr]

]====]

local utils = require('utils')

local guess_pointers = false
local check_vectors = true
local check_pointers = true
local check_enums = true


-- not really a queue, I know
local queue

local args = {...}
if args[1] == '-all' then
    queue = {}
    for k, v in pairs(df.global) do
        if type(v) == 'userdata' then
            table.insert(queue, { v, k })
        end
    end
elseif #args == 1 then
    queue = { { utils.df_expr_to_ref(args[1]), args[1] } }
else
    queue = { { df.global.world, 'world' } }
end

local checkedt = {}
local checkeda = {}
local checkedp = {}
local token = os.time()

local sizetype = 'uintptr_t'

local mem_ranges = {}
local mem_start, mem_end

local function is_valid_address(ptr)
    if ptr < mem_ranges[1].start_addr or ptr > mem_ranges[#mem_ranges].end_addr then
        return false
    end

    for k,mem in ipairs(mem_ranges) do
        if ptr >= mem.start_addr and ptr < mem.end_addr then
            return true
        end
    end

    return false
end

local function is_valid_vector(ref)
    local ints = df.reinterpret_cast(sizetype, ref)

    if ints[0] == 0 and ints[1] == 0 and ints[2] == 0 then
        return true
    end

    -- vector<bool>: ptr, int, ptr
    if df.reinterpret_cast('int32_t', ints:_displace(1)).value == 0 then
        return ints[0] <= ints[2]
    end

    return ints[0] <= ints[1] and ints[1] <= ints[2]
           --and is_valid_address(ints[0]) and is_valid_address(ints[1]) and is_valid_address(ints[2])
end

local function bold(s)
    dfhack.color(8)
    print(s)
    dfhack.color(-1)
end

local function warn(s)
    dfhack.color(6)
    print(s)
    dfhack.color(-1)
end

local function err(s)
    dfhack.color(4)
    print(s)
    dfhack.color(-1)
end

prog = '|/-\\'
local count = -1
local function check_container(obj, path)
    count = count + 1
    if dfhack.is_interactive() and count % 500 == 0 then
        local i = ((count / 500) % 4) + 1
        dfhack.print(prog:sub(i, i) .. '\r')
        dfhack.console.flush()
    end
    for k,v in pairs(obj) do
        if df.isvalid(v) == 'ref' then
            local s, a = v:sizeof()

            if v and v._kind == 'container' and k ~= 'bad' then
                if tostring(v._type):sub(1,6) == 'vector' and check_vectors and not is_valid_vector(a) then
                    local key = tostring(obj._type) .. '.' .. k
                    if not checkedp[key] then
                        checkedp[key] = true
                        bold(path .. ' ' .. tostring(obj._type) .. ' -> ' .. k)
                        err('  INVALID VECTOR, SKIPPING REST OF THE TYPE')
                    end
                    return
                end

                table.insert(queue, { v, path .. '.' .. k })

            elseif v and v._kind == 'struct' then
                local t = v._type

                if check_pointers and not is_valid_address(a) then
                    local key = tostring(obj._type) .. '.' .. k
                    if not checkedp[key] then
                        checkedp[key] = true
                        bold(path .. ' ' .. tostring(obj._type) .. ' -> ' .. k)
                        err('  INVALID ADDRESS, SKIPPING REST OF THE TYPE')
                    end
                    return
                end

                -- the first check is to process pointers only, not nested structures
                if obj:_field(k)._kind == 'primitive' and df.reinterpret_cast('uint32_t',a-8).value == 0xdfdf4ac8 then
                    if not checkedt[t] then
                        checkedt[t] = true
                        local s2 = df.reinterpret_cast(sizetype,a-16).value
                        if s2 == s then
                            --print ('  OK')
                        else
                            bold (t)
                            err ('  NOT OK '.. s .. ' ' .. s2)
                        end
                    end

                    if df.reinterpret_cast('uint32_t',a-4).value ~= token then
                        df.reinterpret_cast('uint32_t',a-4).value = token
                        table.insert(queue, { v, path .. '.' .. k })
                    end

                elseif not checkeda[a] then
                    checkeda[a] = true
                    table.insert(queue, { v, path .. '.' .. k })
                end
            end

        else
            local field = obj:_field(k)
            if field then
                if guess_pointers then
                    if field._type == 'void*'
                    or field._type == 'int64_t' or field._type == 'uint64_t'
                    or field._type == 'int32_t' or field._type == 'uint32_t' then
                        local s,a = field:sizeof()
                        local val = df.reinterpret_cast(sizetype, a).value

                        if is_valid_address(val) and df.reinterpret_cast('uint32_t',a-8).value == 0xdfdf4ac8 then
                            local key = tostring(obj._type) .. '.' .. k
                            if not checkedp[key] then
                                checkedp[key] = true
                                bold(path .. ' ' .. tostring(obj._type) .. ' -> ' .. k)
                                print('  POINTER TO STRUCT '..df.reinterpret_cast(sizetype,a-16).value)
                            end
                        end
                    end
                end

                -- is there a better way to detect enums?
                if check_enums then
                    if field._kind == 'primitive' and field._type[0] and not field._type[field.value] then
                        -- some checks to filter out uninitialised and conditional fields, enums in unions, etc.
                        if not (obj._type == df.unit_preference and k == 'item_type')
                        and not (obj._type == df.unit.T_job and k == 'mood_skill')
                        and not (obj._type == df.unit and k == 'idle_area_type')
                        and not (obj._type == df.history_event_body_abusedst.T_props)
                        and not (field._type == df.skill_rating)
                        and field.value >= -1 and field.value < 1024 then
                            local key = tostring(obj._type) .. '.' .. k .. tostring(field.value)
                            if not checkedp[key] then
                                checkedp[key] = true
                                bold(path .. ' ' .. tostring(obj._type) .. ' -> ' .. k)
                                warn('  INVALID ENUM VALUE ' .. tostring(field._type) .. ' ' .. field.value)
                            end
                        end
                    end
                end
            end
        end
    end
end


for i,mem in ipairs(dfhack.internal.getMemRanges()) do
    if mem.read and mem.write then
        -- concatename nearby regions, otherwise there are too many, and checks are very slow
        if #mem_ranges > 0 and mem.start_addr - mem_ranges[#mem_ranges].end_addr < 1024*1024*10 then
            mem_ranges[#mem_ranges].end_addr = mem.end_addr
        else
            table.insert(mem_ranges, mem)
        end

        if not mem_start then
            mem_start = mem.start_addr
        end
        mem_end = mem.end_addr
    end
end

while #queue > 0 do
    local v = queue[#queue]
    table.remove(queue, #queue)
    check_container(v[1], v[2])
end
print(count .. ' scanned')
