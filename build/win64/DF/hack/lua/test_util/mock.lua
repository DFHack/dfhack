local mock = mkmodule('test_util.mock')

function _patch_impl(patches_raw, callback, restore_only)
    local patches = {}
    for _, v in ipairs(patches_raw) do
        local p = {
            table = v[1],
            key = v[2],
            new_value = v[3],
        }
        p.old_value = p.table[p.key]
        -- no-op to ensure that the value can be restored by the finalizer below
        p.table[p.key] = p.old_value
        table.insert(patches, p)
    end

    return dfhack.with_finalize(
        function()
            for _, p in ipairs(patches) do
                p.table[p.key] = p.old_value
            end
        end,
        function()
            if not restore_only then
                for _, p in ipairs(patches) do
                    p.table[p.key] = p.new_value
                end
            end
            return callback()
        end
    )
end

--[[

Replaces `table[key]` with `value`, calls `callback()`, then restores the
original value of `table[key]`.

Usage:
    patch(table, key, value, callback)
    patch({
        {table, key, value},
        {table2, key2, value2},
    }, callback)

]]
function mock.patch(...)
    local args = {...}
    local patches
    local callback
    if #args == 4 then
        patches = {{args[1], args[2], args[3]}}
        callback = args[4]
    elseif #args == 2 then
        patches = args[1]
        callback = args[2]
    else
        error('expected 2 or 4 arguments')
    end

    return _patch_impl(patches, callback)
end

--[[

Restores the original value of `table[key]` after calling `callback()`.

Equivalent to: patch(table, key, table[key], callback)

Usage:
    restore(table, key, callback)
    restore({
        {table, key},
        {table2, key2},
    }, callback)

]]
function mock.restore(...)
    local args = {...}
    local patches
    local callback
    if #args == 3 then
        patches = {{args[1], args[2]}}
        callback = args[3]
    elseif #args == 2 then
        patches = args[1]
        callback = args[2]
    else
        error('expected 2 or 3 arguments')
    end

    return _patch_impl(patches, callback, true)
end

--[[

Returns a callable object that tracks the arguments it is called with, then
passes those arguments to `callback()`.

The returned object has the following properties:
- `call_count`: the number of times the object has been called
- `call_args`: a table of function arguments (shallow-copied) corresponding
    to each time the object was called

]]
function mock.observe_func(callback)
    local f = {
        call_count = 0,
        call_args = {},
    }

    setmetatable(f, {
        __call = function(self, ...)
            self.call_count = self.call_count + 1
            local args = {...}
            for i,v in ipairs(args) do
                if type(v) == 'table' then
                    -- just a shallow copy, but it offers some ability to
                    -- inspect original values in tables that were altered after
                    -- the call
                    args[i] = copyall(v)
                end
            end
            table.insert(self.call_args, args)
            return callback(...)
        end,
    })

    return f
end

--[[

Returns a callable object similar to `mock.observe_func()`, but which when
called, only returns the given `return_value`(s) with no additional side effects.

Intended for use by `patch()`.

Usage:
    func(return_value [, return_value2 ...])

See `observe_func()` for a description of the return value.

The return value also has an additional `return_values` field, which is a table
of values returned when the object is called. This can be modified.

]]
function mock.func(...)
    local f
    f = mock.observe_func(function()
        return table.unpack(f.return_values)
    end)
    f.return_values = {...}
    return f
end

return mock
