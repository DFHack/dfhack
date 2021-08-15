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

local function validate_patterns(start_idx, patterns)
    for i,pattern in ipairs(patterns) do
        expect.true_(mock_print.call_args[start_idx+i-1][1]:find(pattern))
    end
    return start_idx + #patterns
end

function test.printall_recurse()
    local udatatable = new_int_vector()
    dfhack.with_temp_object(udatatable, function()
        local udataint = df.new('uint32_t')
        dfhack.with_temp_object(udataint, function ()
            udatatable:insert(0, 10)
            udatatable:insert(1, 20)
            udatatable:insert(2, 20)
            udatatable:insert(3, 20)
            udatatable:insert(4, 0)
            udatatable:insert(5, 0)
            local t2 = {}
            local t = {num=5,
                    bool=false,
                    fn=function() end,
                    udatatable=udatatable,
                    udataint=udataint,
                    table=t2}
            t2.cyclic = t
            printall_recurse(t)
            expect.eq(49, mock_print.call_count)
            expect.true_(mock_print.call_args[1][1]:find('^table: '))
            local idx = 2
            local EQ = '^%s+= $'
            while idx <= 49 do
                expect.eq('', mock_print.call_args[idx][1])
                idx = idx + 1
                local str = mock_print.call_args[idx][1]
                if str:startswith('num') then
                    idx = validate_patterns(idx, {'^num$', EQ, '^5$'})
                elseif str:startswith('bool') then
                    idx = validate_patterns(idx, {'^bool$', EQ, '^false$'})
                elseif str:startswith('fn') then
                    idx = validate_patterns(idx, {'^fn$', EQ, '^function: '})
                elseif str:startswith('udatatable') then
                    idx = validate_patterns(idx,
                            {'^udatatable$', EQ, '^<vector<int32_t>%[6%]: ',
                             '%s+', '^0$', EQ, '^10$',
                             '%s+', '^1$', EQ, '^20$',
                             'Repeated 2 times',
                             '%s+', '^4$', EQ, '^0$',
                             'Repeated 1 times'}) -- [sic]
                elseif str:startswith('udataint') then
                    idx = validate_patterns(idx,
                            {'^udataint$', EQ, '^<uint32_t: ',
                             '%s+', '^value$', EQ, '^0$'})
                elseif str:startswith('table') then
                    idx = validate_patterns(idx,
                            {'^table$', EQ, '^table: ',
                             '%s+', '^cyclic$', EQ, '^table: ',
                             '%s+', '^<Cyclic reference'})
                else
                    expect.fail('unhandled print output')
                    break
                end
            end
        end)
    end)
end
