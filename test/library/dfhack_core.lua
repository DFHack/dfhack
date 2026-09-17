config.target = 'core'

function test.safe_index()
    -- Test basic indexing
    local t = {a = 1, b = {c = 2}}
    expect.eq(dfhack.safe_index(t, 'a'), 1)
    expect.eq(dfhack.safe_index(t, 'b', 'c'), 2)

    -- Test nil handling
    expect.eq(dfhack.safe_index(nil, 'a'), nil)
    expect.eq(dfhack.safe_index(t, 'nonexistent'), nil)
    expect.eq(dfhack.safe_index(t, 'b', 'nonexistent'), nil)

    -- Test nested nil handling
    expect.eq(dfhack.safe_index(t, 'b', 'c', 'd'), nil)
end

local function test_ensure_key()
    local t = {}

    -- Test creating new key
    local result = dfhack.ensure_key(t, 'new_key')
    expect.table_eq(result, {})
    expect.table_eq(t.new_key, {})

    -- Test existing key
    t.existing = {value = 1}
    result = dfhack.ensure_key(t, 'existing')
    expect.eq(result, t.existing)
    expect.eq(t.existing.value, 1)

    -- Test with default value
    result = dfhack.ensure_key(t, 'with_default', {default = true})
    expect.table_eq(result, {default = true})
end

function test.ensure_key()
    test_ensure_key()
end

local function test_ensure_keys()
    local t = {}

    -- Test creating nested keys
    local result = dfhack.ensure_keys(t, 'level1', 'level2', 'level3')
    expect.eq(result, t.level1.level2.level3)
    expect.table_eq(t.level1, {})
    expect.table_eq(t.level1.level2, {})
    expect.table_eq(t.level1.level2.level3, {})

    -- Test partial existing path
    t.level1.level2.level3 = {existing = true}
    result = dfhack.ensure_keys(t, 'level1', 'level2', 'level3')
    expect.eq(result, t.level1.level2.level3)
    expect.true_(result.existing)
end

function test.ensure_keys()
    test_ensure_keys()
end

local function test_copyall()
    local t = {a = 1, b = 2, c = {nested = true}}
    local copy = dfhack.copyall(t)

    expect.table_eq(copy, t)
    expect.ne(copy, t) -- Different table reference
    expect.eq(copy.c, t.c) -- Shallow copy
end

function test.copyall()
    test_copyall()
end

local function test_pos_functions()
    -- Test pos2xyz
    local pos = {x = 10, y = 20, z = 30}
    local x, y, z = dfhack.pos2xyz(pos)
    expect.eq(x, 10)
    expect.eq(y, 20)
    expect.eq(z, 30)

    -- Test pos2xyz with invalid pos
    local invalid_pos = {x = -30000}
    expect.eq(dfhack.pos2xyz(invalid_pos), nil)

    -- Test xyz2pos
    local new_pos = dfhack.xyz2pos(5, 10, 15)
    expect.eq(new_pos.x, 5)
    expect.eq(new_pos.y, 10)
    expect.eq(new_pos.z, 15)

    -- Test xyz2pos with nil
    local nil_pos = dfhack.xyz2pos(nil, nil, nil)
    expect.eq(nil_pos.x, -30000)
    expect.eq(nil_pos.y, -30000)
    expect.eq(nil_pos.z, -30000)

    -- Test same_xyz
    expect.true_(dfhack.same_xyz(pos, pos))
    expect.false_(dfhack.same_xyz(pos, {x = 10, y = 20, z = 25}))

    -- Test pos2xy
    local x2, y2 = dfhack.pos2xy(pos)
    expect.eq(x2, 10)
    expect.eq(y2, 20)

    -- Test xy2pos
    local pos2d = dfhack.xy2pos(3, 7)
    expect.eq(pos2d.x, 3)
    expect.eq(pos2d.y, 7)

    -- Test same_xy
    expect.true_(dfhack.same_xy(pos, pos))
    expect.true_(dfhack.same_xy(pos, {x = 10, y = 20, z = 99}))
    expect.false_(dfhack.same_xy(pos, {x = 10, y = 25, z = 30}))
end

function test.pos_functions()
    test_pos_functions()
end

local function test_string_extensions()
    -- Test startswith
    expect.true_(("hello world"):startswith("hello"))
    expect.false_(("hello world"):startswith("world"))
    expect.true_(("test"):startswith("test"))

    -- Test endswith
    expect.true_(("hello world"):endswith("world"))
    expect.false_(("hello world"):endswith("hello"))
    expect.true_(("test"):endswith("test"))

    -- Test split
    local parts = "a,b,c":split(",")
    expect.eq(parts[1], "a")
    expect.eq(parts[2], "b")
    expect.eq(parts[3], "c")

    -- Test split with default delimiter
    local words = "hello world test":split()
    expect.eq(words[1], "hello")
    expect.eq(words[2], "world")
    expect.eq(words[3], "test")

    -- Test trim
    expect.eq(("  hello  "):trim(), "hello")
    expect.eq(("\t\nworld\n\t"):trim(), "world")

    -- Test wrap
    local wrapped = "This is a long string that needs to be wrapped":wrap(20)
    expect.true_(#wrapped > 20) -- Should be multiple lines

    -- Test escape_pattern
    local escaped = "a+b*c":escape_pattern()
    expect.true_(escaped:find("%+"))
    expect.true_(escaped:find("%*"))
end

function test.string_extensions()
    test_string_extensions()
end

local function test_with_finalize()
    local cleanup_called = false
    local result

    dfhack.with_finalize(
        function() cleanup_called = true end,
        function() result = "success" end
    )

    expect.true_(cleanup_called)
    expect.eq(result, "success")
end

function test.with_finalize()
    test_with_finalize()
end

local function test_with_onerror()
    local cleanup_called = false
    local ok, err = dfhack.pcall(function()
        dfhack.with_onerror(
            function() cleanup_called = true end,
            function() error("test error") end
        )
    end)

    expect.false_(ok)
    expect.true_(cleanup_called)
end

function test.with_onerror()
    test_with_onerror()
end

local function test_mkmodule()
    -- Test that mkmodule creates proper module structure
    local test_module = mkmodule('test.temp_module')
    expect.table_eq(test_module, {})
end

function test.mkmodule()
    test_mkmodule()
end

local function test_defclass()
    -- Test basic class creation
    local MyClass = defclass(nil)
    expect.table_eq(MyClass, {})
    expect.eq(MyClass.super, nil)

    -- Test class with parent
    local ParentClass = defclass(nil)
    local ChildClass = defclass(nil, ParentClass)
    expect.eq(ChildClass.super, ParentClass)

    -- Test instance creation
    local instance = MyClass({test_value = 42})
    expect.eq(instance.test_value, 42)
end

function test.defclass()
    test_defclass()
end

local function test_mkinstance()
    local MyClass = defclass(nil)
    local instance = mkinstance(MyClass, {value = 100})
    expect.eq(instance.value, 100)
end

function test.mkinstance()
    test_mkinstance()
end
