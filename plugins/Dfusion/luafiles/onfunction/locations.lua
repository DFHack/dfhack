if WINDOWS then --windows function defintions
	--[=[onfunction.AddFunction(0x55499D+offsets.base(),"Move") --on creature move found with "watch mem=xcoord"
	onfunction.AddFunction(0x275933+offsets.base(),"Die",{creature="edi"})  --on creature death? found by watching dead flag then stepping until new function
	onfunction.AddFunction(0x2c1834+offsets.base(),"CreateCreature",{protocreature="eax"}) --arena
	onfunction.AddFunction(0x349640+offsets.base(),"AddItem",{item="esp"}) --or esp
	onfunction.AddFunction(0x26e840+offsets.base(),"Dig_Create",{item_type="esp"}) --esp+8 -> material esp->block type
	onfunction.AddFunction(0x3d4301+offsets.base(),"Make_Item",{item_type="esp"}) 
	onfunction.AddFunction(0x5af826+offsets.base(),"Hurt",{target="esi",attacker={off=0x74,rtype=DWORD,reg="esp"}}) 
	onfunction.AddFunction(0x3D5886+offsets.base(),"Flip",{building="esi"}) 
	onfunction.AddFunction(0x35E340+offsets.base(),"ItemCreate")--]=]
	--onfunction.AddFunction(0x4B34B6+offsets.base(),"ReactionFinish") --esp item. Ecx creature, edx? 0.34.07
	onfunction.AddFunction(0x72aB6+offsets.base(),"Die",{creature="edi"}) --0.34.07
else --linux
	--[=[onfunction.AddFunction(0x899befe+offsets.base(),"Move") -- found out by attaching watch...
	onfunction.AddFunction(0x850eecd+offsets.base(),"Die",{creature="ebx"})  -- same--]=]
end
