-- Luacov helper functions. Note that this is not a dfhack module since it can't
-- depend on dfhack.lua.

local runner = require('luacov.runner')

-- Called by LuaTools.cpp to set the debug hook for new threads. We could do
-- this in C++, but that's complicated and scary.
function with_luacov(f)
    return function(...)
        debug.sethook(runner.debug_hook, "l")
        return f(...)
    end
end

return _ENV
