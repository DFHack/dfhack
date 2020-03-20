function test.overlappingGlobals()
    local globals = {}
    for name, _ in pairs(df.global) do
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

        expect.true_(prev.last < cur.first, "global variable " .. prev.name .. " overlaps global variable " .. cur.name)
    end
end
