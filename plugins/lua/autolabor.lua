local _ENV = mkmodule('plugins.autolabor')

local gui = require('gui')
local overlay = require('plugins.overlay')
local widgets = require('gui.widgets')

AutolaborOverlay = defclass(AutolaborOverlay, overlay.OverlayWidget)
AutolaborOverlay.ATTRS{
    default_pos={x=7,y=-13},
    default_enabled=true,
    viewscreens='dwarfmode/Info/LABOR/WORK_DETAILS',
    frame={w=29, h=5},
    frame_style=gui.MEDIUM_FRAME,
    frame_background=gui.CLEAR_PEN,
}

function AutolaborOverlay:init()
    self:addviews{
        widgets.Label{
            frame={t=0, l=0},
            text_pen=COLOR_LIGHTRED,
            text='DFHack autolabor is active!',
            visible=isEnabled,
        },
        widgets.Label{
            frame={t=0, l=0},
            text_pen=COLOR_LIGHTRED,
            text='Dwarf Therapist is active!',
            visible=function() return not isEnabled() end,
        },
        widgets.Label{
            frame={t=1, l=0},
            text_pen=COLOR_WHITE,
            text={
                'Any changes made on this', NEWLINE,
                'screen will have no effect.'
            },
        },
    }
end

function AutolaborOverlay:render(dc)
    if df.global.game.external_flag ~= 1 then return end
    AutolaborOverlay.super.render(self, dc)
end

OVERLAY_WIDGETS = {
    overlay=AutolaborOverlay,
}

return _ENV
