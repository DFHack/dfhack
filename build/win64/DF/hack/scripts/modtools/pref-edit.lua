-- Add, remove, or edit the preferences of a unit
-- Not to be confused with "pref_adjust" and "prefchange"
--@ module = true

local help = [====[

modtools/pref-edit
==================
Add, remove, or edit the preferences of a unit.
Requires a modifier, a unit argument, and filters.

- ``-unit <UNIT ID>``:
    The given unit will be affected.
    If not found/provided, the script will try defaulting to the currently selected unit.

Valid modifiers:

- ``-add``:
    Add a new preference to the unit. Filters describe the preference's variables.
- ``-remove``:
    Remove a preference from the unit. Filters describe what preference to remove.
- ``-has``:
    Checks if the unit has a preference matching the filters. Prints a message in the console.
- ``-removeall``:
    Remove all preferences from the unit. Doesn't require any filters.


Valid filters:

- ``-id <VALUE>``:
    This is the ID used for all preferences that require an ID.
    Represents item_type, creature_id, color_id, shape_id, plant_id, poetic_form_id, musical_form_id, and dance_form_id.
    Text IDs (e.g. "TOAD", "AMBER") can be used for all but poetic, musical, and dance.
- ``-item``, ``-creature``, ``-color``, ``-shape``, ``-plant``, ``-poetic``, ``-musical``, ``-dance``:
    Include one of these to describe what the id argument represents.
- ``-type <PREFERENCE TYPE>``:
    This describes the type of the preference. Can be entered either using the numerical ID or text id.
    Run ``lua @df.unit_preference.T_type`` for a full list of valid values.
- ``-subtype <ID>``:
    The value for an item's subtype
- ``-material <ID>``:
    The id of the material. For example "MUSHROOM_HELMET_PLUMP:DRINK" or "INORGANIC:IRON".
- ``-state <STATE ID>``:
    The state of the material. Values can be the numerical or text ID.
    Run ``lua @df.matter_state`` for a full list of valid values.
- ``-active <TRUE/FALSE>``:
    Whether the preference is active or not (?)


Other arguments:

- ``-help``:
    Shows this help page.

Example usage:

- Like drinking dwarf blood::

    modtools/pref-edit -add -item -id DRINK -material DWARF:BLOOD -type LikeFood
]====]

local utils = require 'utils'

local validArgs = utils.invert({
    'help',
    "unit",
    "add",
    "remove",
    "has",
    "id",
    "item",
    "type",
    "creature",
    "color",
    "shape",
    "plant",
    "poetic",
    "musical",
    "dance",
    "subtype",
    "material",
    "state",
    "active",
    "removeall"
})

-- Check if unit has a preference matching all the criteria
-- Note: This may give some false positives if filters aren't as precise as they should be
-- Returns the preference's index if found, otherwise returns false
function hasPreference(unit, details)
  for index, preference in ipairs(unit.status.current_soul.preferences) do
    local valid = true
    for variable, value in pairs(details) do
      if preference[variable] ~= value then
        valid = false
        break
      end
    end

    if valid then
      return index
    end
  end

  -- If we get here, we didn't find it
  return false
end

-- Creates and adds a new preference to the unit's list.
-- Won't add a new preference if an existing one already matches the given details.
function addPreference(unit, details)
  -- Avoid adding if preference already exists
  if hasPreference(unit, details) then
    return
  end

  -- The same ID is used across multiple variables. Even if only wanting to modify the creature_id, you must set all others to the same value
  local id = details.item_type or details.creature_id or details.color_id or details.shape_id or details.plant_id or details.poetic_form_id or details.musical_form_id or details.dance_form_id or -1

  local info = {
    new = true,
    type = details.type or 0,
    item_type = id,
    creature_id = id,
    color_id = id,
    shape_id = id,
    plant_id = id,
    poetic_form_id = id,
    musical_form_id = id,
    dance_form_id = id,
    item_subtype = details.item_subtype or -1,
    mattype = details.mattype or -1,
    matindex = details.matindex or -1,
    mat_state = details.mat_state or 0,
    active = details.active or true,
    prefstring_seed = 1, --?
  }

  -- Do prefstring_seed randomisation?
  -- TODO

  unit.status.current_soul.preferences:insert("#", info)
end

