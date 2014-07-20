--scripts/modtools/skill-change.lua
--author expwnent
--based on skillChange.lua by Putnam
--TODO: update skill level once experience increases/decreases
--TODO: skill rust?

local utils = require 'utils'

validArgs = validArgs or utils.invert({
 'help',
 'skill',
 'mode',
 'value',
 'granularity',
 'unit'
})

mode = mode or utils.invert({
 'add',
 'set',
})

granularity = granularity or utils.invert({
 'experience',
 'level',
})

local args = utils.processArgs({...}, validArgs)

if args.help then
 print([[scripts/modtools/skill-change.lua
arguments
    -help
        print this help message
    -skill skillName
        set the skill that we're talking about
    -mode (add/set)
        are we adding experience/levels or setting them?
    -granularity (experience/levels)
        direct experience, or experience levels?
    -unit id
        id of the target unit
    -value amount
        how much to set/add
]])
 return
end

if not args.unit or not tonumber(args.unit) or not df.unit.find(tonumber(args.unit)) then
 error 'Invalid unit.'
end
args.unit = df.unit.find(tonumber(args.unit))

args.skill = df.job_skill[args.skill]
args.mode = mode[args.mode or 'set']
args.granularity = granularity[args.granularity or 'level']
args.value = tonumber(args.value)

if not args.skill then
 error('invalid skill')
end
if not args.value then
 error('invalid value')
end

local skill
for _,skill_c in ipairs(args.unit.status.current_soul.skills) do
 if skill_c.id == args.skill then
  skill = skill_c
 end
end

if not skill then
 skill = df.unit_skill:new()
 skill.id = args.skill
 args.unit.status.current_soul.skills:insert('#', skill)
end

print('old: ' .. skill.rating .. ': ' .. skill.experience)
if args.granularity == granularity.experience then
 if args.mode == mode.set then
  skill.experience = args.value
 elseif args.mode == mode.add then
  skill.experience = skill.experience + args.value
 else
  error 'bad mode'
 end
elseif args.granularity == granularity.level then
 if args.mode == mode.set then
  skill.rating = df.skill_rating[args.value]
 elseif args.mode == mode.add then
  skill.rating = df.skill_rating[args.value + df.skill_rating[skill.rating]]
 else
  error 'bad mode'
 end
else
 error 'bad granularity'
end

print('new: ' .. skill.rating .. ': ' .. skill.experience)

