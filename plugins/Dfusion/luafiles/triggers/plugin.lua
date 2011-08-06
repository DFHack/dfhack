if FILE then
	return
end
callinfo={}
callinfo.regs={}
callinfo.regs["EAX"]=0
callinfo.regs["EBX"]=1
callinfo.regs["ECX"]=2
callinfo.regs["EDX"]=3
callinfo.regs["ESI"]=4
callinfo.regs["EDI"]=5
callinfo.regs["STACK1"]=6
callinfo.regs["STACK2"]=7
callinfo.regs["STACK3"]=8
callinfo.regs["STACK4"]=9
callinfo.regs["STACK5"]=10

mypos=engine.getmod("triggers_main")

function GetCount()
	return engine.peek(0,triggers.count)
end
function SetCount(val)
	engine.poke(0,triggers.count,val)
end
function NewCallTable(tbl)
	ret=tbl or {}
	for k,v in pairs(callinfo.regs) do
		ret[k]=0
	end
	return ret
end
function PushFunction(off,data)
	local i=GetCount()
	engine.poked(triggers.table.off+i*44,off) -- add function to table
	engine.poked(triggers.data.off+0,data["EAX"]) --set register data...
	engine.poked(triggers.data.off+4,data["EBX"])
	engine.poked(triggers.data.off+8,data["ECX"])
	engine.poked(triggers.data.off+12,data["EDX"])
	engine.poked(triggers.data.off+16,data["ESI"])
	engine.poked(triggers.data.off+20,data["EDI"])
	engine.poked(triggers.data.off+24,data["STACK1"])
	engine.poked(triggers.data.off+28,data["STACK2"])
	engine.poked(triggers.data.off+32,data["STACK3"])
	engine.poked(triggers.data.off+36,data["STACK4"])
	engine.poked(triggers.data.off+40,data["STACK5"])
	SetCount(i+1)
end
function loadTriggers()
	if triggers then return end
	triggers={}
	p=engine.getmod("triggerdata")
	triggers.count={off=engine.peekd(p),rtype=DWORD}
	triggers.table={off=engine.peekd(p+4),rtype=DWORD}
	triggers.ret={off=engine.peekd(p+8),rtype=DWORD}
	triggers.data={off=engine.peekd(p+12),rtype=DWORD}
end
if mypos then
	loadTriggers()
	dofile("dfusion/triggers/functions_menu.lua")
	--return
else
	triggers={}
	
	off=0x56D345+offsets.base()
	print(string.format("Function start %x",off))
	ModData=engine.installMod("dfusion/triggers/triggers.o","triggers_main")
	print("installed")
	modpos=ModData.pos
	modsize=ModData.size
	fdata=engine.newmod("function_body",256)
	
	
	engine.poked(modpos+engine.FindMarker(ModData,"trigercount"),modpos+modsize) -- count of functions
	engine.poked(modpos+engine.FindMarker(ModData,"f_loc"),modpos+modsize+4) -- function table start
	engine.poked(modpos+engine.FindMarker(ModData,"f_data"),fdata) -- function data start
	
	engine.poked(modpos+engine.FindMarker(ModData,"saveplace31"),modpos+modsize+260) -- save function loc
	engine.poked(modpos+engine.FindMarker(ModData,"saveplace32"),modpos+modsize+260) -- save function loc
	engine.poked(modpos+engine.FindMarker(ModData,"saveplace33"),modpos+modsize+260) -- save function loc
	engine.poked(modpos+engine.FindMarker(ModData,"saveplace"),modpos+modsize+256) -- save function loc
	engine.poked(modpos+engine.FindMarker(ModData,"trigcount2"),modpos+modsize) -- count of functions (for zeroing)
	engine.poked(modpos+engine.FindMarker(ModData,"saveplace2"),modpos+modsize+256) -- save function loc
	engine.poked(modpos+engine.FindMarker(ModData,"results"),modpos+modsize+256)  --overwrite function call with results 
	
	triggers.count={off=modpos+modsize,rtype=DWORD}
	triggers.table={off=modpos+modsize+4,rtype=DWORD}
	triggers.ret={off=modpos+modsize+256,rtype=DWORD}
	triggers.data={off=fdata,rtype=DWORD}
	pp=Allocate(4*4)
	engine.poked(pp,triggers.count.off)
	engine.poked(pp+4,triggers.table.off)
	engine.poked(pp+8,triggers.ret.off)
	engine.poked(pp+12,triggers.data.off)
	engine.newmod("triggerdata",0,pp)
	function pokeCall(off)
		engine.pokeb(off,0xe8)
		--b=engine.peekb(off+1)
		engine.poked(off+1,modpos-off-5)
		--engine.pokeb(off+5,b)
	end
	print(string.format("Mod @:%x",modpos))
	dat=engine.peekarb(off,5)
	engine.pokearb(modpos,dat,5)
	pokeCall(off)
end

