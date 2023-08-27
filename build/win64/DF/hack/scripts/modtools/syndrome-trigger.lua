-- triggers scripts when a syndrome is applied
--author expwnent
local usage = [====[

modtools/syndrome-trigger
=========================
Triggers dfhack commands when syndromes are applied to units.

Arguments::

    -clear
        clear any previously registered syndrome triggers

    -syndrome SYN_NAME
        specify a syndrome by its SYN_NAME
        enclose the name in quotation marks if it includes spaces
        example:
            -syndrome "gila monster bite"

    -synclass SYN_CLASS
        any syndrome with the specified SYN_CLASS will act as a trigger
        enclose in quotation marks if it includes spaces
        example:
            -synclass VAMPCURSE

    -command [ commandStrs ]
        specify the command to be executed after infection
        remember to include a space after/before the square brackets!
        the following may be added to appropriate commands where relevant:
            \\UNIT_ID
                inserts the ID of the infected unit
            \\LOCATION
                inserts the x, y, z coordinates of the infected unit
            \\SYNDROME_ID
                inserts the ID of the syndrome
        note that:
            \\anything -> \anything
            anything -> anything
        examples:
            -command [ full-heal -unit \\UNIT_ID ]
                heals units when they acquire the specified syndrome
            -command [ modtools/spawn-flow -flowType Dragonfire -location [ \\LOCATION ] ]
                spawns dragonfire at the location of infected units

]====]
local eventful = require 'plugins.eventful'
local utils = require 'utils'

onInfection = onInfection or {} --as:{_type:table,_array:{_type:table,_array:{_type:table,command:__arg}}}

eventful.enableEvent(eventful.eventType.UNLOAD,1)
eventful.onUnload.syndromeTrigger = function()
 onInfection = {}
end

eventful.enableEvent(eventful.eventType.SYNDROME,5) --requires iterating through every unit, so not cheap, but not slow either

local function processTrigger(args)
 local command = {} --as:string[]
 for i,arg in ipairs(args.command) do --as:string
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
 for _,args in ipairs(onInfection[syn_id] or {}) do
  processTrigger({
    unit = unit,
    unit_syndrome = unit_syndrome,
    syndrome = syndrome,
    command = args.command
  })
 end
end

------------------------------
--argument processing

local validArgs = utils.invert({
 'clear',
 'help',
 'command',
 'syndrome',
 'synclass',
 'unit'
})

local args = utils.processArgs({...}, validArgs)

if args.help then
 print(usage)
 return
end

if args.clear then
 onInfection = {}
 if not args.command and not args.syndrome then
  return
 end
end

if not args.syndrome and not args.synclass then
 qerror('Syndrome not specified! Enter "modtools/syndrome-trigger -help" for more information.')
end

if not args.command then
 qerror('Command not specified! Enter "modtools/syndrome-trigger -help" for more information.')
end

function processSyndrome(syndrome)
 onInfection[syndrome] = onInfection[syndrome] or {}
 table.insert(onInfection[syndrome], args)
end

local synFound = false
for _,syn in ipairs(df.global.world.raws.syndromes.all) do
 local matchedSyn = false
 if args.syndrome then
  if syn.syn_name == args.syndrome then
   if syndrome then
    qerror ('Multiple syndromes with the same name: ' .. syn.syn_name)
   end
   synFound = true
   matchedSyn = true
   processSyndrome(syn.id)
  end
 end
 if not matchedSyn and args.synclass then -- no need to check for a SYN_CLASS match if the syndrome has already been registered
  for _,synclass in ipairs(syn.syn_class) do
   if synclass.value == args.synclass then
    synFound = true
    processSyndrome(syn.id)
   end
  end
 end
end

if not synFound then
 if args.syndrome then
  qerror ('Invalid syndrome name: '..args.syndrome..'')
 elseif args.synclass then
  qerror ('Invalid syndrome class: '..args.synclass..'')
 end
end
