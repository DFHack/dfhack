local dfu=require("dfusion")
local ms=require("memscan")

CustomEmbark=defclass(CustomEmbark,dfu.BinaryPlugin)
CustomEmbark.ATTRS{filename="dfusion/embark/embark.o",name="CustomEmbark",race_caste_data=DEFAULT_NIL}
function CustomEmbark:install()
    local stoff=dfhack.internal.getAddress('start_dwarf_count')
    if stoff==nil then
        error("address for start_dwarf_count not found!")
    end
    local _,race_id_offset=df.sizeof(df.global.ui:_field("race_id"))
    local needle={0x0f,0xb7,0x0d} --movzx,...
    add_dword(needle,race_id_offset)    -- ...word ptr[]
    local mem=ms.get_code_segment() 
    local trg_offset=mem.uint8_t.find(needle,stoff)--maybe endoff=stoff+bignumber
    if trg_offset==nil then
        error("address for race_load not found")
    end
    needle={0x83,0xc8,0xff} -- or eax, 0xFF
    local caste_offset=mem.uint8_t.find(needle,trg_offset)
    if caste_offset==nil or caste_offset-stoff>1000 then
        error("Caste change code not found or found too far!")
    end
    
end
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
		--print("adding:"..line.." id:"..RaceTable[line])
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
	stoff=VersionInfo.getAddress('start_dwarf_count')
	if mypos then --if mod already loaded
		print("Mod already loaded @:"..mypos.." just updating")
		modpos=mypos
		_,modsize=engine.loadobj('dfusion/embark/embark.o')
		
		count=MakeTable(modpos,modsize,names) --just remake tables
	else
	
	_,tofind=df.sizeof(df.global.ui:_field("race_id"))

	loc=offsets.find(stoff,0x0f,0xb7,0x0d,DWORD_,tofind) --MOVZX  ECX,WORD PTR[]
	
	print(string.format("found:%x",loc))
	if((loc~=0)and(loc-stoff<1000)) then
		loc2=offsets.find(loc,0x83,0xc8,0xff) -- or eax, ffffff (for caste)
		if loc2== 0 then
			error ("Location for caste nulling not found!")
		end
		engine.pokeb(loc2,0x90)
		engine.pokeb(loc2+1,0x90)
		engine.pokeb(loc2+2,0x90)
		ModData=engine.installMod("dfusion/embark/embark.o","Embark",256)
		modpos=ModData.pos
		modsize=ModData.size
		local castepos=modpos+engine.FindMarker(ModData,"caste")
		local racepos=modpos+engine.FindMarker(ModData,"race")
		count=MakeTable(modpos,modsize,names)
		engine.poked(castepos,modpos+modsize) --fix array start for race
		engine.poked(racepos,modpos+modsize+count*2) --fix array start for caste
		print("sucess loading mod @:"..modpos)
		-- build race vector after module.
		
		
		--finaly poke in the call!
		engine.pokeb(loc,0x90)
		engine.pokeb(loc+1,0x90)
		engine.pokeb(loc+2,0xe8)
		engine.poked(loc+3,modpos-loc-7) 
		--engine.pokeb(loc+5,0x90)
		
		SetExecute(modpos)
	else
		error("did not find patch location, failing...")
	end

	end
end