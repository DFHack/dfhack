--scripts/modtools/anonymous-script.lua
--author expwnent
--a tool for invoking simple lua scripts without putting them in a file first
--anonymous-script "print(args[1])" arg1 arg2
--# prints "arg1"

local args = {...}

--automatically collect arguments to make the anonymous script more succinct
local f,err = load('local args = {...}; ' .. args[1], '=(anonymous lua script)') --,'=(lua command)', 't')
if err then
 error(err)
end

--we don't care about the result even if they return something for some reason: we just want to ensure its side-effects happen and print appropriate error messages
dfhack.safecall(f,table.unpack(args,2))

