local _ENV = mkmodule('plugins.dig')

local gui = require('gui')
local overlay = require('plugins.overlay')
local widgets = require('gui.widgets')

local main_if = df.global.game.main_interface
local selection_rect = df.global.selection_rect

-- --------------------------------
-- WarmDampDigOverlay
--

WarmDampDigOverlay = defclass(WarmDampDigOverlay, overlay.OverlayWidget)
WarmDampDigOverlay.ATTRS{
    desc='Adds widgets to the dig interface to allow uninterrupted digging through warm and damp tiles.',
    default_pos={x=50,y=-7},
    default_enabled=true,
    viewscreens={
        'dwarfmode/Designate/DIG_DIG',
        'dwarfmode/Designate/DIG_STAIR_UP',
        'dwarfmode/Designate/DIG_STAIR_UPDOWN',
        'dwarfmode/Designate/DIG_STAIR_DOWN',
        'dwarfmode/Designate/DIG_RAMP',
        'dwarfmode/Designate/DIG_CHANNEL',
        'dwarfmode/Designate/ERASE',
    },
    frame={w=41, h=3},
    frame_style=gui.FRAME_MEDIUM,
    frame_background=gui.CLEAR_PEN,
}

function WarmDampDigOverlay:init()
    self:addviews{
        widgets.ToggleHotkeyLabel{
            view_id='toggle',
            frame={t=0, l=0},
            key='CUSTOM_CTRL_W',
            label='Dig through warm/damp tiles',
            initial_option=false,
        },
    }
end

local function get_bounds()
    local pos1 = dfhack.gui.getMousePos(true)
    if not pos1 then return end
    pos1 = xyz2pos(
        math.max(0, math.min(df.global.world.map.x_count-1, pos1.x)),
        math.max(0, math.min(df.global.world.map.y_count-1, pos1.y)),
        math.max(0, math.min(df.global.world.map.z_count-1, pos1.z)))
    local pos2 = xyz2pos(
        math.max(0, math.min(df.global.world.map.x_count-1, selection_rect.start_x)),
        math.max(0, math.min(df.global.world.map.y_count-1, selection_rect.start_y)),
        math.max(0, math.min(df.global.world.map.z_count-1, selection_rect.start_z)))
    local bounds = {
        x1=math.min(pos1.x, pos2.x),
        x2=math.max(pos1.x, pos2.x),
        y1=math.min(pos1.y, pos2.y),
        y2=math.max(pos1.y, pos2.y),
        z1=math.min(pos1.z, pos2.z),
        z2=math.max(pos1.z, pos2.z),
    }

    return bounds
end

function WarmDampDigOverlay:onInput(keys)
    if WarmDampDigOverlay.super.onInput(self, keys) then
        return true
    end
    if not self.subviews.toggle:getOptionValue() and
        main_if.main_designation_selected ~= df.main_designation_type.ERASE
    then
        return
    end
    if main_if.main_designation_doing_rectangles then
        if keys._MOUSE_L and selection_rect.start_z >= 0 then
            local bounds = get_bounds()
            if bounds then
                if self.pending_fn then
                    self.pending_fn()
                end
                self.pending_fn = function() registerWarmDampBox(bounds) end
            end
        end
    else
        if keys._MOUSE_L_DOWN then
            local pos = dfhack.gui.getMousePos()
            if pos then
                registerWarmDampTile(pos)
            end
        end
    end
end

function WarmDampDigOverlay:onRenderFrame(dc, rect)
    WarmDampDigOverlay.super.onRenderFrame(self, dc, rect)
    if self.pending_fn then
        self.pending_fn()
        self.pending_fn = nil
    end
end

-- --------------------------------
-- WarmDampOverlay
--

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

function WarmDampOverlay:onRenderFrame()
    paintScreenWarmDamp()
end

-- --------------------------------
-- CarveOverlay
--

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
    paintScreenCarve()
end

OVERLAY_WIDGETS = {
    asciiwarmdamp=WarmDampOverlay,
    asciicarve=CarveOverlay,
    digwarmdamp=WarmDampDigOverlay,
}

return _ENV
