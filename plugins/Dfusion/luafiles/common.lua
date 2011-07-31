offsets=offsets or {} 
offsets._toff={}
offsets.get = function (name) 
	return offsets._toff[name]
end
offsets.getEx = function (name) 
	return offsets._toff[name]+Process.getBase()
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
offsets.load()
function unlockDF()
	local ranges=Process.getMemRanges()
	for k,v in pairs(ranges) do
		--for k2,v2 in pairs(v) do
		--	print(string.format("%d %s->%s",k,tostring(k2),tostring(v2)))
		--end
		--local num
		--num=0
		--if(v["read"])then num=num+1 end
		--if(v["write"])then	num=num+10 end
		--if(v["execute"]) then num=num+100 end
		--print(string.format("%d %x->%x %s %d",k,v["start"],v["end"],v.name,num))
		local pos=string.find(v.name,".text")
		if pos~=nil then
			v["write"]=true
			Process.setPermisions(v,v)
		end
	end
end
function lockDF()
	local ranges=Process.getMemRanges()
	for k,v in pairs(ranges) do
		--for k2,v2 in pairs(v) do
		--	print(string.format("%d %s->%s",k,tostring(k2),tostring(v2)))
		--end
		--local num
		--num=0
		--if(v["read"])then num=num+1 end
		--if(v["write"])then	num=num+10 end
		--if(v["execute"]) then num=num+100 end
		--print(string.format("%d %x->%x %s %d",k,v["start"],v["end"],v.name,num))
		local pos=string.find(v.name,".text")
		if pos~=nil then
			v["write"]=false
			Process.setPermisions(v,v)
		end
	end
end
-- engine bindings
engine=engine or {}
engine.peekd=Process.readDWord
engine.poked=Process.writeDWord