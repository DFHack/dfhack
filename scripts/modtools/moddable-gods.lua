--scripts/modtools/moddable-gods.lua
--based on moddableGods by Putnam
--edited by expwnent
--[[=begin

modtools/moddable-gods
======================
This is a standardized version of Putnam's moddableGods script. It allows you
to create gods on the command-line.

=end]]
local utils = require 'utils'

validArgs = validArgs or utils.invert({
 'help',
 'name',
 'spheres',
 'gender',
 'depictedAs',
 'domain',
 'description',
-- 'entities',
})
local args = utils.processArgs({...})

if args.help then
 print([[scripts/modtools/moddable-gods.lua
arguments:
    -help
        print this help message
    -name godName
        sets the name of the god to godName
        if there's already a god of that name, the script halts
    -spheres [ sphereList ]
        define a space-separated list of spheres of influence of the god
    -depictedAs str
        often depicted as a str
    -domain str
        set the domain of the god
    -description str
        set the description of the god
]])
 return
end

if not args.name or not args.depictedAs or not args.domain or not args.description or not args.spheres or not args.gender then
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

if args.gender == 'male' then
 args.gender = 1
elseif args.gender == 'female' then
 args.gender = 0
else
 error 'invalid gender'
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
godFig.info.secret = df.historical_figure_info.T_secret:new()

godFig.sex = args.gender
godFig.name.first_name = args.name
for _,sphere in ipairs(args.spheres) do
 godFig.info.spheres:insert('#',df.sphere_type[sphere])
end
df.global.world.history.figures:insert('#',godFig)


