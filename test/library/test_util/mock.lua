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
