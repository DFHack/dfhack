config.target = 'core'

function test.overlappingGlobals()
    local globals = {}
    for name in pairs(df.global) do
        local gvar = df.global:_field(name)
        local size, addr = gvar:sizeof()
        table.insert(globals, {
            name = name,
            first = addr,
            last = addr + size - 1
        })
    end

    table.sort(globals, function(a, b)
        return a.first < b.first
    end)

    for i = 2, #globals do
        local prev = globals[i - 1]
        local cur = globals[i]

        expect.lt(prev.last, cur.first, "global variable " .. prev.name .. " overlaps global variable " .. cur.name)
    end
end

local known_bad_types = {
    -- renderer base class has non-destructible padding declared
    renderer_2d_base=true,
    renderer_2d=true,
    renderer_offscreen=true,

    -- abstract base classes that aren't instantiable
    active_script_varst=true,
    widget_sheet_button=true,
}

if dfhack.getOSType() == 'linux' then
    -- empty destructors are declared inline for these types,
    -- and gcc appears to optimize them out
    known_bad_types.mental_picture_propertyst = true
    known_bad_types.region_block_eventst = true
end

function test.destructors()
    local count = 1
    for name, type in pairs(df) do
        if known_bad_types[name] then
            goto continue
        end
        print(('testing constructor %5d: %s'):format(count, name))
        local ok, v = pcall(function() return type:new() end)
        if not ok then
            print('        constructor failed; skipping destructor test')
        else
            print('        destructor ok')
            expect.true_(v:delete(), "destructor returned false: " .. name)
        end
        count = count + 1
        ::continue::
    end
end
