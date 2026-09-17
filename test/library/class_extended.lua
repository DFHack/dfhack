config.target = 'core'

function test.defclass_basic()
    -- Test basic class creation
    local MyClass = defclass(nil)
    expect.eq(MyClass.__index, MyClass)
    expect.table_eq(MyClass.ATTRS, {})
    expect.eq(MyClass.super, nil)
    expect.ne(getmetatable(MyClass), nil)
end

function test.defclass_with_parent()
    -- Test class creation with parent
    local ParentClass = defclass(nil)
    local ChildClass = defclass(nil, ParentClass)

    expect.eq(ChildClass.super, ParentClass)
    expect.eq(getmetatable(ChildClass).__index, ParentClass)
end

function test.defclass_attrs()
    -- Test class with ATTRS
    local MyClass = defclass(nil)
    MyClass.ATTRS {
        attr1 = 'default1',
        attr2 = 'default2',
    }

    expect.eq(MyClass.ATTRS.attr1, 'default1')
    expect.eq(MyClass.ATTRS.attr2, 'default2')
end

function test.mkinstance()
    -- Test basic instance creation
    local MyClass = defclass(nil)
    local instance = mkinstance(MyClass, {value = 100})

    expect.eq(instance.value, 100)
    expect.eq(getmetatable(instance), MyClass)
end

function test.instance_inheritance()
    -- Test instance method inheritance
    local ParentClass = defclass(nil)
    function ParentClass:parent_method()
        return 'parent'
    end

    local ChildClass = defclass(nil, ParentClass)
    function ChildClass:child_method()
        return 'child'
    end

    local instance = ChildClass({})
    expect.eq(instance:parent_method(), 'parent')
    expect.eq(instance:child_method(), 'child')
end

function test.init_method()
    -- Test init method
    local MyClass = defclass(nil)
    MyClass.ATTRS {
        value = 0,
    }

    function MyClass:init(init_table)
        self.value = (init_table.value or 0) * 2
    end

    local instance = MyClass({value = 5})
    expect.eq(instance.value, 10)
end

