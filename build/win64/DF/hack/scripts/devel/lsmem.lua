-- Prints memory ranges of the DF process.
--[====[

devel/lsmem
===========
Prints memory ranges of the DF process. Useful for checking whether a pointer is
valid, whether a certain library/plugin is loaded, etc.

]====]

function range_contains_any(range, addrs)
    for _, a in ipairs(addrs) do
        if a >= range.start_addr and a < range.end_addr then
            return true
        end
    end
    return false
end

function range_name_match_any(range, names)
    for _, n in ipairs(names) do
        if range.name:lower():find(n, 1, true) or range.name:lower():find(n) then
            return true
        end
    end
    return false
end

local args = {...}
local filter_addrs = {}
local filter_names = {}
for _, arg in ipairs(args) do
    if arg:lower():startswith('0x') then
        arg = arg:sub(3)
    end
    local addr = tonumber(arg, 16)
    if addr then
        table.insert(filter_addrs, addr)
    else
        table.insert(filter_names, arg:lower())
    end
end

for _,v in ipairs(dfhack.internal.getMemRanges()) do
    local access = { '-', '-', '-', 'p' }
    if v.read then access[1] = 'r' end
    if v.write then access[2] = 'w' end
    if v.execute then access[3] = 'x' end
    if not v.valid then
        access[4] = '?'
    elseif v.shared then
        access[4] = 's'
    end
    if (#filter_addrs == 0 or range_contains_any(v, filter_addrs)) and
        (#filter_names == 0 or range_name_match_any(v, filter_names))
    then
        print(string.format('%08x-%08x %s %s', v.start_addr, v.end_addr, table.concat(access), v.name))
    end
end
