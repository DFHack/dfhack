-- Undesignates building base materials for dumping.
function undump_buildings()
    local buildings = df.global.world.buildings.all
    local undumped = 0
    for i = 0, #buildings - 1 do
        local building = buildings[i]
        -- Zones and stockpiles don't have the contained_items field.
        if df.building_actual:is_instance(building) then
            local items = building.contained_items
            for j = 0, #items - 1 do
                local contained = items[j]
                if contained.use_mode == 2 and contained.item.flags.dump then
                    -- print(building, contained.item)
                    undumped = undumped + 1
                    contained.item.flags.dump = false
                end
            end
        end
    end
    
    if undumped > 0 then
        local s = "s"
        if undumped == 1 then s = "" end
        print("Undumped "..undumped.." item"..s..".")
    end
end

undump_buildings()
