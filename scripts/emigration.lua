--Allow stressed dwarves to emigrate from the fortress
-- For 34.11 by IndigoFenix; update and cleanup by PeridexisErrant
-- old version:  http://dffd.bay12games.com/file.php?id=8404
--[[=begin

emigration
==========
Allows dwarves to emigrate from the fortress when stressed,
in proportion to how badly stressed they are and adjusted
for who they would have to leave with - a dwarven merchant
being more attractive than leaving alone (or with an elf).
The check is made monthly.

A happy dwarf (ie with negative stress) will never emigrate.

Usage:  ``emigration enable|disable``

=end]]

local args = {...}
if args[1] == "enable" then
    enabled = true
elseif args[1] == "disable" then
    enabled = false
end

function desireToStay(unit,method,civ_id)
    -- on a percentage scale
    local value = 100 - unit.status.current_soul.personality.stress_level / 5000
    if method == 'merchant' or method == 'diplomat' then
        if civ_id ~= unit.civ_id then value = value*2 end end
    if method == 'wild' then
        value = value*5 end
    return value
end

function desert(u,method,civ)
    u.relations.following = nil
    local line = dfhack.TranslateName(dfhack.units.getVisibleName(u)) .. " has "
    if method == 'merchant' then
        line = line.."joined the merchants"
        u.flags1.merchant = true
        u.civ_id = civ
    elseif method == 'diplomat' then
        line = line.."followed the diplomat"
        u.flags1.diplomat = true
        u.civ_id = civ
    else
        line = line.."abandoned the settlement in search of a better life."
        u.civ_id = -1
        u.flags1.forest = true
        u.animal.leave_countdown = 2
    end
    print(line)
    dfhack.gui.showAnnouncement(line, COLOR_WHITE)
end

function canLeave(unit)
    for _, skill in pairs(unit.status.current_soul.skills) do
        if skill.rating > 14 then return false end
    end
    if unit.flags1.caged
        or unit.race ~= df.global.ui.race_id
        or unit.civ_id ~= df.global.ui.civ_id
        or dfhack.units.isDead(unit)
        or dfhack.units.isOpposedToLife(unit)
        or unit.flags1.merchant
        or unit.flags1.diplomat
        or unit.flags1.chained
        or dfhack.units.getNoblePositions(unit) ~= nil
        or unit.military.squad_id ~= -1
        or dfhack.units.isCitizen(unit)
        or dfhack.units.isSane(unit)
        or unit.profession ~= 103
        or not dfhack.units.isDead(unit)
            then return false end
    return true
end

function checkForDeserters(method,civ_id)
    local allUnits = df.global.world.units.active
    for i=#allUnits-1,0,-1 do   -- search list in reverse
        local u = allUnits[i]
        if canLeave(u) and math.random(100) < desireToStay(u,method,civ_id) then
            desert(u,method,civ_id)
        end
    end
end

function checkmigrationnow()
    local merchant_civ_ids = {}
    local diplomat_civ_ids = {}
    local allUnits = df.global.world.units.active
    for i=0, #allUnits-1 do
        local unit = allUnits[i]
        if dfhack.units.isSane(unit)
        and not dfhack.units.isDead(unit)
        and not dfhack.units.isOpposedToLife(unit)
        and not unit.flags1.tame
        then
            if unit.flags1.merchant then table.insert(merchant_civ_ids, unit.civ_id) end
            if unit.flags1.diplomat then table.insert(diplomat_civ_ids, unit.civ_id) end
        end
    end

    for _, civ_id in pairs(merchant_civ_ids) do checkForDeserters('merchant', civ_id) end
    for _, civ_id in pairs(diplomat_civ_ids) do checkForDeserters('diplomat', civ_id) end
    checkForDeserters('wild', -1)
end

local function event_loop()
    if enabled then
        checkmigrationnow()
        dfhack.timeout(1, 'months', event_loop)
    end
end

dfhack.onStateChange.loadEmigration = function(code)
    if code==SC_MAP_LOADED then
        if enabled then
            print("Emigration enabled.")
            event_loop()
        else
            print("Emigration disabled.")
        end
    end
end

if dfhack.isMapLoaded() then
    dfhack.onStateChange.loadEmigration(SC_MAP_LOADED)
end

