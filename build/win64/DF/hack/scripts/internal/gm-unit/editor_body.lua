-- Body editor module for gui/gm-unit.
--@ module = true

local dialog = require 'gui.dialogs'
local gui = require 'gui'
local widgets = require 'gui.widgets'
local base_editor = reqscript("internal/gm-unit/base_editor")

-- TODO: Trigger recalculation of body sizes after size is edited

Editor_Body_Modifier=defclass(Editor_Body_Modifier, widgets.Window)
Editor_Body_Modifier.ATTRS{
    frame={w=50, h=20},
    resizable=true,
}

function Editor_Body_Modifier:beautifyString(text)
  local out = text
  out = out:lower() --Make lowercase
  out = out:gsub("_", " ") --Replace underscores with spaces
  out = out:gsub("^%l", string.upper) --capitalises first letter

  return out
end

function Editor_Body_Modifier:setPartModifier(indexList, value)
  for _, index in ipairs(indexList) do
    self.target_unit.appearance.bp_modifiers[index] = tonumber(value)
  end
  self:updateChoices()
end

function Editor_Body_Modifier:setBodyModifier(modifierIndex, value)
  self.target_unit.appearance.body_modifiers[modifierIndex] = tonumber(value)
  self:updateChoices()
end

function Editor_Body_Modifier:selected(index, selected)
  dialog.showInputPrompt(
    self:beautifyString(df.appearance_modifier_type[selected.modifier.entry.type]),
    "Enter new value:",
    nil,
    tostring(selected.value),
    function(newValue)
      local value = tonumber(newValue)
      if self.partChoice.type == "part" then
        self:setPartModifier(selected.modifier.idx, value)
      else -- Body
        self:setBodyModifier(selected.modifier.index, value)
      end
    end,
    nil,nil
  )
end

function Editor_Body_Modifier:random()
  local _, selected = self.subviews.modifiers:getSelected()
  -- How modifier randomisation works (to my knowledge):
  -- 7 values are listed in the _APPEARANCE_MODIFIER token
  -- One of the first 6 values is randomly selected with the same odds for any
  -- A random number is rolled within the range of that number, and the next one to get the modifier value

  local startIndex = rng:random(6) -- Will give a number between 0-5 which, when accounting for the fact that the range table starts at 0, gives us the index of which of the first 6 to use

  -- Set the ranges
  local min = selected.modifier.entry.ranges[startIndex]
  local max = selected.modifier.entry.ranges[startIndex+1]

  -- Get the difference between the two
  local difference = math.abs(min - max)

  -- Use the minimum, the difference, and a random roll to work out the new value.
  local roll = rng:random(difference+1) -- difference + 1 because we want to include the max value as an option
  local value = min + roll

  -- Set the modifier to the new value
  if self.partChoice.type == "part" then
    self:setPartModifier(selected.modifier.idx, value)
  else
    self:setBodyModifier(selected.modifier.index, value)
  end
end

