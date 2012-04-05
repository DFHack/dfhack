offsets=offsets or {} 
function offsets.find(startoffset,...)
	-- [=[
	if startoffset== 0 then
		local text=GetTextRegion()
		--print("searching in:"..text.name)
		startoffset=text.start
		endadr=text["end"]
	else
		local reg=GetRegionIn(startoffset)
		--print("searching in:"..reg.name)
		if reg==nil then 
			print(string.format("Warning: memory range for search @:%x not found!",startoffset))
			return 0
		end
		endadr=reg["end"]
	end
	--]=]
	--print(string.format("Searching (%x->%x)",startoffset,endadr))
	local h=hexsearch(startoffset,endadr,...)
	local pos=h:find()
	h=nil
	return pos
end
function offsets.findall(startoffset,...)
	local endadr;
	if startoffset== 0 then
		local text=GetTextRegion()
		--print("searching in:"..text.name)
		startoffset=text.start
		endadr=text["end"]
	else
		local reg=GetRegionIn(startoffset)
		--print("searching in:"..reg.name)
		endadr=reg["end"]
	end
	local h=hexsearch(startoffset,endadr,...)
	local pos=h:findall()
	h=nil
	return pos
end
function offsets.base()
	return Process.getBase()
end
function offsets.getvectors()
	return findVectors()
end
ADDRESS=ANYDWORD