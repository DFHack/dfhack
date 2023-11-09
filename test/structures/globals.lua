config.target = 'core'

local function with_temp_global_address(name, addr, callback, ...)
    dfhack.call_with_finalizer(2, true,
        dfhack.internal.setAddress, name, dfhack.internal.getAddress(name),
        function(...)
            dfhack.internal.setAddress(name, addr)
            callback(...)
        end, ...)
end

function test.unknown_global_address()
    expect.ne(dfhack.internal.getAddress('army_next_id'), 0)
    local old_id = df.global.army_next_id

    with_temp_global_address('army_next_id', 0, function()
        expect.error_match('Cannot read field global.army_next_id: global address not known.', function()
            local _ = df.global.army_next_id
        end)

        expect.error_match('Cannot write field global.army_next_id: global address not known.', function()
            df.global.army_next_id = old_id
        end)

        expect.error_match('Cannot reference field global.army_next_id: global address not known.', function()
            local _ = df.global:_field('army_next_id')
        end)
    end)

    expect.gt(dfhack.internal.getAddress('army_next_id'), 0)
    expect.eq(df.global.army_next_id, old_id)
end
