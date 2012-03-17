function MakeTable(modpos,modsize,names)
	count=0
	castes={}
	--print("Making table")
	for _,line in pairs(names) do
		--print("Line:"..line)
		tpos=string.find(line,":")
		if tpos~=nil  then
			--print("Was line:"..line)
			table.insert(castes,tonumber(string.sub(line,tpos+1)))
			line=string.sub(line,1,tpos-1)
			--print("IS line:"..line)
		else
			table.insert(castes,-1)
		end
		  if RaceTable[line] == nil then 
			error("Failure, "..line.." not found!")
		  end
		engine.pokew(modpos+modsize+count*2,RaceTable[line]) -- add race
		count = count + 1
	end
	i=0
	for _,caste in pairs(castes) do
	
		engine.pokew(modpos+modsize+count*2+i*2,caste) -- add caste
		i= i+1
	end
	
	engine.poked(stoff,count)
	return count
end
function embark(names) 
	RaceTable=RaceTable or BuildNameTable()
	mypos=engine.getmod('Embark')
	stoff=offsets.getEx('StartDwarfs')
	if mypos then --if mod already loaded
		print("Mod already loaded @:"..mypos.." just updating")
		modpos=mypos
		_,modsize=engine.loadobj('dfusion/embark/embark.o')
		
		count=MakeTable(modpos,modsize,names) --just remake tables
	else
	
	tofind=addressOf(df.ui,"race_id")

	loc=offsets.find(stoff,0x0f,0xb7,0x0d,DWORD_,tofind) --MOVZX  ECX,WORD PTR[]

	print("found:"..loc)

	if((loc~=0)and(loc-stoff<1000)) then
		modpos,modsize=engine.loadmod('dfusion/embark/embark.o','Embark',256)
		count=MakeTable(modpos,modsize,names)
		engine.poked(modpos+0x18,modpos+modsize) --fix array start for race
		engine.poked(modpos+0x08,modpos+modsize+count*2) --fix array start for caste
		print("sucess loading mod @:"..modpos)
		-- build race vector after module.
		
		
		--finaly poke in the call!
		engine.pokeb(loc,0x6a)
		engine.pokeb(loc+1,0xFF)
		engine.pokeb(loc+2,0xe8)
		engine.poked(loc+3,modpos-loc-7) 
		--engine.pokeb(loc+5,0x90)
		SetExecute(modpos)
	else
		error("did not find patch location, failing...")
	end

	end
end