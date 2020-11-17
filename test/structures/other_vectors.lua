function test.index_name()
    for _, k in ipairs(df.units_other_id) do
        expect.eq(df.global.world.units.other[k]._kind, 'container')
    end
end

function test.index_name_bad()
    expect.error_match(function()
        expect.eq(df.global.world.units.other.SOME_FAKE_NAME, 'container')
    end, 'not found.$')
end

function test.index_id()
    for i in ipairs(df.units_other_id) do
        expect.eq(df.global.world.units.other[i]._kind, 'container')
    end
end

function test.index_id_bad()
    expect.error_match(function()
        expect.eq(df.global.world.units.other[df.units_other_id._first_item - 1], 'container')
    end, 'Cannot read field')
    expect.error_match(function()
        expect.eq(df.global.world.units.other[df.units_other_id._last_item + 1], 'container')
    end, 'Cannot read field')
end

