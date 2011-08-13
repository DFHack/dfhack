--if(mypos~=0) then
--print("plugin already active")
--maybe set options for reinit?
--return
--end

function analyzeF(off)
	pos=offsets.find(off,0x39,ANYBYTE,0x8c,00,00,00)
	print(string.format("Compare at:%x",pos))
	if pos ==0 then
		return 0
	end
	if(pos-off>0x100) then
		print(string.format("Distance to cmp:%x",pos-off))
		pos =offsets.find(off,CALL)
		print(string.format("Distance to call:%x",pos-off))
		return 0
		--return analyzeF(pos)
	else
		return pos
	end
end
function minEx(...)
	local imin=arg[1]
	for _,v in ipairs(arg) do
		if imin> v and v~=0 then
			imin=v
		end
	end
	return imin
end
function signDword(dw)
	if(dw>0xFFFFFFFF) then
		return dw-0xFFFFFFFF
	end
	return dw
end
--[[
	Warning: not all mov's are acounted for. Found one: mov EAX,WORD PTR[EBP+1EF4] WTF??
	Two more compares are missing. There are calls instead (same function) 
]]--
if not(FILE) then
--print("race num:"..engine.peekw(offsets.getEx("CurrentRace")))
--print(string.format("%x vs %x",offsets.getEx("CurrentRace"),VersionInfo.getGroup("Creatures"):getAddress("current_race")))
add_race=VersionInfo.getGroup("Creatures"):getAddress("current_race")
print("Race num:"..engine.peekw(add_race))
print("Your current race is:"..GetRaceToken(engine.peekw(add_race)))
print("If this is wrong please type 'q'")
if(io.stdin:read()=='q') then
	return
end
end
friendship_in={}
dofile("dfusion/friendship/install.lua")
dofile("dfusion/friendship/patch.lua")
if not(FILE) then
	names=ParseNames("dfusion/friendship/races.txt")--io.open("plugins/friendship/races.txt"):lines()
	friendship_in.install(names)
	friendship_in.patch()
end
function friendship(names)
	friendship_in.install(names)
	friendship_in.patch()
end

