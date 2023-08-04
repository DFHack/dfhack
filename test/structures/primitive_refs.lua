config.target = 'core'

utils = require('utils')

function with_temp_ref(...)
    local args = {...}
    local dtype = 'int32_t'
    if type(args[1]) == 'string' then
        dtype = table.remove(args, 1)
    end
    local count = 1
    if type(args[1]) == 'number' then
        count = table.remove(args, 1)
    end
    local func = table.remove(args, 1)
    assert(type(func) == 'function')
    dfhack.with_temp_object(df.new(dtype, count), func, table.unpack(args))
end

function test.zero_initialize()
    with_temp_ref(function(x)
        expect.eq(x.value, 0)
    end)
end

function test.zero_initialize_array()
    with_temp_ref(2, function(x)
        expect.eq(x:_displace(1).value, 0)
    end)
end

function test.displace_zero()
    with_temp_ref(function(x)
        expect.eq(x, x:_displace(0))
    end)
end

function test.displace_nonzero()
    with_temp_ref(2, function(x)
        expect.ne(x, x:_displace(1))
        expect.eq(utils.addressof(x) + x:sizeof(), utils.addressof(x:_displace(1)))
    end)
end

function test.displace_negative()
    with_temp_ref(2, function(x)
        expect.true_(x:_displace(1):_displace(-1).value)
    end)
end

function test.index_read()
    with_temp_ref(function(x)
        expect.eq(x.value, x[0])
    end)
end

function test.index_write()
    with_temp_ref(function(x)
        x[0] = 1
        expect.eq(x.value, 1)
        expect.eq(x[0], 1)
        x.value = 2
        expect.eq(x.value, 2)
        expect.eq(x[0], 2)
    end)
end

function test.index_write_multi()
    local len = 3
    with_temp_ref(len, function(x)
        for i = 0, len - 1 do
            x[i] = i * i
        end
        for i = 0, len - 1 do
            expect.eq(x[i], i * i, i)
        end
    end)
end

function test.index_read_negative()
    with_temp_ref(function(x)
        expect.error_match('negative index', function()
            expect.true_(x:_displace(1)[-1])
        end)
    end)
end

function test.index_write_negative()
    with_temp_ref(function(x)
        expect.error_match('negative index', function()
            x:_displace(1)[-1] = 7
        end)
    end)
end
