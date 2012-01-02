friendship_civ={}
function friendship_civ.init()
	friendship_civ.count=0x0f
	friendship_civ.firsttime=true
	local mypos=engine.getmod("Friendship_civ")
	local modpos=0
	if mypos then
		modpos=mypos
		_,modsize=engine.loadobj("dfusion/friendship_civ/friendship_c.o")
		_=nil
		friendship_civ.firsttime=false
	else
		modpos,modsize=engine.loadmod("dfusion/friendship_civ/friendship_c.o","Friendship_civ",1024)
		print(string.format("Loaded module @:%x",modpos))
	end
	friendship_civ.modpos=modpos
	friendship_civ.modsize=modsize
end
function friendship_civ.install(civs)
	friendship_civ.init()
	local count=0
	for _,v in pairs(civs) do
		engine.poked(friendship_civ.modpos+friendship_civ.modsize+count*4,v) -- for some reason it compiled strangely
							-- cmp word[ebx+ecx*2],ax -> cmp word[ebx+ecx*2+2],ax
		count = count + 1
	end
	engine.poked(friendship_civ.modpos+0x0a,friendship_civ.modpos+friendship_civ.modsize) -- set ptr to civ ids
	engine.poked(friendship_civ.modpos+friendship_civ.count,count-1)			   -- set count of civs
	SetExecute(friendship_civ.modpos)
	
	if(friendship_civ.firsttime) then
		friendship_civ.patch()
	end
end
function friendship_civ.getcivs()
	if(friendship_civ.firsttime==nil)then
		return nil
	end
	friendship_civ.init()
	local count=engine.peekd(friendship_civ.modpos+friendship_civ.count)+1
	local ret={}
	for i=0, count-1 do
		table.insert(ret,engine.peekd(friendship_civ.modpos+friendship_civ.modsize+i*4))
	end
	return ret
end
function friendship_civ.addciv(civ) --if called with nil add current civ :)
	if civ==nil then
		local cciv=engine.peekd(VersionInfo.getGroup("Creatures"):getAddress("current_civ"))
		friendship_civ.install({cciv})
		return 
	end
	local oldcivs=friendship_civ.getcivs()
	oldcivs=oldcivs or {}
	if type(civ)=="table" then
		for k,v in ipairs(civ) do
			table.insert(oldcivs,v)
		end
	else
		table.insert(oldcivs,civ)
	end
	friendship_civ.install(oldcivs)
end
function friendship_civ.patch_call(off,iseax)
	local calltrg=friendship_civ.modpos
	if not iseax then
		calltrg=calltrg+4
	end
	engine.pokeb(off,0xe8) --this is a call
	engine.poked(off+1,calltrg-off-5) --offset to call to (relative)
	engine.pokeb(off+5,0x90) --nop
end
function friendship_civ.patch()
	--UpdateRanges()
	local civloc= VersionInfo.getGroup("Creatures"):getAddress("current_civ")
	local pos1=offsets.findall(0,0x3B,0x05,DWORD_,civloc)  --eax
	for k,v in pairs(pos1) do print(string.format("%d %x",k,v)) end
	local pos2=offsets.findall(0,0x3B,0x0D,DWORD_,civloc)  --ecx
	for k,v in pairs(pos2) do print(string.format("%d %x",k,v)) end
	
	for k,v in pairs(pos1) do
		print(string.format("Patching eax compare %d: %x",k,v))
		friendship_civ.patch_call(v,true)
	end
	for k,v in pairs(pos2) do
		print(string.format("Patching ecx compare %d: %x",k,v))
		friendship_civ.patch_call(v,false)
	end
end
