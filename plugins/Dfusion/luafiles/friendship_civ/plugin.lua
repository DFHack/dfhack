fc_ui={}
fc_ui.menu=MakeMenu()
function fc_ui.get()
	local mycivs=friendship_civ.getcivs()
	if mycivs~= nil then
		print(" Currently friendly civs:")
		for k,v in pairs(mycivs) do
			print(string.format("%d. %d",k,v))
		end
	else
		print(" Plugin no yet activated.")
	end
end
function fc_ui.add()
	print("Type in civ id to add (leave empty to add current, q cancels):")
	local r
	while r==nil and r~='q' do
	r=io.stdin:read()
	if r=="" then
		r=nil
		break
	end
	if r~='q' then r=tonumber(r) else
		return
	end
	end
	friendship_civ.addciv(r)
end
function fc_ui.remove()
	local mycivs=friendship_civ.getcivs()
	if mycivs~= nil then
		print(" Currently friendly civs:")
		for k,v in pairs(mycivs) do
			print(string.format("%d. %d",k,v))
		end
	else
		print(" Plugin no yet activated, nothing to remove.")
		return
	end
	print("Type in civ id to remove( q cancels):")
	local r
	while r==nil and r~='q' do
		r=io.stdin:read()
		if r~='q' then 
			r=tonumber(r)
			if r>#mycivs then r=nil end
		else
			return
		end
	end
	table.remove(mycivs,r)
	friendship_civ.install(mycivs)
end
fc_ui.menu:add("Add civ",fc_ui.add)
fc_ui.menu:add("Get civs",fc_ui.get)
fc_ui.menu:add("Remove civ",fc_ui.remove)
fc_ui.menu:display()