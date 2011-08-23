mypos=engine.getmod("functions")
function DeathMsg(values)
	local name
	name=engine.peek(values[onfunction.hints["Die"].creature],ptt_dfstring)
	print(name:getval().." died")
end
if mypos then
	print("Onfunction already installed")
	--onfunction.patch(0x189dd6+offsets.base())
else
	onfunction.install()
	dofile("dfusion/onfunction/locations.lua")
	onfunction.SetCallback("Die",DeathMsg)
end