function test.preinit_postinit()
    -- Test preinit and postinit methods
    local call_order = {}

    local MyClass = defclass(nil)
    function MyClass:preinit(init_table)
        table.insert(call_order, 'preinit')
    end

    function MyClass:init(init_table)
        table.insert(call_order, 'init')
    end

    function MyClass:postinit(init_table)
        table.insert(call_order, 'postinit')
    end

    MyClass({})

    expect.eq(#call_order, 3)
    expect.eq(call_order[1], 'preinit')
    expect.eq(call_order[2], 'init')
    expect.eq(call_order[3], 'postinit')
end

function test.inheritance_init_order()
    -- Test init order with inheritance
    local call_order = {}

    local ParentClass = defclass(nil)
    function ParentClass:preinit(init_table)
        table.insert(call_order, 'parent_preinit')
    end

    function ParentClass:init(init_table)
        table.insert(call_order, 'parent_init')
    end

    function ParentClass:postinit(init_table)
        table.insert(call_order, 'parent_postinit')
    end

    local ChildClass = defclass(nil, ParentClass)
    function ChildClass:preinit(init_table)
        table.insert(call_order, 'child_preinit')
    end

    function ChildClass:init(init_table)
        table.insert(call_order, 'child_init')
    end

    function ChildClass:postinit(init_table)
        table.insert(call_order, 'child_postinit')
    end

    ChildClass({})

    expect.eq(#call_order, 6)
    expect.eq(call_order[1], 'child_preinit')
    expect.eq(call_order[2], 'parent_preinit')
    expect.eq(call_order[3], 'parent_init')
    expect.eq(call_order[4], 'child_init')
    expect.eq(call_order[5], 'parent_postinit')
    expect.eq(call_order[6], 'child_postinit')
end

function test.callback_method()
    -- Test callback method
    local MyClass = defclass(nil)
    function MyClass:test_method(arg)
        return arg * 2
    end

    local instance = MyClass({})
    local cb = instance:callback('test_method')

    expect.eq(cb(5), 10)
end

function test.cb_getfield()
    -- Test cb_getfield method
    local MyClass = defclass(nil)
    local instance = MyClass()
    instance.value = 42

    local getter = instance:cb_getfield('value')
    expect.eq(getter(), 42)
end

function test.cb_setfield()
    -- Test cb_setfield method
    local MyClass = defclass(nil)
    local instance = MyClass()
    instance.value = 42

    local setter = instance:cb_setfield('value')
    setter(100)
    expect.eq(instance.value, 100)
end

function test.assign_method()
    -- Test assign method
    local MyClass = defclass(nil)
    local instance = MyClass({value = 1})

    instance:assign({value = 2, new_field = 3})
    expect.eq(instance.value, 2)
    expect.eq(instance.new_field, 3)
end

-- function test.invoke_before()
--     -- Test invoke_before method
--     local call_order = {}

--     local MyClass = defclass(nil)
--     function MyClass:test_method()
--         table.insert(call_order, 'original')
--     end

--     function MyClass:invoke_before_test_method()
--         table.insert(call_order, 'before')
--     end

--     local instance = MyClass({})
--     instance:invoke_before('test_method')

--     expect.eq(#call_order, 2)
--     expect.eq(call_order[1], 'before')
--     expect.eq(call_order[2], 'original')
-- end

-- function test.invoke_after()
--     -- Test invoke_after method
--     local call_order = {}

--     local MyClass = defclass(nil)
--     function MyClass:test_method()
--         table.insert(call_order, 'original')
--     end

--     function MyClass:invoke_after_test_method()
--         table.insert(call_order, 'after')
--     end

--     local instance = MyClass()
--     instance:invoke_after('test_method')

--     expect.eq(#call_order, 2)
--     expect.eq(call_order[1], 'original')
--     expect.eq(call_order[2], 'after')
-- end

function test.attrs_meta()
    -- Test ATTRS metatable behavior
    local MyClass = defclass(nil)
    MyClass.ATTRS {
        attr1 = 'value1',
        attr2 = 'value2',
    }

    -- Test that ATTRS can be called to add attributes
    MyClass.ATTRS {
        attr3 = 'value3',
    }

    expect.eq(MyClass.ATTRS.attr1, 'value1')
    expect.eq(MyClass.ATTRS.attr2, 'value2')
    expect.eq(MyClass.ATTRS.attr3, 'value3')
end

function test.default_nil()
    -- Test DEFAULT_NIL behavior
    local MyClass = defclass(nil)
    MyClass.ATTRS {
        optional = DEFAULT_NIL,
        required = 'default',
    }

    local instance1 = MyClass({})
    expect.eq(instance1.optional, nil)
    expect.eq(instance1.required, 'default')

    local instance2 = MyClass({optional = 'provided'})
    expect.eq(instance2.optional, 'provided')
    expect.eq(instance2.required, 'default')
end

function test.class_reload()
    -- Test that classes can be reloaded
    local MyClass = defclass(nil)
    MyClass.ATTRS {
        version = 1,
    }

    -- Simulate reload by updating ATTRS
    MyClass.ATTRS {
        version = 2,
        new_attr = 'new',
    }

    expect.eq(MyClass.ATTRS.version, 2)
    expect.eq(MyClass.ATTRS.new_attr, 'new')
end

function test.instance_field_access()
    -- Test instance field access patterns
    local MyClass = defclass(nil)
    MyClass.ATTRS {
        public_field = 'public',
    }

    function MyClass:init(init_table)
        self.private_field = 'private'
    end

    local instance = MyClass({})
    expect.eq(instance.public_field, 'public')
    expect.eq(instance.private_field, 'private')

    -- Test setting new fields
    instance.dynamic_field = 'dynamic'
    expect.eq(instance.dynamic_field, 'dynamic')
end
