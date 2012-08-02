dofile("dfusion/offsets_misc.lua")
STD_STRING=0
DWORD=1
WORD=2
BYTE=3
QWORD=4
DOUBLE=5
FLOAT=6

getline=function (inp)
return dfhack.lineedit(inp or "")
end
io.stdin=nil

function printd(...)
	if DEBUG then
		print(...)
	end
end
function GetTextRegion()
	if __TEXT ~=nil then --Chache this, not going to change.
		return __TEXT
	end
	
	ranges__=Process.getMemRanges()
	--print("Ranges:"..#ranges__)
	for k,v in pairs(ranges__) do
		for k2,v2 in pairs(v) do
			--print(string.format("%d %s->%s",k,tostring(k2),tostring(v2)))
		end
		--local num
		--flgs=""
		--if(v["read"])then flgs=flgs..'r' end
		--if(v["write"])then	flgs=flgs..'w' end
		--if(v["execute"]) then flgs=flgs..'e' end
		--if num>=100 then
		--print(string.format("%d %x->%x %s %s",k,v["start"],v["end"],v.name or "",flgs))
		--end
		local pos=string.find(v.name,"Dwarf Fortress.exe") or string.find(v.name,"libs/Dwarf_Fortress")
		if(pos~=nil) and v["execute"] then
			__TEXT=v;
			return v;
		end
	end
	error(".Text region not found!")
end
function UpdateRanges()
	ranges__=Process.getMemRanges()
end
function GetRegionIn(pos)
	ranges__=ranges__ or Process.getMemRanges()
	for k,v in pairs(ranges__) do
		if pos>=v.start and pos<v["end"] then 
			return v
		end
	end
	return nil
end
function GetRegionIn2(pos)
	ranges__=ranges__ or Process.getMemRanges()
	local cr=nil
	for k,v in pairs(ranges__) do
		--for k2,v2 in pairs(v) do
		--	print(string.format("%d %s->%s",k,tostring(k2),tostring(v2)))
		--end
		--local num
		--num=0
		--if(v["read"])then num=num+1 end
		--if(v["write"])then	num=num+10 end
		--if(v["execute"]) then num=num+100 end
		--print(string.format("%d %x->%x %s %x",k,v["start"],v["end"],v.name,pos))
		if pos>=v.start then --This is a hack to counter .text region suddenly shrinking.
			if cr~=nil then
				if v.start < cr.start then -- find region that start is closest
					cr=v
				end
			else
				cr=v
			end
		end
	end
	return cr
end
function ValidOffset(pos)
	ranges__=ranges__ or Process.getMemRanges()
	return GetRegionIn(pos)~=nil
end
function unlockDF()
	local reg=GetTextRegion()
	reg["write"]=true
	Process.setPermisions(reg,reg)
end
function lockDF()
	local reg=GetTextRegion()
	reg["write"]=false
	Process.setPermisions(reg,reg)
end
function SetExecute(pos)
	UpdateRanges()
	local reg=GetRegionIn(pos)
	reg.execute=true
	reg["write"]=true
	Process.setPermisions(reg,reg) -- TODO maybe make a page with only execute permisions or sth
end
-- engine bindings
engine=engine or {}
--[=[ use default peek/pokes for now
engine.peekd=Process.readDWord
engine.poked=Process.writeDWord
engine.peekb=Process.readByte
engine.pokeb=Process.writeByte
engine.peekw=Process.readWord
engine.pokew=Process.writeWord
engine.peekstr_stl=Process.readSTLString
engine.pokestr_stl=Process.writeSTLString
engine.peekstr=Process.readCString
--engine.pokestr=Process.readCString
engine.peekarb=Process.read
engine.pokearb=Process.write
--]=]

function engine.peek(offset,rtype)
	if type(rtype)=="table" then
		if rtype.off ==nil then
			return engine.peekpattern(offset,rtype)
		else
			return engine.peek(rtype.off+offset,rtype.rtype)
		end
	end
	if rtype==STD_STRING then
		return engine.peekstr2(offset)
	elseif rtype==DWORD then
		return engine.peekd(offset)
	elseif rtype==WORD then
		return engine.peekw(offset)
	elseif rtype==BYTE then
		return engine.peekb(offset)
	elseif rtype==QWORD then
		return engine.peekq(offset)
	elseif rtype==FLOAT then
		return engine.peekfloat(offset)
	elseif rtype==DOUBLE then
		return engine.peekdouble(offset)
	else
		error("Invalid peek type")
		return 
	end
end
function engine.poke(offset,rtype,val)
	if type(rtype)=="table" then
		if rtype.off ==nil then
			return engine.pokepattern(offset,rtype,val)
		else
			return engine.poke(rtype.off+offset,rtype.rtype,val)
		end
	end
	if rtype==STD_STRING then
		return engine.pokestr2(offset,val)
	elseif rtype==DWORD then
		return engine.poked(offset,val)
	elseif rtype==WORD then
		return engine.pokew(offset,val)
	elseif rtype==BYTE then
		return engine.pokeb(offset,val)
	elseif rtype==QWORD then
		return engine.pokeq(offset,val)
	elseif rtype==FLOAT then
		return engine.pokefloat(offset,val)
	elseif rtype==DOUBLE then
		return engine.pokedouble(offset,val)
	else
		error("Invalid poke type:"..tostring(rtype))
		return 
	end
end
function engine.sizeof(rtype)
	if rtype==STD_STRING then
		error("String has no constant size")
		return
	elseif rtype==DWORD then
		return 4;
	elseif rtype==WORD then
		return 2;
	elseif rtype==BYTE then
		return 1;
	else
		error("Invalid sizeof type")
		return 
	end
end
function engine.peekpattern(offset,pattern)
	local ret={}
	for k,v in pairs(pattern) do
		--print("k:"..k.." v:"..type(v))
		if type(v)=="table" then
			ret[k]=engine.peek(offset+v.off,v.rtype)
			--print(k.." peeked:"..offset+v.off)
		else 
			ret[k]=v
		end
	end
	ret.__offset=offset
	return ret
end
function engine.pokepattern(offset,pattern,val)
	for k,v in pairs(pattern) do
		--print("k:"..k.." v:"..type(v))
		if type(v)=="table" then
			engine.poke(offset+v.off,v.rtype,val[k]) 
		end
	end
end

function engine.LoadModData(file)
	local T2={}
	T2.symbols={}
	T2.data,T2.size=engine.loadobj(file)
	data,modsize=engine.loadobj(file)
	local T=engine.loadobjsymbols(file)
	for k,v in pairs(T) do

		if v.pos~=0 then
			T2.symbols[v.name]=v.pos
		end
	end
	return T2
end
function engine.FindMarkerCall(moddata,name)
	if moddata.symbols[name] ~=nil then
		return moddata.symbols[name]+1
	end
end
function engine.FindMarker(moddata,name)
	if moddata.symbols[name] ~=nil then
		return engine.findmarker(0xDEADBEEF,moddata.data,moddata.size,moddata.symbols[name])
	end
end
function engine.installMod(file,name,bonussize)
	local T=engine.LoadModData(file)
	local modpos,modsize=engine.loadmod(file,name,bonussize)
	T.pos=modpos
	return T
end

it_menu={}
it_menu.__index=it_menu
function it_menu:add(name,func)
	table.insert(self.items,{func,name})
end
function it_menu:display()
	print("Select choice (q exits):")
	for p,c in pairs(self.items) do
		print(string.format("%3d).%s",p,c[2]))
	end
	local ans
	repeat
		local r
		r=getline("")
		if r==nil then return end
		if r=='q' then return end
		ans=tonumber(r)
		
		if ans==nil or not(ans<=#self.items and ans>0) then
			print("incorrect choice")
		end
		
	until ans~=nil and (ans<=#self.items and ans>0)
	self.items[ans][1]()
end
function MakeMenu()
	local ret={}
	ret.items={}
	setmetatable(ret,it_menu)
	return ret
end

function PrintPattern(loadedpattern)
	for k,v in pairs(loadedpattern) do
		if type(v)== "string" then
			print(k.." "..v)
		else
			print(string.format("%s %d inhex:%x",k,v,v))
		end
	end
end

function printPattern(pattern)
	local i=0;
	local names={}
	names[STD_STRING]="std_string (STD_STRING)"
	names[DWORD]=     "Double word     (DWORD)"
	names[WORD]=	  "Word             (WORD)"
	names[STD_STRING]="Byte             (BYTE)"
	ret={}
	for k,v in pairs(pattern) do
		if type(v)=="table" and v.off~=nil then
		
			if names[v.rtype]~=nil then
				lname=names[v.rtype]
			else
				if type(v.rtype)=="table" then
					lname="Table (prob subpattern)"
				else
					lname="Other"
				end
			end
			print(string.format("%d. %s is %s with offset %x",i,k,lname,v.off))
			table.insert(ret,k)
		else
			print(string.format("%d. %s",i,k))
		end
		i=i+1
	end
	return ret;
end
function editPattern(offset,pattern,name)
	if type(pattern[name].rtype)=="table" then
		if pattern[name].rtype.setval~=nil then
			print(string.format("%x",offset+pattern[name].off))
			local t=engine.peek(offset+pattern[name].off,pattern[name].rtype)
			print("Value is now:"..t:getval())
			print("Enter new value:")
			val=io.stdin:read()
			t:setval(val)
		else
			ModPattern(offset+pattern[name].off,pattern[name].rtype)
		end
		return
	end
	val=engine.peek(offset,pattern[name])
	print("Value is now:"..val)
	print("Enter new value:")
	if pattern[name].rtype==STD_STRING then
		val=io.stdin:read()
	else 
		val=tonumber(io.stdin:read())
	end
	engine.poke(offset,pattern[name],val)
end
function ModPattern(itemoffset,pattern)
	print("Select what to edit:")
	nm=printPattern(pattern)
	q=tonumber(io.stdin:read())
	if q~=nil and q<#nm then
		editPattern(itemoffset,pattern,nm[q+1])
	end
end

function findVectors()
	if __VECTORS ~=nil then --chache
		return __VECTORS
	end
	local text=GetTextRegion()
	local h=hexsearch(text.start,text["end"],0x8b,ANYBYTE,ANYDWORD,0x8b,ANYBYTE,ANYDWORD,0x2b)
	local pos=h:findall()
	local T={}
	for k,v in pairs(pos) do
		local loc1,loc2
		loc1=engine.peekd(v+2)
		loc2=engine.peekd(v+8)
		--print(string.format("%x - %x=%x",loc1,loc2,loc1-loc2))
		if(loc1-loc2==4) then
			if T[loc1-4]~=nil then
				T[loc1-4]=T[loc1-4]+1
			else
				T[loc1-4]=1
			end
		end
	end
	__VECTORS=T
	return T
end

function GetRaceToken(p) --actually gets token...
	local vec=df.global.world.raws.creatures.all
	return vec[p].creature_id
end
function BuildNameTable()
	local rtbl={}
	local vec=df.global.world.raws.creatures.all
	--print(string.format("Vector start:%x",vec.st))
	--print(string.format("Vector end:%x",vec.en))
	--print("Creature count:"..vec.size)
	for k=0,#vec-1 do
		local name=vec[k].creature_id
		--print(k.." "..tostring(name))
		rtbl[name]=k		
	end
	return rtbl;
end
function BuildMaterialTable()
	local rtbl={}
	local vec=engine.peek(offsets.getEx('Materials'),ptr_vector)
	--print(string.format("Vector start:%x",vec.st))
	--print(string.format("Vector end:%x",vec.en))
	--local i=0
	for p=0,vec:size()-1 do
		local off=vec:getval(p)
		--print("First member:"..off)
		local name=engine.peek(off,ptt_dfstring)
		--print("Loading:"..p.."="..name:getval())
		rtbl[name:getval()]=p
		--i=i+1
		--if i>100 then
		--	io.stdin:read()
		--	i=0
		--end
	end
	return rtbl;
end
function BuildWordTables()
	local names={}
	local rnames={}
	local vector=engine.peek(offsets.getEx('WordVec'),ptr_vector)
for i =0,vector:size()-1 do
	local off=vector:getval(i)
	local n=engine.peekstr(off)
	names[i]=n
	rnames[n]=i
end
	return names,rnames
end
function ParseScript(file)
	
	io.input(file)
	f=""
	first=0
	nobraces=0
	function updFunction()
	if f~="" then
			first=0
			if nobraces==0 then
				f=f.."}"
			end
			nobraces=0
			print("Doing:"..f)
			assert(loadstring(f))()

			f=""
		end
	end
	while true do

      local line = io.read("*line")
      if line == nil then break end
	  if string.sub(line,1,2)==">>" then
		updFunction()
		if string.find(line,"%b()") then
		f=string.sub(line,3)
		nobraces=1
		else
		f=string.sub(line,3).."{"
		end
		--print(string.sub(line,3)..)
	  else
		if first~=0 then
			f=f..","
		else
			first=1
		end
		f=f..string.format('%q',line)
		
	  end
	
    end
	updFunction()
end
function ParseNames(path)
	local ret={}
	local ff=io.open(path)
	for n in ff:lines() do
		table.insert(ret,n)
	end
	return ret
end
function getSelectedUnit()
	if df.global.ui.main.mode~=23 then
		return nil
	end
	local unit_indx=df.global.ui_selected_unit
	if unit_indx<#df.global.world.units.active-1 then
		return df.global.world.units.active[unit_indx]
	else
		return nil
	end
end
function getxyz() -- this will return pointers x,y and z coordinates.
	local x=df.global.cursor.x
	local y=df.global.cursor.y
	local z=df.global.cursor.z	
	return x,y,z -- return the coords
end
function getCreatureAtPos(x,y,z) -- gets the creature index @ x,y,z coord
	--local x,y,z=getxyz() --get 'X' coords
	local vector=df.global.world.units.all -- load all creatures
	for i = 0, #vector-1 do -- look into all creatures offsets
		local curpos=vector[i].pos --get its coordinates
		local cx=curpos.x 
		local cy=curpos.y
		local cz=curpos.z
		if cx==x and cy==y and cz==z then --compare them
			return vector[i] --return index
		end
	end
	--print("Creature not found!")
	return nil
	
end
function getCreatureAtPointer()
	return getCreatureAtPos(getxyz())
end
function getCreature()
	local unit=getSelectedUnit()
	if unit==nil then
		unit=getCreatureAtPointer()
	end
	--any other selection methods...
	return unit
end
function getNemesisId(unit)
	for k,v in pairs(unit.refs) do
		if df.general_ref_is_nemesisst:is_instance(v) then
			return v.nemesis_id
		end
	end
end
function getNemesis(unit)
	local id=getNemesisId(unit)
	if id then
		return df.nemesis_record.find(id)
	end
end
function Allocate(size)
	local ptr=engine.getmod('General_Space')
	if ptr==nil then
		ptr=engine.newmod("General_Space",4096) -- some time later maybe make some more space
		engine.poked(ptr,4)
	end
	
	local curptr=engine.peekd(ptr)
	curptr=curptr+size
	engine.poked(ptr,curptr)
	return curptr-size+ptr
end
dofile("dfusion/patterns.lua")
dofile("dfusion/patterns2.lua")
dofile("dfusion/itempatterns.lua")
dofile("dfusion/buildingpatterns.lua")