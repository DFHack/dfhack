-- Wakes up the sleeping, breaks up parties and stops breaks.

local utils = require 'utils'

local args = {...}
local burrows = {}
local bnames = {}

if not dfhack.isMapLoaded() then
    qerror('Map is not loaded.')
end

for _,v in ipairs(args) do
    local b = dfhack.burrows.findByName(v)
    if not b then
        qerror('Unknown burrow: '..v)
    end
    table.insert(bnames, v)
    table.insert(burrows, b)
end

local in_siege = false

function is_in_burrows(pos)
    if #burrows == 0 then
        return true
    end
    for _,v in ipairs(burrows) do
        if dfhack.burrows.isAssignedTile(v, pos) then
            return true
        end
    end
end

function add_thought(unit, code)
    for _,v in ipairs(unit.status.recent_events) do
        if v.type == code then
            v.age = 0
            return
        end
    end

    unit.status.recent_events:insert('#', { new = true, type = code })
end

function wake_unit(unit)
    local job = unit.job.current_job
    if not job or job.job_type ~= df.job_type.Sleep then
        return
    end

    if job.completion_timer > 0 then
        unit.counters.unconscious = 0
        add_thought(unit, df.unit_thought_type.SleepNoiseWake)
    elseif job.completion_timer < 0 then
        add_thought(unit, df.unit_thought_type.Tired)
    end

    job.pos:assign(unit.pos)

    job.completion_timer = 0

    unit.path.dest:assign(unit.pos)
    unit.path.path.x:resize(0)
    unit.path.path.y:resize(0)
    unit.path.path.z:resize(0)

    unit.counters.job_counter = 0
end

function stop_break(unit)
    local counter = dfhack.units.getMiscTrait(unit, df.misc_trait_type.OnBreak)
    if counter then
        counter.id = df.misc_trait_type.TimeSinceBreak
        counter.value = 100800 - 30*1200
        add_thought(unit, df.unit_thought_type.Tired)
    end
end

-- Check siege for thought purpose
for _,v in ipairs(df.global.ui.invasions.list) do
    if v.flags.siege and v.flags.active then
        in_siege = true
        break
    end
end

-- Stop rest
for _,v in ipairs(df.global.world.units.active) do
    local x,y,z = dfhack.units.getPosition(v)
    if x and dfhack.units.isCitizen(v) and is_in_burrows(xyz2pos(x,y,z)) then
        if not in_siege and v.military.squad_id < 0 then
            add_thought(v, df.unit_thought_type.LackProtection)
        end
        wake_unit(v)
        stop_break(v)
    end
end

-- Stop parties
for _,v in ipairs(df.global.ui.parties) do
    local pos = utils.getBuildingCenter(v.location)
    if is_in_burrows(pos) then
        v.timer = 0
        for _, u in ipairs(v.units) do
            add_thought(unit, df.unit_thought_type.Tired)
        end
    end
end

local place = 'the halls and tunnels'
if #bnames > 0 then
    if #bnames == 1 then
        place = bnames[1]
    else
        place = table.concat(bnames,', ',1,#bnames-1)..' and '..bnames[#bnames]
    end
end
dfhack.gui.showAnnouncement(
    'A loud siren sounds throughout '..place..', waking the sleeping and startling the awake.',
    COLOR_BROWN, true
)
