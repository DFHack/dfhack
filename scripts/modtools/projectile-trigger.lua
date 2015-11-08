--scripts/modtools/projectile-trigger.lua
--author expwnent
--based on Putnam's projectileExpansion
--TODO: trigger based on contaminants
--[[=begin

modtools/projectile-trigger
===========================
This triggers dfhack commands when projectiles hit their targets.

=end]]
local eventful = require 'plugins.eventful'
local utils = require 'utils'

materialTriggers = materialTriggers or {}

eventful.enableEvent(eventful.eventType.UNLOAD,1)
eventful.onUnload.projectileTrigger = function()
 materialTriggers = {}
end

function processTrigger(args)
 local command2 = {}
 for _,arg in ipairs(args.command) do
  if arg == '\\LOCATION' then
   table.insert(command2,args.pos.x)
   table.insert(command2,args.pos.y)
   table.insert(command2,args.pos.z)
  elseif arg == '\\PROJECTILE_ID' then
   table.insert(command2,args.projectile.id)
  elseif arg == '\\FIRER_ID' then
   table.insert(command2,args.projectile.firer.id)
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
 local table = {}
 table.pos = projectile.cur_pos
 table.projectile = projectile
 table.item = projectile.item
 for _,args in ipairs(materialTriggers[matStr] or {}) do
  utils.fillTable(args,table)
  processTrigger(args)
  utils.unfillTable(args,table)
 end
end

validArgs = validArgs or utils.invert({
 'help',
 'clear',
 'command',
 'material',
})

local args = utils.processArgs({...}, validArgs)

if args.help then
 print([[scripts/modtools/projectile-trigger.lua
arguments
    -help
        print this help message
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
]])
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


