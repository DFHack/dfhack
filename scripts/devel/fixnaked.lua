function fixnaked()
local total_fixed = 0
local total_uncovered = 0
local total_noshirt = 0
local total_noshoes = 0

for fnUnitCount,fnUnit in ipairs(df.global.world.units.all) do
    if fnUnit.race == df.global.ui.race_id then
		local listEvents = fnUnit.status.recent_events
		--for lkey,lvalue in pairs(listEvents) do
		--		print(df.unit_thought_type[lvalue.type],lvalue.type,lvalue.age,lvalue.subtype,lvalue.severity)
		--end

		local found = 1
		local fixed = 0
		while found == 1 do
			local events = fnUnit.status.recent_events
			found = 0
			for k,v in pairs(events) do
				if v.type == 109 then
					events:erase(k)
					found = 1
					total_uncovered = total_uncovered + 1
					fixed = 1
					break
				end
				if v.type == 110 then
					events:erase(k)
					found = 1
					total_noshirt = total_noshirt + 1
					fixed = 1
					break
				end
				if v.type == 111 then
					events:erase(k)
					found = 1
					total_noshoes = total_noshoes + 1
					fixed = 1
					break
				end
			end
		end
		if fixed == 1 then
			total_fixed = total_fixed + 1
			print(total_fixed, total_uncovered+total_noshirt+total_noshoes,dfhack.TranslateName(dfhack.units.getVisibleName(fnUnit)))
		end
	end
end
print("thought 109 = "..df.unit_thought_type[109])
print("thought 110 = "..df.unit_thought_type[110])
print("thought 111 = "..df.unit_thought_type[111])
print("Total Fixed: "..total_fixed)
print("Total thoughts removed: "..total_uncovered)
print("Total thoughts removed: "..total_noshirt)
print("Total thoughts removed: "..total_noshoes)

end
fixnaked()
