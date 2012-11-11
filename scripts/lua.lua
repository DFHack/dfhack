local args={...}
if args[1]=="--file" or args[1]=="-f" then
    local f,err=loadfile (args[2])
    if f==nil then
        qerror(err)
    end
    dfhack.pcall(f,table.unpack(args,3))
elseif args[1]=="--save" or args[1]=="-s" then
    if df.global.world.cur_savegame.save_dir=="" then
        qerror("Savefile not loaded")
    end
    local fname=args[2] or "dfhack.lua"
    fname=string.format("data/save/%s/%s",df.global.world.cur_savegame.save_dir,fname)
    local f,err=loadfile (fname)
    if f==nil then
        qerror(err)
    end
    dfhack.pcall(f,table.unpack(args,3))
elseif args[1]~=nil then
    local f,err=load(args[1],'=(lua command)', 't')
    if f==nil then
        qerror(err)
    end
    dfhack.pcall(f,table.unpack(args,2))
else
    dfhack.interpreter("lua","lua.history")
end