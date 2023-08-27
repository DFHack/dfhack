-- A front-end for the teleport script

--[====[

gui/teleport
============

A front-end for the `teleport` script that allows choosing a unit and destination
using the in-game cursor.

]====]

guidm = require 'gui.dwarfmode'
widgets = require 'gui.widgets'

function uiMultipleUnits()
    return #df.global.game.unit_cursor.list > 1
end

TeleportSidebar = defclass(TeleportSidebar, guidm.MenuOverlay)

TeleportSidebar.ATTRS = {
    sidebar_mode=df.ui_sidebar_mode.ViewUnits,
}

function TeleportSidebar:init()
    self:addviews{
        widgets.Label{
            frame = {b=1, l=1},
            text = {
                {key = 'UNITJOB_ZOOM_CRE',
                    text = ': Zoom to unit, ',
                    on_activate = self:callback('zoom_unit'),
                    enabled = function() return self.unit end},
                {key = 'UNITVIEW_NEXT', text = ': Next',
                    on_activate = self:callback('next_unit'),
                    enabled = uiMultipleUnits},
                NEWLINE,
                NEWLINE,
                {key = 'SELECT', text = ': Choose, ', on_activate = self:callback('choose')},
                {key = 'LEAVESCREEN', text = ': Back', on_activate = self:callback('back')},
                NEWLINE,
                {key = 'LEAVESCREEN_ALL', text = ': Exit to map', on_activate = self:callback('dismiss')},
            },
        },
    }
    self.in_pick_pos = false
end

function TeleportSidebar:choose()
    if not self.in_pick_pos then
        self.in_pick_pos = true
    else
        dfhack.units.teleport(self.unit, xyz2pos(pos2xyz(df.global.cursor)))
        self:dismiss()
    end
end

function TeleportSidebar:back()
    if self.in_pick_pos then
        self.in_pick_pos = false
    else
        self:dismiss()
    end
end

function TeleportSidebar:zoom_unit()
    df.global.cursor:assign(xyz2pos(pos2xyz(self.unit.pos)))
    self:getViewport():centerOn(self.unit.pos):set()
end

function TeleportSidebar:next_unit()
    self:sendInputToParent('UNITVIEW_NEXT')
end

function TeleportSidebar:onRenderBody(p)
    p:seek(1, 1):pen(COLOR_WHITE)
    if self.in_pick_pos then
        p:string('Select destination'):newline(1):newline(1)

        local cursor = df.global.cursor
        local block = dfhack.maps.getTileBlock(pos2xyz(cursor))
        if block then
            p:string(df.tiletype[block.tiletype[cursor.x % 16][cursor.y % 16]], COLOR_CYAN)
        else
            p:string('Unknown tile', COLOR_RED)
        end
    else
        self.unit = dfhack.gui.getAnyUnit(self._native.parent)
        p:string('Select unit:'):newline(1):newline(1)
        if self.unit then
            local name = dfhack.TranslateName(dfhack.units.getVisibleName(self.unit))
            p:string(name)
            if name ~= '' then p:newline(1) end
            p:string(dfhack.units.getProfessionName(self.unit), dfhack.units.getProfessionColor(self.unit))
            p:newline(1)
        else
            p:string('No unit selected', COLOR_LIGHTRED)
        end
    end
end

function TeleportSidebar:onInput(keys)
    TeleportSidebar.super.onInput(self, keys)
    TeleportSidebar.super.propagateMoveKeys(self, keys)
end

function TeleportSidebar:onGetSelectedUnit()
    return self.unit
end

if not dfhack.isMapLoaded() then
    qerror('This script requires a fortress map to be loaded')
end

TeleportSidebar():show()
