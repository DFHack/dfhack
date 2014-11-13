-- scripts/repeat.lua
-- repeatedly calls a lua script, eg "repeat -time 1 months -command cleanowned"; to disable "repeat -cancel cleanowned"
-- repeat -help for details
-- author expwnent
-- vaguely based on a script by Putnam

local repeatUtil = require 'repeat-util'
local utils = require 'utils'

validArgs = validArgs or utils.invert({
 'help',
 'cancel',
 'name',
 'time',
 'timeUnits',
 'command'
})

local args = utils.processArgs({...}, validArgs)

if args.help then
 print([[repeat.lua
 repeat -help
  print this help message
 repeat -cancel bob
  cancels the repetition with the name bob
 repeat -name jim -time delay -timeUnits units -printResult true -command [ printArgs 3 1 2 ]
  -name sets the name for the purposes of cancelling and making sure you don't schedule the same repeating event twice
    if not specified, it's set to the first argument after -command
  -time delay -timeUnits units
   delay is some positive integer
   units is some valid time unit for dfhack.timeout(delay,timeUnits,function)
  -command [ ... ]
   specify the command to be run
 ]])
 return
end

if args.cancel then
 repeatUtil.cancel(args.cancel)
 if args.name then
  repeatUtil.cancel(args.name)
 end
 return
end

args.time = tonumber(args.time)
if not args.name then
 args.name = args.command[1]
end

if not args.timeUnits then
 args.timeUnits = 'ticks'
end

local callCommand = function()
 dfhack.run_command(table.unpack(args.command))
end

repeatUtil.scheduleEvery(args.name,args.time,args.timeUnits,callCommand)

