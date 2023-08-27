-- Interface powered, user friendly, unit editor

local gui = require('gui')
local widgets = require('gui.widgets')
local args = {...}

rng = rng or dfhack.random.new(nil, 10)

local target
--TODO: add more ways to guess what unit you want to edit
if args[1] ~= nil then
    target = df.unit.find(args[1])
else
    target = dfhack.gui.getSelectedUnit(true)
end

if target == nil then
    qerror("Please select a unit to edit in the game UI")
end
local editors = {}
function add_editor(editor_class)
    table.insert(editors, editor_class)
end

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


-------------------------------various subeditors---------
------- skill editor
local editor_skills = reqscript("internal/gm-unit/editor_skills")
add_editor(editor_skills.Editor_Skills)

------- civilization editor
local editor_civ = reqscript("internal/gm-unit/editor_civilization")
add_editor(editor_civ.Editor_Civ)

------- counters editor
local editor_counters = reqscript("internal/gm-unit/editor_counters")
add_editor(editor_counters.Editor_Counters)

------- profession editor
local editor_prof = reqscript("internal/gm-unit/editor_profession")
add_editor(editor_prof.Editor_Prof)

------- wounds editor
local editor_wounds = reqscript("internal/gm-unit/editor_wounds")
add_editor(editor_wounds.Editor_Wounds)

------- attributes editor
local editor_attrs = reqscript("internal/gm-unit/editor_attributes")
add_editor(editor_attrs.Editor_Attrs)

------- orientation editor
local editor_orientation = reqscript("internal/gm-unit/editor_orientation")
add_editor(editor_orientation.Editor_Orientation)

------- body / body part editor
local editor_body = reqscript("internal/gm-unit/editor_body")
add_editor(editor_body.Editor_Body)

------- colors editor
local editor_colors = reqscript("internal/gm-unit/editor_colors")
add_editor(editor_colors.Editor_Colors)

------- beliefs editor
local editor_beliefs = reqscript("internal/gm-unit/editor_beliefs")
add_editor(editor_beliefs.Editor_Beliefs)

------- personality editor
local editor_personality = reqscript("internal/gm-unit/editor_personality")
add_editor(editor_personality.Editor_Personality)

-------------------------------main window----------------
local MAIN_TITLE = "GameMaster's unit editor"

Editor_Unit = defclass(Editor_Unit, widgets.Window)
Editor_Unit.ATTRS{
    frame_title=MAIN_TITLE,
    frame={t=20, r=3, w=50, h=30},
    resizable=true,
    target_unit = DEFAULT_NIL,
}

function Editor_Unit:init()
    local main = widgets.FilteredList{
            view_id='main_list',
            choices=self:make_choices(),
            on_submit=function(_,choice) choice.on_submit() end
        }

    local pages = {main}
    for _,editor in ipairs(editors) do
        table.insert(pages,
                editor{view_id=editor.ATTRS.frame_title,
                    target_unit=self.target_unit})
    end

    self:addviews{
        widgets.Pages{
            view_id='pages',
            frame={t=0, l=0, b=2},
            subviews=pages,
        },
        widgets.HotkeyLabel{
            frame={b=0, l=0},
            label='Back',
            key='LEAVESCREEN',
        },
    }
end

function Editor_Unit:make_choices()
    local choices = {}
    for _,editor in ipairs(editors) do
        local title = editor.ATTRS.frame_title
        table.insert(choices, {text=title, search_key=title:lower(),
                on_submit=function()
                    self.frame_title = title
                    local pages = self.subviews.pages
                    pages:setSelected(title)
                    local page = pages:getSelectedPage()
                    if page.onOpen then page:onOpen() end
                end})
    end
    return choices
end

function Editor_Unit:onInput(keys)
    local pages = self.subviews.pages
    if pages:getSelected() == 1 or
            (not keys.LEAVESCREEN and not keys._MOUSE_R_DOWN) then
        return Editor_Unit.super.onInput(self, keys)
    end
    local page = pages:getSelectedPage()
    if page.onClose then page:onClose() end
    self.frame_title = MAIN_TITLE
    pages:setSelected(1)
    self.subviews.main_list.edit:setFocus(true)
    return true
end

-------------------------------screen management----------------

Editor_Screen = defclass(Editor_Screen, gui.ZScreen)
Editor_Screen.ATTRS {
    focus_path='gm-unit',
    target_unit=DEFAULT_NIL,
}

function Editor_Screen:init()
    self:addviews{Editor_Unit{target_unit=self.target_unit}}
end

function Editor_Screen:onDismiss()
    view = nil
end

view = view and view:raise() or Editor_Screen{target_unit=target}:show()
