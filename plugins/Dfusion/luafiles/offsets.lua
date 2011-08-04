

function f_dwarves()
	pos=offsets.find(0,0x24,0x14,0x07,0,0,0,0xeb,0x08,0x8d) --search pattern
	if pos~=0 then
		return pos+2-offsets.base();
	else
		return 0;
	end
end
offsets.new("StartDwarfs",f_dwarves) -- finds the starting dwarf count
function f_creatures()
	--01C48034-base=0x1258034
	local val=0
	--print("Enter creature count:");
	--local r=io.stdin:read()
	for k,v in pairs(offsets.getvectors()) do
		if (v>60) and (v<100) then --used count changed some time ago... two hits second one is the right one. Maybe some smarter way could be better?
		--new version first one is right?? needs more testing thou...
			val= k-offsets.base()
			print(string.format("%x",val))
			break;
		end
		--vec=engine.peek(k,ptr_vector);
		
		--[[if(vec:size()==tonumber(r)) then
			val=k-offsets.base()
			print(string.format("off:%x",k))
		end--]]
	end
	offsets.new("AdvCreatureVec",val)
	return val
end
offsets.new("CreatureVec",f_creatures)
function f_words()
	local val=0
	for k,v in pairs(offsets.getvectors()) do
		local toff=engine.peekd(engine.peekd(k))
		if(engine.peekstr(toff)=="ABBEY") then
			val=k-offsets.base()
		end
	end
	return val
end
offsets.newlazy("WordVec",f_words)
function f_creatureptr() --a creature number under the pointer
	pos=offsets.find(0,0xa1,ANYDWORD,0x83,0xf8,0xff,0x75)
	print("Offset="..pos)
	if pos~=0 then
		pos=engine.peekd(pos+1)
		return pos-offsets.base()
	else
		return 0
	end
end
offsets.new("CreaturePtr",f_creatureptr)

function f_creaturegloss() --creature race vector
	for k,v in pairs(offsets.getvectors()) do
		if k~=0 then
			--print("Looking into:"..string.format("%x",k).." used:"..v)
			
			local vec=engine.peek(k,ptr_vector)
			if vec:size()>0 and vec:size()<100000 and vec:getval(0)~=0 then
				--print("\tval:"..string.format("%x",vec:getval(0)))
				local token=engine.peek(vec:getval(0),ptt_dfstring)
				--print("\t\tval:".. token:getval())
				if token:getval()=="TOAD" then -- more offsets could be found this way
					return k-offsets.base()
				end
			end
		end
	end
	return 0
end
offsets.new("CreatureGloss",f_creaturegloss)
--reaction vec: search for TAN_A_HIDE
function f_racevec() --current race
	--find all movsx anyreg,address
	local den={}
	local pos=offsets.findall(0,0x0f,0xbf,ANYBYTE,ADDRESS)
	for k,v in pairs(pos) do
		local add
		if v~=0 then
			add=engine.peekd(v+3)			
			if den[add]~=nil then
				den[add]= den[add]+1
			else
				den[add]=1
			end
		end

	end

	for k,v in pairs(den) do
		if v <60 then
			den[k]=nil
		end
		
	end
	for k,v in pairs(den) do
		print("Looking into:"..string.format("%x",k).." used:"..v.." Race:"..engine.peekw(k))
		if engine.peekw(k) >0 and engine.peekw(k)<1000 then
			
			return k-offsets.base()
		end
	end
	
	
	return 0
end
offsets.new("CurrentRace",f_racevec)
function f_pointer() --adventure (not only?) pointer x,y,z
	print("\n")
	local den={}
	local pos=0
	repeat
		pos=offsets.find(pos+3,0x0f,0xb7,ANYBYTE,ADDRESS)
		local add=engine.peekd(pos+3)
		local add2=engine.peekd(pos+13)
		local add3=engine.peekd(pos+23)
		if( math.abs(add-add2)==4 or math.abs(add-add3)==4) then

		if den[add]~=nil then
			den[add]= den[add]+1
		else
			den[add]=1
		end
		end
	until pos==0
	for k,v in pairs(den) do
		print("Looking into:"..string.format("%x",k).." used:"..v)
		return k-offsets.base()-4
	end
	return 0
end
offsets.new("Xpointer",f_pointer)
function f_adventure()
	RaceTable=RaceTable or BuildNameTable() -- this gets all the races' numbers
	--[[print("input chars race:")
	repeat
		r=io.stdin:read()
		if RaceTable[r]==nil then print("Incorrect race...") end
	until RaceTable[r]~=nil -- query till correct race is inputed
	rnum=RaceTable[r] --get its num
	print("Race:"..rnum)]]--
	myoff=0
	print("input player's creature's name (lowercaps):")
	r=io.stdin:read()
	
	for k,v in pairs(offsets.getvectors()) do -- now lets look through all vector offsets
		off=engine.peekd(k) --get vector start
		off=engine.peekd(off) --get 0 element (or first) in adventurer mode its our hero
		name=engine.peekstr(off)
		if(name==r) then
		--if engine.peek(off+140)==rnum then -- creature start+140 is the place where race is kept
			print(string.format("%x race:%x",k,engine.peekw(off+140)))
			myoff=k -- ok found it
			break
		end
	end
	if myoff~=0 then
		crvec=engine.peek(myoff,ptr_vector)
		print(string.format("player offset:%x",crvec:getval(0)))
		local legidVec=engine.peek(crvec:getval(0),ptr_Creature.legends)
		print(string.format("legends offset:%x",legidVec:getval(0)))
		local vtable=engine.peekd(legidVec:getval(0))
		print(string.format("vtable offset:%x",vtable))
		offsets.new("vtableLegends",vtable-offsets.base())
		return myoff-offsets.base() --save the offset for laters
	else
		return 0 --indicate failure
	end
