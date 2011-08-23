if WINDOWS then --windows function defintions
	onfunction.AddFunction(0x55499D+offsets.base(),"Move") --on creature move found with "watch mem=xcoord"
	onfunction.AddFunction(0x275933+offsets.base(),"Die",{creature="edi"})  --on creature death? found by watching dead flag then stepping until new function
else --linux
	onfunction.AddFunction(0x899befe+offsets.base(),"Move") -- found out by attaching watch...
	onfunction.AddFunction(0x850eecd+offsets.base(),"Die",{creature="ebx"})  -- same
end