function Editor_Body_Modifier:step(amount)
  local _, selected = self.subviews.modifiers:getSelected()

  -- Build a table of description ranges
  local ranges = {}
  for index, value in ipairs(selected.modifier.entry.desc_range) do
    -- Only add a new entry if: There are none, or the value is higher than the previous range
    if #ranges == 0 or value > ranges[#ranges] then
      table.insert(ranges, value)
    end
  end

  -- Now determine what range the modifier currently falls into
  local currentValue = selected.value
  local rangeIndex

  for index, value in ipairs(ranges) do
    if ranges[index+1] then -- There's still a next entry
      if currentValue < ranges[index+1] then -- The current value is less than the next entry
        rangeIndex = index
        break
      end
    else -- This is the last entry
      rangeIndex = index
    end
  end

  -- Finally, move the modifier's value up / down in range tiers based on given amount
  local newTier = math.min(#ranges, math.max(1, rangeIndex + amount)) -- Clamp values to not go beyond bounds of ranges
  local newValue = ranges[newTier]

  if self.partChoice.type == "part" then
    self:setPartModifier(selected.modifier.idx, newValue)
  else
    self:setBodyModifier(selected.modifier.index, newValue)
  end
end

function Editor_Body_Modifier:updateChoices()
  local choices = {}

  for index, modifier in ipairs(self.partChoice.modifiers) do
    local currentValue
    if self.partChoice.type == "part" then
      currentValue = self.target_unit.appearance.bp_modifiers[modifier.idx[1]]
    else -- Body
      currentValue = self.target_unit.appearance.body_modifiers[modifier.index]
    end
    table.insert(choices, {text = self:beautifyString(df.appearance_modifier_type[modifier.entry.type]) .. ": " .. currentValue, value = currentValue, modifier = modifier})
  end

  self.subviews.modifiers:setChoices(choices)
end

function Editor_Body_Modifier:init(args)
  self.target_unit = args.target_unit

  self:addviews{
    widgets.List{
      frame = {t=0, b=2,l=1},
      view_id = "modifiers",
      on_submit = self:callback("selected"),
    },
    widgets.Label{
      frame = {b=1, l=1},
      text = {
        {text = ": back ", key = "LEAVESCREEN"},
        {text = ": edit modifier ", key = "SELECT"},
        {text = ": raise ", key = "KEYBOARD_CURSOR_RIGHT", on_activate = self:callback("step", 1)},
      },
    },
    widgets.Label{
      frame = {b=0, l=1},
      text = {
        {text = ": reduce ", key = "KEYBOARD_CURSOR_LEFT", on_activate = self:callback("step", -1)},
        {text = ": randomise selected", key = "CUSTOM_R", on_activate = self:callback("random")},
      },
    }
  }
end

function Editor_Body_Modifier:onInput(keys)
    if keys.LEAVESCREEN or keys._MOUSE_R_DOWN then
        self:setFocus(false)
        self.visible = false
    else
        Editor_Body_Modifier.super.onInput(self, keys)
    end
    return true -- we're modal
end

Editor_Body=defclass(Editor_Body, base_editor.Editor)
Editor_Body.ATTRS{
    frame_title = "Body appearance editor"
}

function makePartList(caste)
  local list = {}
  local lookup = {} -- Stores existing part's index in the list

  for index, modifier in ipairs(caste.bp_appearance.modifiers) do
    local name
    if modifier.noun ~= "" then
      name = modifier.noun
    else
      name = caste.body_info.body_parts[modifier.body_parts[0]].name_singular[0].value -- Use the name of the first body part modified
    end

    -- Make a new entry if this is a new part
    if lookup[name] == nil then
      local entryIndex = #list + 1
      table.insert(list, {name = name, modifiers = {}})
      lookup[name] = entryIndex
    end

    -- Find idxes associated with this modifier. These are what will be used later when setting the unit's appearance
    local idx = {}
    for searchIndex, modifierId in ipairs(caste.bp_appearance.modifier_idx) do
      if modifierId == index then
        table.insert(idx, searchIndex)
      end
    end

    -- Add modifiers to list of part
    table.insert(list[lookup[name]].modifiers, {index = index, entry = modifier, idx = idx})
  end

  return list
end

function Editor_Body:updateChoices()
  local choices = {}
  local caste = df.creature_raw.find(self.target_unit.race).caste[self.target_unit.caste]

  -- Body is a special case
  if #caste.body_appearance_modifiers > 0 then
    local bodyEntry = {text = "Body", modifiers = {}, type = "body"}
    for index, modifier in ipairs(caste.body_appearance_modifiers) do
      table.insert(bodyEntry.modifiers, {index = index, entry = modifier})
    end
    table.insert(choices, bodyEntry)
  end

  local partList = makePartList(caste)
  for index, partEntry in ipairs(partList) do
    table.insert(choices, {text = partEntry.name:gsub("^%l", string.upper), modifiers = partEntry.modifiers, type = "part"})
  end

  self.subviews.featureSelect:setChoices(choices)
end

function Editor_Body:partSelected(index, choice)
  local modifier = self.subviews.modifier
  modifier.visible = true
  modifier:setFocus(true)
  modifier.partChoice = choice
  modifier:updateChoices()
  modifier.frame_title = choice.text .. " - Select a modifier"
end

function Editor_Body:init(args)
  if self.target_unit == nil then
    qerror("invalid unit")
  end

  self:addviews{
    widgets.List{
      frame = {t=0, b=2,l=0},
      view_id = "featureSelect",
      on_submit = self:callback("partSelected"),
    },
    widgets.Label{
      frame = {b=0, l=0},
      text = {
        {text = ": select feature ", key = "SELECT"},
      },
    },
    Editor_Body_Modifier{
      view_id = 'modifier',
      visible = false,
      target_unit = self.target_unit,
    },
  }

  self:updateChoices()
end
