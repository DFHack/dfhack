-- Rename various objects via gui.
--[====[

gui/rename
==========
Backed by `rename`, this script allows entering the desired name
via a simple dialog in the game ui.

* ``gui/rename [building]`` in :kbd:`q` mode changes the name of a building.

  .. image:: /docs/images/rename-bld.png

  The selected building must be one of stockpile, workshop, furnace, trap, or siege engine.
  It is also possible to rename zones from the :kbd:`i` menu.

* ``gui/rename [unit]`` with a unit selected changes the nickname.

  Unlike the built-in interface, this works even on enemies and animals.

* ``gui/rename unit-profession`` changes the selected unit's custom profession name.

  .. image:: /docs/images/rename-prof.png

  Likewise, this can be applied to any unit, and when used on animals it overrides
  their species string.

The ``building`` or ``unit`` options are automatically assumed when in relevant UI state.

]====]
local gui = require 'gui'
local dlg = require 'gui.dialogs'
local widgets = require 'gui.widgets'
local plugin = require 'plugins.rename'

local mode = ...
local focus = dfhack.gui.getCurFocus()

RenameDialog = defclass(RenameDialog, dlg.InputBox)
function RenameDialog:init(info)
    self:addviews{
        widgets.Label{
            view_id = 'controls',
            text = {
                {key = 'CUSTOM_ALT_C', text = ': Clear, ',
                    on_activate = function()
                        self.subviews.edit.text = ''
                    end},
                {key = 'CUSTOM_ALT_S', text = ': Special chars',
                    on_activate = curry(dfhack.run_script, 'gui/cp437-table')},
            },
            frame = {b = 0, l = 0, r = 0, w = 70},
        }
    }
    -- calculate text_width once
    self.subviews.controls:getTextWidth()
end

function RenameDialog:getWantedFrameSize()
    local x, y = self.super.getWantedFrameSize(self)
    x = math.max(x, self.subviews.controls.text_width)
    return x, y + 2
end

function showRenameDialog(title, text, input, on_input)
    RenameDialog{
        frame_title = title,
        text = text,
        text_pen = COLOR_GREEN,
        input = input,
        on_input = on_input,
    }:show()
end

local function verify_mode(expected)
    if mode ~= nil and mode ~= expected then
        qerror('Invalid UI state for mode '..mode)
    end
end

local unit = dfhack.gui.getSelectedUnit(true)
local building = dfhack.gui.getSelectedBuilding(true)

if building and (not unit or mode == 'building') then
    verify_mode('building')

    if plugin.canRenameBuilding(building) then
        showRenameDialog(
            'Rename Building',
            'Enter a new name for the building:',
            building.name,
            curry(plugin.renameBuilding, building)
        )
    else
        dlg.showMessage(
            'Rename Building',
            'Cannot rename this type of building.', COLOR_LIGHTRED
        )
    end
elseif unit then
    if mode == 'unit-profession' then
        showRenameDialog(
            'Rename Unit',
            'Enter a new profession for the unit:',
            unit.custom_profession,
            function(newval)
                unit.custom_profession = newval
            end
        )
    else
        verify_mode('unit')

        local vname = dfhack.units.getVisibleName(unit)
        local vnick = ''
        if vname and vname.has_name then
            vnick = vname.nickname
        end

        showRenameDialog(
            'Rename Unit',
            'Enter a new nickname for the unit:',
            vnick,
            curry(dfhack.units.setNickname, unit)
        )
    end
elseif mode then
    verify_mode(nil)
end
