-- Sets and edits unit needs.
--@ module = true

--[[ Notes:
> References to "fulfillment" are using that spelling, rather than "fulfilment"
> Originally had methods for resetting current_focus, undistracted_focus, and the has_unmet_needs flags, however these aren't important, and the game will automatically update these values on the next need update anyway.
> For versions before 0.44.12-r3, "PrayOrMeditate" needs to be replaced with "PrayOrMedidate". There was a typo in the dfhack structures.
> When using script functions, use numerical IDs for representing needs.
]]

local help = [====[

modtools/set-need
=================
Sets and edits unit needs.

Valid commands:

:add:
    Add a new need to the unit.
    Requires a -need argument, and target.
    -focus and -level can be used to set starting values, otherwise they'll fall back to defaults.
:remove:
    Remove an existing need from the unit.
    Requires a need target, and target.
:edit:
    Change an existing need in some way.
    Requires a need target, at least one effect, and a target.
:revert:
    Revert a unit's needs list back to its original selection and need strengths.
    Focus levels are preserved if the unit has a need before and after.
    Requires a target.

Valid need targets:

:need <ID>:
    ID of the need to target. For example 0 or DrinkAlcohol.
    If the need is PrayOrMeditate, a -deity argument is also required.
:deity <HISTFIG ID>:
    Required when using PrayOrMeditate needs. This value should be the historical figure ID of the deity in question.
:all:
    All of the target's needs will be affected.

Valid effects:

:focus <NUMBER>:
    Set the focus level of the targeted need. 400 is the value used when a need has just been satisfied.
:level <NUMBER>:
    Set the need level of the targeted need. Default game values are:
    1 (Slight need), 2 (Moderate need), 5 (Strong need), 10 (Intense need)

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
    Prints a list of all needs + their IDs.
:listunit:
    Prints a list of all a unit's needs, their strengths, and their current focus.

Usage example - Satisfy all citizen's needs::

    modtools/set-need -edit -all -focus 400 -citizens
]====]

local utils = require 'utils'

local validArgs = utils.invert({
    "add",
    "remove",
    "edit",
    "need",
    "deity",
    "all",
    "focus",
    "level",
    "citizens",
    "unit",
    "revert",
    "help",
    "list",
    "listunit"
})

local focusDefault = 0
local focusSatisfied = 400

-- Returns true if unit has the given need, false if not.
-- deityId is an optional argument that records the deity's historical figure ID for PrayOrMeditate needs
function unitHasNeed(unit, need, deityId)
  return not not getUnitNeed(unit, need, deityId)
end

-- Returns the unit's need data, followed by its index in the needs list if the unit has the given need. Otherwise, returns nil.
-- deityId is an optional argument that records the deity's historical figure ID for PrayOrMeditate needs
function getUnitNeed(unit, need, deityId)
  for index, needInstance in ipairs(unit.status.current_soul.personality.needs) do
    if needInstance.id == need then
      if needInstance.id ~= df.need_type.PrayOrMeditate then --Worship is a special case
        return needInstance, index
      elseif needInstance.deity_id == deityId then -- Only return the need if it's actually targeting the right deity
        return needInstance, index
      else
      end
    end
  end

  -- If we get here, unit doesn't have it
end

-- Returns the unit's focus level for the given need, or nil if they don't have that need
-- deityId is an optional argument that records the deity's historical figure ID for PrayOrMeditate needs
function getFocus(unit, need, deityId)
  local needInstance = getUnitNeed(unit, need, deityId)

  if needInstance then
    return needInstance.focus_level
  end
end

-- Returns the unit's need level for the given need, or nil if they don't have that need
-- deityId is an optional argument that records the deity's historical figure ID for PrayOrMeditate needs
-- You can use getNeedLevelString to get the label that the game uses for that level e.g. "Strong", "Slight"
function getNeedLevel(unit, need, deityId)
  local needInstance = getUnitNeed(unit, need, deityId)

  if needInstance then
    return needInstance.need_level
  end
end

