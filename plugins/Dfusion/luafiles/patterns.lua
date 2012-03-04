ptt_dfstring={}
if WINDOWS then
	ptt_dfstring.ptr={off=0,rtype=DWORD}
	ptt_dfstring.size={off=16,rtype=DWORD}
	ptt_dfstring.alloc={off=20,rtype=DWORD}

	function ptt_dfstring:getval()
		--print(string.format("GETTING FROM:%x",self.__offset))
		if self.size<16 then
			--print(string.format("GETTING FROM:%x",self.__offset))
			return string.sub(engine.peekstr(self.__offset),1,self.size)
		else
			--print(string.format("GETTING FROM:%x",self.ptr))
			return string.sub(engine.peekstr(self.ptr),1,self.size)
		end
	end
	function ptt_dfstring:setval(newstring)
		local offset=self.__offset
		local strl=string.len(newstring)
		if strl<16 then
			--print(string.format("GETTING FROM:%x",self.__offset))
		
			engine.poked(offset+ptt_dfstring.size.off,strl)
			engine.poked(offset+ptt_dfstring.alloc.off,15)
			engine.pokestr(offset,newstring)
		
		else
			local loc
			if engine.peekd(offset+ptt_dfstring.alloc.off) > strl then
				loc=engine.peekd(offset)
				print("Will fit:"..loc.." len:"..strl)
			else
				loc=Allocate(strl+1)
				engine.poked(offset+ptt_dfstring.alloc.off,strl)
				print("Will not fit:"..loc.." len:"..strl)
			end
			--print(string.format("GETTING FROM:%x",self.ptr))
			engine.poked(self.__offset+ptt_dfstring.size.off,strl)
			engine.pokestr(loc,newstring)
			engine.poked(self.__offset,loc)
		end
	end
else
	--ptt_dfstring.ptr={off=0,rtype=DWORD}
	function ptt_dfstring:getval()
		return engine.peekstr_stl(self.__offset)
	end
end
--if(COMPATMODE) then
--ptr_vector={}
--ptr_vector.st={off=4,rtype=DWORD}
--ptr_vector.en={off=8,rtype=DWORD}
--else
ptr_vector={}
ptr_vector.st={off=0,rtype=DWORD}
ptr_vector.en={off=4,rtype=DWORD}
ptr_vector.alloc={off=8,rtype=DWORD}
--end
function ptr_vector:clone(settype)
	local ret={}
	for k,v in pairs(self) do
		ret[k]=v
	end
	ret.type=settype
	return ret
end
function ptr_vector:size()
	return (self.en-self.st)/engine.sizeof(self.type)
end
ptr_vector.type=DWORD
function ptr_vector:getval(num)
	if self.st==0 then return error("Vector empty.") end
	--print("Wants:"..num.." size:"..self:size())
	if num>=self:size() then error("Index out of bounds in vector.") end
	return engine.peek(self.st+engine.sizeof(self.type)*num,self.type)
end
function ptr_vector:setval(num,val)
	return engine.poke(self.st+engine.sizeof(self.type)*num,self.type,val)
end
function ptr_vector:append(val)
	if self.alloc - self.en > 0 then
		local num=self:size()
		self.en=self.en+engine.sizeof(self.type)
		self:setval(val,num)
	else
		error("larger than allocated arrays not implemented yet")
		local num=self:size()
		local ptr=Allocate(num*2)
		
	end
end
ptt_dfflag={}
function ptt_dfflag.flip(self,num) --flip one bit in flags
	local of=math.floor (num/8);
	
	self[of]=bit.bxor(self[of],bit.lshift(1,num%8))
end

function ptt_dfflag.get(self,num) -- get one bit in flags
	local of=math.floor (num/8);
	
	if bit.band(self[of],bit.lshift(1,num%8))~=0 then
		return true
	else
		return false
	end
end
function ptt_dfflag.set(self,num,val) --set to on or off one bit in flags
	if (self:get(num)~=val) then
		self:flip(num)
	end
end
function ptt_dfflag.new(size) -- create new flag pattern of size(in bytes)
	local ret={}
	for i=0,size-1 do
		ret[i]={off=i,rtype=BYTE};
	end
	ret.flip=ptt_dfflag.flip --change to metatable stuff...
	ret.get=ptt_dfflag.get
	ret.set=ptt_dfflag.set
	return ret;
