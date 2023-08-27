-- Remove uninteresting dead units from the unit list.
--[====[

fix/dead-units
==============
Removes uninteresting dead units from the unit list. Doesn't seem to give any
noticeable performance gain, but migrants normally stop if the unit list grows
to around 3000 units, and this script reduces it back.

]====]
local units = df.global.world.units.active
local count = 0
local MONTH = 1200 * 28
local YEAR = MONTH * 12

for i=#units-1,0,-1 do
    local unit = units[i]
    if dfhack.units.isDead(unit) and not dfhack.units.isOwnRace(unit) then
        local remove = false
        if dfhack.units.isMarkedForSlaughter(unit) then
            remove = true
        elseif unit.hist_figure_id == -1 then
            remove = true
        elseif not dfhack.units.isOwnCiv(unit) and
               not (dfhack.units.isMerchant(unit) or dfhack.units.isDiplomat(unit)) then
            remove = true
        end
        if remove and unit.counters.death_id ~= -1 then
            --  Keep recent deaths around for a month before culling them. It's
            --  annoying to have that rampaging FB just be gone from both the
            --  other and dead lists, and you may want to keep killed wildlife
            --  around for a while too. We don't have a time of death for
            --  slaughtered units, so they go the first time.
            local incident = df.incident.find(unit.counters.death_id)
            if incident then
                local incident_time = incident.event_year * YEAR + incident.event_time
                local now = df.global.cur_year * YEAR + df.global.cur_year_tick
                if now - incident_time < MONTH then
                    remove = false  --  Wait a while before culling it.
                end
            end
        end
        if remove then
            count = count + 1
            units:erase(i)
        end
    end
end

print('Units removed from active: '..count)
