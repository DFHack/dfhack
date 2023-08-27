-- trigger commands when projectiles hit targets
--author expwnent
--based on Putnam's projectileExpansion
--TODO: trigger based on contaminants
local usage = [====[

modtools/projectile-trigger
===========================
This triggers dfhack commands when projectiles hit their targets.  Usage::

    -clear
        unregister all triggers
    -material
        specify a material for projectiles that will trigger the command
        examples:
            INORGANIC:IRON
            CREATURE_MAT:DWARF:BRAIN
            PLANT_MAT:MUSHROOM_HELMET_PLUMP:DRINK
    -command [ commandList ]
        \\LOCATION
        \\PROJECTILE_ID
        \\FIRER_ID
        \\anything -> \anything
        anything -> anything

]====]
local eventful = require 'plugins.eventful'
local utils = require 'utils'

materialTriggers = materialTriggers or {} --as:{_type:table,command:__arg,material:__arg}[][]

eventful.enableEvent(eventful.eventType.UNLOAD,1)
eventful.onUnload.projectileTrigger = function()
 materialTriggers = {}
end

function processTrigger(args)
 local command2 = {} --as:string[]
 for _,arg in ipairs(args.command) do
  if arg == '\\LOCATION' then
   table.insert(command2,tostring(args.pos.x))
   table.insert(command2,tostring(args.pos.y))
   table.insert(command2,tostring(args.pos.z))
  elseif arg == '\\PROJECTILE_ID' then
   table.insert(command2,tostring(args.projectile.id))
  elseif arg == '\\FIRER_ID' then
   table.insert(command2,tostring(args.projectile.firer.id))
  elseif string.sub(arg,1,1) == '\\' then
   table.insert(command2,string.sub(arg,2))
  else
   table.insert(command2,arg)
  end
 end
 dfhack.run_command(table.unpack(command2))
end

eventful.onProjItemCheckImpact.expansion = function(projectile)
 local matStr = dfhack.matinfo.decode(projectile.item):getToken()
 for _,args in ipairs(materialTriggers[matStr] or {}) do
  processTrigger({
   pos = projectile.cur_pos,
   projectile = projectile,
   item = projectile.item,
   command = args.command,
   material = args.material
  })
 end
end

local validArgs = utils.invert({
 'help',
 'clear',
 'command',
 'material',
})

local args = utils.processArgs({...}, validArgs)

if args.help then
 print(usage)
 return
end

if args.clear then
 materialTriggers = {}
end

if not args.command then
 return
end

if not args.material then
 error 'specify a material'
end

if not dfhack.matinfo.find(args.material) then
 error ('invalid material: ' .. args.material)
end

if not materialTriggers[args.material] then
 materialTriggers[args.material] = {}
end
table.insert(materialTriggers[args.material], args)
