adv_tools=adv_tools or {}
adv_tools.menu=adv_tools.menu or MakeMenu()
function adv_tools.ressurect()
	myoff=offsets.getEx("AdvCreatureVec")
	vector=engine.peek(myoff,ptr_vector) 
	indx=GetCreatureAtPos(getxyz())
	if indx<0 then indx=0 end
	--print(string.format("%x",vector:getval(indx)))
	v2=engine.peek(vector:getval(indx),ptr_Creature.hurt1)
	for i=0,v2:size()-1 do
		v2:setval(i,0)
	end
	v2=engine.peek(vector:getval(indx),ptr_Creature.hurt2)
	v2.type=DWORD
	for i=0,v2:size()-1 do
		v2:setval(i,0)
	end
	engine.poke(vector:getval(indx),ptr_Creature.bloodlvl,60000) --give blood
	engine.poke(vector:getval(indx),ptr_Creature.bleedlvl,0) --stop some bleeding...
	local flg=engine.peek(vector:getval(indx),ptr_Creature.flags)
	flg:set(1,false) --ALIVE
    flg:set(39,false) -- leave body yet again
    flg:set(37,false) -- something todo with wounds- lets you walk again.
    flg:set(58,true) -- makes them able to breathe
    flg:set(61,true) -- gives them sight
    engine.poke(vector:getval(indx),ptr_Creature.flags,flg)
end

function adv_tools.wagonmode() --by rumrusher
   --first three lines same as before (because we will need an offset of creature at location x,y,z)
   myoff=offsets.getEx("AdvCreatureVec")
   vector=engine.peek(myoff,ptr_vector)
   indx=GetCreatureAtPos(getxyz())
   --indx=0
   --print(string.format("%x",vector:getval(indx)))
   flg=engine.peek(vector:getval(indx),ptr_Creature.flags) --get flags
   flg:set(1,false)
   flg:set(74,false)
   engine.poke(vector:getval(indx),ptr_Creature.flags,flg)
   print("To stay normal press y, else hit Enter turn Wagon mode on.")
   r=io.stdin:read() -- repeat for it too work... also creature will be dead.
   if r== "y" then
      flg=engine.peek(vector:getval(indx),ptr_Creature.flags)
      flg:set(1,false)
      engine.poke(vector:getval(indx),ptr_Creature.flags,flg)
   else
      flg=engine.peek(vector:getval(indx),ptr_Creature.flags)
      flg:set(1,false)
      flg:flip(74)
      engine.poke(vector:getval(indx),ptr_Creature.flags,flg)
   end
end
function selectall()
  local retvec={} --return vector (or a list)
  myoff=offsets.getEx("AdvCreatureVec")
  vector=engine.peek(myoff,ptr_vector) --standart start
  for i=0,vector:size()-1 do --check all creatures
     local off
     off=vector:getval(i)
     local flags=engine.peek(off,ptr_Creature.flags)
     if flags:get(1)==true then  --if dead ...
        table.insert(retvec,off)--... add it to return vector
     end
  end
  return retvec --return the "return vector" :)
end
function adv_tools.hostilate()
	vector=engine.peek(offsets.getEx("AdvCreatureVec"),ptr_vector)
	id=GetCreatureAtPos(getxyz()) 
	print(string.format("Vec:%d cr:%d",vector:size(),id))
	off=vector:getval(id)
	crciv=engine.peek(vector:getval(id),ptr_Creature.civ)
	curciv=engine.peek(vector:getval(0),ptr_Creature.civ)
	
	if curciv==crciv then
		print("Friendly-making enemy")
		engine.poke(off,ptr_Creature.civ,-1)
		flg=engine.peek(off,ptr_Creature.flags)
		flg:set(17,true)
		engine.poke(off,ptr_Creature.flags,flg)
	else
		print("Enemy- making friendly")
		engine.poke(off,ptr_Creature.civ,curciv)
		flg=engine.peek(off,ptr_Creature.flags)
		flg:set(17,false)
		flg:set(19,false)
		engine.poke(off,ptr_Creature.flags,flg)
	end
end
