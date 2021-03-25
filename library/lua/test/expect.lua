-- Internal implementations of expect.*() functions for tests

-- NOTE: do not use this module in tests directly! The test wrapper (ci/test.lua)
-- wraps these functions to track which tests have passed/failed, and calling
-- these functions directly will not work as expected.


local expect = mkmodule('test.expect')

function expect.true_(value, comment)
    return not not value, comment, 'expected true, got ' .. tostring(value)
end

function expect.false_(value, comment)
    return not value, comment, 'expected false, got ' .. tostring(value)
end

function expect.fail(comment)
    return false, comment or 'check failed, no reason provided'
end

function expect.nil_(value, comment)
    return value == nil, comment, 'expected nil, got ' .. tostring(value)
end

function expect.eq(a, b, comment)
    return a == b, comment, ('%s ~= %s'):format(a, b)
end

function expect.ne(a, b, comment)
    return a ~= b, comment, ('%s == %s'):format(a, b)
end

function expect.lt(a, b, comment)
    return a < b, comment, ('%s >= %s'):format(a, b)
end

function expect.le(a, b, comment)
    return a <= b, comment, ('%s > %s'):format(a, b)
end

function expect.gt(a, b, comment)
    return a > b, comment, ('%s <= %s'):format(a, b)
end

function expect.ge(a, b, comment)
    return a >= b, comment, ('%s < %s'):format(a, b)
end

local function table_eq_recurse(a, b, keys, known_eq)
    if a == b then return true end
    local checked = {}
    for k,v in pairs(a) do
        if type(a[k]) == 'table' then
            if known_eq[a[k]] and known_eq[a[k]][b[k]] then goto skip end
            table.insert(keys, tostring(k))
            if type(b[k]) ~= 'table' then
                return false, keys, {tostring(a[k]), tostring(b[k])}
            end
            if not known_eq[a[k]] then known_eq[a[k]] = {} end
            for eq_tab,_ in pairs(known_eq[a[k]]) do
                known_eq[eq_tab][b[k]] = true
            end
            known_eq[a[k]][b[k]] = true
            if not known_eq[b[k]] then known_eq[b[k]] = {} end
            for eq_tab,_ in pairs(known_eq[b[k]]) do
                known_eq[eq_tab][a[k]] = true
            end
            known_eq[b[k]][a[k]] = true
            local matched, keys_at_diff, diff =
                    table_eq_recurse(a[k], b[k], keys, known_eq)
            if not matched then return false, keys_at_diff, diff end
            keys[#keys] = nil
        elseif a[k] ~= b[k] then
            table.insert(keys, tostring(k))
            return false, keys, {tostring(a[k]), tostring(b[k])}
        end
        ::skip::
        checked[k] = true
    end
    for k in pairs(b) do
        if not checked[k] then
            table.insert(keys, tostring(k))
            return false, keys, {tostring(a[k]), tostring(b[k])}
        end
    end
    return true
end

function expect.table_eq(a, b, comment)
    if (type(a) ~= 'table' and type(a) ~= 'userdata') or
            (type(b) ~= 'table' and type(b) ~= 'userdata') then
        return false, comment, 'operands to table_eq must be tables or userdata'
    end
    local keys, known_eq = {}, {}
    local matched, keys_at_diff, diff = table_eq_recurse(a, b, keys, known_eq)
    if matched then return true end
    local keystr = '['..keys_at_diff[1]..']'
    for i=2,#keys_at_diff do keystr = keystr..'['..keys_at_diff[i]..']' end
    return false, comment,
            ('key %s: "%s" ~= "%s"'):format(keystr, diff[1], diff[2])
end

function expect.error(func, ...)
    local ok, ret = pcall(func, ...)
    if ok then
        return false, 'no error raised by function call'
    else
        return true
    end
end

function expect.error_match(func, matcher, ...)
    local ok, err = pcall(func, ...)
    if ok then
        return false, 'no error raised by function call'
    elseif type(matcher) == 'string' then
        if not tostring(err):match(matcher) then
            return false, ('error "%s" did not match "%s"'):format(err, matcher)
        end
    elseif not matcher(err) then
        return false, ('error "%s" did not satisfy matcher'):format(err)
    end
    return true
end

function expect.pairs_contains(table, key, comment)
    for k, v in pairs(table) do
        if k == key then
            return true
        end
    end
    return false, comment, ('could not find key "%s" in table'):format(key)
end

function expect.not_pairs_contains(table, key, comment)
    for k, v in pairs(table) do
        if k == key then
            return false, comment, ('found key "%s" in table'):format(key)
        end
    end
    return true
end

return expect
