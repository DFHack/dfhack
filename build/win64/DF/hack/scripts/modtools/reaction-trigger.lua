-- trigger commands when custom reactions complete
-- author expwnent
-- replaces autoSyndrome
--@ module = true
local usage = [====[

modtools/reaction-trigger
=========================
Triggers dfhack commands when custom reactions complete, regardless of whether
it produced anything, once per completion.  Arguments::

    -clear
        unregister all reaction hooks
    -reactionName name
        specify the name of the reaction
    -syndrome name
        specify the name of the syndrome to be applied to valid targets
    -allowNonworkerTargets
        allow other units to be targeted if the worker is invalid or ignored
    -allowMultipleTargets
        allow all valid targets within range to be affected
        if absent:
            if running a script, only one target will be used
            if applying a syndrome, then only one target will be infected
    -ignoreWorker
        ignores the worker when selecting the targets
    -dontSkipInactive
        when selecting targets in range, include creatures that are inactive
        dead creatures count as inactive
    -range [ x y z ]
        controls how far elligible targets can be from the workshop
        defaults to [ 0 0 0 ] (on a workshop tile)
        negative numbers can be used to ignore outer squares of the workshop
        line of sight is not respected, and the worker is always within range
    -resetPolicy policy
        the policy in the case that the syndrome is already present
        policy
            NewInstance (default)
            DoNothing
            ResetDuration
            AddDuration
    -command [ commandStrs ]
        specify the command to be run on the target(s)
        special args
            \\WORKER_ID
            \\TARGET_ID
            \\BUILDING_ID
            \\LOCATION
            \\REACTION_NAME
            \\anything -> \anything
            anything -> anything
        when used with -syndrome, the target must be valid for the syndrome
        otherwise, the command will not be run for that target

]====]
local eventful = require 'plugins.eventful'
local syndromeUtil = require 'syndrome-util'
local utils = require 'utils'

reactionHooks = reactionHooks or {} --as:{_type:table,_array:{_type:table,_array:{_type:table,reactionName:__arg,syndrome:__arg,allowNonworkerTargets:__arg,allowMultipleTargets:__arg,resetPolicy:__arg,command:__arg}}}

eventful.enableEvent(eventful.eventType.UNLOAD,1)
eventful.onUnload.reactionTrigger = function()
 reactionHooks = {}
end

local function findSyndrome(name)
 for _,syndrome in ipairs(df.global.world.raws.syndromes.all) do
  if syndrome.syn_name == name then
   return syndrome
  end
 end
end

function getWorkerAndBuilding(job)
 local workerId = -1
 local buildingId = -1
 for _,generalRef in ipairs(job.general_refs) do
  if generalRef:getType() == df.general_ref_type.UNIT_WORKER then
   if workerId ~= -1 then
    print(job)
    printall(job)
    error('reaction-trigger: two workers on same job: ' .. workerId .. ', ' .. generalRef.unit_id) --hint:df.general_ref_unit_workerst
   else
    workerId = generalRef.unit_id --hint:df.general_ref_unit_workerst
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
    error('reaction-trigger: two buildings same job: ' .. buildingId .. ', ' .. generalRef.building_id) --hint:df.general_ref_building_holderst
   else
    buildingId = generalRef.building_id --hint:df.general_ref_building_holderst
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
 local result = {} --as:string[]
 for _,arg in ipairs(command) do
  if arg == '\\WORKER_ID' then
   table.insert(result,''..worker.id)
  elseif arg == '\\TARGET_ID' then
   table.insert(result,''..target.id)
  elseif arg == '\\BUILDING_ID' then
   table.insert(result,''..building.id)
  elseif arg == '\\LOCATION' then
   table.insert(result,''..job.pos.x)
   table.insert(result,''..job.pos.y)
   table.insert(result,''..job.pos.z)
  elseif arg == '\\REACTION_NAME' then
   table.insert(result,''..job.reaction_name)
  elseif string.sub(arg,1,1) == '\\' then
   table.insert(result,string.sub(arg,2))
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

 local worker_id,building_id = getWorkerAndBuilding(job)
 local worker = df.unit.find(worker_id)
 local building = df.building.find(building_id)
 if not worker or not building then
  --this probably means that it finished before EventManager could get a copy of the job while the job was running
  --TODO: consider printing a warning once
  return
 end

 local function doAction(action)
  local didSomething = false
  if action.syndrome and not action.ignoreWorker then
   local syndrome = findSyndrome(action.syndrome)
   if syndrome then
    didSomething = syndromeUtil.infectWithSyndromeIfValidTarget(worker, syndrome, syndromeUtil.ResetPolicy[action.resetPolicy]) or didSomething
   end
  end
  if action.command and not action.ignoreWorker and (not action.syndrome or didSomething) then
   local processed = processCommand(job, worker, worker, building, action.command)
   dfhack.run_command(table.unpack(processed))
   didSomething = true
  end
  if didSomething and not action.allowMultipleTargets then
   return
  end
  if not action.allowNonworkerTargets and not action.allowMultipleTargets then
   return
  end
  local function foreach(unit)
   local didSomething = false
   if unit == worker or not (action.dontSkipInactive or dfhack.units.isActive(unit)) then
    return false
   end
   local xRange, yRange, zRange = 0, 0, 0
   if action.range then
     xRange,yRange,zRange = tonumber(action.range[1]), tonumber(action.range[2]), tonumber(action.range[3])
   end
   if unit.pos.z < building.z - zRange or unit.pos.z > building.z + zRange then
    return false
   elseif unit.pos.x < building.x1 - xRange or unit.pos.x > building.x2 + xRange then
    return false
   elseif unit.pos.y < building.y1 - yRange or unit.pos.y > building.y2 + yRange then
    return false
   else
    if action.syndrome then
     local syndrome = findSyndrome(action.syndrome)
     if syndrome then
      didSomething = syndromeUtil.infectWithSyndromeIfValidTarget(unit,syndrome,syndromeUtil.ResetPolicy[action.resetPolicy]) or didSomething
     end
    end
    if action.command and ( (not action.syndrome) or didSomething ) then
     local processed = processCommand(job, worker, unit, building, action.command)
     dfhack.run_command(table.unpack(processed))
     didSomething = true
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
eventful.enableEvent(eventful.eventType.JOB_COMPLETED,0) --0 is necessary to catch cancelled jobs and not trigger them

local validArgs = utils.invert({
 'help',
 'clear',
 'reactionName',
 'syndrome',
 'command',
 'allowNonworkerTargets',
 'allowMultipleTargets',
 'range',
 'ignoreWorker',
 'dontSkipInactive',
 'resetPolicy'
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
 reactionHooks = {}
end

if not args.reactionName then
 return
end

if not reactionHooks[args.reactionName] then
 reactionHooks[args.reactionName] = {}
end

if args.syndrome and not findSyndrome(args.syndrome) then
 error('Could not find syndrome ' .. args.syndrome)
end

table.insert(reactionHooks[args.reactionName], args)
