config.target = 'core'

local utils = require 'utils'

function test.getval()
    -- Test with static value
    expect.eq(utils.getval(42), 42)
    expect.eq(utils.getval("hello"), "hello")

    -- Test with function
    expect.eq(utils.getval(function() return 100 end), 100)
    expect.eq(utils.getval(function(x) return x * 2 end, 5), 10)
end

function test.compare()
    expect.eq(utils.compare(1, 2), -1)
    expect.eq(utils.compare(2, 1), 1)
    expect.eq(utils.compare(1, 1), 0)

    expect.eq(utils.compare('a', 'b'), -1)
    expect.eq(utils.compare('b', 'a'), 1)
    expect.eq(utils.compare('a', 'a'), 0)
end

function test.compare_name()
    expect.eq(utils.compare_name('', ''), 0)
    expect.eq(utils.compare_name('', 'a'), 1)
    expect.eq(utils.compare_name('a', ''), -1)
    expect.eq(utils.compare_name('a', 'b'), -1)
    expect.eq(utils.compare_name('b', 'a'), 1)
end

function test.compare_field()
    local cmp = utils.compare_field('value')
    expect.lt(cmp({value = 1}, {value = 2}), 0)
    expect.gt(cmp({value = 2}, {value = 1}), 0)
    expect.eq(cmp({value = 1}, {value = 1}), 0)
end

