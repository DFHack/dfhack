function getTypename(object)
	local tbl
	local ret={}
	if getmetatable(object)~=nil then
		local tbl=getmetatable(object)
		for k,v in pairs(xtypes) do
			if v==tbl then
				return k
			end
		end
		for k,v in pairs(xtypes.containers) do
			if v==tbl then
				return k
			end
		end
	end
	if object.name~= nil then
		return object.name
	end
	return "?"
end
function getFields(object)
	local tbl
	local ret={}
	if getmetatable(object)==xtypes["struct-type"].wrap then
		tbl=rawget(object,"mtype")
	elseif getmetatable(object)==xtypes["struct-type"] then
		tbl=object
	else
		error("Not an class_type or a class_object")
	end
	for k,v in pairs(tbl.types) do
		table.insert(ret,{k,v[2],getTypename(v[1])})
		--ret[v[2]]=k
		--print(string.format("%s %x",k,v[2]))
	end
	table.sort(ret,function (a,b) return a[2]>b[2] end)
	return ret
end
function editField(tbl,field,typename)
	if EditType[typename] ~= nil then
		EditType[typename](tbl[field])
	else
		print("Cur value:"..tostring(tbl[field]))
		val=getline("Enter newvalue:")
		tbl[field]=val
	end
end
EditType={}
EditType["df-flagarray"]=function(trg)
	local fields=rawget(trg,"mtype").index.names
	print("Flag count:"..trg.size)
	print("Name count:"..#fields)
	for i=0,#fields do
		print(string.format("%3d %20s %s",i,fields[i],tostring(trg[i-1])))
	end
	number=getline("enter flag id to flip:")
	number=tonumber(number)
	if number then
		trg[fields[number]]= not trg[fields[number]]
	end
end
EditType["enum-type"]=function(trg)
	local fields=rawget(trg,"mtype").names
	local typename=getTypename(rawget(trg,"mtype").etype)
	for k,v in pairs(fields) do
		print(string.format("%3d %s",k,v))
	end
	local cval=trg:get()
	if fields[cval]~= nil then
		print(string.format("Current value:%d (%s)",cval,fields[cval]))
	else
		print(string.format("Current value:%d",cval))
	end
	number=getline("enter new value:")
	number=tonumber(number)
	if number then
		trg:set(number)
	end
end
EditType["static-array"]=function(trg)
	local item_type=rawget(trg,"mtype").item_type
	local typename=getTypename(item_type)
	number=getline(string.format("Select item (max %d, item-type '%s'):",trg.size,typename))
	number=tonumber(number)
	if number then
		EditType[typename](trg[number])
	end
end
EditType["stl-vector"]=EditType["static-array"]
EditType["df-array"]=EditType["static-array"]
EditType["struct-type"]=function(trg)
	local mtype=rawget(trg,"mtype")
	local fields=getFields(trg)
	for k,v in pairs(fields) do
		print(string.format("%4d %25s %s",k,v[1],v[3]))
	end
	number=getline("Choose field to edit:")
	number=tonumber(number)
	if number then
		local v=fields[number]
		editField(trg,v[1],v[3])
	end
end
EditType["pointer"]=function(trg)
	local mtype=rawget(trg,"mtype").ptype
	local typename=getTypename(mtype)
	if(trg:tonumber()==0) then
			print("pointer points to nowhere!")
			return
	end
	print("Auto dereferencing pointer! type:"..typename)
	if EditType[typename] ~= nil then
		EditType[typename](trg:deref())
	else
		print("Cur value:"..tostring(trg:deref()))
		val=getline("Enter newvalue:")
		trg:setref(val)
	end
end

function EditDF()
	local i=1
	local tbl={}
	for k,v in pairs(rawget(df,"types")) do		
		print(string.format("%4d %25s %s",i,k,getTypename(v)))
		tbl[i]={k,getTypename(v)}
		i=i+1
	end
	number=dfhack.lineedit("select item to edit (q to quit):")
	if number and tonumber(number) then
		local entry=tbl[tonumber(number)]
		if entry==nil then
			return
		end
		editField(df,entry[1],entry[2])
		--[=[
		if EditType[entry[2]] ~= nil then
			EditType[entry[2]](df[entry[1]])
		else
			print("Cur value:"..tostring(df[entry[1]]))
			val=getline("Enter newvalue:")
			df[entry[1]]=val
		end
		--]=]
	end
end
function EditObject(obj)
	EditType[getTypename(obj)](obj)
end