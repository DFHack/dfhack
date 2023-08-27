-- Sets or modifies a skill of a unit
--author expwnent
--based on skillChange.lua by Putnam
--TODO: skill rust?
local help = [====[

modtools/skill-change
=====================
Sets or modifies a skill of a unit.  Args:

:-skill skillName:  set the skill that we're talking about
:-mode (add/set):   are we adding experience/levels or setting them?
:-granularity (experience/level):
                    direct experience, or experience levels?
:-unit id:          id of the target unit
:-value amount:     how much to set/add
:-loud:             if present, prints changes to console
]====]
local utils = require 'utils'

local validArgs = utils.invert({
    'help',
    'skill',
    'mode',
    'value',
    'granularity',
    'unit',
    'loud',
})

local modes = utils.invert({
    'add',
    'set',
})

local granularities = utils.invert({
    'experience',
    'level',
})

local args = utils.processArgs({...}, validArgs)

if args.help then
    print(help)
    return
end

if not args.unit or not tonumber(args.unit) or not df.unit.find(tonumber(args.unit)) then
    error 'Invalid unit.'
end
local unit = df.unit.find(tonumber(args.unit))

local job_skill = df.job_skill[args.skill] --as:df.job_skill
local mode = modes[args.mode or 'set']
local granularity = granularities[args.granularity or 'level']
local value = tonumber(args.value)

if not job_skill then
    error('invalid skill')
end
if not value then
    error('invalid value')
end

local skill
for _,skill_c in ipairs(unit.status.current_soul.skills) do
    if skill_c.id == job_skill then
        skill = skill_c
    end
end

if not skill then
    skill = df.unit_skill:new()
    skill.id = job_skill
    utils.insert_sorted(unit.status.current_soul.skills,skill,'id')
end

if args.loud then
    print('old: ' .. skill.rating .. ': ' .. skill.experience)
end

if granularity == granularities.experience then
    if mode == modes.set then
        skill.experience = value
    elseif mode == modes.add then
    -- Changing of skill levels when experience increases/decreases hacked in by Atkana
    -- https://github.com/DFHack/scripts/pull/27
        local function nextCost(rating)
            if rating == 0 then
                return 1
            else
                return (400 + (100 * rating))
            end
        end
        local newExp = skill.experience + value
        if (newExp < 0) or (newExp > nextCost(skill.rating+1)) then
            if newExp > 0 then --positive
                repeat
                    newExp = newExp - nextCost(skill.rating+1)
                    skill.rating = skill.rating + 1
                until newExp < nextCost(skill.rating)
            else -- negative
                repeat
                    newExp = newExp + nextCost(skill.rating)
                    skill.rating = math.max(skill.rating - 1, 0)
                until (newExp >= 0) or skill.rating == 0
                -- hack because I can't maths. Will only happen if loop stopped because skill was 0
                if newExp < 0 then newExp = 0 end
            end
        end
        -- Update exp
        skill.experience = newExp
    else
        error 'bad mode'
    end
elseif granularity == granularities.level then
    if mode == modes.set then
        skill.rating = value
    elseif mode == modes.add then
        skill.rating = value + skill.rating
    else
        error 'bad mode'
    end
else
    error 'bad granularity'
end

if args.loud then
    print('new: ' .. skill.rating .. ': ' .. skill.experience)
end
