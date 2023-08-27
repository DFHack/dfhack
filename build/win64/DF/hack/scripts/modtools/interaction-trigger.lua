-- triggers scripts based on unit interactions
--author expwnent
local help = [====[

modtools/interaction-trigger
============================
This triggers events when a unit uses an interaction on another. It works by
scanning the announcements for the correct attack verb, so the attack verb
must be specified in the interaction. It includes an option to suppress this
announcement after it finds it.

Usage::

    -clear
        unregisters all triggers
    -onAttackStr str
        trigger the command when the attack verb is "str". both onAttackStr and onDefendStr MUST be specified
    -onDefendStr str
        trigger the command when the defend verb is "str". both onAttackStr and onDefendStr MUST be specified
    -suppressAttack
        delete the attack announcement from the combat logs
    -suppressDefend
        delete the defend announcement from the combat logs
    -command [ commandStrs ]
        specify the command to be executed
        commandStrs
            \\ATTACK_VERB
            \\DEFEND_VERB
            \\ATTACKER_ID
            \\DEFENDER_ID
            \\ATTACK_REPORT
            \\DEFEND_REPORT
            \\anything -> \anything
            anything -> anything

You must specify both an attack string and a defend string to guarantee
correct performance. Either will trigger the script when it happens, but
it will not be triggered twice in a row if both happen.

]====]
local eventful = require 'plugins.eventful'
local utils = require 'utils'

attackTriggers = attackTriggers or {} --as:number[][]
defendTriggers = defendTriggers or {} --as:number[][]
commands = commands or {} --as:{_type:table,_array:{_type:table,onAttackStr:string,onDefendStr:string,command:'string[]',suppressAttack:bool,suppressDefend:bool}}
commandCount = commandCount or 0

eventful.enableEvent(eventful.eventType.INTERACTION,1) --cheap, so every tick is fine
eventful.enableEvent(eventful.eventType.UNLOAD,1)

eventful.onUnload.interactionTrigger = function()
 attackTriggers = {}
 defendTriggers = {}
 commands = {}
 commandCount = 0
end

local function processTrigger(args)
 local command = {} --as:string[]
 for _,arg in ipairs(args.command) do
  if arg == '\\ATTACK_VERB' then
   table.insert(command,args.attackVerb)
  elseif arg == '\\DEFEND_VERB' then
   table.insert(command,args.defendVerb)
  elseif arg == '\\ATTACKER_ID' then
   table.insert(command,tostring(args.attackerId))
  elseif arg == '\\DEFENDER_ID' then
   table.insert(command,tostring(args.defenderId))
  elseif arg == '\\ATTACK_REPORT' then
   table.insert(command,tostring(args.attackReport))
  elseif arg == '\\DEFEND_REPORT' then
   table.insert(command,tostring(args.defendReport))
  elseif string.sub(arg,1,1) == '\\' then
   table.insert(command,string.sub(arg,2))
  else
   table.insert(command,arg)
  end
 end
 dfhack.run_command(table.unpack(command))
end

eventful.onInteraction.interactionTrigger = function(attackVerb, defendVerb, attacker, defender, attackReport, defendReport)
 local suppressAttack = false
 local suppressDefend = false
 local todo = {} --as:bool[]
 for _,trigger in ipairs(attackTriggers[attackVerb] or {}) do
  todo[trigger] = true
 end
 for _,trigger in ipairs(defendTriggers[defendVerb] or {}) do
  todo[trigger] = true
 end
 for k,v in pairs(todo) do
  local command = commands[k]
  suppressAttack = suppressAttack or command.suppressAttack
  suppressDefend = suppressDefend or command.suppressDefend
  command.attackVerb = attackVerb
  command.defendVerb = defendVerb
  command.attackReport = attackReport
  command.defendReport = defendReport
  command.attackerId = attacker
  command.defenderId = defender
  processTrigger(command)
 end

 local eraseReport = function(unit,report)
  for i,v in ipairs(unit.reports.log.Combat) do
   if v == report then
    unit.reports.log.Combat:erase(i)
    break
   end
  end
 end
 if suppressAttack or suppressDefend then
  attacker = df.unit.find(tonumber(attacker)) --luacheck: retype
  defender = df.unit.find(tonumber(defender)) --luacheck: retype
 end
 if suppressAttack then
  eraseReport(attacker,attackReport)
  eraseReport(defender,attackReport)
 end
 if suppressDefend then
  eraseReport(attacker,defendReport)
  eraseReport(defender,defendReport)
 end
  --TODO: get rid of combat report on LHS of screen
end

----------------------------------------------------
--argument processing

local validArgs = utils.invert({
 'clear',
 'help',
 'onAttackStr',
 'onDefendStr',
 'command',
 'suppressAttack',
 'suppressDefend',
})

local args = utils.processArgs({...}, validArgs)

if args.help then
 print(help)
 return
end

if args.clear then
 attackTriggers = {}
 defendTriggers = {}
 commands = {}
 commandCount = 0
end

if not args.command then
 return
end

commands[commandCount] = args

if args.onAttackStr then
 if not attackTriggers[args.onAttackStr] then
  attackTriggers[args.onAttackStr] = {}
 end
 table.insert(attackTriggers[args.onAttackStr],commandCount)
end

if args.onDefendStr then
 if not defendTriggers[args.onDefendStr] then
  defendTriggers[args.onDefendStr] = {}
 end
 table.insert(defendTriggers[args.onDefendStr],commandCount)
end

commandCount = commandCount+1
