-- tests print-related functions added by dfhack.lua

local dfhack = dfhack

local mock_print = mock.func()
local function test_wrapper(test_fn)
    mock.patch({{dfhack, 'print', mock_print},
                {dfhack, 'println', mock_print}},
               test_fn)
    mock_print = mock.func()
end
config.wrapper = test_wrapper

function test.printall_table()
    printall({a='b'})
    expect.eq(1, mock_print.call_count)
    expect.true_(mock_print.call_args[1][1]:find('a%s+= b'))
end

function test.printall_string()
    printall('a')
    expect.eq(0, mock_print.call_count)
end

function test.printall_number()
    printall(10)
    expect.eq(0, mock_print.call_count)
end

function test.printall_nil()
    printall(nil)
    expect.eq(0, mock_print.call_count)
end

function test.printall_boolean()
    printall(false)
    expect.eq(0, mock_print.call_count)
end

function test.printall_function()
    printall(function() end)
    expect.eq(0, mock_print.call_count)
end

local function new_int_vector()
    -- create a vector of integers by cloning one from world. we do it this way
    -- because we can't allocate typed vectors from lua directly.
    local vector = df.global.world.busy_buildings:new()
    vector:resize(0)
    return vector
end

function test.printall_userdata()
    local utable = new_int_vector()
    dfhack.with_temp_object(utable, function()
            utable:insert(0, 10)
            utable:insert(1, 20)
            printall(utable)
            expect.eq(2, mock_print.call_count)
            expect.true_(mock_print.call_args[1][1]:find('0%s+= 10'))
            expect.true_(mock_print.call_args[2][1]:find('1%s+= 20'))
        end)
end

function test.printall_ipairs_table()
    printall_ipairs({'a', 'b'})
    expect.eq(2, mock_print.call_count)
    expect.true_(mock_print.call_args[1][1]:find('1%s+= a'))
    expect.true_(mock_print.call_args[2][1]:find('2%s+= b'))
end

function test.printall_ipairs_string()
    printall_ipairs('a')
    expect.eq(0, mock_print.call_count)
end

function test.printall_ipairs_number()
    printall_ipairs(10)
    expect.eq(0, mock_print.call_count)
end

function test.printall_ipairs_nil()
    printall_ipairs(nil)
    expect.eq(0, mock_print.call_count)
end

function test.printall_ipairs_boolean()
    printall_ipairs(false)
    expect.eq(0, mock_print.call_count)
end

function test.printall_ipairs_function()
    printall_ipairs(function() end)
    expect.eq(0, mock_print.call_count)
end

function test.printall_ipairs_userdata()
    local utable = new_int_vector()
    dfhack.with_temp_object(utable, function()
            utable:insert(0, 10)
            utable:insert(1, 20)
            printall_ipairs(utable)
            expect.eq(2, mock_print.call_count)
            expect.true_(mock_print.call_args[1][1]:find('0%s+= 10'))
            expect.true_(mock_print.call_args[2][1]:find('1%s+= 20'))
        end)
end

function test.printall_recurse()
    -- TODO
end
