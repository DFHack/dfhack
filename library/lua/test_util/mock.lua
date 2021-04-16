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

function mock.func(return_value)
    local f = {
        return_value = return_value,
        call_count = 0,
        call_args = {},
    }

    setmetatable(f, {
        __call = function(self, ...)
            self.call_count = self.call_count + 1
            table.insert(self.call_args, {...})
            return self.return_value
        end,
    })

    return f
end

return mock
