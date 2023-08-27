-- Changes the beliefs (values) of units.
--@ module = true

--[[ Notes:
> When using these script functions in a script, use modtools/set-need's rebuildNeeds function to rebuild needs to reflect new beliefs.
> When using script functions, use numerical IDs for representing beliefs.
> The random assignment is placeholder for now. I don't understand how they work
]]
local help = [====[

modtools/set-belief
===================
Changes the beliefs (values) of units.
Requires a belief, modifier, and a target.

Valid beliefs:

:all:
    Apply the edit to all the target's beliefs
:belief <ID>:
    ID of the belief to edit. For example, 0 or LAW.

Valid modifiers:

:set <-50-50>:
    Set belief to given strength.
:tier <1-7>:
    Set belief to within the bounds of a strength tier:

    ===== ========
    Value Strength
    ===== ========
    1     Lowest
    2     Very Low
    3     Low
    4     Neutral
    5     High
    6     Very High
    7     Highest
    ===== ========

:modify <amount>:
    Modify current belief strength by given amount.
    Negative values need a ``\`` before the negative symbol e.g. ``\-1``
:step <amount>:
    Modify current belief tier up/down by given amount.
    Negative values need a ``\`` before the negative symbol e.g. ``\-1``
:random:
    Use the default probabilities to set the belief to a new random value.
:default:
    Belief will be set to cultural default.

Valid targets:

:citizens:
    All (sane) citizens of your fort will be affected. Will do nothing in adventure mode.
:unit <UNIT ID>:
    The given unit will be affected.

If no target is given, the provided unit can't be found, or no unit id is given with the unit
argument, the script will try and default to targeting the currently selected unit.

Other arguments:

:help:
    Shows this help page.
:list:
    Prints a list of all beliefs + their IDs.
:noneed:
    By default, unit's needs will be recalculated to reflect new beliefs after every run.
    Use this argument to disable that functionality.
:listunit:
    Prints a list of all a unit's beliefs. Cultural defaults are marked with ``*``.

]====]

local utils = require 'utils'

local validArgs = utils.invert({
  "all",
  "belief",
  "set",
  "tier",
  "modify",
  "step",
  "random",
  "default",
  "citizens",
  "unit",
  "help",
  "list",
  "noneed",
  "listunit"
})

rng = rng or dfhack.random.new(nil, 10)

-- These ranges represent the bounds at which the description for a belief changes
local tierRanges = {{min = -50, max = -41, weight = 4}, {min = -40, max = -26, weight = 20}, {min = -25, max = -11, weight = 85}, {min = -10, max = 10, weight = 780}, {min = 11, max = 25, weight = 85}, {min = 26, max = 40, weight = 20}, {min = 41, max = 50, weight = 4}}

---------------------
local function clamp(value, low, high)
  return math.min(high, math.max(low, value))
end

local function weightedRoll(weightedTable)
  local maxWeight = 0
  for index, result in ipairs(weightedTable) do
    maxWeight = maxWeight + result.weight
  end

  local roll = rng:random(maxWeight) + 1
  local currentNum = roll
  local result

  for index, currentResult in ipairs(weightedTable) do
    currentNum = currentNum - currentResult.weight
    if currentNum <= 0 then
      result = currentResult.id
      break
    end
  end

  return result
end

local function randomiseWithinBounds(min, max)
  local range = math.abs(min - max)
  local roll = rng:random(range+1)

  return min + roll
end

---------------------
-- Set unit's belief to a given value
-- Use this as the main method of changing a unit's beliefs!
function setUnitBelief(unit, belief, value, forcePersonal)
  -- Clamp value
  local value = clamp(value, -50, 50)

  -- Remove current belief (if exists)
  removeUnitBelief(unit, belief)

  -- We may want to avoid adding personal beliefs of the same strength as what's already represented by the unit's culture
  -- If the tiers match, forcePersonal must be true for the belief to be added
  local forcePersonal = forcePersonal or true
  local cultureTier = getBeliefTier(getUnitCultureBelief(unit, belief))
  local newBeliefTier = getBeliefTier(value)

  if cultureTier ~= newBeliefTier or forcePersonal then
    addUnitBelief(unit, belief, value)
  end
end

-- Returns true if the unit's belief of the given type is cultural, rather than a personal belief. Otherwise, returns false.
function isCultureBelief(unit, belief)
  return not isPersonalBelief(unit, belief)
end

-- Returns true if the unit's belief of the given type is personal, rather than their culture's default. Otherwise, returns false.
function isPersonalBelief(unit, belief)
  local found = false

  for index, beliefInstance in ipairs(unit.status.current_soul.personality.values) do
    if beliefInstance.type == belief then
      found = true
      break
    end
  end

  return found
