-- reaction-trigger.lua
-- author expwnent
-- replaces autoSyndrome: trigger commands when custom reactions are completed

local eventful = require 'plugins.eventful'
local syndromeUtil = require 'syndromeUtil'

reactionHooks = reactionHooks or {}

local function getWorkerAndBuilding(job)
 local workerId = -1
 local buildingId = -1
 for _,generalRef in ipairs(job.general_refs) do
  if generalRef:getType() == df.general_ref_type.UNIT_WORKER then
   if workerId ~= -1 then
    print(job)
    printall(job)
    error('reaction-trigger: two workers on same job: ' .. workerId .. ', ' .. generalRef.unit_id)
   else
    workerId = generalRef.unit_id
    if workerId == -1 then
     print(job)
     printall(job)
     error('reaction-trigger: invalid worker')
    end
   end
  elseif generalRef:getType() == df.general_ref_type.BUILDING_HOLDER then
   if buildingId ~= -1 then
    print(job)
    printall(job)
    error('reaction-trigger: two buildings same job: ' .. buildingId .. ', ' .. generalRef.building_id)
   else
    buildingId = generalRef.building_id
    if buildingId == -1 then
     print(job)
     printall(job)
     error('reaction-trigger: invalid building')
    end
   end
  end
 end
 return workerId,buildingId
end

local function processCommand(job, worker, target, building, command)
 local result = {}
 for _,arg in ipairs(command) do
  if arg == '\\WORKER_ID' then
   table.insert(result,''..worker.id)
  elseif arg == '\\TARGET_ID' then
   table.insert(restul,''..target.id)
  elseif arg == '\\BUILDING_ID' then
   table.insert(result,''..building.id)
  elseif arg == '\\LOCATION' then
   table.insert(result,''..job.pos.x)
   table.insert(result,''..job.pos.y)
   table.insert(result,''..job.pos.z)
  elseif arg == '\\REACTION_NAME' then
   table.insert(result,''..job.reaction_name)
  else
   table.insert(result,arg)
  end
 end
 return result
end

eventful.onJobCompleted.reactionTrigger = function(job)
 if job.completion_timer > 0 then
  return
 end
 
-- if job.job_type ~= df.job_type.CustomReaction then
--  --TODO: support builtin reaction triggers if someone asks
--  return
-- end
 
 if not job.reaction_name or job.reaction_name == '' then
  return
 end
-- print('reaction name: ' .. job.reaction_name)
 if not job.reaction_name or not reactionHooks[job.reaction_name] then
  return
 end
 
 local worker,building = getWorkerAndBuilding(job)
 worker = df.unit.find(worker)
 building = df.building.find(building)
 if not worker or not building then
  --this probably means that it finished before EventManager could get a copy of the job while the job was running
  --TODO: consider printing a warning once
  return
 end
 
 local function doAction(action)
  local didSomething
  if action.args then
   local processed = processCommand(job, worker, worker, building, action.args)
   print(dfhack.run_command(table.unpack(processed)))
  end
  if action.syndrome then
   didSomething = syndromeUtil.infectWithSyndromeIfValidTarget(worker, action.syndrome, action.resetPolicy) or didSomething
  end
  if action.workerOnly then
   return
  end
  if didSomething and not action.allowMultipleTargets then
   return
  end
  local function foreach(unit)
   if unit == worker then
    return false
   elseif unit.pos.z ~= building.z then
    return false
   elseif unit.pos.x < building.x1 or unit.pos.x > building.x2 then
    return false
   elseif unit.pos.y < building.y1 or unit.pos.y > building.y2 then
    return false
   else
    if action.args then
     processCommand(job, worker, unit, building, action.args)
    end
    if action.syndrome then
     didSomething = syndrome.infectWithSyndromeIfValidTarget(unit,action.syndrome,action.resetPolicy) or didSomething
    end
    if didSomething and not action.allowMultipleTargets then
     return true
    end
    return false
   end
  end
  for _,unit in ipairs(df.global.world.units.all) do
   if foreach(unit) then
    break
   end
  end
 end
 for _,action in ipairs(reactionHooks[job.reaction_name]) do
  doAction(action)
 end
end

eventful.enableEvent(eventful.eventType.JOB_COMPLETED,0)

local args = {...}
local i=1
local reactionName
local action = {}
action.workerOnly = true
action.allowMultipleTargets = false
--action.args = {}
--action.syndrome, action.resetPolicy, action.workerOnly, action.allowMultipleTargets, action.args
while i <= #args do
 if action.args then
  table.insert(action.args, args[i])
  i = i+1
 else
  if args[i] == '-clear' then
   reactionHooks = {}
   i = i+1
  elseif args[i] == '-reactionName' then
   reactionName = args[i+1]
   i = i+2
  elseif args[i] == '-syndrome' then
   if action.syndrome then
    error('only one syndrome at a time permitted')
   end
   for _,syndrome in ipairs(df.global.world.raws.syndromes.all) do
    if syndrome.syn_name == args[i+1] then
     action.syndrome = syndrome
     break
    end
   end
   if not action.syndrome then
    error('could not find syndrome ' .. args[i+1])
   end
   i = i+2
  elseif args[i] == '-workerOnly' then
   action.workerOnly = args[i+1] == 'true'
   i = i+2
  elseif args[i] == '-allowMultipleTargets' then
   action.allowMultipleTargets = args[i+1] == 'true'
   i = i+2
  elseif args[i] == '-command' then
   action.args = {}
   i = i+1
  elseif args[i] == '-resetPolicy' then
   action.resetPolicy = syndromeUtil.ResetPolicy[args[i+1]]
   if not action.resetPolicy then
    error('invalid reset policy: ' .. args[i+1])
   end
   i = i+2
  else
   error('invalid arguments')
  end
 end
end

if not reactionName then
 return
end

if action.args then
 print(action.args)
 printall(action.args)
end

if not reactionHooks[reactionName] then
 reactionHooks[reactionName] = {}
end
table.insert(reactionHooks[reactionName], action)

