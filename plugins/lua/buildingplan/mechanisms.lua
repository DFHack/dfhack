local _ENV = mkmodule('plugins.buildingplan.mechanisms')

local overlay = require('plugins.overlay')
local widgets = require('gui.widgets')

--------------------------------
-- MechanismOverlay
--

MechanismOverlay = defclass(MechanismOverlay, overlay.OverlayWidget)
MechanismOverlay.ATTRS{
    default_pos={x=5,y=5},
    default_enabled=true,
    viewscreens='dwarfmode/LinkingLever',
    frame={w=57, h=13},
}

function MechanismOverlay:init()
    self:addviews{
        widgets.CycleHotkeyLabel{
            view_id='safety1',
            frame={t=4, l=4, w=40},
            key='CUSTOM_G',
            label='Lever mechanism safety:',
            options={
                {label='Any', value=0},
                {label='Magma', value=2, pen=COLOR_RED},
                {label='Fire', value=1, pen=COLOR_LIGHTRED},
            },
            initial_option=0,
        },
        widgets.CycleHotkeyLabel{
            view_id='safety2',
            frame={t=5, l=4, w=40},
            key='CUSTOM_SHIFT_G',
            label='Target mechanism safety:',
            options={
                {label='Any', value=0},
                {label='Magma', value=2, pen=COLOR_RED},
                {label='Fire', value=1, pen=COLOR_LIGHTRED},
            },
            initial_option=0,
        },
        widgets.HotkeyLabel{
            frame={t=6, l=9, w=48, h=3},
            key='CUSTOM_M',
            label='Choose',
            auto_height=false,
            on_activate=function() end,
        },
        widgets.HotkeyLabel{
            frame={t=9, l=9, w=48, h=3},
            key='CUSTOM_SHIFT_M',
            label='Choose',
            auto_height=false,
            on_activate=function() end,
        },
    }
end

function MechanismOverlay:onInput(keys)
    if keys._MOUSE_L and self:getMousePos() then
        MechanismOverlay.super.onInput(self, keys)
        -- don't let clicks bleed through the panel
        return true
    end
end

return _ENV
