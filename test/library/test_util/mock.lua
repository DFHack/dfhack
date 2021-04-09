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
        [{test_table, 'func1'}] = function() return 3 end,
        [{test_table, 'func2'}] = function() return 4 end,
    }, function()
        expect.eq(test_table.func1(), 3)
        expect.eq(test_table.func2(), 4)
    end)
    expect.eq(test_table.func1(), 1)
    expect.eq(test_table.func2(), 2)
end