end

-- Returns the strength of the unit's belief of the given type
function getUnitBelief(unit, belief)
  local found = false
  local strength = 0

  for index, beliefInstance in ipairs(unit.status.current_soul.personality.values) do
    if beliefInstance.type == belief then
      found = true
      strength = beliefInstance.strength
      break
    end
  end

  if not found then -- Is cultural belief
    strength = getUnitCultureBelief(unit, belief)
  end

  return strength
end

-- Removes the unit's personal belief of given type, if they have it. This means they'll revert to their culture's belief value instead.
-- Returns true if a belief of the type was found and removed, false if not.
function removeUnitBelief(unit, belief)
  local upers = unit.status.current_soul.personality
  local removed = false

  for index, beliefInstance in ipairs(upers.values) do
    if beliefInstance.type == belief then
      upers.values:erase(index)
      removed = true
      break
    end
  end

  return removed
end

-- Add a personal belief to the unit at a given value
-- Note: Doesn't do checking to ensure doesn't already exist!
function addUnitBelief(unit, belief, value)
  local upers = unit.status.current_soul.personality
  local value = clamp(value, -50, 50)

  upers.values:insert("#", {new = true, type = belief, strength = value})
end

-- Gives a list of all the unit's beliefs and their values.
-- Returns a table where the keys are the belief names and its values are its strength
-- If tiers is true, the value is the tier of the belief instead of the trait's strength
local function getUnitBeliefList(unit, tiers)
  local list = {}

  for id, beliefName in ipairs(df.value_type) do
    if beliefName ~= 'NONE' then
      local strength = getUnitBelief(unit, id)

      if tiers then
        list[beliefName] = getBeliefTier(strength)
      else
        list[beliefName] = strength
      end
    end
  end

  return list
end

-- Sets unit's belief to a random value within the given tier.
function setUnitBeliefTier(unit, belief, tier)
  local tier = clamp(tier, 1, 7)
  -- Randomly generate a strength value which falls between within the new tier, and set that as the unit's belief strength
  local strength = randomiseWithinBounds(tierRanges[tier].min, tierRanges[tier].max)
  setUnitBelief(unit, belief, strength)
end

-- Move the unit's belief in tier increments
function stepUnitBelief(unit, belief, modifier)
  local currentTier = getBeliefTier(getUnitBelief(unit, belief))
  local newTier = clamp(currentTier + modifier, 1, 7)

  -- Only do so if new tier is different from the current one
  if currentTier ~= newTier then
    setUnitBeliefTier(unit, belief, newTier)
  end
end

-- Modify the unit's belief by a given amount
function modifyUnitBelief(unit, belief, modifier)
  local currentStrength = getUnitBelief(unit, belief)
  local newStrength = clamp(currentStrength + modifier, -50, 50)

  setUnitBelief(unit, belief, newStrength)
end

-- Randomly assigns a tier based on default weight values and passes that on to setUnitBeliefTier
-- (Could alternatively use randomBeliefValue and setUnitBelief)
function randomiseUnitBelief(unit, belief)
  local randomTier = randomBeliefTier()

  setUnitBeliefTier(unit, belief, randomTier)
end

-- Get the "default" belief that the unit uses, based on their culture
-- Priority is Cultural Identity -> Civ's values -> Nothing
function getUnitCultureBelief(unit, belief)
  if belief == df.value_type.NONE then return 0 end
  local upers = unit.status.current_soul.personality

  if upers.cultural_identity ~= -1 then
    return df.cultural_identity.find(upers.cultural_identity).values[df.value_type[belief]]
  elseif upers.civ_id ~= -1 then
    return df.historical_entity.find(upers.civ_id).resources.values[df.value_type[belief]]
  else
    return 0 --outsiders have no culture
  end
end

-- Gets what tier of belief the given strength value falls into
-- 1: Lowest | 2: Very Low | 3: Low | 4: Neutral | 5: High | 6: Very High | 7: Highest
function getBeliefTier(strength)
  local range = 1
  for index, data in ipairs(tierRanges) do
    if strength >= data.min and strength <= data.max then
      range = index
      break
    end
  end

  return range
end

-- Currently a hack solution to randomisation.
-- Doesn't respect culture weights (if they have any impact)
function randomBeliefTier()
  local weightedTable = {} --as:{_type:table,id:number,weight:number}[]

  for rangeIndex, data in ipairs(tierRanges) do
    local addition = {}
    addition.id = rangeIndex
    addition.weight = data.weight
    table.insert(weightedTable, addition)
  end

  -- Determine the range tier
  local result = weightedRoll(weightedTable)

  return result
