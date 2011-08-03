items={} --> first lets make a menu table
items.menu=MakeMenu()


function items.dest() 
	myoff=offsets.getEx("Items") -- first find out where "item vector" is
	vector=engine.peek(myoff,ptr_vector) --  get list of items
	for i=0,vector:size()-1 do --look at each item
		flg=engine.peek(vector:getval(i),ptr_item.flags)
		flg:set(17,1)
		engine.poke(vector:getval(i),ptr_item.flags,flg)
	end
end
function items.eggs() 
	myoff=offsets.getEx("Items") -- first find out where "item vector" is
	vector=engine.peek(myoff,ptr_vector) --  get list of items
	for i=0,vector:size()-1 do --look at each item
		rti=engine.peek(vector:getval(i),ptr_item.RTI)
		if ptr_item.getname(nil,rti)=="item_eggst" then
			egg=engine.peek(vector:getval(i),ptr_subitems["item_eggst"])
			egg.isfertile=1
			egg.hatchtime=0xffffff
			--egg.race=123 -- change race for fun times
			engine.poke(vector:getval(i),ptr_subitems["item_eggst"],egg)
		end
	end
end
function editFlags(offset)
	while true do
	flags=engine.peek(offset,ptr_item.flags)
	for i=0,8*8-1 do
		if flags:get(i) then
			print(i.." is true")
		else
			print(i.." is false")
		end
	end
	print(" enter number to switch flag or not a number to quit:")
	q=tonumber(io.stdin:read())
	if q==nil then return end
	flags:flip(q)
	engine.poke(offset,ptr_item.flags,flags)
	end
end
function editCovering(offset)
	off=engine.peek(offset,ptr_item.ptr_covering)
	if off == 0 then
		print("No coverings found.")
	end
	vec=engine.peek(off,ptr_vector)
	print("Covering list:")
	for i=0,vec:size()-1 do
		cov=engine.peek(vec:getval(i),ptr_cover)
		print(string.format("%d. mat=%d submat=%d state=%d",i,cov.mat,cov.submat,cov.state))
	end
	print("To edit type number:")
	q=tonumber(io.stdin:read())
	if q==nil then return end
	if q>=vec:size() or q<0 then return end
	off=vec:getval(q)
	cov=engine.peek(off,ptr_cover)
	print("Enter mat:")
	q=tonumber(io.stdin:read())
	if q==nil then q=0xffff end
	print("Enter submat:")
	v=tonumber(io.stdin:read())
	if v==nil then v=0xffff end
	print("Enter state:")
	y=tonumber(io.stdin:read())
	if y==nil then y=0 end
	cov.mat=q
	cov.submat=v
	cov.state=y
	engine.poke(off,ptr_cover,cov)
end
function editMaterial(offset)
	print("Mat id 0 to 18 is in normal materials (inorganic, amber etc...) after that creature mat (with submat2 being race)")
	print("from 219 with submat2=0xffffffff (type not a number) it reads legends id, from 419  submat2 means plant id")
	print("Probably submat is not used :? ")
	mat=engine.peek(offset,ptr_item.mat)
	submat=engine.peek(offset,ptr_item.submat)
	submat2=engine.peek(offset,ptr_item.submat2)
	lid=engine.peek(offset,ptr_item.legendid)
	print(string.format("Now is mat=%d, submat=%d submat2=%d legend id=%d",mat,submat,submat2,lid))
	print("Enter mat:")
	q=tonumber(io.stdin:read())
	if q==nil then return end
	print("Enter submat:")
	v=tonumber(io.stdin:read())
	if v==nil then v=0xffff end
	print("Enter submat2:")
	z=tonumber(io.stdin:read())
	if z==nil then z=0xffffffff end
	print("Enter legendid:")
	y=tonumber(io.stdin:read())
	if y==nil then y=0xffffffff end
	engine.poke(offset,ptr_item.mat,q)
	engine.poke(offset,ptr_item.submat,v)
	engine.poke(offset,ptr_item.legendid,y)
	engine.poke(offset,ptr_item.submat2,z)
	print("Done")
