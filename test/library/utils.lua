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
