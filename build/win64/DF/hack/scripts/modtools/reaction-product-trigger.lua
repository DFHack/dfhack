-- trigger commands before/after reactions produce items
-- author expwnent
--@ module = true
local usage = [====[

modtools/reaction-product-trigger
=================================
This triggers dfhack commands when reaction products are produced, once per
product.  Usage::

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

]====]
local eventful = require 'plugins.eventful'
local utils = require 'utils'

--TODO: onUnload
productHooks = productHooks or {} --as:{_type:table,_array:{_type:table,_array:{_type:table,clear:__arg,reactionName:__arg,command:__arg}}}

eventful.enableEvent(eventful.eventType.UNLOAD,1)
eventful.onUnload.reactionProductTrigger = function()
 productHooks = {}
end

--productHooks.before = productHooks.before or {}
--productHooks.after = productHooks.after or {}

local function processArgs(args, reaction, reaction_product, unit, input_items, input_reagents, output_items, buildingId)
 local result = {} --as:string[]
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
   if not buildingId then
    error('BUILDING_ID is not supported for adventure mode reactions')
   end
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
 --adv mode reactions have no associated building.
 local _,buildingId
 if unit.job.current_job then
  _,buildingId = dfhack.script_environment('modtools/reaction-trigger').getWorkerAndBuilding(unit.job.current_job)
 end
 for _,hook in ipairs(productHooks[reaction.code] or {}) do
  local command = hook.command
  local processed = processArgs(command, reaction, reaction_product, unit, input_items, input_reagents, output_items, buildingId)
  dfhack.run_command(table.unpack(processed))
 end
end

eventful.onReactionComplete.reactionProductTrigger = function(reaction,reaction_product,unit,input_items,input_reagents,output_items)
 afterProduce(reaction,reaction_product,unit,input_items,input_reagents,output_items)
end

local validArgs = utils.invert({
 'help',
 'clear',
 'reactionName',
 'command',
})

if moduleMode then
 return
end

local args = utils.processArgs({...}, validArgs)

if args.help then
 print(usage)
 return
end

if args.clear then
 productHooks = {}
 return
end

if not args.reactionName then
 error('No reactionName.')
end

if not args.command then
 error('No command.')
end

productHooks[args.reactionName] = productHooks[args.reactionName] or {}
table.insert(productHooks[args.reactionName], args)
