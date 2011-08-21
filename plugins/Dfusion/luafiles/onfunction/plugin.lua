mypos=engine.getmod("functions")
function DeathMsg(values)
	local name
	if WINDOWS then
		name=engine.peek(values.edi,ptt_dfstring)
	else
		name=engine.peek(values.ebx,ptt_dfstring)
	end
	print(name:getval().." died")
end
if mypos then
	print("Onfunction already installed")
	--onfunction.patch(0x189dd6+offsets.base())
else
	onfunction.install()
	if WINDOWS then
		onfunction.AddFunction(0x55499D+offsets.base(),"Move") --on creature move found with "watch mem=xcoord"
		onfunction.AddFunction(0x275933+offsets.base(),"Die")  --on creature death? found by watching dead flag then stepping until new function
	else
		--onfunction.AddFunction(0x0899be82+offsets.base(),"Move") -- found out by attaching watch...
		onfunction.AddFunction(0x899befe+offsets.base(),"Move") -- found out by attaching watch...
		onfunction.AddFunction(0x850eecd+offsets.base(),"Die")  -- same
	end
	onfunction.SetCallback("Die",DeathMsg)
end
