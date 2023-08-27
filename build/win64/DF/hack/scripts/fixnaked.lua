--removes unhappy thoughts due to lack of clothing
--[====[

fixnaked
========
Removes all unhappy thoughts due to lack of clothing.

]====]

function fixnaked()
local total_fixed = 0
local total_removed = 0

for fnUnitCount,fnUnit in ipairs(df.global.world.units.all) do
    if fnUnit.race == df.global.plotinfo.race_id and fnUnit.status.current_soul then
        local found = true
        local fixed = false
        while found do
            local emotions = fnUnit.status.current_soul.personality.emotions
            found = false
            for k,v in pairs(emotions) do
                if v.thought == df.unit_thought_type.Uncovered
                   or v.thought == df.unit_thought_type.NoShirt
                   or v.thought == df.unit_thought_type.NoShoes
                   or v.thought == df.unit_thought_type.NoCloak
                   or v.thought == df.unit_thought_type.OldClothing
                   or v.thought == df.unit_thought_type.TatteredClothing
                   or v.thought == df.unit_thought_type.RottedClothing then
                    emotions:erase(k)
                    found = true
                    total_removed = total_removed + 1
                    fixed = true
                    break
                end
            end
        end
        if fixed then
            total_fixed = total_fixed + 1
            print(total_fixed, total_removed, dfhack.TranslateName(dfhack.units.getVisibleName(fnUnit)))
        end
    end
end
print("Total Fixed: "..total_fixed)
end
fixnaked()
