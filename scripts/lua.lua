-- Execute lua commands interactively or from files.

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
    if string.match(cmd, "^[~!]") then
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
        end
    end
else
    dfhack.interpreter("lua","lua.history")
end
