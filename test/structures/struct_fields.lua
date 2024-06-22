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

local function listFieldNames(t)
    local names = {}
    for name in pairs(t._fields) do
        table.insert(names, name)
    end
    return names
end

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
    expect.table_eq(listFieldNames(df.coord), COORD_FIELD_NAMES)
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

function test.count()
    expect.eq(df.unit._fields.relationship_ids.count, 9)
end

function test.index_enum()
    expect.eq(df.unit._fields.relationship_ids.index_enum, df.unit_relationship_type)
end

function test.ref_target()
    expect.eq(df.unit._fields.hist_figure_id.ref_target, df.historical_figure)
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

function test.subclass_order()
    -- ensure that parent class fields come before subclass fields
    local hierarchy = {df.item, df.item_actual, df.item_crafted, df.item_constructed, df.item_bedst}
    local field_names = {}
    for _, t in pairs(hierarchy) do
        field_names[t] = listFieldNames(t)
    end
    for ic = 1, #hierarchy do
        for ip = 1, ic - 1 do
            local parent_fields = listFieldNames(hierarchy[ip])
            local child_fields = listFieldNames(hierarchy[ic])
            child_fields = table.pack(table.unpack(child_fields, 1, #parent_fields))
            child_fields.n = nil
            expect.table_eq(child_fields, parent_fields, ('compare %s to %s'):format(hierarchy[ip], hierarchy[ic]))
        end
    end
end
