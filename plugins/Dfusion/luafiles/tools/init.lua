tools={}
tools.menu=MakeMenu()
function tools.setrace(name)
	RaceTable=BuildNameTable()
	print("Your current race is:"..GetRaceToken(df.global.ui.race_id))
	local id
	if name == nil then
		print("Type new race's token name in full caps (q to quit):")
		repeat
			entry=getline()
			if entry=="q" then
				return
			end
			id=RaceTable[entry]
		until id~=nil
	else
		id=RaceTable[name]
		if id==nil then
			error("Name not found!")
		end
	end
	df.global.ui.race_id=id
end
tools.menu:add("Set current race",tools.setrace)
function tools.GiveSentience(names)
	RaceTable=RaceTable or BuildNameTable() --slow.If loaded don't load again
	if names ==nil then
		ids={}
		print("Type race's  token name in full caps to give sentience to:")
		repeat
			entry=getline()
			id=RaceTable[entry]
		until id~=nil
		table.insert(ids,id)
	else
		ids={}
		for _,name in pairs(names) do
			id=RaceTable[name]
			table.insert(ids,id)
		end
	end
	for _,id in pairs(ids) do
		local races=df.global.world.raws.creatures.all

		local castes=races[id].caste
		print(string.format("Caste count:%i",castes.size))
		for i =0,#castes-1 do
			
			print("Caste name:"..castes[i].caste_id.."...")
			
			local flags=castes[i].flags
			--print(string.format("%x",flagoffset))
			if flags.CAN_SPEAK then
				print("\tis sentient.")
			else
				print("\tnon sentient. Allocating IQ...")
				flags.CAN_SPEAK=true
			end
		end
	end
end
tools.menu:add("Give Sentience",tools.GiveSentience)
function tools.embark()
	off=offsets.find(0,0x66, 0x83, 0x7F ,0x1A ,0xFF,0x74,0x04)
	if off~=0 then
		engine.pokeb(off+5,0x90)
		engine.pokeb(off+6,0x90)
		print("Found and patched")
	else
		print("not found")
	end
end
tools.menu:add("Embark anywhere",tools.embark)
function tools.getCreatureId(vector) --redo it to getcreature by name/id or something
	tnames={}
	rnames={}
	--[[print("vector1 size:"..vector:size())
	print("vector2 size:"..vector2:size())]]--
	for i=0,vector:size()-1 do
		--print(string.format("%x",vector:getval(i)))
		
		local name=engine.peek(vector:getval(i),ptt_dfstring):getval()
		local lid= tools.getlegendsid(vector:getval(i))
		if lid ~=0 then
			print(i..")*Creature Name:"..name.." race="..engine.peekw(vector:getval(i)+ptr_Creature.race.off).." legendid="..lid)
		else
			print(i..") Creature Name:"..name.." race="..engine.peekw(vector:getval(i)+ptr_Creature.race.off))
		end
		if name ~="" and name~=nil then
			tnames[i]=name
			rnames[name]=i
		end
	end
	print("=====================================")
	print("type in name or number:")
	r=getline()
	if tonumber(r) ==nil then
		indx=rnames[r]
		if indx==nil then return end
	else
		r=tonumber(r)
		if r<vector:size() then indx=r else return end
	end
	return indx
end
function tools.change_adv(unit,nemesis)
	if nemesis==nil then
		nemesis=true --default value is nemesis switch too.
	end
	if unit==nil then
		unit=getCreatureAtPointer()
	end
	if unit==nil then
		error("Invalid unit!")
	end
	local other=df.global.world.units.active
	local unit_indx
	for k,v in pairs(other) do
		if v==unit then
			unit_indx=k
			break
		end
	end
	if unit_indx==nil then
		error("Unit not found in array?!") --should not happen
	end
	other[unit_indx]=other[0]
	other[0]=unit
	if nemesis then --basicly copied from advtools plugin...
		local nem=getNemesis(unit)
		local other_nem=getNemesis(other[unit_indx])
		if other_nem then
			other_nem.flags[0]=false
			other_nem.flags[1]=true
		end
		if nem then
			nem.flags[0]=true
			nem.flags[2]=true
			for k,v in pairs(df.global.world.nemesis.all) do
				if v.id==nem.id then
					df.global.ui_advmode.player_id=k
				end
			end
		else
			error("Current unit does not have nemesis record, further working not guaranteed")
		end
	end
