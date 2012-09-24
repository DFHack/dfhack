local args={...}
if args[1]=="--file" or args[1]=="-f" then
    local f=loadfile (args[2])
    dfhack.pcall(f,table.unpack(args,3))
elseif args[1]~=nil then
    local f=load(args[1],'=(lua command)', 't',)
    dfhack.pcall(f,table.unpack(args,2))
else
    dfhack.interpreter("lua","lua.history")
end