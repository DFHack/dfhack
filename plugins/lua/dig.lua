local _ENV = mkmodule('plugins.dig')

local overlay = require('plugins.overlay')
local pathable = require('plugins.pathable')

WarmDampOverlay = defclass(WarmDampOverlay, overlay.OverlayWidget)
WarmDampOverlay.ATTRS{
    desc='Makes warm and damp tiles visible when in ASCII mode.',
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
    overlay_only=true,
}

function WarmDampOverlay:onRenderFrame()
    pathable.paintScreenWarmDamp()
end

CarveOverlay = defclass(CarveOverlay, overlay.OverlayWidget)
CarveOverlay.ATTRS{
    desc='Makes existing carving designations visible when in ASCII mode.',
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
