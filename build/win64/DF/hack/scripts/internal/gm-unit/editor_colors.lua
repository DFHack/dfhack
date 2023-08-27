-- Colors editor module for gui/gm-unit.
--@ module = true

local dialog = require 'gui.dialogs'
local widgets = require 'gui.widgets'
local base_editor = reqscript("internal/gm-unit/base_editor")

Editor_Colors=defclass(Editor_Colors, base_editor.Editor)
Editor_Colors.ATTRS{
    frame_title = "Colors editor"
}

function patternString(patternId)
  local pattern = df.descriptor_pattern.find(patternId)
  local prefix
  if pattern.pattern == 0 then --Monochrome
    return df.descriptor_color.find(pattern.colors[0]).name
  elseif pattern.pattern == 1 then --Stripes
    prefix = "striped"
  elseif pattern.pattern == 2 then --Iris_eye
    return df.descriptor_color.find(pattern.colors[2]).name .. " eyes"
  elseif pattern.pattern == 3 then --Spots
    prefix = "spotted" --that's a guess
  elseif pattern.pattern == 4 then --Pupil_eye
    return df.descriptor_color.find(pattern.colors[2]).name .. " eyes"
  elseif pattern.pattern == 5 then --mottled
    prefix = "mottled"
  end
  local out = prefix .. " "
  for i=0, #pattern.colors-1 do
    if i == #pattern.colors-1 then
      out = out .. "and " .. df.descriptor_color.find(pattern.colors[i]).name
    elseif i == #pattern.colors-2 then
      out = out .. df.descriptor_color.find(pattern.colors[i]).name .. " "
    else
      out = out .. df.descriptor_color.find(pattern.colors[i]).name .. ", "
    end
  end
  return out
end

function Editor_Colors:random()
  local featureChoiceIndex, featureChoice = self.subviews.features:getSelected() -- This is the part / feature that's selected
  local caste = df.creature_raw.find(self.target_unit.race).caste[self.target_unit.caste]

  -- Nil check in case there are no features
  if featureChoiceIndex == nil then
    return
  end

  local options = {}

  for index, patternId in ipairs(featureChoice.mod.pattern_index) do
    local addition = {}
    addition.patternId = patternId
    addition.index = index -- This is the position of the pattern within the modifier index. It's this value (not the pattern ID), that's used in the unit appearance to select their color
    addition.weight = featureChoice.mod.pattern_frequency[index]
    table.insert(options, addition)
  end

  -- Now create a table from this to use for the weighted roller
  -- We'll use the index as the item appears in options for the id
  local weightedTable = {}
  for index, entry in ipairs(options) do
    local addition = {}
    addition.id = index
    addition.weight = entry.weight
    table.insert(weightedTable, addition)
  end

  -- Roll randomly. The result will give us the index of the option to use
  local result = weightedRoll(weightedTable)

  -- Set the unit's appearance for the feature to the new pattern
  self.target_unit.appearance.colors[featureChoice.index] = options[result].index

  -- Notify the user on the change, so they get some visual feedback that something has happened
  local pluralWord
  if featureChoice.mod.unk_6c == 1 then
    pluralWord = "are"
  else
    pluralWord = "is"
  end

  dialog.showMessage("Color randomised!",
    featureChoice.text .. " " .. pluralWord .." now " .. patternString(options[result].patternId),
    nil, nil)
end

function Editor_Colors:colorSelected(index, choice)
  -- Update the modifier for the unit
  self.target_unit.appearance.colors[self.modIndex] = choice.index
end

function Editor_Colors:featureSelected(index, choice)
  -- Store the index of the modifier we're editing
  self.modIndex = choice.index

  -- Generate color choices
  local colorChoices = {}

  for index, patternId in ipairs(choice.mod.pattern_index) do
    table.insert(colorChoices, {text = patternString(patternId), index = index})
  end

  dialog.showListPrompt(
    "Choose color",
    "Select feature's color", nil,
    colorChoices,
    function(selectIndex, selectChoice)
      self:colorSelected(selectIndex, selectChoice)
    end,
    nil, nil,
    true
  )
end

function Editor_Colors:updateChoices()
  local caste = df.creature_raw.find(self.target_unit.race).caste[self.target_unit.caste]
  local choices = {}
  for index, colorMod in ipairs(caste.color_modifiers) do
    table.insert(choices, {text = colorMod.part:gsub("^%l", string.upper), mod = colorMod, index = index})
  end

  self.subviews.features:setChoices(choices)
end

function Editor_Colors:init(args)
  if self.target_unit == nil then
    qerror("invalid unit")
  end

  self:addviews{
    widgets.List{
      frame = {t=0, b=2,l=0},
      view_id = "features",
      on_submit = self:callback("featureSelected"),
    },
    widgets.Label{
      frame = {b=0, l=0},
      text = {
        {text = ": edit feature ", key = "SELECT"},
        {text = ": randomise color", key = "CUSTOM_R", on_activate = self:callback("random")},
      },
    }
  }

  self:updateChoices()
end