function addNeed(unit, need, focus, level, deityId)
  -- Setup default values
  local need = need
  if type(need) == "string" then
    need = df.need_type[need] --luacheck:retype
  end

  local focus = focus or focusDefault -- focusDefault defined earlier in script: default is 0
  local level = level or 1 -- 1 = Slight need
  local deityId = deityId or -1

  -- Remove existing need of type
  removeNeed(unit, need, deityId)

  -- Add new need
  unit.status.current_soul.personality.needs:insert("#", {new = true, id = need, focus_level = focus, need_level = level, deity_id = deityId})
end

function removeNeed(unit, need, deityId)
  local needInstance, index = getUnitNeed(unit, need, deityId)

  if not needInstance then
    return false
  else
    unit.status.current_soul.personality.needs:erase(index)
    return true
  end
end

function removeNeedAll(unit)
  while #unit.status.current_soul.personality.needs > 0 do
    unit.status.current_soul.personality.needs:erase(0)
  end
end

-- Sets need level of need. Vanilla values are 1 for Slight, 2 for Moderate, 5 for Strong, 10 for Intense
-- deityId is an optional argument that records the deity's historical figure ID for PrayOrMeditate needs
function setLevel(unit, need, level, deityId)
  local needInstance = getUnitNeed(unit, need, deityId)

  -- Only modify the need if the unit actually has it
  if needInstance then
    needInstance.need_level = level
  end
end

-- Set the need level of all a unit's needs, if for some reason you want to do that...
function setLevelAll(unit, level)
  for index, needInstance in ipairs(unit.status.current_soul.personality.needs) do
    needInstance.need_level = level
  end
end

-- Sets all needs to given focus
function setFocusAll(unit, focus)
  local focus = focus or focusDefault
  for index, needInstance in ipairs(unit.status.current_soul.personality.needs) do
    needInstance.focus_level = focus
  end
end

-- Set given need to given focus
function setFocus(unit, need, focus, deityId)
  local focus = focus or focusDefault
  local needInstance = getUnitNeed(unit, need, deityId)

  -- Only modify the need if the unit actually has it
  if needInstance then
    needInstance.focus_level = focus
  end
end

