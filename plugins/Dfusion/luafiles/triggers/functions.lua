function func.Find_Print()
	pos=offsets.find(offsets.base(),0x73,0x02,0x8b,0xce,0x53,0x6a,0x01,0x6a,0x06,CALL) -- a hack for now...
	return engine.peekd(pos+10)+pos+14-offsets.base()
end
function func.PrintMessage(msg,color1,color2)
	func.f_print_pos= func.f_print_pos or func.Find_Print()
	print(string.format("Print @:%x",func.f_print_pos))
	debuger.suspend()
	d=NewCallTable() -- make a call table
	t=Allocate(string.len(msg))
	engine.pokestr(t,msg)
	--print(string.format("Message location:%x",t))
	d["ECX"]=t --set ecx to message location
	d["STACK5"]=color1 -- push to stack color1
	d["STACK4"]=color2 -- push to stack color2
	d["STACK3"]=0 -- this is usually 0 maybe a struct pointing to location of this message?
	PushFunction(func.f_print_pos+offsets.base(),d) -- prep to call function
	-- was 0x27F030
	debuger.resume()
end
