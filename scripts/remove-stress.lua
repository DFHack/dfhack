-- Sets stress to negative one million
-- With unit selected, affects that unit.  Use "remove-stress all" to affect all units.

--By Putnam; http://www.bay12forums.com/smf/index.php?topic=139553.msg5820486#msg5820486

local utils = require 'utils'

validArgs = validArgs or utils.invert({
 'help',
 'all'
})

local args = utils.processArgs({...}, validArgs)

if args.help then
 print([[
remove-stress [-all]
  sets the stress level of every unit to -1000000, or just the selected unit if the '-all' argument is not given
]])
 return
end

if args.all then
 for k,v in ipairs(df.global.world.units.active) do
  v.status.current_soul.personality.stress_level=-1000000
 end
else
 local unit = dfhack.gui.getSelectedUnit()
 if unit then
  unit.status.current_soul.personality.stress_level=-1000000
 else
  error 'Invalid usage: No unit selected and -all argument not given.'
 end
end