-- Replicates the base game calculations that determine the strength of a unit's needs based on their personality traits, beliefs, and devotion to gods.
-- Returns appropriate need strength value, or 0 if the unit shouldn't have the need.
local needDefaultsInfo = { --as:{_type:table,trait:{_type:table,id:string},belief:{_type:table,id:string},special:bool,negative:bool}[]
  [df.need_type.Socialize] = {trait = {id = "GREGARIOUSNESS"}},
  [df.need_type.DrinkAlcohol] = {trait = {id = "IMMODERATION"}},
  [df.need_type.PrayOrMeditate] = {special = true},
  [df.need_type.StayOccupied] = {trait = {id = "ACTIVITY_LEVEL"}, belief = {id = "HARD_WORK"}},
  [df.need_type.BeCreative] = {trait = {id = "ART_INCLINED"}},
  [df.need_type.Excitement] = {trait = {id = "EXCITEMENT_SEEKING"}},
  [df.need_type.LearnSomething] = {trait = {id = "CURIOUS"}, belief = {id = "KNOWLEDGE"}},
  [df.need_type.BeWithFamily] = {belief = {id = "FAMILY"}},
  [df.need_type.BeWithFriends] = {belief = {id = "FRIENDSHIP"}},
  [df.need_type.HearEloquence] = {belief = {id = "ELOQUENCE"}},
  [df.need_type.UpholdTradition] = {belief = {id = "TRADITION"}},
  [df.need_type.SelfExamination] = {belief = {id = "INTROSPECTION"}},
  [df.need_type.MakeMerry] = {belief = {id = "MERRIMENT"}},
  [df.need_type.CraftObject] = {belief = {id = "CRAFTSMANSHIP"}},
  [df.need_type.MartialTraining] = {belief = {id = "MARTIAL_PROWESS"}},
  [df.need_type.PracticeSkill] = {belief = {id = "SKILL"}},
  [df.need_type.TakeItEasy] = {belief = {id = "LEISURE_TIME"}},
  [df.need_type.MakeRomance] = {belief = {id = "ROMANCE"}},
  [df.need_type.SeeAnimal] = {belief = {id = "NATURE"}},
  [df.need_type.SeeGreatBeast] = {special = true},
  [df.need_type.AcquireObject] = {trait = {id = "GREED"}},
  [df.need_type.EatGoodMeal] = {trait = {id = "IMMODERATION"}},
  [df.need_type.Fight] = {trait = {id = "VIOLENT"}},
  [df.need_type.CauseTrouble] = {trait = {id = "DISCORD"}, belief = {id = "HARMONY", negative = true}},
  [df.need_type.Argue] = {trait = {id = "FRIENDLINESS", negative = true}},
  [df.need_type.BeExtravagant] = {trait = {id = "IMMODESTY"}},
  [df.need_type.Wander] = {belief = {id = "NATURE"}},
  [df.need_type.HelpSomebody] = {trait = {id = "ALTRUISM"}},
  [df.need_type.ThinkAbstractly] = {trait = {id = "ABSTRACT_INCLINED"}},
  [df.need_type.AdmireArt] = {belief = {id = "ARTWORK"}}
}
local needLevelValues = {1, 2, 5, 10} --Value for need level for each need level tier
--luacheck: out=number
function getUnitDefaultNeedStrength(unit, need, deityId)
  local entry = needDefaultsInfo[need]
  if entry == nil then return 0 end
  local setbelief = dfhack.reqscript("modtools/set-belief")
  local setpersonality = dfhack.reqscript("modtools/set-personality")

  if entry.special ~= true then
    -- Record strength of need granted by the need's associated belief +/ trait
    -- The highest value will represent the need's need level tier
    local beliefStrength = 0
    local traitStrength = 0

    if entry.trait ~= nil then
      local traitTier = setpersonality.getTraitTier(setpersonality.getUnitTraitBase(unit, df.personality_facet_type[entry.trait.id]))

      -- Need level for a trait is based on how far the trait's tier is divorced from 4 in its required direction
      -- A trait tier of 4 gives a value of 1, plus one for each level divorce
      if traitTier == 4 then
        traitStrength = 1
      elseif entry.trait.negative and traitTier < 4 then -- The need uses a negative trait value to boost, and unit has
        traitStrength = 1 + math.abs(traitTier - 4)
      elseif not entry.trait.negative and traitTier > 4 then --The need uses a positive trait value to boost, and unit has
        traitStrength = 1 + math.abs(traitTier - 4)
      end
    end
    if entry.belief ~= nil then
      local beliefTier = setbelief.getBeliefTier(setbelief.getUnitBelief(unit, df.value_type[entry.belief.id]))

      -- Need level for a belief is based on how far the trait's tier is beyond 4 in its required direction
      -- A need tier of 4 gives a value of 0, plus one for each level of divorce
      if entry.belief.negative and beliefTier < 4 then -- The need uses a negative belief value to boost, and unit has
        beliefStrength = math.abs(beliefTier - 4)
      elseif not entry.belief.negative and beliefTier > 4 then --The need uses a positive belief value to boost, and unit has
        beliefStrength = math.abs(beliefTier - 4)
      end
    end

    -- Highest value of the two is used - there is no interaction with one being lower / even set in the opposite direction.
    if beliefStrength ~= 0 or traitStrength ~= 0 then
      return needLevelValues[math.max(beliefStrength, traitStrength)]
    else
      return 0
    end


  else --There are a couple of needs that don't conform to the regular setup

    if need == df.need_type.PrayOrMeditate then
      -- Worship uses the strength of the unit's link to their deity to determine the strength of their need
      -- Fun fact: The thresholds used to change the unit's relationship to the deity (e.g. "casual worshipper")
      -- aren't the same as the ones used to change need strength!

      -- Find the deity listing in the historical unit's links (if applicable)
      local linkStrength = 0

      if unit.hist_figure_id ~= -1 then
        for index, link in ipairs(df.historical_figure.find(unit.hist_figure_id).histfig_links) do
          if df.histfig_hf_link_deityst:is_instance(link) and link.target_hf == deityId then
            linkStrength = link.link_strength
            break
          end
        end
      end

      -- The following are the ranges for each need strength tier
      -- Most of the values are exact, but some are educated guesses based on experimentation
      local needTier = 0

      if linkStrength <= 24 then
        needTier = 0
      elseif linkStrength >= 25 and linkStrength <= 49 then
        needTier = 1
      elseif linkStrength >= 50 and linkStrength <= 69 then
        needTier = 2
      elseif linkStrength >= 70 and linkStrength <= 89 then
        needTier = 3
      else
        needTier = 4
      end

      if needTier ~= 0 then
        return needLevelValues[needTier]
      else
        return 0
      end

    elseif need == df.need_type.SeeGreatBeast then
      -- See great beast need uses the excitement trait and nature belief, but not in the usual way.
      -- Excitement increases need tier for every tier it is above base, but the need will only appear if nature is at least 2 levels above base.
      -- As such, unlike other needs that get their strength from traits, this need can't go up to intense need.

      local strength = 0
      local natureTier = setbelief.getBeliefTier(setbelief.getUnitBelief(unit, df.value_type.NATURE))
      local excitementTier = setpersonality.getTraitTier(setpersonality.getUnitTraitBase(unit, df.personality_facet_type.EXCITEMENT_SEEKING))

      if natureTier >= 6 and excitementTier > 4 then
        strength = excitementTier - 4
      end

      if strength > 0 then
        return needLevelValues[strength]
      else
        return 0
      end
    end
  end
