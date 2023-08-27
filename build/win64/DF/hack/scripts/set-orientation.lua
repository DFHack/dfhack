-- Edit a unit's orientation
-- Not to be confused with kane_t's script of the same name
--@ module = true
local help = [====[

set-orientation
===============
Edit a unit's orientation.
Interest levels are 0 for Uninterested, 1 for Romance, 2 for Marry.

:unit <UNIT ID>:
    The given unit will be affected.
    If not found/provided, the script will try defaulting to the currently selected unit.
:male <INTEREST>:
    Set the interest level towards male sexes
:female <INTEREST>:
    Set the interest level towards female sexes
:opposite <INTEREST>:
    Set the interest level towards the opposite sex to the unit
:same <INTEREST>:
    Set the interest level towards the same sex as the unit
:random:
    Randomise the unit's interest towards both sexes, respecting their ORIENTATION token odds.

Other arguments:

:help:
    Shows this help page.
:view:
    Print the unit's orientation values in the console.
]====]

local utils = require 'utils'

local validArgs = utils.invert({
  "unit",
  "male",
  "female",
  "opposite",
  "same",
  "random",
  "help",
  "view"
})

rng = rng or dfhack.random.new(nil, 10)

-- General function used for rolling weighted tables
function weightedRoll(weightedTable)
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

function getInterestString(interest)
  if interest == 0 then
    return "Uninterested"
  elseif interest == 1 then
    return "Romance"
  elseif interest == 2 then
    return "Marry"
  end
end

function getSexString(sex)
  if sex == -1 then
    return "none"
  elseif sex == 0 then
    return "female"
  elseif sex == 1 then
    return "male"
  end
end

-- Gets the interest level for the unit
-- Types are: male, female, opposite, same
function getInterest(unit, type)
  local sexname
  if type == "opposite" then
    sexname = getSexString(oppositeSex(unit.sex))
  elseif type == "same" then
    sexname = getSexString(unit.sex)
  else
    sexname = type
  end

  local romancename
  local marryname

  if sexname == "none" then
    return 0
  elseif sexname == "male" then
    romancename = "romance_male"
    marryname = "marry_male"
  elseif sexname == "female" then
    romancename = "romance_female"
    marryname = "marry_female"
  else
    qerror("unexpected sex: " .. sexname)
  end

  local ori = unit.status.current_soul.orientation_flags
  local interest = 0
  if ori[romancename] == true then
    interest = 1
  elseif ori[marryname] == true then
    interest = 2
  end

  return interest
end

-- Sets the orientation of the unit to the given parameters
-- Sex is the sex number (listed in unit.sex) of the sex (0 = female, 1 = male)
function setOrientation(unit, sex, interest)
  -- Store the unit's orientation flags in a form that's quicker to type :p
  local ori = unit.status.current_soul.orientation_flags
  ori.indeterminate = false

  -- Check if unit is a historical figure.
  -- Their historical figure entry also has a set of orientation flags to edit.
  local hori
  if unit.hist_figure_id ~= -1 then
    hori = df.historical_figure.find(unit.hist_figure_id).orientation_flags
    hori.indeterminate = false
  end

  local sexname = getSexString(sex)
  local romancename
  local marryname

  if sexname == "none" then
    -- Alas, there are no flags for attraction to sexless beings
    return
  elseif sexname == "male" then
    romancename = "romance_male"
    marryname = "marry_male"
  elseif sexname == "female" then
    romancename = "romance_female"
    marryname = "marry_female"
  else
    qerror("unexpected sex: " .. sexname)
  end

  ori[romancename] = false
  ori[marryname] = false
  if hori then
    hori[romancename] = false
    hori[marryname] = false
  end

  if interest == 1 then
    ori[romancename] = true
    if hori then
      hori[romancename] = true
    end
  elseif interest == 2 then
    ori[marryname] = true
    if hori then
      hori[marryname] = true
    end
  end
end

-- Randomises + sets the unit's orientation towards the given sex, respecting its caste's ORIENTATION token values
-- Sex is the sex number (listed in unit.sex) of the sex (0 = female, 1 = male)
function randomiseOrientation(unit, sex)
  -- Do nothing if not an orientation sex
  if sex == -1 then
    return
  end

  local caste = df.creature_raw.find(unit.race).caste[unit.caste]

  -- Build a weighted table for use in the weighted roll function
  local sexname = getSexString(sex)
  local orientationname
  if sexname == "male" then
    orientationname = "orientation_male"
  elseif sexname == "female" then
    orientationname = "orientation_female"
  else
    qerror("unexpected sex: " .. sexname)
  end
  local rolltable = {
    {id = 0, weight = caste[orientationname][0]}, -- Uninterested
    {id = 1, weight = caste[orientationname][1]}, -- Romance
    {id = 2, weight = caste[orientationname][2]} -- Marry
  }

  -- Roll a value for interest, and use that
  local result = weightedRoll(rolltable)
  setOrientation(unit, sex, result)
end

-- Takes sex number (listed in unit.sex), returns number for opposite sex
function oppositeSex(sex)
  if sex == 1 then
    return 0
  elseif sex == 0 then
    return 1
  else
    return -1
  end
end

function main(...)
  local args = utils.processArgs({...}, validArgs)

  -- Help
  if args.help then
    print(help)
    return
  end

  -- Unit
  local unit
  if args.unit and tonumber(args.unit) then
    unit = df.unit.find(tonumber(args.unit))
  end

  if unit == nil then
    unit = dfhack.gui.getSelectedUnit(true)
  end

  if unit == nil then
    qerror("Couldn't find unit.")
  end

  -- View
  if args.view then
    print("Orientation of " .. dfhack.TranslateName(unit.name) .. ":")
    print("Male: " .. getInterestString(getInterest(unit, "male")))
    print("Female: " .. getInterestString(getInterest(unit, "female")))
    return
  end

  -- Random check
  if args.random then
    randomiseOrientation(unit, 0) -- Randomise female attraction
    randomiseOrientation(unit, 1) -- Randomise male attraction
    return
  end

  -- Determine target sex
  if not args.male and not args.female and not args.opposite and not args.same then
    qerror("Please provide a sex and value to edit. Valid sexes are -male, -female, -opposite, and -same.")
  end

  local maleInterest
  local femaleInterest
  if args.male then
    maleInterest = tonumber(args.male)
  end
  if args.female then
    femaleInterest = tonumber(args.female)
  end
  if args.same then
    if unit.sex == 0 then
      femaleInterest = tonumber(args.same)
    elseif unit.sex == 1 then
      maleInterest = tonumber(args.same)
    else
      qerror("asexual creatures do not have a same sex")
    end
  end
  if args.opposite then
    local opposite = oppositeSex(unit.sex)
    if opposite == 0 then
      femaleInterest = tonumber(args.opposite)
    elseif opposite == 1 then
      maleInterest = tonumber(args.opposite)
    else
      qerror("there is no opposite sex")
    end
  end

  -- Run function
  if maleInterest then
    setOrientation(unit, 1, maleInterest)
  end
  if femaleInterest then
    setOrientation(unit, 0, femaleInterest)
  end
end

if not dfhack_flags.module then
  main(...)
end
