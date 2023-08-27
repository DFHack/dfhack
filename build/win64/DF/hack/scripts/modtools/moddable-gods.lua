-- Create gods from the command-line
--based on moddableGods by Putnam
--edited by expwnent
local usage = [====[

modtools/moddable-gods
======================
This is a standardized version of Putnam's moddableGods script. It allows you
to create gods on the command-line.

Arguments::

    -name godName
        sets the name of the god to godName
        if there's already a god of that name, the script halts
    -spheres [ sphereList ]
        define a space-separated list of spheres of influence of the god
    -gender male|female|neuter
        sets the gender of the god
    -depictedAs str
        often depicted as a str
    -verbose
        if specified, prints details about the created god

]====]
local utils = require 'utils'

local validArgs = utils.invert({
 'help',
 'name',
 'spheres',
 'gender',
 'depictedAs',
 'verbose',
-- 'entities',
})
local args = utils.processArgs({...}, validArgs)

if args.help then
 print(usage)
 return
end

if not args.name or not args.depictedAs or not args.spheres or not args.gender then
 error('All arguments must be specified.')
end

local templateGod
for _,fig in ipairs(df.global.world.history.figures) do
 if fig.flags.deity then
  templateGod = fig
  break
 end
end
if not templateGod then
 error 'Could not find template god.'
end

local gender
if args.gender == 'male' then
 gender = 1
elseif args.gender == 'female' then
 gender = 0
elseif args.gender == "neuter" then
 gender = -1
else
 error 'invalid gender'
end

local race
for k,v in ipairs(df.global.world.raws.creatures.all) do
    if v.creature_id == args.depictedAs or v.name[0] == args.depictedAs then
        race = k
        break
    end
end
if not race then
  error('invalid race: ' .. args.depictedAs)
end

for _,fig in ipairs(df.global.world.history.figures) do
 if fig.name.first_name == args.name then
  print('god ' .. args.name .. ' already exists. Skipping')
  return
 end
end

local godFig = df.historical_figure:new()
godFig.appeared_year = -1
godFig.born_year = -1
godFig.born_seconds = -1
godFig.curse_year = -1
godFig.curse_seconds = -1
godFig.old_year = -1
godFig.old_seconds = -1
godFig.died_year = -1
godFig.died_seconds = -1
godFig.name.has_name = true
godFig.breed_id = -1
godFig.flags:assign(templateGod.flags)
godFig.id = df.global.hist_figure_next_id
df.global.hist_figure_next_id = 1+df.global.hist_figure_next_id
godFig.info = df.historical_figure_info:new()
godFig.info.spheres = {new=true}
godFig.info.known_info = df.historical_figure_info.T_known_info:new()
godFig.race = race
godFig.caste = 0
godFig.sex = gender
godFig.name.first_name = args.name
for _,sphere in ipairs(args.spheres) do
 godFig.info.spheres.spheres:insert('#',df.sphere_type[sphere])
end
df.global.world.history.figures:insert('#',godFig)

if args.verbose then
  print(godFig.name.first_name .. " created as historical figure " .. tostring(godFig.id))
end
