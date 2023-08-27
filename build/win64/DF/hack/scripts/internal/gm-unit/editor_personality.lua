-- Beliefs editor module for gui/gm-unit.
--@ module = true

local dialog = require 'gui.dialogs'
local widgets = require 'gui.widgets'
local base_editor = reqscript("internal/gm-unit/base_editor")
local setpersonality = dfhack.reqscript("modtools/set-personality")
local setneed = dfhack.reqscript("modtools/set-need")


Editor_Personality = defclass(Editor_Personality, base_editor.Editor)
Editor_Personality.ATTRS{
    frame_title = "Personality editor"
}

function Editor_Personality:randomiseSelected()
  local index, choice = self.subviews.traits:getSelected()

  setpersonality.randomiseUnitTrait(self.target_unit, choice.trait)
  self:updateChoices()
end

function Editor_Personality:step(amount)
  local index, choice = self.subviews.traits:getSelected()

  setpersonality.stepUnitTrait(self.target_unit, choice.trait, amount)
  self:updateChoices()
end

function Editor_Personality:updateChoices()
  local choices = {}

  for index, traitName in ipairs(df.personality_facet_type) do
    if traitName ~= 'NONE' then
      local niceText = traitName
      niceText = niceText:lower()
      niceText = niceText:gsub("_", " ")
      niceText = niceText:gsub("^%l", string.upper)

      local strength = setpersonality.getUnitTraitBase(self.target_unit, index)

      table.insert(choices, {["text"] = niceText .. ": " .. strength, ["trait"] = index, ["value"] = strength, ["name"] = niceText})
    end
  end

  self.subviews.traits:setChoices(choices)
end

function Editor_Personality:averageTrait(index, choice)
  setpersonality.averageUnitTrait(self.target_unit, choice.trait)
  self:updateChoices()
end

function Editor_Personality:editTrait(index, choice)
  dialog.showInputPrompt(
    choice.name,
    "Enter new value:",
    COLOR_WHITE,
    tostring(choice.value),
    function(newValue)
      setpersonality.setUnitTrait(self.target_unit, choice.trait, tonumber(newValue))
      self:updateChoices()
    end
  )
end

function Editor_Personality:onClose()
  setneed.rebuildNeeds(self.target_unit)
end

function Editor_Personality:init(args)
  if self.target_unit==nil then
      qerror("invalid unit")
  end

  self:addviews{
    widgets.List{
      frame = {t=0, b=4,l=0},
      view_id = "traits",
      on_submit = self:callback("editTrait"),
    },
    widgets.Label{
      frame = {b=0, l=0},
      text = {
        {text = ": edit value ", key = "SELECT"},
        {text = ": randomise selected ", key = "CUSTOM_R", on_activate = self:callback("randomiseSelected")},
        NEWLINE,
        {text = ": raise ", key = "KEYBOARD_CURSOR_RIGHT", on_activate = self:callback("step", 1)},
        {text = ": reduce ", key = "KEYBOARD_CURSOR_LEFT", on_activate = self:callback("step", -1)},
        {text = ": set to caste average", key = "STRING_A047", on_activate = function()
            self:averageTrait(self.subviews.traits:getSelected())
         end},
      },
    },
  }

  self:updateChoices()
end
