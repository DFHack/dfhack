-- Rename various objects via gui.

local gui = require 'gui'
local dlg = require 'gui.dialogs'
local plugin = require 'plugins.rename'

local mode = ...
local focus = dfhack.gui.getCurFocus()

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
        dlg.showInputPrompt(
            'Rename Building',
            'Enter a new name for the building:', COLOR_GREEN,
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
        dlg.showInputPrompt(
            'Rename Unit',
            'Enter a new profession for the unit:', COLOR_GREEN,
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

        dlg.showInputPrompt(
            'Rename Unit',
            'Enter a new nickname for the unit:', COLOR_GREEN,
            vnick,
            curry(dfhack.units.setNickname, unit)
        )
    end
elseif mode then
    verify_mode(nil)
end
