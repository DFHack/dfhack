--scripts/modtools/anonymous-script.lua
--author expwnent
--a tool for invoking simple lua scripts without putting them in a file first
--[[=begin

modtools/anonymous-script
=========================
This allows running a short simple Lua script passed as an argument instead of
running a script from a file. This is useful when you want to do something too
complicated to make with the existing modtools, but too simple to be worth its
own script file.  Example::

    anonymous-script "print(args[1])" arg1 arg2
    # prints "arg1"

=end]]
local args = {...}

--automatically collect arguments to make the anonymous script more succinct
--load(ld,source,mode,env)
local f,err = load('local args = {...}; ' .. args[1], '=(anonymous lua script)') --,'=(lua command)', 't')
if err then
 error(err)
end

--we don't care about the result even if they return something for some reason: we just want to ensure its side-effects happen and print appropriate error messages
dfhack.safecall(f,table.unpack(args,2))

