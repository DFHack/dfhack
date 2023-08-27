-- Beliefs editor module for gui/gm-unit.
--@ module = true

local dialog = require 'gui.dialogs'
local widgets = require 'gui.widgets'
local base_editor = reqscript("internal/gm-unit/base_editor")
local setbelief = reqscript("modtools/set-belief")
local setneed = reqscript("modtools/set-need")

Editor_Beliefs = defclass(Editor_Beliefs, base_editor.Editor)
Editor_Beliefs.ATTRS{
  frame_title = "Beliefs editor"
}

function Editor_Beliefs:randomiseSelected()
  local index, choice = self.subviews.beliefs:getSelected()

  setbelief.randomiseUnitBelief(self.target_unit, choice.belief)
  self:updateChoices()
end

function Editor_Beliefs:step(amount)
  local index, choice = self.subviews.beliefs:getSelected()

  setbelief.stepUnitBelief(self.target_unit, choice.belief, amount)
  self:updateChoices()
end

function Editor_Beliefs:updateChoices()
  local choices = {}

  for index, belief in ipairs(df.value_type) do
    if belief ~= 'NONE' then
      local niceText = belief
      niceText = niceText:lower()
      niceText = niceText:gsub("_", " ")
      niceText = niceText:gsub("^%l", string.upper)

      local strength = setbelief.getUnitBelief(self.target_unit, index)
      local symbolAddition = ""
      if setbelief.isCultureBelief(self.target_unit, index) then
        symbolAddition = "*"
      end

      table.insert(choices, {["text"] = niceText .. ": " .. strength .. symbolAddition, ["belief"] = index, ["value"] = strength, ["name"] = niceText})
    end
  end

  self.subviews.beliefs:setChoices(choices)
end

function Editor_Beliefs:average(index, choice)
  setbelief.removeUnitBelief(self.target_unit, choice.belief)
  self:updateChoices()
end

function Editor_Beliefs:edit(index, choice)
  dialog.showInputPrompt(
    choice.name,
    "Enter new value:",
    COLOR_WHITE,
    tostring(choice.value),
    function(newValue)
      setbelief.setUnitBelief(self.target_unit, choice.belief, tonumber(newValue), true)
      self:updateChoices()
    end
  )
end

function Editor_Beliefs:onClose()
  setneed.rebuildNeeds(self.target_unit)
end

function Editor_Beliefs:init(args)
  if self.target_unit==nil then
      qerror("invalid unit")
  end

  self:addviews{
    widgets.List{
      frame = {t=0, b=5,l=0},
      view_id = "beliefs",
      on_submit = self:callback("edit"),
    },
    widgets.Label{
      frame = {b=0, l=0},
      text = {
        {text = ": edit value ", key = "SELECT"},
        {text = ": randomise selected ", key = "CUSTOM_R", on_activate = self:callback("randomiseSelected")},
        NEWLINE,
        {text = ": raise ", key = "KEYBOARD_CURSOR_RIGHT", on_activate = self:callback("step", 1)},
        {text = ": reduce", key = "KEYBOARD_CURSOR_LEFT", on_activate = self:callback("step", -1)},
        NEWLINE,
        {text = "* denotes cultural default "},
        NEWLINE,
        {text = ": set to cultural default", key = "STRING_A047", on_activate = function()
            self:average(self.subviews.beliefs:getSelected())
         end}
      },
    },
  }

  self:updateChoices()
end