end

-- Updates the unit's list of needs to reflect their traits and beliefs. New needs may be added, and old ones removed or altered to a new level.
-- Will override changes that scripts have made (like adding back removed needs).
-- Preserves focus levels for any need that was preserved between jumps.
function rebuildNeeds(unit)
  local oldLevels = {}
  local oldWorshipLevels = {}

  local beliefs = {} -- Stores beliefs and their tiers
  local traits = {} -- Stores traits and their tiers

  local setbelief = dfhack.reqscript("modtools/set-belief")
  local setpersonality = dfhack.reqscript("modtools/set-personality")

  -- Record current focus levels.
  for index, needInstance in ipairs(unit.status.current_soul.personality.needs) do
    if df.need_type[needInstance.id] ~= df.need_type.PrayOrMeditate then
      oldLevels[needInstance.id] = needInstance.focus_level
    else -- Worship needs are handled slightly differently
      oldWorshipLevels[needInstance.deity_id] = needInstance.focus_level
    end
  end

  -- Remove all needs
  removeNeedAll(unit)

  -- Go through all non-worship needs and add any that the unit should have, while also setting the focus to what it was before (if applicable)
  for needId, needName in ipairs(df.need_type) do
    if needId ~= df.need_type.PrayOrMeditate then
      local needLevel = getUnitDefaultNeedStrength(unit, needId)

      if needLevel > 0 then -- Only add the need if it has a strength
        addNeed(unit, needId, oldLevels[needId] or focusDefault, needLevel)
      end
    end
  end

  -- Now, handle all worship needs (provided unit is historical figure)
  if unit.hist_figure_id ~= -1 then
    for index, link in ipairs(df.historical_figure.find(unit.hist_figure_id).histfig_links) do
      if df.histfig_hf_link_deityst:is_instance(link) then
        local needLevel = getUnitDefaultNeedStrength(unit, df.need_type.PrayOrMeditate, link.target_hf)

        if needLevel > 0 then
          addNeed(unit, df.need_type.PrayOrMeditate, oldWorshipLevels[link.target_hf] or focusDefault, needLevel, link.target_hf)
        end
      end
    end
  end
end