function test.make_index_sequence()
    local seq = utils.make_index_sequence(1, 5)
    expect.eq(#seq, 5)
    expect.eq(seq[1], 1)
    expect.eq(seq[5], 5)

    local seq2 = utils.make_index_sequence(10, 15)
    expect.eq(#seq2, 6)
    expect.eq(seq2[1], 10)
    expect.eq(seq2[6], 15)
end

function test.make_sort_order()
    local data = {{value = 3}, {value = 1}, {value = 2}}
    local ordering = {{key = function(item) return item.value end}}

    local order = utils.make_sort_order(data, ordering)
    expect.eq(order[1], 2) -- Index of value 1
    expect.eq(order[2], 3) -- Index of value 2
    expect.eq(order[3], 1) -- Index of value 3
end

function test.clone()
    -- Test shallow clone
    local t = {a = 1, b = 2}
    local cloned = utils.clone(t, false)
    expect.table_eq(cloned, t)
    expect.ne(cloned, t)

    -- Test deep clone
    local nested = {a = {b = {c = 1}}}
    local deep_cloned = utils.clone(nested, true)
    expect.eq(deep_cloned.a.b.c, 1)
    expect.ne(deep_cloned.a, nested.a)
end

function test.clone_with_default()
    local obj = {a = 1, b = 2, c = 3}
    local default = {a = 1, b = 2, c = 3, d = 4}

    local result = utils.clone_with_default(obj, default)
    expect.eq(result.a, nil) -- Same as default
    expect.eq(result.b, nil) -- Same as default
    expect.eq(result.c, nil) -- Same as default
    expect.eq(result.d, nil) -- Not in obj

    -- Test with different values
    local obj2 = {a = 5, b = 2, c = 3}
    local result2 = utils.clone_with_default(obj2, default)
    expect.eq(result2.a, 5) -- Different from default
    expect.eq(result2.b, nil) -- Same as default
end

function test.parse_bitfield_int()
    local type_ref = {'flag1', 'flag2', 'flag3'}

    local result = utils.parse_bitfield_int(5, type_ref) -- Binary 101
    expect.true_(result.flag1)
    expect.false_(result.flag2)
    expect.true_(result.flag3)

    -- Test with zero
    local zero_result = utils.parse_bitfield_int(0, type_ref)
    expect.eq(zero_result, nil)
end

function test.list_bitfield_flags()
    local bitfield = {flag1 = true, flag2 = false, flag3 = true}
    local list = utils.list_bitfield_flags(bitfield)

    expect.eq(#list, 2)
    expect.true_(list[1] == 'flag1' or list[1] == 'flag3')
    expect.true_(list[2] == 'flag1' or list[2] == 'flag3')
end

function test.linear_index()
    local data = {{id = 1}, {id = 2}, {id = 3}}

    local idx, obj = utils.linear_index(data, 2, 'id')
    expect.eq(idx, 2)
    expect.eq(obj.id, 2)

    -- Test without field
    local idx2, obj2 = utils.linear_index(data, {id = 2})
    expect.eq(idx2, 2)
    expect.eq(obj2.id, 2)

    -- Test not found
    local idx3, obj3 = utils.linear_index(data, 99, 'id')
    expect.eq(idx3, nil)
    expect.eq(obj3, nil)
end

function test.binsearch()
    local data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}

    local item, found, pos = utils.binsearch(data, 5)
    expect.eq(item, 5)
    expect.true_(found)
    expect.eq(pos, 5)

    -- Test not found
    local item2, found2, pos2 = utils.binsearch(data, 11)
    expect.eq(item2, nil)
    expect.false_(found2)
end

function test.insert_sorted()
    local data = {1, 3, 5, 7, 9}

    local added, item, pos = utils.insert_sorted(data, 4)
    expect.true_(added)
    expect.eq(item, 4)
    expect.eq(pos, 3)
    expect.eq(#data, 6)

    -- Test duplicate
    local added2, item2, pos2 = utils.insert_sorted(data, 5)
    expect.false_(added2)
    expect.eq(item2, 5)
end

function test.insert_or_update()
    local data = {1, 3, 5, 7, 9}

    local added, item, pos = utils.insert_or_update(data, 4)
    expect.true_(added)
    expect.eq(item, 4)

    -- Test update existing
    local added2, item2, pos2 = utils.insert_or_update(data, 5)
    expect.false_(added2)
    expect.eq(item2, 5)
end

function test.erase_sorted_key()
    local data = {1, 3, 5, 7, 9}

    local found, item, pos = utils.erase_sorted_key(data, 5)
    expect.true_(found)
    expect.eq(item, 5)
    expect.eq(#data, 4)

    -- Test not found
    local found2, item2, pos2 = utils.erase_sorted_key(data, 10)
    expect.false_(found2)
end

function test.search_text()
    local text = "Hello World Test"

    -- Test basic search
    expect.true_(utils.search_text(text, "Hello"))
    expect.true_(utils.search_text(text, "World"))
    expect.false_(utils.search_text(text, "NotFound"))

    -- Test multiple tokens
    expect.true_(utils.search_text(text, {"Hello", "World"}))
    expect.false_(utils.search_text(text, {"Hello", "NotFound"}))
end

function test.split_string()
    local text = "a,b,c"
    local parts = utils.split_string(text, ",")

    expect.eq(#parts, 3)
    expect.eq(parts[1], "a")
    expect.eq(parts[2], "b")
    expect.eq(parts[3], "c")
end

function test.normalizePath()
    expect.eq(utils.normalizePath("path\\to\\file"), "path/to/file")
    expect.eq(utils.normalizePath("path//to//file"), "path/to/file")
    expect.eq(utils.normalizePath("path/to/file"), "path/to/file")
end

function test.invert()
    local t = {a = 1, b = 2, c = 3}
    local inverted = utils.invert(t)

    expect.eq(inverted[1], 'a')
    expect.eq(inverted[2], 'b')
    expect.eq(inverted[3], 'c')
end

function test.tabulate()
    local result = utils.tabulate(function(i) return i * 2 end, 1, 5)

    expect.eq(#result, 5)
    expect.eq(result[1], 2)
    expect.eq(result[5], 10)
end

function test.fillTable()
    local t1 = {a = 1}
    local t2 = {b = 2, c = 3}

    utils.fillTable(t1, t2)
    expect.eq(t1.a, 1)
    expect.eq(t1.b, 2)
    expect.eq(t1.c, 3)
end

function test.unfillTable()
    local t1 = {a = 1, b = 2, c = 3}
    local t2 = {b = 2, c = 3}

    utils.unfillTable(t1, t2)
    expect.eq(t1.a, 1)
    expect.eq(t1.b, nil)
    expect.eq(t1.c, nil)
end

function test.df_shortcut_var()
    -- Test global shortcut
    local result = utils.df_shortcut_var('world')
    expect.eq(result, df.global.world)

    -- Test non-existent
    local result2 = utils.df_shortcut_var('nonexistent_var')
    expect.eq(result2, nil)
end

local function test_df_expr_to_ref()
    -- Test simple global
    expect.eq(utils.df_expr_to_ref('df.global.world'), df.global.world)

    -- Test field access
    expect.eq(utils.df_expr_to_ref('df.global.world.original_save_version'),
              df.global.world.original_save_version)

    -- Test array access
    expect.eq(utils.df_expr_to_ref('df.global.world[0]'), df.global.world[0])
end

function test.df_expr_to_ref()
    test_df_expr_to_ref()
end

local function test_OrderedTable()
    local t = utils.OrderedTable()
    local keys = {'first', 'second', 'third', 'fourth'}

    for i, key in ipairs(keys) do
        t[key] = i
    end

    local collected_keys = {}
    for k, v in pairs(t) do
        table.insert(collected_keys, k)
    end

    expect.eq(#collected_keys, 4)
    expect.eq(collected_keys[1], 'first')
    expect.eq(collected_keys[4], 'fourth')
end

function test.OrderedTable()
    test_OrderedTable()
end
