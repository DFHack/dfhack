--local bit = require("bit")

if not(FILE) then
	tools.menu:add("Change site type",tools.changesite)
	tools.menu:add("Change site flags",tools.changeflags)
	tools.menu:add("Run script file",tools.runscript)
	tools.menu:add("Hostilate creature",tools.hostilate)
	tools.menu:add("Protect site from item scattering",tools.protectsite)
	tools.menu:add("Print current mouse block",tools.mouseBlock)
	--tools.menu:add("XXX",tools.fixwarp)
	tools.menu:display()
	
	--[[choices={
	{tools.setrace,"Change race"},
	{tools.GiveSentience,"Give Sentience"},
	{tools.embark,"Embark anywhere"},
	{tools.change_adv,"Change Adventurer"},
	{tools.changesite,"Change site type"},
	{tools.runscript,"Run script file"},
	{tools.MakeFollow,"Make creature follow"},
	{function () return end,"Quit"}}
	print("Select choice:")
	for p,c in pairs(choices) do
		print(p..")."..c[2])
	end
	repeat
	ans=tonumber(io.stdin:read())
	if ans==nil or not(ans<=table.maxn(choices) and ans>0) then
		print("incorrect choice")
	end
	until ans~=nil and (ans<=table.maxn(tdir) and ans>0)
	choices[ans][1]()]]--
end