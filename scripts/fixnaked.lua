function fixnaked()
local total_fixed = 0
local total_removed = 0

for fnUnitCount,fnUnit in ipairs(df.global.world.units.all) do
    if fnUnit.race == df.global.ui.race_id then
        local listEvents = fnUnit.status.recent_events
        --for lkey,lvalue in pairs(listEvents) do
        --    print(df.unit_thought_type[lvalue.type],lvalue.type,lvalue.age,lvalue.subtype,lvalue.severity)
        --end

        local found = 1
        local fixed = 0
        while found == 1 do
            local events = fnUnit.status.recent_events
            found = 0
            for k,v in pairs(events) do
                if v.type == df.unit_thought_type.Uncovered
                   or v.type == df.unit_thought_type.NoShirt
                   or v.type == df.unit_thought_type.NoShoes
                   or v.type == df.unit_thought_type.NoCloak
                   or v.type == df.unit_thought_type.OldClothing
                   or v.type == df.unit_thought_type.TatteredClothing
                   or v.type == df.unit_thought_type.RottedClothing then
                    events:erase(k)
                    found = 1
                    total_removed = total_removed + 1
                    fixed = 1
                    break
                end
            end
        end
        if fixed == 1 then
            total_fixed = total_fixed + 1
            print(total_fixed, total_removed, dfhack.TranslateName(dfhack.units.getVisibleName(fnUnit)))
        end
    end
end
print("Total Fixed: "..total_fixed)
end
fixnaked()
