-- Execute lua commands interactively or from files.
--[[=begin

lua
===
There are the following ways to invoke this command:

1. ``lua`` (without any parameters)

   This starts an interactive lua interpreter.

2. ``lua -f "filename"`` or ``lua --file "filename"``

   This loads and runs the file indicated by filename.

3. ``lua -s ["filename"]`` or ``lua --save ["filename"]``

   This loads and runs the file indicated by filename from the save
   directory. If the filename is not supplied, it loads "dfhack.lua".

4. ``:lua`` *lua statement...*

   Parses and executes the lua statement like the interactive interpreter would.

=end]]

local args={...}
local cmd = args[1]

if cmd=="--file" or cmd=="-f" then
    local f,err=loadfile (args[2])
    if f==nil then
        qerror(err)
    end
    dfhack.safecall(f,table.unpack(args,3))
elseif cmd=="--save" or cmd=="-s" then
    if df.global.world.cur_savegame.save_dir=="" then
        qerror("Savefile not loaded")
    end
    local fname=args[2] or "dfhack.lua"
    fname=string.format("data/save/%s/%s",df.global.world.cur_savegame.save_dir,fname)
    local f,err=loadfile (fname)
    if f==nil then
        qerror(err)
    end
    dfhack.safecall(f,table.unpack(args,3))
elseif cmd~=nil then
    -- Support some of the prefixes allowed by dfhack.interpreter
    local prefix
    if string.match(cmd, "^[~@!]") then
        prefix = string.sub(cmd, 1, 1)
        cmd = 'return '..string.sub(cmd, 2)
    end

    local f,err=load(cmd,'=(lua command)', 't')
    if f==nil then
        qerror(err)
    end

    local rv = table.pack(dfhack.safecall(f,table.unpack(args,2)))

    if rv[1] and prefix then
        print(table.unpack(rv,2,rv.n))
        if prefix == '~' then
            printall(rv[2])
        elseif prefix == '@' then
            printall_ipairs(rv[2])
        end
    end
else
    dfhack.interpreter("lua","lua.history")
end
