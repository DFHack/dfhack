local _ENV = mkmodule('plugins.dig')

local overlay = require('plugins.overlay')
local pathable = require('plugins.pathable')

WarmDampOverlay = defclass(WarmDampOverlay, overlay.OverlayWidget)
WarmDampOverlay.ATTRS{
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

function WarmDampOverlay:onRenderFrame(dc)
    pathable.paintScreenWarmDamp()
end

OVERLAY_WIDGETS = {overlay=WarmDampOverlay}

return _ENV
