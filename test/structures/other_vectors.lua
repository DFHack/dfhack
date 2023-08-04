config.target = 'core'

function test.index_name()
    for _, k in ipairs(df.units_other_id) do
        expect.eq(df.global.world.units.other[k]._kind, 'container')
    end
end

function test.index_name_bad()
    expect.error_match('not found.$', function()
        expect.eq(df.global.world.units.other.SOME_FAKE_NAME, 'container')
    end)
end

function test.index_id()
    for i in ipairs(df.units_other_id) do
        expect.eq(df.global.world.units.other[i]._kind, 'container', df.units_other_id[i])
    end
end

function test.index_id_bad()
    expect.error_match('Cannot read field', function()
        expect.eq(df.global.world.units.other[df.units_other_id._first_item - 1], 'container')
    end)
    expect.error_match('Cannot read field', function()
        expect.eq(df.global.world.units.other[df.units_other_id._last_item + 1], 'container')
    end)
end
