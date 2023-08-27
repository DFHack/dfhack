-- Orientation editor module for gui/gm-unit.
--@ module = true

local widgets = require 'gui.widgets'
local setorientation = dfhack.reqscript("set-orientation")
local base_editor = reqscript("internal/gm-unit/base_editor")


Editor_Orientation=defclass(Editor_Orientation, base_editor.Editor)
Editor_Orientation.ATTRS{
    frame_title = "Orientation editor"
}

function Editor_Orientation:sexSelected(index, choice)
  local newInterest = choice.interest + 1
  -- Cycle back around if out of bounds
  if newInterest > 2 then
    newInterest = 0
  end

  setorientation.setOrientation(self.target_unit, choice.sex, newInterest)
  self:updateChoices()
end

function Editor_Orientation:random()
  local index, choice = self.subviews.sex:getSelected()

  setorientation.randomiseOrientation(self.target_unit, choice.sex)
  self:updateChoices()
end

function Editor_Orientation:updateChoices()
  local choices = {}
  -- Male
  local maleInterest = setorientation.getInterest(self.target_unit, "male")
  local maleInterestString = setorientation.getInterestString(maleInterest)
  table.insert(choices, {text = "Male: " .. maleInterestString, interest = maleInterest, sex = 1})
  -- Female
  local femaleInterest = setorientation.getInterest(self.target_unit, "female")
  local femaleInterestString = setorientation.getInterestString(femaleInterest)
  table.insert(choices, {text = "Female: " .. femaleInterestString, interest = femaleInterest, sex = 0})

  self.subviews.sex:setChoices(choices)
end

function Editor_Orientation:init(args)
  if self.target_unit == nil then
    qerror("invalid unit")
  end

  self:addviews{
    widgets.List{
      frame = {t=0, b=2,l=0},
      view_id = "sex",
      on_submit = self:callback("sexSelected"),
    },
    widgets.Label{
      frame = {b=0, l=0},
      text = {
        {text = ": cycle selected ", key = "SELECT"},
        {text = ": randomise selected", key = "CUSTOM_R", on_activate = self:callback("random")},
      },
    }
  }

  self:updateChoices()
end
