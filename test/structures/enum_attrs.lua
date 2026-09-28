config.target = 'core'

-- https://github.com/DFHack/dfhack/issues/1860
-- enum.attrs indexes never return nil (out-of-range indexes return a default
-- entry), so ipairs() on an attrs table needs a bounded __ipairs that stops
-- where ipairs() on the enum itself stops

local function enum_keys(enum)
    local keys = {}
    for i in ipairs(enum) do
        table.insert(keys, i)
    end
    return keys
end

-- iterate attrs with a hard cap so a regression fails instead of hanging
local function attrs_keys(enum, limit)
    local keys, values = {}, {}
    for i, attrs in ipairs(enum.attrs) do
        table.insert(keys, i)
        values[i] = attrs
        if #keys > limit then
            return keys, values, false
        end
    end
    return keys, values, true
end

local function check_attrs_ipairs(enum, name)
    local expected = enum_keys(enum)
    local keys, values, terminated = attrs_keys(enum, #expected + 1)
    expect.true_(terminated, 'ipairs(' .. name .. '.attrs) did not terminate')
    expect.table_eq(expected, keys)
    for _, i in ipairs(keys) do
        expect.eq(values[i], enum.attrs[i],
                  'wrong attrs entry yielded for ' .. name .. '[' .. i .. ']')
    end
end

function test.plain_enum()
    check_attrs_ipairs(df.item_type, 'df.item_type')
end

function test.complex_enum()
    expect.true_(df.pronoun_type._complex)
    check_attrs_ipairs(df.pronoun_type, 'df.pronoun_type')
end

function test.out_of_range_index()
    -- invalid indexes must keep returning the default entry
    expect.true_(df.item_type.attrs[df.item_type._last_item + 1])
    expect.eq(df.item_type.attrs[df.item_type._last_item + 1],
              df.item_type.attrs[df.item_type._last_item + 100])
end
