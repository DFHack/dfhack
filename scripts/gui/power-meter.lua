-- Interface front-end for power-meter plugin.

local utils = require 'utils'
local gui = require 'gui'
local guidm = require 'gui.dwarfmode'
local dlg = require 'gui.dialogs'

local plugin = require('plugins.power-meter')
local bselector = df.global.ui_build_selector

PowerMeter = defclass(PowerMeter, guidm.MenuOverlay)

PowerMeter.focus_path = 'power-meter'

PowerMeter.ATTRS {
    frame_background = false
}

function PowerMeter:init()
    self:assign{
        min_power = 0, max_power = -1, invert = false,
    }
end

function PowerMeter:onShow()
    PowerMeter.super.onShow(self)

    -- Send an event to update the errors
    bselector.plate_info.flags.whole = 0
    self:sendInputToParent('BUILDING_TRIGGER_ENABLE_WATER')
end

function PowerMeter:onRenderBody(dc)
    dc:fill(0,0,dc.width-1,13,gui.CLEAR_PEN)
    dc:seek(1,1):pen(COLOR_WHITE)
    dc:string("Power Meter"):newline():newline(1)
    dc:string("Placement"):newline():newline(1)

    dc:string("Excess power range:")

    dc:newline(3):key('BUILDING_TRIGGER_MIN_WATER_DOWN')
    dc:key('BUILDING_TRIGGER_MIN_WATER_UP')
    dc:string(": Min ")
    if self.min_power <= 0 then
        dc:string("(any)")
    else
        dc:string(''..self.min_power)
    end

    dc:newline(3):key('BUILDING_TRIGGER_MAX_WATER_DOWN')
    dc:key('BUILDING_TRIGGER_MAX_WATER_UP')
    dc:string(": Max ")
    if self.max_power < 0 then
        dc:string("(any)")
    else
        dc:string(''..self.max_power)
    end
    dc:newline():newline(1)

    dc:key('CUSTOM_I'):string(": ")
    if self.invert then
        dc:string("Inverted")
    else
        dc:string("Not inverted")
    end
end

function PowerMeter:onInput(keys)
    if keys.CUSTOM_I then
        self.invert = not self.invert
    elseif keys.BUILDING_TRIGGER_MIN_WATER_UP then
        self.min_power = self.min_power + 10
    elseif keys.BUILDING_TRIGGER_MIN_WATER_DOWN then
        self.min_power = math.max(0, self.min_power - 10)
    elseif keys.BUILDING_TRIGGER_MAX_WATER_UP then
        if self.max_power < 0 then
            self.max_power = 0
        else
            self.max_power = self.max_power + 10
        end
    elseif keys.BUILDING_TRIGGER_MAX_WATER_DOWN then
        self.max_power = math.max(-1, self.max_power - 10)
    elseif keys.LEAVESCREEN then
        self:dismiss()
        self:sendInputToParent('LEAVESCREEN')
    elseif keys.SELECT then
        if #bselector.errors == 0 then
            if not plugin.makePowerMeter(
                bselector.plate_info,
                self.min_power, self.max_power, self.invert
            )
            then
                dlg.showMessage(
                    'Power Meter',
                    'Could not initialize.', COLOR_LIGHTRED
                )

                self:dismiss()
                self:sendInputToParent('LEAVESCREEN')
                return
            end

            self:sendInputToParent('SELECT')
            if bselector.stage ~= 1 then
                self:dismiss()
            end
        end
    elseif self:propagateMoveKeys(keys) then
        return
    end
end

if dfhack.gui.getCurFocus() ~= 'dwarfmode/Build/Position/Trap'
or bselector.building_subtype ~= df.trap_type.PressurePlate
then
    qerror("This script requires the main dwarfmode view in build pressure plate mode")
end

local list = PowerMeter()
list:show()
