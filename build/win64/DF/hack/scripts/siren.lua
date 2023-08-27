-- Wakes up the sleeping, ends parties
--[====[

siren
=====
Wakes up sleeping units and stops parties, either everywhere or in the burrows
given as arguments. In return, adds bad thoughts about noise, tiredness and lack
of protection. The script is intended for emergencies, e.g. when a siege
appears, and all your military is partying.

]====]

local utils = require 'utils'

local args = {...}
local burrows = {} --as:df.burrow[]
local bnames = {} --as:string[]

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

function add_thought(unit, emotion, thought)
    unit.status.current_soul.personality.emotions:insert('#', { new = true,
    type = emotion,
    unk2=1,
    strength=1,
    thought=thought,
    subthought=0,
    severity=0,
    flags={},
    unk7=0,
    year=df.global.cur_year,
    year_tick=df.global.cur_year_tick})
end

function wake_unit(unit)
    local job = unit.job.current_job
    if not job or job.job_type ~= df.job_type.Sleep then
        return
    end

    if job.completion_timer > 0 then
        unit.counters.unconscious = 0
        add_thought(unit, df.emotion_type.Grouchiness, df.unit_thought_type.Drowsy)
    elseif job.completion_timer < 0 then
        add_thought(unit, df.emotion_type.Grumpiness, df.unit_thought_type.Drowsy)
    end

    job.pos:assign(unit.pos)

    job.completion_timer = 0

    unit.path.dest:assign(unit.pos)
    unit.path.path.x:resize(0)
    unit.path.path.y:resize(0)
    unit.path.path.z:resize(0)

    unit.counters.job_counter = 0
end

-- Check siege for thought purpose
for _,v in ipairs(df.global.plotinfo.invasions.list) do
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
            add_thought(v, df.emotion_type.Nervousness, df.unit_thought_type.LackProtection)
        end
        wake_unit(v)
    end
end

-- Stop parties
for _,v in ipairs(df.global.plotinfo.parties) do
    local pos = utils.getBuildingCenter(v.location)
    if is_in_burrows(pos) then
        v.timer = 0
        for _, u in ipairs(v.units) do
            add_thought(u, df.emotion_type.Grumpiness, df.unit_thought_type.Drowsy)
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
