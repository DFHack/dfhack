function simple_embark(num)
	local stoff=dfhack.internal.getAddress('start_dwarf_count')
	print("Starting dwarves found:"..df.reinterpret_cast('int32_t', stoff).value)
	local tmp_val=df.new('int32_t')
	local size,pos=tmp_val:sizeof()
	tmp_val.value=num
	local ret=dfhack.internal.patchMemory(stoff,tmp_val,size)
	if ret then
		print("Success!")
	else
		qerror("Failed to patch in number")
	end
end
if not(FILE) then
print("Type in new amount (more than 6, less than 15000):")
	repeat
		ans=tonumber(io.read())
		if ans==nil or not(ans<=15000 and ans>0) then
			print("incorrect choice")
		end
	until ans~=nil and (ans<=15000 and ans>0)
	simple_embark(ans)
end