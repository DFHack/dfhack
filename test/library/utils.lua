local utils = require 'utils'

function test.OrderedTable()
    local t = utils.OrderedTable()
    local keys = {'a', 'c', 'e', 'd', 'b', 'q', 58, -1.2}
    for i = 1, #keys do
        t[keys[i]] = i
    end
    local i = 1
    for k, v in pairs(t) do
        expect.eq(k, keys[i], 'key order')
        expect.eq(v, i, 'correct value')
        i = i + 1
    end
end

function test.invert()
    local t = {}
    local i = utils.invert{'a', 4.4, false, true, 5, t}
    expect.eq(i.a, 1)
    expect.eq(i[4.4], 2)
    expect.eq(i[false], 3)
    expect.eq(i[true], 4)
    expect.eq(i[5], 5)
    expect.eq(i[t], 6)
    expect.eq(i[700], nil)
    expect.eq(i.foo, nil)
    expect.eq(i[{}], nil)
end

function test.invert_nil()
    local i = utils.invert{'a', nil, 'b'}
    expect.eq(i.a, 1)
    expect.eq(i[nil], nil)
    expect.eq(i.b, 3)
end

function test.invert_overwrite()
    local i = utils.invert{'a', 'b', 'a'}
    expect.eq(i.b, 2)
    expect.eq(i.a, 3)
end
