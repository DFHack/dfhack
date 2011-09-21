offsets=offsets or {} 
offsets._toff={}
offsets._foff={}
offsets.get = function (name)
	if offsets._toff[name] == nil then
		offsets.searchoffset(name,true)
	end
	return offsets._toff[name]
end
offsets.getEx = function (name) 
	--return offsets._toff[name]+Process.getBase()
	return offsets.get(name)+Process.getBase()
end
offsets.load = function ()
	local f=io.open("dfusion/offsets.txt")
	local line=f:read()
	while line~=nil do
		--print(line)
		local sppliter=string.find(line,":")
		offsets._toff[string.sub(line,1,sppliter-2)]=tonumber(string.sub(line,sppliter+2))
		line=f:read()
	end
end
offsets.save = function ()
	local f=io.open("dfusion/offsets.txt","w")
	for k,v in pairs(offsets._toff) do 
		
		f:write(string.format("%s : 0x%x\n",k,v))
	end
	f:close()
end
function offsets.new(name, func)
	if type(func)=="function" then
		table.insert(offsets._foff,{name,func,false})
	else
		offsets._toff[name]=func
	end
	--offsets._foff[name]={func,false}
end
function offsets.newlazy(name, func)
	table.insert(offsets._foff,{name,func,true})
	--offsets._foff[name]={func,true}
end
function offsets.searchoffset(num,forcelazy)
	v=offsets._foff[num]
	print("Finding offset:"..v[1])
	if (v[3] and focelazy) or  not v[3] then
		local pos=v[2]()
		if pos== 0 then
			error("Offset not found for:"..v[1])
		else
			offsets._toff[v[1]]=pos
		end
	end
end
function offsets.searchoffsets(forcelazy)
	for k,v in pairs(offsets._foff) do
		offsets.searchoffset(k,forcelazy)
	end
end
function offsets.find(startoffset,...)
	local endadr=GetTextRegion()["end"];
	--[=[if startoffset== 0 then
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
	end--]=]
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
offsets.load()
ADDRESS=ANYDWORD
dofile("dfusion/offsets.lua")