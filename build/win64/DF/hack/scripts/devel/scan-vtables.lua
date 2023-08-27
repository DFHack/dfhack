-- Scan and dump likely vtable addresses
memscan = require('memscan')

local osType = dfhack.getOSType()
if osType ~= 'linux' then
    qerror('unsupported OS: ' .. osType)
end

local df_ranges = {}
for i,mem in ipairs(dfhack.internal.getMemRanges()) do
    if mem.read and (
        string.match(mem.name,'/dwarfort%.exe$')
        or string.match(mem.name,'/dwarfort$')
        or string.match(mem.name,'/Dwarf_Fortress$')
        or string.match(mem.name,'Dwarf Fortress%.exe')
        or string.match(mem.name,'/libg_src_lib.so$')
    )
    then
        table.insert(df_ranges, mem)
    end
end

function is_df_addr(a)
    for _, mem in ipairs(df_ranges) do
        if a >= mem.start_addr and a < mem.end_addr then
            return true
        end
    end
    return false
end

for _, range in ipairs(df_ranges) do
    if (not range.read) or range.write or range.execute or range.name:match('g_src') then
        goto next_range
    end

    local area = memscan.MemoryArea.new(range.start_addr, range.end_addr)
    for i = 1, area.uintptr_t.count - 1 do
        -- take every pointer-aligned value in memory mapped to the DF executable, and see if it is a valid vtable
        -- start by following the logic in Process::doReadClassName() and ensure it doesn't crash
        local vtable = area.uintptr_t:idx2addr(i)
        local typeinfo = area.uintptr_t[i - 1]
        if is_df_addr(typeinfo + 8) then
            local typestring = df.reinterpret_cast('uintptr_t', typeinfo + 8)[0]
            if is_df_addr(typestring) then
                -- rule out false positives by checking that the vtable points to a table of valid pointers
                -- TODO: check that the pointers are actually function pointers
                local vlen = 0
                while is_df_addr(vtable + (8*vlen)) and is_df_addr(df.reinterpret_cast('uintptr_t', vtable + (8*vlen))[0]) do
                    vlen = vlen + 1
                    break -- for now, any vtable with one valid pointer is valid enough
                end
                if vlen > 0 then
                    -- some false positives can be ruled out if the string.char() call in read_c_string() throws an error for invalid characters
                    local ok, name = pcall(function()
                        return memscan.read_c_string(df.reinterpret_cast('char', typestring))
                    end)
                    if ok then
                        -- GCC strips the "_Z" prefix from typeinfo names, so add it back
                        local demangled_name = dfhack.internal.cxxDemangle('_Z' .. name)
                        if demangled_name and not demangled_name:match('[<>]') and not demangled_name:match('^std::') then
                            print(("<vtable-address name='%s' value='0x%x'/>"):format(demangled_name, vtable))
                        end
                    end
                end
            end
        end

    end
    ::next_range::
end
