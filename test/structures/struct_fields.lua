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

    expect.error_match('string expected', function()
        expect.nil_(df.coord._fields[2])
    end)
    expect.error_match('string expected', function()
        expect.nil_(df.coord._fields[nil])
    end)
end

function test.readonly()
    expect.error_match(READONLY_MSG, function()
        df.coord._fields.x = 'foo'
    end)
    expect.error_match(READONLY_MSG, function()
        df.coord._fields.nonexistent = 'foo'
    end)
    expect.nil_(df.coord._fields.nonexistent)

    -- should have no effect
    df.coord._fields.x.name = 'foo'
    expect.eq(df.coord._fields.x.name, 'x')
end

function test.circular_refs_init()
    expect.eq(df.job._fields.list_link.type, df.job_list_link)
    expect.eq(df.job_list_link._fields.item.type, df.job)
end

function test.subclass_match()
    for f, parent in pairs(df.viewscreen._fields) do
        local child = df.viewscreen_titlest._fields[f]
        expect.table_eq(parent, child, f)
    end
end