end

-- Uses above, but then gets a strength value within the tier's bounds
function randomBeliefValue()
  local randomTier = randomBeliefTier()

  -- Get a random value within that range
  return randomiseWithinBounds(tierRanges[randomTier].min, tierRanges[randomTier].max)
end

-- Print unit's beliefs into the dfhack console. Cultural beliefs are marked with a *
function printUnitBeliefs(unit)
  print("Beliefs for " .. dfhack.TranslateName(unit.name) .. ":")
  for id, name in ipairs(df.value_type) do
    if name ~= 'NONE' then
      local strength = getUnitBelief(unit, id)
      local cultural = isCultureBelief(unit, id)

      local out = name
      if cultural then
        out = out .. "*"
      end
      out = out .. " " .. strength

      print(out)
    end
  end
end

function main(...)
  local args = utils.processArgs({...}, validArgs)
  local setneed = dfhack.reqscript("modtools/set-need")

  if args.help then
    print(help)
    return
  end

  if args.list then
    for index, valueName in ipairs(df.value_type) do
      if valueName ~= 'NONE' then
        print(index .. " (" .. valueName .. ")")
      end
    end
    return
  end

  -- Valid target check
  local unitsList = {} --as:df.unit[]
  if not args.citizens then
    -- Assume trying to target a unit
    local unit
    if args.unit then
        if tonumber(args.unit) then
            unit = df.unit.find(tonumber(args.unit))
        end
    end

    -- If unit ID wasn't provided / unit couldn't be found,
    -- Try getting selected unit
    if unit == nil then
        unit = dfhack.gui.getSelectedUnit(true)
    end

    if unit == nil then
        qerror("Couldn't find unit. If you don't want to target a specific unit, use -citizens.")
    else
        table.insert(unitsList, unit)
    end
  elseif args.citizens then
    -- Technically this will exclude insane citizens, but this is the
    -- easiest thing that dfhack provides

    -- Abort if not in Fort mode
    if not dfhack.world.isFortressMode() then
        qerror("-citizens argument only available in Fortress Mode.")
    end

    for _, unit in pairs(df.global.world.units.active) do
        if dfhack.units.isCitizen(unit) then
            table.insert(unitsList, unit)
        end
    end
  end

  -- Belief check
  local beliefsList = {} --as:number[]
  if not args.belief and not args.all and not args.listunit then
    qerror("Please specify a belief to edit using -belief <ID> or -all.")
  end
  if not args.all then -- One belief provided
    local beliefId
    if tonumber(args.belief) then
      beliefId = tonumber(args.belief)
    else
      beliefId = df.value_type[args.belief:upper()]
    end
    table.insert(beliefsList, beliefId)
  else -- To modify: all
    for id, keyName in ipairs(df.value_type) do
      if keyName ~= 'NONE' then
        table.insert(beliefsList, id)
      end
    end
  end

  -- Modifier check
  local modifyStyle
  local value
  if args.set then
    modifyStyle = "set"
    value = tonumber(args.set)
  elseif args.tier then
    modifyStyle = "tier"
    value = tonumber(args.tier)
  elseif args.modify then
    modifyStyle = "modify"
    value = tonumber(args.modify)
  elseif args.step then
    modifyStyle = "step"
    value = tonumber(args.step)
  elseif args.random then
    modifyStyle = "random"
  elseif args.default then
    modifyStyle = "default"
  elseif args.listunit then --A slight hack but it's fine
    modifyStyle = "list"
  else
    qerror("Please provide a valid modifier.")
  end

  -- Execute
  for index, unit in ipairs(unitsList) do
    for index, belief in ipairs(beliefsList) do
      if modifyStyle == "set" then
        setUnitBelief(unit, belief, value)
      elseif modifyStyle == "tier" then
        setUnitBeliefTier(unit, belief, value)
      elseif modifyStyle == "modify" then
        modifyUnitBelief(unit, belief, value)
      elseif modifyStyle == "step" then
        stepUnitBelief(unit, belief, value)
      elseif modifyStyle == "random" then
        randomiseUnitBelief(unit, belief)
      elseif modifyStyle == "default" then
        removeUnitBelief(unit, belief)
      end
    end

    if modifyStyle == "list" then
        printUnitBeliefs(unit)
    end
    if not args.noneed and not args.listunit then
      setneed.rebuildNeeds(unit)
    end
  end
end

if not dfhack_flags.module then
  main(...)
end
