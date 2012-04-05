mypos=engine.getmod("functions")
function DeathMsg(values)
	local name
	local u=engine.cast(df.unit,values[onfunction.hints["Die"].creature])
	
	print(u.name.first_name.." died")
end
if mypos then
	print("Onfunction already installed")
	--onfunction.patch(0x189dd6+offsets.base())
else
	onfunction.install()
	dofile("dfusion/onfunction/locations.lua")
	onfunction.SetCallback("Die",DeathMsg)
end
