
function friendship_in.install(names)
RaceTable=RaceTable or BuildNameTable()
mypos=engine.getmod("Friendship")
if mypos then
	modpos=mypos
	_,modsize=engine.loadobj("dfusion/friendship/friendship.o")
	_=nil
else
	modpos,modsize=engine.loadmod("dfusion/friendship/friendship.o","Friendship",1024)
	print(string.format("Loaded module @:%x",modpos))
end
count=0
for _,v in pairs(names) do
	if RaceTable[v] == nil then 
		--print("Failure, "..v.." not found!")
		error("Failure, "..v.." not found!")
		--break --maybe some indication of failure? and cleanup?
	end
	engine.pokew(modpos+modsize+count*2+4+2,RaceTable[v]) -- for some reason it compiled strangely
						-- cmp word[ebx+ecx*2],ax -> cmp word[ebx+ecx*2+2],ax
	count = count + 1
end
engine.poked(modpos+0x8f,modpos+modsize+4) -- set ptr to creatures
engine.poked(modpos+0x94,count)			   -- set count of creatures
engine.poked(modpos+0xb9,modpos+modsize) -- set safe location
engine.poked(modpos+0xc3,modpos+modsize) -- set safe location
SetExecute(modpos)
end
function pokeCall(off)
	engine.pokeb(off,0xe8)
	b=engine.peekb(off+1)
	engine.poked(off+1,modpos-off-5)
	engine.pokeb(off+5,b)
end