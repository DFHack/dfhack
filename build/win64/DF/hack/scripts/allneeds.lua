-- Prints the sum of all citizens' needs.

local fort_needs = {}
for _, unit in pairs(df.global.world.units.all) do
    if not dfhack.units.isCitizen(unit) or not dfhack.units.isAlive(unit) then
        goto skipunit
    end

    local mind = unit.status.current_soul.personality.needs
    -- sum need_level and focus_level for each need
    for _,need in pairs(mind) do
        local needs = ensure_key(fort_needs, need.id)
        needs.cumulative_need = (needs.cumulative_need or 0) + need.need_level
        needs.cumulative_focus = (needs.cumulative_focus or 0) + need.focus_level
        needs.citizen_count = (needs.citizen_count or 0) + 1
    end

    :: skipunit ::
end

local sorted_fort_needs = {}
for id, need in pairs(fort_needs) do
    table.insert(sorted_fort_needs, {
        df.need_type[id],
        need.cumulative_need,
        need.cumulative_focus,
        need.citizen_count
    })
end

table.sort(sorted_fort_needs, function(a, b)
    return a[2] > b[2]
end)

-- Print sorted output
print(([[%20s %8s %8s %10s]]):format("Need", "Weight", "Focus", "# Dwarves"))
for _, need in pairs(sorted_fort_needs) do
    print(([[%20s %8.f %8.f %10d]]):format(need[1], need[2], need[3], need[4]))
end
