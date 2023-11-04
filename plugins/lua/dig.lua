local _ENV = mkmodule('plugins.dig')

local gui = require('gui')
local overlay = require('plugins.overlay')
local pathable = require('plugins.pathable')
local widgets = require('gui.widgets')

WarmDampOverlay = defclass(WarmDampOverlay, overlay.OverlayWidget)
WarmDampOverlay.ATTRS{
    default_pos={x=50,y=-6},
    default_enabled=true,
    version=2,
    viewscreens={
        'dwarfmode/Designate/DIG_DIG',
        'dwarfmode/Designate/DIG_REMOVE_STAIRS_RAMPS',
        'dwarfmode/Designate/DIG_STAIR_UP',
        'dwarfmode/Designate/DIG_STAIR_UPDOWN',
        'dwarfmode/Designate/DIG_STAIR_DOWN',
        'dwarfmode/Designate/DIG_RAMP',
        'dwarfmode/Designate/DIG_CHANNEL',
        'dwarfmode/Designate/DIG_FROM_MARKER',
        'dwarfmode/Designate/DIG_TO_MARKER',
    },
    default_enabled=true,
    frame={w=40, h=3},
    frame_style=gui.FRAME_MEDIUM,
}

function WarmDampOverlay:init()
    self:addviews{
        widgets.ToggleHotkeyLabel{
            frame={t=0, l=0},
            key='CUSTOM_CTRL_W'
            label='Dig through warm/damp tiles',
            initial_option=false,
        },
    }
end

function WarmDampOverlay:onRenderFrame(dc, rect)
    WarmDampOverlay.super.onRenderFrame(self, dc, rect)
    pathable.paintScreenWarmDamp()
end

local
function WarmDampOverlay:onInput(keys)
    if WarmDampOverlay.super.onInput(self, keys) then
        return true
    end
    if keys._MOUSE_L_DOWN then
        local pos = dfhack.gui.getMousePos()

    end
end

CarveOverlay = defclass(CarveOverlay, overlay.OverlayWidget)
CarveOverlay.ATTRS{
    viewscreens={
        'dwarfmode/Designate/SMOOTH',
        'dwarfmode/Designate/ENGRAVE',
        'dwarfmode/Designate/TRACK',
        'dwarfmode/Designate/FORTIFY',
        'dwarfmode/Designate/ERASE',
    },
    default_enabled=true,
    overlay_only=true,
}

function CarveOverlay:onRenderFrame()
    pathable.paintScreenCarve()
end

OVERLAY_WIDGETS = {
    asciiwarmdamp=WarmDampOverlay,
    asciicarve=CarveOverlay,
}

return _ENV
