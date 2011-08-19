onfunction={}
function onfunction.install()
	ModData=engine.installMod("dfusion/onfunction/functions.o","functions")
	modpos=ModData.pos
	modsize=ModData.size
	onfunction.pos=modpos
	trgpos=engine.getpushvalue()
	print(string.format("Function installed in:%x function to call is: %x",modpos,trgpos))
	local firstpos=modpos+engine.FindMarker(ModData,"function")
	engine.poked(firstpos,trgpos-firstpos) --call first function
	engine.poked(modpos+engine.FindMarker(ModData,"function2"),modpos+engine.FindMarker(ModData,"function3")) -- function table start
	
end
function OnFunction(values)
	print("Onfunction called!")
	print("Data:")
	for k,v in pairs(values) do
		print(string.format("%s=%x",k,v))
	end
	return 0 --todo return real address
end
function onfunction.patch(addr)
	
	if(engine.peekb(addr)~=0xe8) then
		error("Incorrect address, not a function call")
	else
		--todo add to list of functions after patch
		engine.poked(addr+1,onfunction.pos-addr-1)
	end
end
mypos=engine.getmod("functions")
if mypos then
	print("Onfunction already installed")
else
	onfunction.install()
end