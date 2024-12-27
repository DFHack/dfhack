-- tools for registering callbacks periodically

local _ENV = mkmodule("repeat-util")

repeating = repeating or {}

dfhack.onStateChange.repeatUtilStateChange = function(code)
    if code == SC_WORLD_UNLOADED then
        repeating = {}
    end
end

function isScheduled(name)
    return repeating[name] ~= nil
end

function listScheduled()
    local result = {}
    for name, _ in pairs(repeating) do
        table.insert(result, name)
    end
    return result
end

function cancel(name)
    if not repeating[name] then
        return false
    end
    if repeating[name] ~= -1 then
        dfhack.timeout_active(repeating[name], nil)
    end
    repeating[name] = nil
    return true
end

function scheduleEvery(name, time, timeUnits, func)
    cancel(name)
    local function helper()
        local now_ms = dfhack.getTickCount()
        func()
        dfhack.internal.recordRepeatRuntime(name, now_ms)

        if repeating[name] then
            repeating[name] = dfhack.timeout(time, timeUnits, helper)
        end
    end
    repeating[name] = -1
    helper()
end

function scheduleUnlessAlreadyScheduled(name, time, timeUnits, func)
    if repeating[name] then
        return
    end
    scheduleEvery(name, time, timeUnits, func)
end

return _ENV
