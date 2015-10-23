-- scripts/modtools/reaction-trigger-transition.lua
-- author expwnent
--[[=begin

modtools/reaction-trigger-transition
====================================
Scans raw files and creates a file to help modders transition from
autoSyndrome to reaction-trigger.

Prints useful things to the console and a file to help modders
transition from autoSyndrome to reaction-trigger.  This script
is basically an apology for breaking backward compatibiility,
and will be removed eventually.

=end]]
local function maybeQuote(str)
 if str == '' or string.find(str,' ') then
  return ('"' .. str .. '"')
 else
  return str
 end
end

warnings = ''
output = ''
for _,reaction in ipairs(df.global.world.raws.reactions) do
 local function foreachProduct(product)
  local prodType = product:getType()
  if prodType ~= df.reaction_product_type.item then
   return
  end
  if product.item_type ~= df.item_type.BOULDER then
   return
  end
  if product.mat_index < 0 then
   return
  end
  local inorganic = df.global.world.raws.inorganics[product.mat_index]
  local didInorganicName
  for _,syndrome in ipairs(inorganic.material.syndrome) do
   local workerOnly = true
   local allowMultipleTargets = false;
   local command
   local commandStr
   local destroyRock = true;
   local foundAutoSyndrome = false;
   local resetPolicy;
   for i,synclass in ipairs(syndrome.syn_class) do
    synclass = synclass.value
    if false then
    elseif synclass == '\\AUTO_SYNDROME' then
     foundAutoSyndrome = true
    elseif synclass == '\\ALLOW_NONWORKER_TARGETS' then
     workerOnly = false
    elseif synclass == '\\ALLOW_MULTIPLE_TARGETS' then
     allowMultipleTargets = true
    elseif synclass == '\\PRESERVE_ROCK' then
     destroyRock = false
    elseif synclass == '\\RESET_POLICY DoNothing' then
     resetPolicy = 'DoNothing'
    elseif synclass == '\\RESET_POLICY ResetDuration' then
     resetPolicy = 'ResetDuration'
    elseif synclass == '\\RESET_POLICY AddDuration' then
     resetPolicy = 'AddDuration'
    elseif synclass == '\\RESET_POLICY NewInstance' then
     resetPolicy = 'NewInstance'
    elseif synclass == '\\COMMAND' then
     command = ''
    elseif command then
     if synclass == '\\LOCATION' then
      command = command .. '\\LOCATION '
     elseif synclass == '\\WORKER_ID' then
      command = command .. '\\WORKER_ID '
     elseif synclass == '\\REACTION_INDEX' then
      warnings = warnings .. ('Warning: \\REACTION_INDEX is deprecated. Use \\REACTION_NAME instead.\n')
      command = command .. '\\REACTION_NAME '
     else
      commandStr = true
      command = command .. maybeQuote(synclass) .. ' '
     end
    end
   end
   if foundAutoSyndrome then
    if destroyRock then
     warnings = warnings .. ('Warning: instead of destroying the rock, do not produce it in the first place.\n')
    end
    if workerOnly then
     workerOnly = 'true'
    else
     workerOnly = 'false'
    end
    if allowMultipleTargets then
     allowMultipleTargets = 'true'
    else
     allowMultipleTargets = 'false'
    end
    local reactionTriggerStr = 'modtools/reaction-trigger -reactionName ' .. maybeQuote(reaction.code) --.. '"'
    if workerOnly ~= 'true' then
     reactionTriggerStr = reactionTriggerStr .. ' -workerOnly ' .. workerOnly
    end
    if allowMultipleTargets ~= 'false' then
     reactionTriggerStr = reactionTriggerStr .. ' -allowMultipleTargets ' .. allowMultipleTargets
    end
    if resetPolicy and resetPolicy ~= 'NewInstance' then
     reactionTriggerStr = reactionTriggerStr .. ' -resetPolicy ' .. resetPolicy
    end
    if #syndrome.ce > 0 then
     if syndrome.syn_name == '' then
      warnings = warnings .. ('Warning: give this syndrome a name!\n')
     end
     reactionTriggerStr = reactionTriggerStr .. ' -syndrome ' .. maybeQuote(syndrome.syn_name) .. ''
    end
    if command and commandStr then
     reactionTriggerStr = reactionTriggerStr .. ' -command [ ' .. command .. ']'
    end
    if (not command or command == '') and (not syndrome.syn_name or syndrome.syn_name == '') then
     --output = output .. '#'
    else
     if not didInorganicName then
--      output = output .. '# ' .. (inorganic.id) .. '\n'
      didInorganicName = true
     end
     output = output .. (reactionTriggerStr) .. '\n'
    end
   end
  end
 end
 for _,product in ipairs(reaction.products) do
  foreachProduct(product)
 end
end

print(warnings)
print('\n\n\n\n')
print(output)
local file = io.open('reaction-trigger-transition.txt', 'w+')
--io.output(file)
--file:write(warnings)
--file:write('\n\n\n\n')
file:write(output)
file:flush()
--io.flush(file)
io.close(file)
--io.output()
print('transition information written to reaction-trigger-transition.txt')


