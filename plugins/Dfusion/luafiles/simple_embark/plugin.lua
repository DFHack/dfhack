function simple_embark(num)
stoff=VersionInfo.getAddress('start_dwarf_count')
print("Starting dwarves found:"..engine.peekd(stoff))
engine.poked(stoff,num)
end
if not(FILE) then
print("Type in new ammount:")
	repeat
		ans=tonumber(io.read())
		if ans==nil or not(ans<=15000 and ans>0) then
			print("incorrect choice")
		end
	until ans~=nil and (ans<=15000 and ans>0)
	simple_embark(ans)
end