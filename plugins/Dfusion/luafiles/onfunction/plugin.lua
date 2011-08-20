onfunction=onfunction or {}
function onfunction.install()
	ModData=engine.installMod("dfusion/onfunction/functions.o","functions",4)
	modpos=ModData.pos
	modsize=ModData.size
	onfunction.pos=modpos
	trgpos=engine.getpushvalue()
	print(string.format("Function installed in:%x function to call is: %x",modpos,trgpos))
	local firstpos=modpos+engine.FindMarker(ModData,"function")
	engine.poked(firstpos,trgpos-firstpos-4) --call Lua-Onfunction
	onfunction.fpos=modpos+engine.FindMarker(ModData,"function3")
	engine.poked(modpos+engine.FindMarker(ModData,"function2"),modpos+modsize)
	engine.poked(onfunction.fpos,modpos+modsize)
	SetExecute(modpos)
	onfunction.calls={}
end
function OnFunction(values)
	print("Onfunction called!")
	print("Data:")
	for k,v in pairs(values) do
		print(string.format("%s=%x",k,v))
	end
	print("stack:")
	for i=0,2 do 
		print(string.format("%d %x",i,engine.peekd(values.esp+i*4)))
	end
	return  onfunction.calls[values.ret] --returns real function to call
end
function onfunction.patch(addr)
	
	if(engine.peekb(addr)~=0xe8) then
		error("Incorrect address, not a function call")
	else
		
		onfunction.calls[addr+5]=addr+engine.peekd(addr+1)+5 --adds real function to call
		engine.poked(addr+1,engine.getmod("functions")-addr-5)
		
	end
end
mypos=engine.getmod("functions")
if mypos then
	print("Onfunction already installed")
	onfunction.patch(0x189dd6+offsets.base())
else
	onfunction.install()
end