end
--[[
	Creature:
	0 name (df_string) 28
	28 nick (df_string) 56
	56 surname- namearray(7*dword(4)) 84 ...
	140 race (dword) 144
	224 flags
	264 civ (dword)
	252 ID
	592 following ID
	904 bleed vector? hurt vector or sth...
	0x790 legends id?
	2128 known names? or knowledge?
	flags:
	0    Can the dwarf move or are they waiting for their movement timer
    1   Dead (might also be set for incoming/leaving critters that are alive)
    2   Currently in mood
    3   Had a mood
    4   "marauder" -- wide class of invader/inside creature attackers
    5   Drowning
    6   Active merchant
    7   "forest" (used for units no longer linked to merchant/diplomacy, they just try to leave mostly)
    8   Left (left the map)
    9   Rider
    10  Incoming
    11  Diplomat
    12  Zombie
    13  Skeleton
    14  Can swap tiles during movement (prevents multiple swaps)
    15  On the ground (can be conscious)
    16  Projectile
    17  Active invader (for organized ones)
    18  Hidden in ambush
    19  Invader origin (could be inactive and fleeing)
    20  Will flee if invasion turns around
    21  Active marauder/invader moving inward
    22  Marauder resident/invader moving in all the way
    23  Check against flows next time you get a chance
    24  Ridden
    25  Caged
    26  Tame
    27  Chained
    28  Royal guard
    29  Fortress guard
    30  Suppress wield for beatings/etc
    31  Is an important historical figure 
	32 swiming
	
]]--
ptr_Creature={}
local posoff=0 --VersionInfo.getGroup("Creatures"):getGroup("creature"):getOffset("position")
ptr_Creature.x={off=posoff,rtype=WORD} --ok
ptr_Creature.y={off=posoff+2,rtype=WORD} --ok
ptr_Creature.z={off=posoff+4,rtype=WORD} --ok
ptr_Creature.flags={off=0,rtype=ptt_dfflag.new(10)}
ptr_Creature.name={off=0,rtype=ptt_dfstring}
ptr_Creature.ID={off=252,rtype=DWORD} --ok i guess
ptr_Creature.followID={off=592,rtype=DWORD} --ok
ptr_Creature.race={off=140,rtype=DWORD} --ok
ptr_Creature.civ={off=264,rtype=DWORD}
ptr_Creature.legends={off=344,rtype=ptr_vector} --ok
ptr_Creature.hurt1={off=0x308,rtype=ptr_vector:clone(BYTE)} --byte vector...
ptr_Creature.hurt2={off=0x338,rtype=ptr_vector}
ptr_Creature.wounds={off=0x388,rtype=ptr_vector}
ptr_Creature.itemlist1={off=0x1D0,rtype=ptr_vector}
ptr_Creature.itemlist2={off=0x288,rtype=ptr_vector}
ptr_Creature.bloodlvl={off=0x490,rtype=DWORD}
ptr_Creature.bleedlvl={off=0x494,rtype=DWORD}

ptr_CrGloss={}
ptr_CrGloss.token={off=0,rtype=ptt_dfstring}
ptr_CrGloss.castes={off=296,rtype=ptr_vector}

ptr_CrCaste={}
ptr_CrCaste.name={off=0,rtype=ptt_dfstring}
ptr_CrCaste.flags_ptr={off=0x5A0,rtype=DWORD} --size 17?
--[=[
	Flags:
	57 - is sentient (allows setting labours)
--]=]
ptr_LEntry={} -- all size 256
ptr_LEntry.name={off=36,rtype=ptt_dfstring}
ptr_LEntry.id1={off=160,rtype=DWORD}
ptr_LEntry.id2={off=164,rtype=DWORD}
ptr_LEntry.somlist={off=220,rtype=DWORD}

ptr_dfname={}
for i=0,6 do
ptr_dfname[i]={off=i*4,rtype=DWORD}
end
--[[
	Site docs:
	0x38 name struct todo...
	0x78 type:
		0 - mountain halls (yours)
		1 - dark fort
		2 - cave
		3 - mountain hall (other)
		4 - forest
		5 - hamlet
		6 - imp location
		7 - lair
		8 - fort
		9 - camp
	0x7a some sort of id?
	0x84 some vec (ids)
	0x94 some other vec (ids)
	0xa4 some vec (prts)
	0x118 ptr to sth
	
	0x14c ptr to mapdata
]]--


ptr_site={}
ptr_site.type={off=0x78,rtype=WORD}
ptr_site.id={off=0x7a,rtype=DWORD}
ptr_site.name={off=0x38,rtype=ptr_dfname}
ptr_site.flagptr={off=0x118,rtype=DWORD}

ptr_legends2={}
ptr_legends2.id={off=0,rtype=DWORD}
ptr_legends2.follow={off=0x18,rtype=DWORD}

ptr_material={}
ptr_material.token={off=0,rtype=ptt_dfstring}