-- Changes the provided details of a unit's existing preference at the given index
local function modifyPreference(unit, details, index)
  for k, v in pairs(details) do
    unit.status.current_soul.preferences[index][k] = v
  end
end

-- Removes a preference matching provided details
-- Returns true if preference existed and was removed, false if not
function removePreference(unit, details)
  local index = hasPreference(unit, details)

  if index then
    unit.status.current_soul.preferences:erase(index)
    return true
  else
    return false
  end
end

-- Removes all preferences from a unit
function removeAll(unit)
  while #unit.status.current_soul.preferences > 0 do
    unit.status.current_soul.preferences:erase(0)
  end
end

function main(...)
  local args = utils.processArgs({...}, validArgs)

  if args.help then
    print(help)
    return
  end

  local unit
  if args.unit and tonumber(args.unit) then
    unit = df.unit.find(tonumber(args.unit))
  end

  -- If unit ID wasn't provided / unit couldn't be found,
  -- Try getting selected unit
  if unit == nil then
    unit = dfhack.gui.getSelectedUnit(true)
  end

  if unit == nil then
    qerror("Couldn't find unit.")
  end

  -- Handle Modifiers
  local modifier
  if args.add then
    modifier = "add"
  elseif args.remove then
    modifier = "remove"
  elseif args.has then
    modifier = "has"
  elseif args.removeall then
    modifier = "removeall"
  else
    qerror("Please provide a valid modifier.")
  end

  -- Handle IDs
  local id
  if args.id and tonumber(args.id) then
    id = tonumber(args.id)
  end

  if args.id and not id then -- Gotta find what the ID was representing...
    if args.item then
      id = df.item_type[args.id]
    elseif args.creature then
      for index, raw in ipairs(df.global.world.raws.creatures.all) do
        if raw.creature_id == args.id then
          id = index
          break
        end
      end

      if not id then
        qerror("Couldn't find provided creature")
      end
    elseif args.color then
      for index, raw in ipairs(df.global.world.raws.descriptors.colors) do
        if raw.id == args.id then
          id = index
          break
        end
      end

      if not id then
        qerror("Couldn't find provided color")
      end
    elseif args.shape then
      for index, raw in ipairs(df.global.world.raws.descriptors.shapes) do
        if raw.id == args.id then
          id = index
          break
        end
      end

      if not id then
        qerror("Couldn't find provided shape")
      end
    elseif args.plant then
      for index, raw in ipairs(df.global.world.raws.plants.all) do
        if raw.id == args.id then
          id = index
          break
        end
      end

      if not id then
        qerror("Couldn't find provided plant")
      end
    end
  end

  -- Handle type
  local type
  if args.type and tonumber(args.type) then
    type = tonumber(args.type)
  elseif args.type then
    type = df.unit_preference.T_type[args.type]
  end

  -- Handle material
  local mattype
  local matindex
  if args.material then
    local material = dfhack.matinfo.find(args.material)
    mattype = material.type
    matindex = material.index
  end

  -- Handle mat_state
  local state
  if args.state and tonumber(args.state) then
    state = tonumber(args.state)
  elseif args.mat_state then
    state = df.matter_state[args.mat_state]
  end

  -- Handle active
  local active
  if args.active then
    if args.active:lower() == "true" then
      active = true
    else -- Assume false
      active = false
    end
  end

  -- Build the details table to pass on to other functions
  -- It's fine (and expected) if entries in here are nil
  local details = {
    type = type,
    item_subtype = args.subtype,
    mattype = mattype,
    matindex = matindex,
    mat_state = state,
    active = active,
  }
  -- Add ids to the details
  local idsList = {"item_type", "creature_id", "color_id", "shape_id", "plant_id", "poetic_form_id", "musical_form_id", "dance_form_id"}
  for index, addId in ipairs(idsList) do
    details[addId] = id
  end

  if modifier == "add" then
    addPreference(unit, details)
  elseif modifier == "remove" then
    removePreference(unit, details)
  elseif modifier == "has" then
    if hasPreference(unit, details) then
      print("Unit has matching preference.")
    else
      print("Unit doesn't have matching preference.")
    end
  elseif modifier == "removeall" then
    removeAll(unit)
  end
end

if not dfhack_flags.module then
  main(...)
end