end
function items.select()
	myoff=offsets.getEx("Items") 
	vector=engine.peek(myoff,ptr_vector) 
	tx,ty,tz=getxyz()
	T={}
	for i=0,vector:size()-1 do --this finds all item offsets that are on pointer
		itoff=vector:getval(i)
		x=engine.peek(itoff,ptr_item.x)
		y=engine.peek(itoff,ptr_item.y)
		z=engine.peek(itoff,ptr_item.z)
		if x==tx and y==ty and z==tz then
				table.insert(T,itoff)
		end
	end
	print("Items under cursor:")
	i=1
	for _,v in pairs(T) do
		RTI=engine.peek(v,ptr_item.RTI)
		print(i..". "..ptr_item.getname(nil,RTI))
		i=i+1
	end
	print("Type number to edit or 'q' to exit")
	while true do
		q=io.stdin:read()
		if q=='q' then return end
		if tonumber(q) ~=nil and tonumber(q)<i then break end
	end
	return T[tonumber(q)]
end
function items.select_creature(croff)
	vector=engine.peek(croff,ptr_Creature.itemlist2)
	print("Vector size:"..vector:size())
	T={}
	for i=0,vector:size()-1 do 
		table.insert(T,vector:getval(i)) -- item list is itemptr+location?
	end
	print("Items in inventory:")
	i=1
	for _,v in pairs(T) do
		RTI=engine.peek(engine.peekd(v),ptr_item.RTI)
		print(i..". "..ptr_item.getname(nil,RTI).." locations:"..engine.peekw(v+4).." "..engine.peekw(v+6))
		i=i+1
	end
	print("Type number to edit or 'q' to exit")
	while true do
		q=io.stdin:read()
		if q=='q' then return end
		if tonumber(q) ~=nil and tonumber(q)<i then break end
	end
	return engine.peekd(T[tonumber(q)])
end
function items.edit(itoff)
	if itoff==nil then
		itoff=items.select()
	end
	print(string.format("Item offset:%x",itoff))
	print("Type what to edit:")
	print("1. material")
	print("2. flags")
	print("3. covering")
	Z={}
	Z[1]=editMaterial
	Z[2]=editFlags
	Z[3]=editCovering
	name=ptr_item.getname(nil,engine.peek(itoff,ptr_item.RTI))
	if name~=nil and ptr_subitems[name]~=nil then
		print("4. Item specific edit")
		--Z[4]=items.fedit[name]
		Z[4]="dummy"
	end
	while true do
		q=io.stdin:read()
		if q=='q' then return end
		if tonumber(q) ~=nil and tonumber(q)<#Z+1 then break end
	end
	
	if Z[tonumber(q)]=="dummy" then
		ModPattern(itoff,ptr_subitems[name])
	else
		Z[tonumber(q)](itoff)
	end
end
function items.printref()
	itoff=items.select()
	vec=engine.peek(itoff,ptr_item.ref)
	for i=0, vec:size()-1 do
		toff=vec:getval(i)
		print(RTTI_GetName(engine.peekd(toff)))
	end
	print("Decorations:")
	vec=engine.peek(itoff,ptr_item.decorations)
	for i=0, vec:size()-1 do
		toff=vec:getval(i)
		print(ptr_decoration_gen.getname(nil,engine.peek(toff,ptr_decoration_gen.RTI)))
	end
end
function items.edit_adv()
	vec=engine.peek(offsets.getEx("CreatureVec"),ptr_vector)
	items.edit(items.select_creature(vec:getval(0)))
end
if not(FILE) then -- if not in script mode
	
	items.menu:add("Destroy all",items.dest)
	items.menu:add("Hatch eggs",items.eggs)
	items.menu:add("Edit item",items.edit)
	items.menu:add("Print ref",items.printref)
	items.menu:add("Edit adventurer's items",items.edit_adv)
	items.menu:display()
	
end