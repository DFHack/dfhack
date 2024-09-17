local _ENV = mkmodule('plugins.autobutcher.nestboxesEvent')
---------------------------------------------------------------------------------------------------
local nestboxesCommon = require('plugins.autobutcher.common')
local printLocal = nestboxesCommon.printLocal
local printDetails = nestboxesCommon.printDetails
local utils = require('utils')
---------------------------------------------------------------------------------------------------
local function copyEggFields(source_egg, target_egg)
  printDetails('start copyEggFields')
  target_egg.incubation_counter = source_egg.incubation_counter
  target_egg.egg_flags = utils.clone(source_egg.egg_flags)
  target_egg.hatchling_flags1 = utils.clone(source_egg.hatchling_flags1)
  target_egg.hatchling_flags2 = utils.clone(source_egg.hatchling_flags2)
  target_egg.hatchling_flags3 = utils.clone(source_egg.hatchling_flags3)
  target_egg.hatchling_flags4 = utils.clone(source_egg.hatchling_flags4)
  target_egg.hatchling_training_level = source_egg.hatchling_training_level
  target_egg.hatchling_mother_id = source_egg.hatchling_mother_id
  target_egg.mother_hf = source_egg.mother_hf
  target_egg.father_hf = source_egg.mother_hf
  target_egg.mothers_caste = source_egg.mothers_caste
  target_egg.fathers_caste = source_egg.fathers_caste

  local mothers_genes = df.unit_genes:new()
  mothers_genes.appearance:assign(source_egg.mothers_genes.appearance)
  mothers_genes.colors:assign(source_egg.mothers_genes.colors)

  local fathers_genes  = df.unit_genes:new()
  fathers_genes.appearance:assign(source_egg.fathers_genes.appearance)
  fathers_genes.colors:assign(source_egg.fathers_genes.colors)

  target_egg.mothers_genes = mothers_genes
  target_egg.fathers_genes = fathers_genes
  printDetails("mothers_genes fathers_genes done")

  target_egg.hatchling_civ_id = source_egg.hatchling_civ_id
  printDetails('hatchling_civ_id done')
  printDetails('end copyEggFields')
end --copyEggFields
---------------------------------------------------------------------------------------------------
local function resizeEggStack(egg_stack, new_stack_size)
  printDetails('start resizeEggStack')
  egg_stack.stack_size = new_stack_size
  --TODO check if weight or size need adjustment
  printDetails('end resizeEggStack')
end --resizeEggStack
---------------------------------------------------------------------------------------------------
local function createNewEggStack(original_eggs, new_stack_count)
  printDetails('start createNewEggStack')
  printDetails('about to create new egg stack')
  printDetails(('type= %s'):format(original_eggs:getType()))
  printDetails(('creature= %s'):format(original_eggs.race))
  printDetails(('caste= %s '):format(original_eggs.caste))
  printDetails(('stack size for new eggs = %s '):format(new_stack_count))

  local created_items =
  dfhack.items.createItem(
    df.unit.find(original_eggs.hatchling_mother_id),
    original_eggs:getType(),
    -1,
    original_eggs.race,
    original_eggs.caste
  )
  printDetails('created new egg stack')
  local created_egg_stack = created_items[0] or created_items[1]
  printDetails(df.creature_raw.find(created_egg_stack.race).creature_id)
  printDetails('about to copy fields from orginal eggs')
  copyEggFields(original_eggs, created_egg_stack)

  printDetails('about to resize new egg stack')
  resizeEggStack(created_egg_stack, new_stack_count)

  printDetails('about to move new stack to nestbox')
  if dfhack.items.moveToBuilding(created_egg_stack, dfhack.items.getHolderBuilding(original_eggs)) then
    printDetails('moved new egg stack to nestbox')
  else
    printLocal('move of separated eggs to nestbox failed')
  end
  printDetails('end createNewEggStack')
end --createNewEggStack
---------------------------------------------------------------------------------------------------
local function splitEggStack(source_egg_stack, to_be_left_in_source_stack)
  printDetails('start splitEggStack')
  local egg_count_in_new_stack_size = source_egg_stack.stack_size - to_be_left_in_source_stack
  if egg_count_in_new_stack_size > 0 then
    createNewEggStack(source_egg_stack, egg_count_in_new_stack_size)
    resizeEggStack(source_egg_stack, to_be_left_in_source_stack)
  else
    printDetails('nothing to do, wrong egg_count_in_new_stack_size')
  end
  printDetails('end splitEggStack')
