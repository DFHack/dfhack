-- repeatedly call a lua script
-- author expwnent
-- vaguely based on a script by Putnam

local repeatUtil = require 'repeat-util'
local utils = require 'utils'

local validArgs = utils.invert({
 'help',
 'cancel',
 'name',
 'time',
 'timeUnits',
 'command',
 'list'
})

local args = utils.processArgs({...}, validArgs)

if args.help then
 print(usage)
 return
end

if args.cancel then
 repeatUtil.cancel(args.cancel)
 if args.name then
  repeatUtil.cancel(args.name)
 end
elseif args.command then
 local time = tonumber(args.time)

 if not args.name then
  args.name = args.command[1]
 end

 if not args.timeUnits then
  args.timeUnits = 'ticks'
 end

 local callCommand = function()
  dfhack.run_command(table.unpack(args.command))
 end

 repeatUtil.scheduleEvery(args.name,time,args.timeUnits,callCommand)
end

if args.list then
 for k in pairs(repeatUtil.repeating) do
  print(k)
 end
 return
end