end
offsets.newlazy("AdvCreatureVec",f_adventure) -- register a way to find this offset
--7127F0
function f_legends()
	pos=1
	T={}
	repeat
	pos=offsets.find(pos+1,0x50,0xb8,ANYDWORD,0xe8,ANYDWORD,0x8b,0xf0)
	off=engine.peekd(pos+2)
	vec=engine.peek(off,ptr_vector)
	if vec:size()~=0 then
		if T[off]~=nil then
			T[off].c=T[off].c+1
		else
			T[off]={c=1,vsize=vec:size()}
		end
		
	end

	until pos==0
	for k,v in pairs(T) do
		vec=engine.peek(k,ptr_vector)
		print(string.format("off:%x used:%i size:%d",k,v.c,v.vsize))
		print(string.format("fith elements id:%d",engine.peekd(vec:getval(5))))
		if engine.peekd(vec:getval(5))==5 then
		--if v.c==2 and v.vsize>1000 then
			return k-offsets.base()
		end
	end
	return 0
end
offsets.newlazy("Legends",f_legends)
function f_playerlegendid()
	local off=offsets.getEx("Legends")
	local pos=1
	repeat
		pos=offsets.find(pos+1,0xa1,DWORD_,off+4,0x2b,0x05,DWORD_,off)
		val=engine.peekd(pos+16)
		if engine.peekd(val)~=0 then
		--if val >offsets.base() then
			return val-offsets.base()
		end
		--print(string.format("%x",val))
	until pos==0
	return 0
end
offsets.newlazy("PlayerLegend",f_playerlegendid)

function f_world()
	local pos=offsets.base()
	T={}
	while pos~=0 do
		--pos=offsets.find(pos+6,0xa1,DWORD_,mapoffset,0x8b,0x4c,0x88,0xFC)
		pos=offsets.find(pos+6,0x8b,0x35,ANYDWORD,0x85,0xf6)--,0x8b,0x4c,0x88,0xFC)
		--pos2=offsets.find(pos,0x8b,0x34,0x88)
		
		if pos~=0 then
			add=engine.peekd(pos+2);
			--if pos2 ~=0 and pos2-pos<25 then
			--	print(string.format("Address:%x dist:%d Map:%x",pos2,pos2-pos,add))
			--	
			--end
			if add~=0 then
				if T[add]~=nil then
					T[add]=T[add]+1
				else
					T[add]=1
				end
			end
		end
		
	end
	local kk,vv
	vv=0
	for k,v in pairs(T) do
		if v>vv then
			vv=v
			kk=k
		end
		--print(string.format("Address:%x, times used:%d",k,v))
	end
	return kk-offsets.base()
end
offsets.new("WorldData",f_world)

function f_sites()
	local pos=offsets.base()
	T={}
	while pos~=0 do
		
		pos=offsets.find(pos+17,0xA1,ANYDWORD, --mov eax, ptr to some biger thing
						0x8B,0x90,0x24,0x01,0x00,0x00, --mov edx [eax+0x124]
						0x2b,0x90,0x20,0x01,0x00,0x00, --sub edx [eax+0x120]
						EOL)
		if pos~=0 then
			add=engine.peekd(pos+1)
			return add-offsets.base()
			
		end
		
	end
	return 0
end
offsets.newlazy("SiteData",f_sites) --actually has a lot more data... 
function f_items()
	local pos=offsets.base()
	while pos~= 0 do
		pos=offsets.find(pos+17,0x8b,0x0d,ANYDWORD, --mov eax, ptr to some biger thing
							0x8B,0x54,0x24,0x34)
		if pos~=0 then
			--print(string.format("%x",engine.peekd(pos+2)))
			local ret=engine.peekd(pos+2)-offsets.base()
			return ret
		end
	end
	return 0
end
offsets.new("Items",f_items)
function f_materials()
	for k,v in pairs(offsets.getvectors()) do
		--print("Looking into:"..string.format("%x",k).." used:"..v)
		local vec=engine.peek(k,ptr_vector)
		if vec:getval(0)~=0 then
			--print("\tval:"..string.format("%x",vec:getval(0)))
			local token=engine.peek(vec:getval(0),ptt_dfstring)
			if token:getval()~=""then
			--print("\t\tval:".. token:getval())
				if token:getval()=="IRON" then 
					--print("Found:"..string.format("%x",k).." used:"..v)
					return k-offsets.base()
				end
			end
		end
	end
	return 0
end
offsets.new("Materials",f_materials)