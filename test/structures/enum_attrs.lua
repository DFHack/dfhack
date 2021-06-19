function test.getmetatable()
    expect.eq(getmetatable(df.item_type.attrs), 'item_type.attrs')
end

local function check_valid_attr_entry(enum_type, index)
    local entry = enum_type.attrs[index]
    local suffix = ('%s entry at index %s'):format(enum_type._attr_entry_type, index)
    expect.ne(entry, nil, 'nil ' .. suffix)
    expect.true_(enum_type._attr_entry_type:is_instance(entry), 'invalid ' .. suffix)
end

function test.valid_items()
    for i in ipairs(df.item_type) do
        check_valid_attr_entry(df.item_type, i)
        -- expect.true_(df.item_type.attrs[i])
        -- expect.true_(df.item_type._attr_entry_type:is_instance(df.item_type.attrs[i]))
    end
end

function test.valid_items_name()
    for i, name in ipairs(df.item_type) do
        expect.eq(df.item_type.attrs[i], df.item_type.attrs[name])
    end
end

function test.valid_items_unique()
    -- check that every enum item has its own attrs entry
    local addr_to_name = {}
    for _, name in ipairs(df.item_type) do
        local _, addr = df.sizeof(df.item_type.attrs[name])
        if addr_to_name[addr] then
            expect.fail(('attrs shared between "%s" and "%s"'):format(name, addr_to_name[name]))
        else
            addr_to_name[addr] = name
        end
    end
end

function test.invalid_items()
    check_valid_attr_entry(df.item_type, df.item_type._first_item - 1)
    check_valid_attr_entry(df.item_type, df.item_type._last_item + 1)
end

function test.invalid_items_shared()
    expect.eq(df.item_type.attrs[df.item_type._first_item - 1], df.item_type.attrs[df.item_type._last_item + 1])
end

function test.length()
    expect.eq(#df.item_type.attrs, 0)
end

local function max_attrs_length(enum_type)
    return enum_type._last_item - enum_type._first_item + 1
end

function test.pairs()
    local i = 0
    for _ in pairs(df.item_type.attrs) do
        i = i + 1
        if i > max_attrs_length(df.item_type) then
            expect.fail('pairs() returned too many items: ' .. tostring(i))
            break
        end
    end
    expect.eq(i, 0)
end

function test.ipairs()
    local i = 0
    for index, value in ipairs(df.item_type.attrs) do
        if i == 0 then
            expect.eq(index, df.item_type._first_item, 'ipairs() returned wrong start index')
        end
        i = i + 1
        if i > max_attrs_length(df.item_type) then
            expect.fail('ipairs() returned too many items: ' .. tostring(i))
            break
        end
        expect.eq(value, df.item_type.attrs[i + df.item_type._first_item],
            'ipairs() returned incorrect attrs for item at index: ' .. tostring(index))
    end
    expect.eq(i, max_attrs_length(df.item_type))
end
