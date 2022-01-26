-- Internal implementations of expect.*() functions for tests

-- NOTE: do not use this module in tests directly! The test wrapper (ci/test.lua)
-- wraps these functions to track which tests have passed/failed, and calling
-- these functions directly will not work as expected.


local expect = mkmodule('test_util.expect')

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
    return a == b, comment, ('"%s" ~= "%s"'):format(a, b)
end

function expect.ne(a, b, comment)
    return a ~= b, comment, ('"%s" == "%s"'):format(a, b)
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

function expect.str_find(pattern, str_to_match, comment)
    if type(str_to_match) ~= 'string' then
        return false, comment, 'expected string, got ' .. type(str_to_match)
    end
    return str_to_match:find(pattern), comment,
            ('pattern "%s" not matched in "%s"'):format(pattern, str_to_match)
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

local function matches(obj, matcher)
    if type(matcher) == 'string' then
        return tostring(obj):match(matcher)
    elseif type(matcher) == 'function' then
        return matcher(obj)
    end
    return false
end

-- matches errors thrown from the specified function. the check passes if an
-- error is thrown and the thrown error matches the specified matcher.
--
-- matcher can be:
--  a string, interpreted as a lua pattern that matches the error message
--  a function that takes an err object and returns a boolean (true means match)
function expect.error_match(matcher, func, comment)
    local ok, err = pcall(func)
    if ok then
        return false, comment, 'no error raised by function call'
    end
    if matches(err, matcher) then return true end
    local matcher_str = ''
    if type(matcher) == 'string' then
        matcher_str = (': "%s"'):format(matcher)
    end
    return false, comment,
            ('error "%s" did not satisfy matcher%s'):format(err, matcher_str)
end

function expect.error(func, comment)
    return expect.error_match('.*', func, comment)
end

-- matches error messages output from dfhack.printerr() when the specified
-- callback is run. the check passes if all printerr messages are matched by
-- specified matchers and no matchers remain unmatched.
--
-- matcher can be:
--  a string, interpreted as a lua pattern that matches a message
--  a function that takes the string message and returns a boolean (true means
--    match)
--  a populated table that can be used to match multiple messages (explained
--    in more detail below)
--
-- if matcher is a table, it can contain:
--  a list of strings and/or functions which will be matched in order
--  a map of strings and/or functions to positive integers, which will be
--    matched (in any order) the number of times specified
--
-- when this function attempts to match a message, it will first try the next
-- matcher in the list (that is, the next numeric key). if that matcher doesn't
-- exist or doesn't match, it will try all string and function keys whose values
-- are numeric and > 0.
--
-- if a mapped matcher is matched, it will have its value decremented by 1.
function expect.printerr_match(matcher, func, comment)
    local saved_printerr = dfhack.printerr
    local messages = {}
    dfhack.printerr = function(msg) table.insert(messages, msg) end
    dfhack.with_finalize(
        function() dfhack.printerr = saved_printerr end,
        func)
    if type(matcher) ~= 'table' then matcher = {matcher} end
    while #messages > 0 do
        local msg = messages[1]
        if matches(msg, matcher[1]) then
            table.remove(matcher, 1)
            goto continue
        end
        for k,v in pairs(matcher) do
            if type(v) == 'number' and v > 0 and matches(msg, k) then
                local remaining = v - 1
                if remaining == 0 then
                    matcher[k] = nil
                else
                    matcher[k] = remaining
                end
                goto continue
            end
        end
        break -- failed to match message
        ::continue::
        table.remove(messages, 1)
    end
    local errmsgs, unmatched_messages, extra_matchers = {}, {}, {}
    for _,msg in ipairs(messages) do
        table.insert(unmatched_messages, ('"%s"'):format(msg))
    end
    if #unmatched_messages > 0 then
        table.insert(errmsgs,
                     ('unmatched dfhack.printerr() messages: %s'):format(
                        table.concat(unmatched_messages, ', ')))
    end
    for k,v in ipairs(matcher) do
        table.insert(extra_matchers, ('"%s"'):format(tostring(v)))
        matcher[k] = nil
    end
    for k,v in pairs(matcher) do
        table.insert(extra_matchers,
                     ('"%s"=%s'):format(tostring(k), tostring(v)))
    end
    if #extra_matchers > 0 then
        table.insert(errmsgs,
                     ('unmatched or invalid matchers: %s'):format(
                        table.concat(extra_matchers, ', ')))
    end
    if #errmsgs > 0 then
        return false, comment, table.concat(errmsgs, '; ')
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
