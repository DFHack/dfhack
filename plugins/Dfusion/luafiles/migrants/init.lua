--install part
function migrants(names)
RaceTable=RaceTable or BuildNameTable()
mypos=engine.getmod("Migrants")
if mypos then
	print("Migrant mod is running already @:"..mypos)
	modpos=mypos
	_,modsize=engine.loadobj("dfusion/migrants/migrants.o")
	count=0
	for _,v in pairs(names) do
		if RaceTable[v] == nil then 
			print("Failure, "..v.." not found!")
			break --maybe some indication of failure? and cleanup?
		end
		engine.pokew(modpos+modsize+count*2+4,RaceTable[v]) 
		count = count + 1
	end
	seedpos=modpos+modsize
	engine.poked(seedpos,math.random(1234567)) -- seed the generator :)
	engine.poked(modpos+0x1c,count) --max size for div
	
else
	modpos,modsize=engine.loadmod("dfusion/migrants/migrants.o","Migrants",400)
	print(string.format("Loaded module @:%x",modpos))
	count=0
	for _,v in pairs(names) do
		if RaceTable[v] == nil then 
			print("Failure, "..v.." not found!")
			break --maybe some indication of failure? and cleanup?
		end
		engine.pokew(modpos+modsize+count*2+4,RaceTable[v]) 
							
		count = count + 1
	end

	seedpos=modpos+modsize
	engine.poked(modpos+0x04,seedpos)
	engine.poked(modpos+0x15,seedpos)

	engine.poked(seedpos,math.random(1234567)) -- seed the generator :)
	engine.poked(modpos+0x1c,count) --max size for div

	engine.poked(modpos+0x26,seedpos+4) --start of array

	--patch part
	--pos=62873C+DF
	-- pattern: A1,DWORD_,"CURRENTRACE",56,89,ANYBYTE,ANYBYTE,34,e8
	_,raceoff=df.sizeof(df.global.ui:_field('race_id'))
	pos=offsets.find(offsets.base(),0xa1,DWORD_,raceoff,0x56,0x89,ANYBYTE,ANYBYTE,0x34,0xe8)
	function pokeCall(off)
		engine.pokeb(off,0xe8)
		engine.poked(off+1,modpos-off-5)
	end
	if pos~=0 then
		print(string.format("Found @:%x",pos))
		pokeCall(pos)
	else
		print("Not found patch location!!!")
	end
	end

end