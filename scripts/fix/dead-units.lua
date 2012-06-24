-- Remove uninteresting dead units from the unit list.

local units = df.global.world.units.active
local dwarf_race = df.global.ui.race_id
local dwarf_civ = df.global.ui.civ_id
local count = 0

for i=#units-1,0,-1 do
    local unit = units[i]
    local flags1 = unit.flags1
    local flags2 = unit.flags2
    if flags1.dead and unit.race ~= dwarf_race then
        local remove = false
        if flags2.slaughter then
            remove = true
        elseif not unit.name.has_name then
            remove = true
        elseif unit.civ_id ~= dwarf_civ and
               not (flags1.merchant or flags1.diplomat) then
            remove = true
        end
        if remove then
            count = count + 1
            units:erase(i)
        end
    end
end

print('Units removed from active: '..count)
