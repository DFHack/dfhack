-- scripts/modtools/reaction-product-trigger.lua
-- author expwnent
-- trigger commands just before and after custom reactions produce items
--@ module = true
--[[=begin

modtools/reaction-product-trigger
=================================
This triggers dfhack commands when reaction products are produced, once per
product.

=end]]
local eventful = require 'plugins.eventful'
local utils = require 'utils'

--TODO: onUnload
productHooks = productHooks or {}

reactionInputItems = reactionInputItems

function preserveReagents()
 reactionInputItems:resize(0)
end

eventful.enableEvent(eventful.eventType.UNLOAD,1)
eventful.onUnload.reactionProductTrigger = function()
 productHooks = {}
end

--productHooks.before = productHooks.before or {}
--productHooks.after = productHooks.after or {}

local function processArgs(args, reaction, reaction_product, unit, input_items, input_reagents, output_items, buildingId)
 local result = {}
 for _,arg in ipairs(args) do
  if arg == '\\WORKER_ID' then
   table.insert(result,tostring(unit.id))
  elseif arg == '\\REACTION' then
   table.insert(result,reaction.code)
--  elseif arg == '\\REACTION_PRODUCT' then
--   table.insert(result,reaction_product)
  elseif arg == '\\INPUT_ITEMS' then
   --table.insert(result,'[')
   for _,item in ipairs(input_items) do
    table.insert(result,tostring(item.id))
   end
   --table.insert(result,']')
  elseif arg == '\\OUTPUT_ITEMS' then
   --table.insert(result,'[')
   for _,item in ipairs(output_items) do
    table.insert(result,tostring(item.id))
   end
   --table.insert(result,']')
  elseif arg == '\\BUILDING_ID' then
   table.insert(result,tostring(buildingId))
  elseif string.sub(arg,1,1) == '\\' then
   table.insert(result,string.sub(arg,2))
  else
   table.insert(result,arg)
  end
 end
 return result
end

local function afterProduce(reaction,reaction_product,unit,input_items,input_reagents,output_items)
 --printall(unit.job.current_job)
 local _,buildingId = dfhack.script_environment('modtools/reaction-trigger').getWorkerAndBuilding(unit.job.current_job)
 for _,hook in ipairs(productHooks[reaction.code] or {}) do
  local command = hook.command
  local processed = processArgs(command, reaction, reaction_product, unit, input_items, input_reagents, output_items, buildingId)
  dfhack.run_command(table.unpack(processed))
 end
end

eventful.onReactionComplete.reactionProductTrigger = function(reaction,reaction_product,unit,input_items,input_reagents,output_items,call_native)
 reactionInputItems = input_items
 --print(reaction.code)
 --print(#output_items)
 --print('call_native exists? ' .. tostring(not not call_native))
 --print('\n')
 if call_native then
  --beforeProduce(reaction,unit,input_items,input_reagents,output_items,call_native)
 else
  afterProduce(reaction,reaction_product,unit,input_items,input_reagents,output_items)
 end
 reactionInputItems = nil
end

validArgs = validArgs or utils.invert({
 'help',
 'clear',
 'reactionName',
 'command',
})

if moduleMode then
 return
end

local args = {...} or {}
args = utils.processArgs(args, validArgs)

if args.help then
 print([[scripts/modtools/reaction-product-trigger.lua
arguments:
    -help
        print this help message
    -clear
        unregister all reaction hooks
    -reactionName name
        specify the name of the reaction
    -command [ commandStrs ]
        specify the command to be run on the target(s)
        special args
            \\WORKER_ID
            \\REACTION
            \\BUILDING_ID
            \\LOCATION
            \\INPUT_ITEMS
            \\OUTPUT_ITEMS
            \\anything -> \anything
            anything -> anything
]])
 return
end

if args.clear then
 productHooks = {}
end

if not args.reactionName then
 error('No reactionName.')
end

if not args.command then
 error('No command.')
end

productHooks[args.reactionName] = productHooks[args.reactionName] or {}
table.insert(productHooks[args.reactionName], args)

