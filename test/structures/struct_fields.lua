config.target = 'core'

local COORD_FIELD_NAMES = {'x', 'y', 'z', 'isValid', 'clear'}
local COORD_FIELD_EXPECTED_DATA = {
    x = {type_name='int16_t'},
    y = {type_name='int16_t'},
    z = {type_name='int16_t'},
    isValid = {type_name='function'},
    clear = {type_name='function'},
}

local READONLY_MSG = 'Attempt to change a read%-only table.'

function test.access()
    local fields = df.coord._fields
    expect.true_(fields)
    expect.eq(fields, df.coord._fields)

    for name, expected in pairs(COORD_FIELD_EXPECTED_DATA) do
        expect.true_(fields[name], name)
        expect.eq(fields[name].name, name, name)
        expect.eq(fields[name].type_name, expected.type_name, name)
        expect.eq(type(fields[name].offset), 'number', name)
        expect.eq(type(fields[name].mode), 'number', name)
        expect.eq(type(fields[name].count), 'number', name)
    end
end

function test.globals_original_name()
    for name, info in pairs(df.global._fields) do
        expect.eq(type(info.original_name), 'string', name)
    end
end

function test.order()
    local i = 0
    for name in pairs(df.coord._fields) do
        i = i + 1
        expect.eq(name, COORD_FIELD_NAMES[i], i)
    end
    expect.eq(i, #COORD_FIELD_NAMES)
end

function test.nonexistent()
    expect.nil_(df.coord._fields.nonexistent)
end

function test.readonly()
    expect.error_match(READONLY_MSG, function()
        df.coord._fields.x = 'foo'
    end)
    expect.error_match(READONLY_MSG, function()
        df.coord._fields.nonexistent = 'foo'
    end)
    -- see TODOs in LuaTypes.cpp
    -- expect.error_match(READONLY_MSG, function()
    --     df.coord._fields.x.name = 'foo'
    -- end)

    expect.eq(df.coord._fields.x.name, 'x')
    expect.nil_(df.coord._fields.nonexistent)
end