-- Returns tier of fulfillment based on given focus level
function getFulfillmentTier(focus)
  if focus <= -100000 then
    return 1
  elseif focus >= -99999 and focus <= -10000 then
    return 2
  elseif focus >= -9999 and focus <= -1000 then
    return 3
  elseif focus >= -999 and focus <= 99 then
    return 4
  elseif focus >= 100 and focus <= 199 then
    return 5
  elseif focus >= 200 and focus <= 299 then
    return 6
  else
    return 7
  end
end

-- Returns the in game label used to describe each tier of fulfillment
-- Get Tier via getFulfillmentTier
local fulfillmentStrings = {"Badly distracted", "Distracted", "Unfocused", "Not distracted", "Untroubled", "Level-headed", "Unfettered"}
function getFulfillmentString(tier)
  return fulfillmentStrings[tier]
end

-- Returns the need label used in game to describe a need of the given need_level
local needLevelStrings = {"Slight", "Moderate", "Strong", "Intense"}
function getNeedLevelString(level)
  return needLevelStrings[getNeedLevelTier(level)]
end

-- Returns the tier of need level.
-- 1 = Slight (1), 2 = Moderate (2), 3 = Strong (5), 4 = Intense (10)
function getNeedLevelTier(level)
  -- By vanilla standards, these values only ever occur as 1, 2, 5, and 10.
  -- However we'll use ranges just in case some modder decides to use different values.
  if level <= 1 then
    return 1
  elseif level >= 2 and level < 5 then
    return 2
  elseif level >= 5 and level < 10 then
    return 3
  else
    return 4
  end
end

-- Print unit's needs, levels, and focus into dfhack console
function printUnitNeeds(unit)
  print("Needs for " .. dfhack.TranslateName(unit.name) .. ":")
  for index, needInstance in ipairs(unit.status.current_soul.personality.needs)  do
    local name = df.need_type[needInstance.id]
    local level = needInstance.need_level
    local focus = needInstance.focus_level

    print(name .. ": Focus = " .. focus .. " Level = " .. level)
  end
end

function main(...)
  local args = utils.processArgs({...}, validArgs)

  if args.help then
      print(help)
      return
  end

  if args.list then
    for index, needName in ipairs(df.need_type) do
      print(index .. " (" .. needName .. ")")
    end
    return
  end

  -- Command check
  local command
  if args.add then
    command = "add"
  elseif args.remove then
    command = "remove"
  elseif args.edit then
    command = "edit"
  elseif args.revert then
    command = "revert"
  end

  if not command and not args.listunit then
    qerror("Please specify a command to use. Valid commands are -add, -remove, -edit, and -revert.")
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

  -- Generic checks
  local need
  if args.need then
    if tonumber(args.need) then
      need = tonumber(args.need)
    else
      need = df.need_type[args.need]
    end
  end

  local deity
  if args.deity then
    deity = tonumber(args.deity)
  end

  local focus
  if args.focus then
    focus = tonumber(args.focus)
  end

  local level
  if args.level then
    level = tonumber(args.level)
  end

  -- Execute / finish checks

  for index, unit in ipairs(unitsList) do
    if command == "add" then
      if not need then
        qerror("-add requires a -need argument.")
      end

      addNeed(unit, need, focus or 0, level or 1, deity)
    elseif command == "remove" then
      if args.all then
        removeNeedAll(unit)
      else
        if not need then
          qerror("-remove requires a need target.")
        end

        removeNeed(unit, need, deity)
      end
    elseif command == "edit" then
      if args.all then
        if focus then
          setFocusAll(unit, focus)
        end
        if level then
          setLevelAll(unit, focus)
        end
      else
        if not need then
          qerror("-edit requires a need target.")
        end

        if focus then
          setFocus(unit, need, focus, deity)
        end
        if level then
          setLevel(unit, need, level, deity)
        end
      end
    elseif command == "revert" then
      rebuildNeeds(unit)
    elseif args.listunit then
      printUnitNeeds(unit)
    end
  end
end

if not dfhack_flags.module then
  main(...)
end
