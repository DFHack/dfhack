-- Pauses the game with a warning if a creature is starving, dehydrated, or very drowsy.
-- By Meneth32, and PeridexisErrant

starvingUnits = starvingUnits or {}
dehydratedUnits = dehydratedUnits or {}
sleepyUnits = sleepyUnits or {}

local units = df.global.world.units.active

local function findRaceCaste(unit)
    local rraw = df.creature_raw.find(unit.race)
    return rraw, safe_index(rraw, 'caste', unit.caste)
end

local function getSexString(sex)
    local sexStr = ""
    if sex==0 then sexStr=string.char(12)
    elseif sex==1 then sexStr=string.char(11)
    end
    return string.char(40)..sexStr..string.char(41)
end

local function nameOrSpeciesAndNumber(unit)
    if unit.name.has_name then
        return dfhack.TranslateName(dfhack.units.getVisibleName(unit))..' '..getSexString(unit.sex),true
    else
        return 'Unit #'..unit.id..' ('..df.creature_raw.find(unit.race).caste[unit.caste].caste_name[0]..' '..getSexString(unit.sex)..')',false
    end
end

local function checkVariable(var, limit, description, map, unit)
    local rraw = findRaceCaste(unit)
    local species = rraw.name[0]
    local name = nameOrSpeciesAndNumber(unit)
    if var > limit then
        if not map[unit.id] then
            map[unit.id] = true
            return species.." "..name.." is "..description.."!"
        end
    else
        map[unit.id] = false
    end
end

local function doCheck()
    local msg
    for i=#units-1, 0, -1 do
        local unit = units[i]
        local rraw = findRaceCaste(unit)
        if rraw and not unit.flags1.dead and not dfhack.units.isOpposedToLife(unit) then
            msg = checkVariable(unit.counters2.hunger_timer, 75000, 'starving', starvingUnits, unit)
            msg = msg or checkVariable(unit.counters2.thirst_timer, 50000, 'dehydrated', dehydratedUnits, unit)
            msg = msg or checkVariable(unit.counters2.sleepiness_timer, 150000, 'very drowsy', sleepyUnits, unit)
        end
    end
    if msg then
        df.global.pause_state = true
        dfhack.gui.showPopupAnnouncement(msg, 7, true)
    end
end

doCheck()
