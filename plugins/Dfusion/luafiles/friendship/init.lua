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
function minEx(list)
	local imin=list[1]
	for _,v in ipairs(list) do
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

friendship_in={}
dofile("dfusion/friendship/install.lua")
dofile("dfusion/friendship/patch.lua")

function friendship(names)
	friendship_in.install(names)
	friendship_in.patch()
end

