local mock = mkmodule('test_util.mock')


--[[
Usage:
    patch(table, key, value, callback)
    patch({
        {table, key, value},
        {table2, key2, value2}
    }, callback)
]]
function mock.patch(...)
    local args = {...}
    if #args == 4 then
        args = {{
            {args[1], args[2], args[3]}
        }, args[4]}
    end
    if #args ~= 2 then
        error('expected 2 or 4 arguments')
    end

    local callback = args[2]
    local patches = {}
    for _, v in ipairs(args[1]) do
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
            for _, p in ipairs(patches) do
                p.table[p.key] = p.new_value
            end
            return callback()
        end
    )
end

return mock
