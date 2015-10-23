--scripts/modtools/random-trigger.lua
--triggers random scripts
--[[=begin

modtools/random-trigger
=======================
This triggers random dfhack commands with specified probabilities.
Register a few scripts, then tell it to "go" and it will pick one
based on the probability weights you specified.  Outcomes are mutually
exclusive. To make independent random events, call the script multiple
times.

=end]]
local utils = require 'utils'
local eventful = require 'plugins.eventful'

outcomeLists = outcomeLists or {}
randomGen = randomGen or dfhack.random.new()

eventful.enableEvent(eventful.eventType.UNLOAD, 1)
eventful.onUnload.randomTrigger = function()
 outcomeLists = {}
end

validArgs = validArgs or utils.invert({
 'help',
 'command',
 'outcomeListName',
 'weight',
 'seed',
 'trigger',
 'preserveList',
 'withProbability',
 'listOutcomes',
 'clear',
})

local function triggerEvent(outcomeListName)
 local outcomeList = outcomeLists[outcomeListName]
 local r = randomGen:random(outcomeList.total)
 local sum = 0
 --print ('r = ' .. r)
 for i,outcome in ipairs(outcomeList.outcomes) do
  sum = sum + outcome.weight
  if sum > r then
   local temp = outcome.command
   --print('triggering outcome ' .. i .. ': "' .. table.concat(temp, ' ') .. '"')
   --dfhack.run_command(table.unpack(temp))
   dfhack.run_script(table.unpack(temp))
   break
  else
   --print ('sum = ' .. sum .. ' <= r = ' .. r)
  end
 end
 --print('Done.')
 --dfhack.print('\n')
end

local args = utils.processArgs({...}, validArgs)

if args.help then
 print([[scripts/modtools/random-trigger.lua
Allows mutually-exclusive random events. Register a list of scripts along with positive integer relative weights, then tell the script to select one of them with the specified probabilities and run it.
The weights must be positive integers, but they do NOT have to sum to 100 or any other particular number.
The outcomes are mutually exclusive: only one will be triggered.
If you want multiple independent random events, call the script multiple times.
99% of the time, you won't need to worry about this, but just in case, you can specify a name of a list of outcomes to prevent interference from other scripts that call this one.
That also permits situations where you don't know until runtime what outcomes you want.
For example, you could make a reaction-trigger that registers the worker as a mayor candidate, then run this script to choose a random mayor from the list of units that did the mayor reaction.

arguments:
    -help
        print this help message
    -outcomeListName name
        specify the name of this list of outcomes to prevent interference if two scripts are registering outcomes at the same time
        if none is specified, the default outcome list is selected automatically
    -command [ commandStrs ]
        specify the command to be run if this outcome is selected
        must be specified unless the -trigger argument is given
    -weight n
        the relative probability weight of this outcome
        n must be a non-negative integer
        if not specified, n=1 is used by default
    -trigger
        selects a random script based on the specified outcomeList (or the default one if none is specified)
    -preserveList
        when combined with trigger, preserves the list of outcomes so you don't have to register them again
        it is extremely highly recommended that you always specify the outcome list name when you give this command to prevent almost certain interference
            if you want to trigger one of 5 outcomes three times, you might want this option even without -outcomeListName
        most of the time, you won't want this
        will NOT be preserved after the user saves/loads (ask expwnent if you want this: it's not that hard but if nobody wants it I won't bother)
        performance will be slightly faster if you preserve the outcome lists when possible and trigger them multiple times instead of reregistering each time, but the effect should be small
    -withProbability p
        p is a real number between 0 and 1 inclusive
        triggers the command immediately with this probability
    -seed s
        sets the random seed (guarantees the same sequence of random numbers will be produced internally)
        use for debugging purposes
    -listOutcomes
        lists the currently registered list of outcomes of the outcomeList along with their probability weights
        use for debugging purposes
    -clear
        unregister everything
]])
 return
end

if args.clear then
 outcomeLists = {}
end

if args.weight and not tonumber(args.weight) then
 error ('Invalid weight: ' .. args.weight)
end
args.weight = (args.weight and tonumber(args.weight)) or 1
if args.weight ~= math.floor(args.weight) then
 error 'Noninteger weight.'
end
if args.weight < 0 then
 error 'invalid weight: must be non-negative'
end

if args.seed then
 randomGen:init(tonumber(args.seed), 37) --37 is probably excessive and definitely arbitrary
end

args.outcomeListName = args.outcomeListName or ''
args.outcomeListName = 'outcomeList ' .. args.outcomeListName

if args.withProbability then
 args.withProbability = tonumber(args.withProbability)
 if not args.withProbability or args.withProbability < 0 or args.withProbability > 1 then
  error('Invalid withProbability: ' .. (args.withProbability or 'nil'))
 end
 if randomGen:drandom() < args.withProbability then
  dfhack.run_command(table.unpack(args.command))
 end
end

if args.trigger then
 triggerEvent(args.outcomeListName)
 if not args.preserveList then
  outcomeLists[args.outcomeListName] = nil
 end
 return
end

if args.listOutcomes then
 local outcomeList = outcomeLists[args.outcomeListName]
 if not outcomeList then
  print ('No outcomes registered.')
  return
 end
 print ('Total weight: ' .. outcomeList.total)
 for _,outcome in ipairs(outcomeList.outcomes) do
  print(' outcome weight ' .. outcome.weight .. ': ' .. table.concat(outcome.command, ' '))
 end
 print('\n')
 return
end

if not args.command then
 return
end

--actually register
local outcomeList = outcomeLists[args.outcomeListName]
if not outcomeList then
 outcomeLists[args.outcomeListName] = {}
 outcomeList = outcomeLists[args.outcomeListName]
end

outcomeList.total = args.weight + (outcomeList.total or 0)
local outcome = {}
outcome.weight = args.weight
outcome.command = args.command
outcomeList.outcomes = outcomeList.outcomes or {}
table.insert(outcomeList.outcomes, outcome)