end
tools.menu:add("Change Adventurer",tools.change_adv)

function tools.MakeFollow(unit,trgunit)
	
	if unit == nil then
		unit=getCreature()
	end
	if unit== nil then
		error("Invalid creature")
	end
	if trgunit==nil then
		trgunit=df.global.world.units.active[0]
	end
	unit.relations.group_leader_id=trgunit.id
	local u_nem=getNemesis(unit)
	local t_nem=getNemesis(trgunit)
	if u_nem then
		u_nem.group_leader_id=t_nem.id
	end
	if t_nem and u_nem then
		t_nem.companions:insert(#t_nem.companions,u_nem.id)
	end
end
tools.menu:add("Make creature follow",tools.MakeFollow)
function tools.getsite(names)
	if words==nil then --do once its slow.
		words,rwords=BuildWordTables()
	end
	
	if names==nil then
	print("Type words that are in the site name, FULLCAPS, no modifiers (lovely->LOVE), q to quit:")
	names={}
	repeat
	w=getline();
	
	if rwords[w]~=nil then
		table.insert(names,w)
		print("--added--")
	end
	
	until w=='q'
	end
	
	tnames={}
	for _,v in pairs(names) do
		if rwords[v] ~=nil then
			table.insert(tnames,rwords[v]) --get word numbers
		end
	end
	
	local offsites=engine.peekd(offsets.getEx("SiteData"))+0x120
	snames={" pfort"," dfort","  cave","mohall","forest","hamlet","imploc","  lair","  fort","  camp"}
	vector=engine.peek(offsites,ptr_vector)
	print("Number of sites:"..vector:size())
	print("List of hits:")
	for i =0,vector:size()-1 do
		off=vector:getval(i)
		
		good=true
		r=""
		hits=0
		sname=engine.peek(off,ptr_site.name)
		for k=0,6 do
			vnum=sname[k]--engine.peekd(off+0x38+k*4)
			tgood=false
			
			if vnum~=0xFFFFFFFF then
				--print(string.format("%x",vnum))
				if names[vnum]~=nil then
				r=r..names[vnum].." "
				end
				for _,v in pairs(tnames) do
					if vnum==v then
						tgood=true
						--print("Match")
						hits=hits+1
						break
					end
				end
				if not tgood then
					good=false
				end
			end
		end
	
		if(good) and (hits>0)then
		--if true then
			--print("=====================")
			typ=engine.peek(off,ptr_site.type)--engine.peekw(off+0x78)
			flg=engine.peekd(engine.peek(off,ptr_site.flagptr))
			--flg=engine.peekd(off+224)
			--flg2=engine.peekw(off)
			--tv=engine.peek(off+0x84,ptr_vector)
			--tv2=engine.peek(off+0xA4,ptr_vector)
			
			print(string.format("%d)%s off=%x type=%s\t flags=%x",i,r,off,snames[typ+1],flg))
			
			if i%100==99 then
				r=getline()
			end
			
		end
	end
	print("Type which to change (q cancels):")
	repeat
		r=getline()
		n=tonumber(r)
		if(r=='q') then return end
	until n~=nil
	return vector:getval(n)
end
function tools.changesite(names)
	off=tools.getsite(names)
	snames={"Mountain halls (yours)","Dark fort","Cave","Mountain hall (NPC)","Forest retreat","Hamlet","Important location","Lair","Fort","Camp"}
	
	print("Type in the site type (q cancels):")
	for k,v in pairs(snames) do
		print((k-1).."->"..v)
	end
	repeat
		r=getline()
		n2=tonumber(r)
		if(r=='q') then return end
	until n2~=nil
	--off=vector:getval(n)
	print(string.format("%x->%d",off,n2))
	engine.poke(off,ptr_site.type,n2)
end
function tools.project(unit,trg)
	if unit==nil then
		unit=getCreatureAtPointer()
	end
	
	if unit==nil then
		error("Failed to project unit. Unit not selected/valid")
	end
	-- todo: add projectile to world, point to unit, add flag to unit, add gen-ref to projectile.
	local p=df.proj_unitst:new()
	local startpos={x=unit.pos.x,y=unit.pos.y,z=unit.pos.z}
	p.origin_pos=startpos
	p.target_pos=trg
	p.cur_pos=startpos
	p.prev_pos=startpos
	p.unit=unit
	--- wtf stuff
	p.unk14=100
	p.unk16=-1
	p.unk23=-1
	p.fall_delay=5
	p.fall_counter=5
	p.collided=true
	-- end wtf
	local citem=df.global.world.proj_list
	local maxid=1
	local newlink=df.proj_list_link:new()
	newlink.item=p
	while citem.item~= nil do
		if citem.item.id>maxid then maxid=citem.item.id end
		if citem.next ~= nil then 
			citem=citem.next
		else
			break
		end
	end
	p.id=maxid+1
	newlink.prev=citem
	citem.next=newlink
	
	local proj_ref=df.general_ref_projectile:new()
	proj_ref.projectile_id=p.id
	unit.refs:insert(#unit.refs,proj_ref)
	unit.flags1.projectile=true
end
function tools.empregnate(unit)
	if unit==nil then
		unit=getSelectedUnit()
	end
	
	if unit==nil then
		unit=getCreatureAtPos(getxyz())
	end
	
	if unit==nil then
		error("Failed to empregnate. Unit not selected/valid")
	end
	if unit.curse then
		unit.curse.add_tags2.STERILE=false
	end
	local genes = unit.appearance.genes
	if unit.relations.pregnancy_ptr == nil then
		print("creating preg ptr.")
		if false then
			print(string.format("%x %x",df.sizeof(unit.relations:_field("pregnancy_ptr"))))
			return
		end
		unit.relations.pregnancy_ptr = { new = true, assign = genes }
	end
	local ngenes = unit.relations.pregnancy_ptr
	if #ngenes.appearance ~= #genes.appearance or #ngenes.colors ~= #genes.colors then
		print("Array sizes incorrect, fixing.")
		ngenes:assign(genes);
	end
	print("Setting preg timer.")
	unit.relations.pregnancy_timer=10
	unit.relations.pregnancy_mystery=1
end
tools.menu:add("Empregnate",tools.empregnate)
function tools.changeflags(names)
	myflag_pattern=ptt_dfflag.new(3*8)
	off=tools.getsite(names)
	offflgs=engine.peek(off,ptr_site.flagptr)
	q=''
	print(string.format("Site offset %x flags offset %x",off,offflgs))
	repeat
		print("flags:")
		
		--off=vector:getval(n)
		flg=engine.peek(offflgs,myflag_pattern)
		r=""
		for i=0,3*8-1 do
			if flg:get(i)==1 then
				r=r.."x"
			else
				r=r.."o"
			end
			if i%8==7 then
				print(i-7 .."->"..r)
				r=""
			end
		end
		print("Type number to flip, or 'q' to quit.")
		q=getline()
		n2=tonumber(q)
		if n2~=nil then
			
			flg:flip(n2)
			engine.poke(offflgs,myflag_pattern,flg)
		end
	until q=='q'
end
function tools.hostilate()
	vector=engine.peek(offsets.getEx("CreatureVec"),ptr_vector)
	id=engine.peekd(offsets.getEx("CreaturePtr"))
	print(string.format("Vec:%d cr:%d",vector:size(),id))
	off=vector:getval(id)
	crciv=engine.peek(off,ptr_Creature.civ)
	print("Creatures civ:"..crciv)
	curciv=engine.peekd(offsets.getEx("CurrentRace")-12)
	print("My civ:"..curciv)
	if curciv==crciv then
		print("Friendly-making enemy")
		engine.poke(off,ptr_Creature.civ,-1)
		flg=engine.peek(off,ptr_Creature.flags)
		flg:set(17,0)
		print("flag 51:"..tostring(flg:get(51)))
		engine.poke(off,ptr_Creature.flags,flg)
	else
		print("Enemy- making friendly")
		engine.poke(off,ptr_Creature.civ,curciv)
		flg=engine.peek(off,ptr_Creature.flags)
		flg:set(17,1)
		flg:set(19,0)
		engine.poke(off,ptr_Creature.flags,flg)
	end
end
function tools.mouseBlock()
	local xs,ys,zs
	xs,ys,zs=getxyz()
	xs=math.floor(xs/16)
	ys=math.floor(ys/16)
	print("Mouse block is:"..xs.." "..ys.." "..zs)
end
function tools.fixwarp()
   local mapoffset=offsets.getEx("WorldData")--0x131C128+offsets.base()
   local x=engine.peek(mapoffset+24,DWORD)
   local y=engine.peek(mapoffset+28,DWORD)
   local z=engine.peek(mapoffset+32,DWORD)
   --vec=engine.peek(mapoffset,ptr_vector)
   
   print("Blocks loaded:"..x.." "..y.." "..z)
   print("Select type:")
   print("1. All (SLOW)")
   print("2. range (x0 x1 y0 y1 z0 z1)")
   print("3. One block around pointer")
   print("anything else- quit")
   q=getline()
   n2=tonumber(q)
   if n2==nil then return end
   if n2>3 or n2<1 then return end
   local xs,xe,ys,ye,zs,ze
   if n2==1 then
      xs=0
      xe=x-1
      ys=0
      ye=y-1
      zs=0
      ze=z-1
   elseif n2==2 then
      print("enter x0:")
      xs=tonumber(getline())
      print("enter x1:")
      xe=tonumber(getline())
      print("enter y0:")
      ys=tonumber(getline())
      print("enter y1:")
      ye=tonumber(getline())
      print("enter z0:")
      zs=tonumber(getline())
      print("enter z1:")
      ze=tonumber(getline())
      function clamp(t,vmin,vmax)
         if t> vmax then return vmax end
         if t< vmin then return vmin end
         return t
      end
      xs=clamp(xs,0,x-1)
      ys=clamp(ys,0,y-1)
      zs=clamp(zs,0,z-1)
      xe=clamp(xe,xs,x-1)
      ye=clamp(ye,ys,y-1)
      ze=clamp(ze,zs,z-1)
   else
      xs,ys,zs=getxyz()
      xs=math.floor(xs/16)
      ys=math.floor(ys/16)
      xe=xs
      ye=ys
      ze=zs
   end
   local xblocks=engine.peek(mapoffset,DWORD)
   local flg=bit.bnot(bit.lshift(1,3))
   for xx=xs,xe do
      local yblocks=engine.peek(xblocks+xx*4,DWORD)
      for yy=ys,ye do
         local zblocks=engine.peek(yblocks+yy*4,DWORD)
         for zz=zs,ze do
            local myblock=engine.peek(zblocks+zz*4,DWORD)
            if myblock~=0 then
               for i=0,255 do
                  local ff=engine.peek(myblock+0x67c+i*4,DWORD)
                  ff=bit.band(ff,flg) --set 14 flag to 1
                  engine.poke(myblock+0x67c+i*4,DWORD,ff)
               end
            end
         end
         print("Blocks done:"..xx.." "..yy)
      end
   end
end