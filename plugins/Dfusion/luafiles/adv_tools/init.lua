adv_tools= {}
adv_tools.menu=MakeMenu()
--TODO make every tool generic (work for both modes)
function adv_tools.reincarnate(swap_soul) --only for adventurer i guess
	if swap_soul==nil then
		swap_soul=true
	end
	local adv=df.global.world.units.active[0]
	if adv.flags1.dead==false then
		error("You are not dead (yet)!")
	end
	local hist_fig=getNemesis(adv).figure
	if hist_fig==nil then
		error("No historical figure for adventurer...")
	end
	local events=df.global.world.history.events
	local trg_hist_fig
	for i=#events-1,0,-1 do -- reverse search because almost always it will be last entry
		if df.history_event_hist_figure_diedst:is_instance(events[i]) then
			--print("is instance:"..i)
			if events[i].hfid==hist_fig.id then
				--print("Is same id:"..i)
				trg_hist_fig=events[i].slayer
				if trg_hist_fig then
					trg_hist_fig=df.historical_figure.find(trg_hist_fig)
				end
				break
			end
		end
	end
	if trg_hist_fig ==nil then
		error("Slayer not found")
	end
	
	local trg_unit=trg_hist_fig.unit_id
	if trg_unit==nil then
		error("Unit id not found!")
	end
	local trg_unit_final=df.unit.find(trg_unit)
	
	tools.change_adv(trg_unit_final)
	if swap_soul then --actually add a soul...
		t_soul=adv.status.current_soul
		adv.status.current_soul=df.NULL
		adv.status.souls:resize(0)
		trg_unit_final.status.current_soul=t_soul
		trg_unit_final.status.souls:insert(#trg_unit_final.status.souls,t_soul)
	end
end
adv_tools.menu:add("Reincarnate",adv_tools.reincarnate)
function adv_tools.ressurect()
	
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
