-- repeat.lua
-- repeatedly calls a lua script, eg "repeat -time 1 months -command cleanowned"; to disable "repeat -cancel cleanowned"
-- repeat -help for details
-- author expwnent
-- vaguely based on a script by Putnam

local repeatUtil = require 'plugins.repeatUtil'

local args = {...}
if args[1] == '-cancel' then
 repeatUtil.cancel(args[2])
 return
elseif args[1] == '-help' then
 print([[repeat.lua
 repeat -help
  print this help message
 repeat -cancel bob
  cancels the repetition with the name bob
 repeat -name jim -time delay timeUnits -printResult true -command printArgs 3 1 2
  except for -command, arguments can go in any order
  -name sets the name for the purposes of cancelling and making sure you don't schedule the same repeating event twice
    if not specified, it's set to the first argument after -command
  -time delay timeUnits
   delay is some positive integer
   timeUnits is some valid time unit for dfhack.timeout(delay,timeUnits,function)
  -printResult true
   print the results of the command
  -printResult false
   suppress the results of the command
  -command ...
   specify the command to be run
 ]])
end

local name=nil
local time
local timeUnits
local i=1
local command={}
local printResult=true
while i <= #args do
 if args[i] == '-name' then
  name = args[i+1]
  i = i + 2
 elseif args[i] == '-time' then
  time = tonumber(args[i+1])
  timeUnits = args[i+2]
  i = i+3
 elseif args[i] == '-command' then
  name = name or args[i+1]
  for j=i+1,#args,1 do
   table.insert(command,args[j])
  end
  break
 elseif args[i] == '-printResult' then
  if args[i+1] == "true" then
   printOutput = true
  elseif args[i+1] == "false" then
   printOutput = false
  else
   qerror("repeat -printResult " .. args[i+1] .. ": expected true or false")
  end
  i = i+2
 else
  qerror('Improper arguments to repeat.')
 end
end

repeatUtil.scheduleEvery(name,time,timeUnits,function()
 result = dfhack.run_command(table.unpack(command))
 if printResult then
  print(result)
 end
end)

