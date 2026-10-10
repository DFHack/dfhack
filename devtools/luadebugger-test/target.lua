local t = {}
local shared_upval = 42
local function helper(x)
    local inner = x * 2
    return inner + 1
end
function t.work(n)
    local acc = 0
    for i = 1, n do
        acc = acc + helper(i)
    end
    return acc
end
local function spawn()
    local co = coroutine.create(function()
        local mine = 'coro-local'
        return helper(10) + #mine
    end)
    local ok, res = coroutine.resume(co)
    return res
end
return t.work(3) + spawn()