end --splitEggStack
---------------------------------------------------------------------------------------------------
local function countForbiddenEggsForRaceInClaimedNestobxes(race)
  printDetails(('start countForbiddenEggsForRaceInClaimedNestobxes'))
  local eggs_count = 0
  for _, nestbox in ipairs(df.global.world.buildings.other.NEST_BOX) do
    if nestbox.claimed_by ~= -1 then
      printDetails(('Found claimed nextbox'))
      for _, nestbox_contained_item in ipairs(nestbox.contained_items) do
        if nestbox_contained_item.use_mode == df.building_item_role_type.TEMP then
          printDetails(('Found claimed nextbox containing items'))
          if df.item_type.EGG == nestbox_contained_item.item:getType() then
            printDetails(('Found claimed nextbox containing items that are eggs'))
            if nestbox_contained_item.item.egg_flags.fertile and nestbox_contained_item.item.flags.forbid then
              printDetails(('Eggs are fertile and forbidden'))
              if nestbox_contained_item.item.race == race then
                printDetails(('Eggs belong to %s'):format(race))
                printDetails(
                  ('eggs_count %s + new %s'):format(
                    eggs_count,
                    nestbox_contained_item.item.stack_size)
                )
                eggs_count = eggs_count + nestbox_contained_item.item.stack_size
                printDetails(('eggs count after adding current nestbox %s '):format(eggs_count))
              end
            end
          end
        end
      end
    end
  end
  printDetails(('end countForbiddenEggsForRaceInClaimedNestobxes'))
  return eggs_count
end --countForbiddenEggsForRaceInClaimedNestobxes
---------------------------------------------------------------------------------------------------
function validateEggs(eggs)
  if not eggs.egg_flags.fertile then
    printDetails('Newly laid eggs are not fertile, do nothing')
    return false
  end

  local should_be_nestbox = dfhack.items.getHolderBuilding(eggs)
  if should_be_nestbox ~= nil then
    for _, nestbox in ipairs(df.global.world.buildings.other.NEST_BOX) do
      if nestbox == should_be_nestbox then
        printDetails('Found nestbox, continue with egg handling')
        return true
      end
    end
    printDetails('Newly laid eggs are in building different than nestbox, we were to late')
    return false
  else
    printDetails('Newly laid eggs are not in building, we were to late')
    return false
  end
  return true
end --validateEggs
---------------------------------------------------------------------------------------------------
function handleEggs(eggs, base_target, add_missing_animals, split_stacks, missing_animals_count)
  printDetails(('start handleEggs'))

  local race = eggs.race
  printDetails(('Handling eggs for race %s'):format(race))
  printDetails(('base_target: %s'):format(base_target))
  printDetails(('add_missing_animals: %s'):format(race))
  printDetails(('split_stacks: %s'):format(tostring(split_stacks)))
  printDetails(('missing_animals_count: %s'):format(tostring(missing_animals_count)))
  local target_eggs = base_target
  local creature = df.creature_raw.find(eggs.race).creature_id

  if add_missing_animals then
    printDetails(('adding missing animal count %s to target'):format(missing_animals_count))
    target_eggs = target_eggs + missing_animals_count
  end

  local current_eggs = eggs.stack_size

  local total_count = current_eggs
  total_count = total_count + countForbiddenEggsForRaceInClaimedNestobxes(race)
  printLocal(("total_count %s"):format(total_count));
  printLocal(("current_eggs %s"):format(current_eggs));
  printLocal(("target_eggs %s"):format(target_eggs));

  printLocal(('Total count for %s egg(s) is %s'):format(creature, total_count))

  if total_count - current_eggs < target_eggs then
    local egg_count_to_leave_in_source_stack = current_eggs

    if split_stacks and total_count > target_eggs then
      printLocal(("1 total_count %s"):format(total_count));
      printLocal(("1 current_eggs %s"):format(current_eggs));
      printLocal(("1 target_eggs %s"):format(target_eggs));
      egg_count_to_leave_in_source_stack = target_eggs - total_count + current_eggs
      printLocal(("egg_count_to_leave_in_source_stack %s"):format(egg_count_to_leave_in_source_stack));
      splitEggStack(eggs, egg_count_to_leave_in_source_stack)
    end

    eggs.flags.forbid = true

    if eggs.flags.in_job then
      local job_ref = dfhack.items.getSpecificRef(eggs, df.specific_ref_type.JOB)
      if job_ref then
        printDetails(('About to remove job related to egg(s)'))
        dfhack.job.removeJob(job_ref.data.job)
        eggs.flags.in_job = false
      end
    end

    printLocal(
      ('Previously existing %s egg(s) %s is lower than maximum %s (%s base target + %s missing animals), forbidden %s egg(s) out of %s new'):format(
        creature,
        total_count - current_eggs,
        target_eggs,
        base_target,
        missing_animals_count,
        egg_count_to_leave_in_source_stack,
        current_eggs
      )
    )
  else
    printLocal(
      ('Total count for %s egg(s) %s is over maximum %s (%s base target + %s missing animals), newly laid egg(s) %s , no action taken.'):format(
        creature,
        total_count,
        target_eggs,
        base_target,
        missing_animals_count,
        current_eggs
      )
    )
  end
  printDetails(('end handleEggs'))
end --handleEggs
---------------------------------------------------------------------------------------------------
return _ENV
