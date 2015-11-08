--scripts/modtools/syndrome-trigger.lua
--author expwnent
--triggers scripts when units are infected with syndromes
--[[=begin

modtools/syndrome-trigger
=========================
Triggers dfhack commands when syndromes are applied to units.

=end]]
local eventful = require 'plugins.eventful'
local utils = require 'utils'

onInfection = onInfection or {}

eventful.enableEvent(eventful.eventType.UNLOAD,1)
eventful.onUnload.syndromeTrigger = function()
 onInfection = {}
end

eventful.enableEvent(eventful.eventType.SYNDROME,5) --requires iterating through every unit, so not cheap, but not slow either

local function processTrigger(args)
 local command = {}
 for i,arg in ipairs(args.command) do
  if arg == '\\SYNDROME_ID' then
   table.insert(command, '' .. args.syndrome.id)
  elseif arg == '\\UNIT_ID' then
   table.insert(command, '' .. args.unit.id)
  elseif arg == '\\LOCATION' then
   table.insert(command, '' .. args.unit.pos.x)
   table.insert(command, '' .. args.unit.pos.y)
   table.insert(command, '' .. args.unit.pos.z)
  elseif string.sub(arg,1,1) == '\\' then
   table.insert(command, string.sub(arg,2))
  else
   table.insert(command, arg)
  end
 end
 dfhack.run_command(table.unpack(command))
end

eventful.onSyndrome.syndromeTrigger = function(unitId, syndromeIndex)
 local unit = df.unit.find(unitId)
 local unit_syndrome = unit.syndromes.active[syndromeIndex]
 local syn_id = unit_syndrome['type']
 if not onInfection[syn_id] then
  return
 end
 local syndrome = df.syndrome.find(syn_id)
 local table = {}
 table.unit = unit
 table.unit_syndrome = unit_syndrome
 table.syndrome = syndrome
 for _,args in ipairs(onInfection[syn_id] or {}) do
  utils.fillTable(args,table)
  processTrigger(args)
  utils.unfillTable(args,table)
 end
end

------------------------------
--argument processing

validArgs = validArgs or utils.invert({
 'clear',
 'help',
 'command',
 'syndrome'
})

local args = utils.processArgs({...}, validArgs)

if args.help then
 print([[scripts/modtools/syndrome-trigger.lua
arguments
    -help
        print this help message
    -clear
        clear all triggers
    -syndrome name
        specify the name of a syndrome
    -command [ commandStrs ]
        specify the command to be executed after infection
        args
            \\SYNDROME_ID
            \\UNIT_ID
            \\LOCATION
            \\anything -> \anything
            anything -> anything
]])
 return
end

if args.clear then
 onInfection = {}
end

if not args.command then
 return
end

if not args.syndrome then
 error 'Select a syndrome.'
end

local syndrome
for _,syn in ipairs(df.global.world.raws.syndromes.all) do
 if syn.syn_name == args.syndrome then
  if syndrome then
   error ('Multiple syndromes with same name: ' .. syn.syn_name)
  end
  syndrome = syn.id
 end
end

if not syndrome then
 error ('Could not find syndrome named ' .. args.syndrome)
end

onInfection[syndrome] = onInfection[syndrome] or {}
table.insert(onInfection[syndrome], args)

