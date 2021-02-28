-- Luacov helper functions. Note that this is not a dfhack module since it can't
-- depend on dfhack.lua.

local runner = require('luacov.runner')

print('runner.debug_hook:', runner.debug_hook)

function init()
    runner.init()
    print('** initializing luacov')
    print('**   set debug hook to:', debug.gethook())
end

-- Called by LuaTools.cpp to set the debug hook for new threads. We could do
-- this in C++, but that's complicated and scary.
function with_luacov(f)
    print('** wrapping function', f)
    return function(...)
        print('** setting debug hook')
        print('**   was:', debug.gethook())
        debug.sethook(runner.debug_hook, "l")
        print('**   is now:', debug.gethook())
        print('**   running function:', f)
        print('**   params:', ...)
        return f(...)
    end
end


return _ENV
