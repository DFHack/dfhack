config.target = 'core'

local mock = require('test_util.mock')

local test_table = {
    func1 = function()
        return 1
    end,
    func2 = function()
        return 2
    end,
}

function test.patch_single()
    expect.eq(test_table.func1(), 1)
    mock.patch(test_table, 'func1', function() return 3 end, function()
        expect.eq(test_table.func1(), 3)
    end)
    expect.eq(test_table.func1(), 1)
end

function test.patch_single_nested()
    expect.eq(test_table.func1(), 1)
    mock.patch(test_table, 'func1', function() return 3 end, function()
        expect.eq(test_table.func1(), 3)
        mock.patch(test_table, 'func1', function() return 5 end, function()
            expect.eq(test_table.func1(), 5)
        end)
        expect.eq(test_table.func1(), 3)
    end)
    expect.eq(test_table.func1(), 1)
end

function test.patch_multiple()
    expect.eq(test_table.func1(), 1)
    expect.eq(test_table.func2(), 2)
    mock.patch({
        {test_table, 'func1', function() return 3 end},
        {test_table, 'func2', function() return 4 end},
    }, function()
        expect.eq(test_table.func1(), 3)
        expect.eq(test_table.func2(), 4)
    end)
    expect.eq(test_table.func1(), 1)
    expect.eq(test_table.func2(), 2)
end

function test.patch_nil_old_value()
    local t = {}
    mock.patch(t, 1, 2, function()
        expect.eq(t[1], 2)
    end)
    expect.eq(t[1], nil)
    expect.eq(#t, 0)
end

function test.patch_nil_new_value()
    local t = {2}
    mock.patch(t, 1, nil, function()
        expect.eq(t[1], nil)
        expect.eq(#t, 0)
    end)
    expect.eq(t[1], 2)
end

function test.patch_nil_key()
    local called = false
    expect.error_match('table index is nil', function()
        mock.patch({}, nil, 'value', function()
            called = true
        end)
    end)
    expect.false_(called)
end

function test.patch_nil_table()
    local called = false
    expect.error(function()
        mock.patch(nil, 1, 2, function()
            called = true
        end)
    end)
    expect.false_(called)
end

function test.patch_complex_key()
    local key = {'key'}
    local t = {[key] = 'value'}
    expect.eq(t[key], 'value')
    mock.patch(t, key, 2, function()
        expect.eq(t[key], 2)
    end)
    expect.eq(t[key], 'value')
end

function test.patch_callback_return_value()
    local a, b = mock.patch({}, 'k', 'v', function()
        return 3, 4
    end)
    expect.eq(a, 3)
    expect.eq(b, 4)
end

function test.patch_invalid_value()
    dfhack.with_temp_object(df.new('int8_t'), function(i)
        i.value = 1
        local called = false
        expect.error_match('integer expected', function()
            mock.patch(i, 'value', 2, function()
                expect.eq(i.value, 2)
                called = true
                i.value = 'a'
            end)
        end)
        expect.true_(called)
        expect.eq(i.value, 1)
    end)
end

function test.patch_invalid_value_initial()
    dfhack.with_temp_object(df.new('int8_t'), function(i)
        i.value = 1
        expect.error_match('integer expected', function()
            mock.patch(i, 'value', 'a', function()
                expect.fail('patch() callback called unexpectedly')
            end)
        end)
        expect.eq(i.value, 1)
    end)
end

function test.patch_invalid_value_initial_multiple()
    dfhack.with_temp_object(df.new('int8_t', 2), function(i)
        i[0] = 1
        i[1] = 2
        expect.error_match('integer expected', function()
            mock.patch({
                {i, 0, 3},
                {i, 1, 'a'},
            }, function()
                expect.fail('patch() callback called unexpectedly')
            end)
        end)
        expect.eq(i[0], 1)
        expect.eq(i[1], 2)
    end)
end

function test.restore_single()
    local t = {k = 1}
    mock.restore(t, 'k', function()
        expect.eq(t.k, 1)
        t.k = 2
        expect.eq(t.k, 2)
    end)
    expect.eq(t.k, 1)
end

function test.restore_multiple()
    local t = {a = 1, b = 2}
    mock.restore({
        {t, 'a'},
        {t, 'b'},
    }, function()
        expect.eq(t.a, 1)
        expect.eq(t.b, 2)
        t.a = 3
        t.b = 4
        expect.eq(t.a, 3)
        expect.eq(t.b, 4)
    end)
    expect.eq(t.a, 1)
    expect.eq(t.b, 2)
end

function test.func_call_count()
    local f = mock.func()
    expect.eq(f.call_count, 0)
    f()
    expect.eq(f.call_count, 1)
    f()
    expect.eq(f.call_count, 2)
end

function test.func_call_args()
    local f = mock.func()
    expect.eq(#f.call_args, 0)
    f()
    f(7)
    expect.eq(#f.call_args, 2)
    expect.eq(#f.call_args[1], 0)
    expect.eq(#f.call_args[2], 1)
    expect.eq(f.call_args[2][1], 7)
end

function test.func_call_args_nil()
    local f = mock.func()
    f(nil)
    f(2, nil, 4)
    expect.table_eq(f.call_args[1], {nil})
    expect.table_eq(f.call_args[2], {2, nil, 4})
    expect.eq(#f.call_args[2], 3)
end

function test.func_call_return_value()
    local f = mock.func(7)
    expect.eq(f(), 7)
    f.return_values = {9}
    expect.eq(f(), 9)
end

function test.func_call_return_multiple_values()
    local f = mock.func(7, 5, {imatable='snarfsnarf'})
    local a, b, c = f()
    expect.eq(7, a)
    expect.eq(5, b)
    expect.table_eq({imatable='snarfsnarf'}, c)
end

function test.observe_func()
    -- basic end-to-end test for common cases;
    -- most edge cases are covered by mock.func() tests
    local counter = 0
    local function target()
        counter = counter + 1
        return counter
    end
    local observer = mock.observe_func(target)

    expect.eq(observer(), 1)
    expect.eq(counter, 1)
    expect.eq(observer.call_count, 1)
    expect.table_eq(observer.call_args, {{}})

    expect.eq(observer('x', 'y'), 2)
    expect.eq(counter, 2)
    expect.eq(observer.call_count, 2)
    expect.table_eq(observer.call_args, {{}, {'x', 'y'}})
end

function test.observe_func_error()
    local function target()
        error('asdf')
    end
    local observer = mock.observe_func(target)

    expect.error_match('asdf', function()
        observer('x')
    end)
    -- make sure the call was still tracked
    expect.eq(observer.call_count, 1)
    expect.table_eq(observer.call_args, {{'x'}})
end
