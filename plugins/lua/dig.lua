local _ENV = mkmodule('plugins.dig')

local gui = require('gui')
local overlay = require('plugins.overlay')
local widgets = require('gui.widgets')

local toolbar_textures = dfhack.textures.loadTileset('hack/data/art/damp_dig_toolbar.png', 8, 12, true)

local main_if = df.global.game.main_interface
local selection_rect = df.global.selection_rect

local BASELINE_OFFSET = 42

local function get_l_offset(parent_rect)
    local w = parent_rect.width
    if w <= 177 then
        return BASELINE_OFFSET + w - 114
    end
    return BASELINE_OFFSET + (w+1)//2 - 26
end

-- --------------------------------
-- WarmDampDigConfig
--

WarmDampDigConfig = defclass(WarmDampDigConfig, widgets.Panel)
WarmDampDigConfig.ATTRS {
    frame={w=26, h=18, b=7},
}

function WarmDampDigConfig:init()
    self.dcount = getCurLevelDesignatedCount()
    self.z = df.global.window_z

    local panel = widgets.Panel{
        frame_style=gui.FRAME_MEDIUM,
        frame_background=gui.CLEAR_PEN,
        autoarrange_subviews=true,
        autoarrange_gap=1,
        subviews={
            widgets.Label{
                text={
                    'Mark newly designated', NEWLINE,
                    'tiles for:'
                },
            },
            widgets.ToggleHotkeyLabel{
                view_id='damp',
                key='CUSTOM_CTRL_D',
                label='Damp dig:',
                initial_option=false,
                on_change=setDampPaintEnabled,
            },
            widgets.ToggleHotkeyLabel{
                view_id='warm',
                key='CUSTOM_CTRL_W',
                label='Warm dig:',
                initial_option=false,
                on_change=setWarmPaintEnabled,
            },
            widgets.Divider{
                frame={h=1},
                frame_style=gui.FRAME_INTERIOR,
                frame_style_l=false,
                frame_style_r=false,
            },
            widgets.Label{
                text={
                    'Mark/unmark ',
                    {text=function() return self.dcount end, pen=COLOR_YELLOW},
                    {text=function() return (' tile%s'):format(self.dcount == 1 and '' or 's') end},
                    NEWLINE,
                    'currently designated on', NEWLINE,
                    'this level for:'
                },
            },
            widgets.HotkeyLabel{
                key='CUSTOM_CTRL_N',
                label='Damp dig',
                on_activate=toggleCurLevelDampDig,
            },
            widgets.HotkeyLabel{
                key='CUSTOM_CTRL_M',
                label='Warm dig',
                on_activate=toggleCurLevelWarmDig,
            },
        },
    }
    self:addviews{
        panel,
        widgets.HelpButton{command='dig'},
    }
end

function WarmDampDigConfig:render(dc)
    self.subviews.damp:setOption(getDampPaintEnabled())
    self.subviews.warm:setOption(getWarmPaintEnabled())
    if df.global.window_z ~= self.z then
        self.dcount = getCurLevelDesignatedCount()
        self.z = df.global.window_z
    end
    WarmDampDigConfig.super.render(self, dc)
end

function WarmDampDigConfig:onInput(keys)
    if keys._MOUSE_L and not self:getMouseFramePos() then
        self.parent_view:dismiss()
        return
    end
    return WarmDampDigConfig.super.onInput(self, keys) or
        keys._MOUSE_L and self:getMouseFramePos()
end

function WarmDampDigConfig:preUpdateLayout(parent_rect)
    self.frame.l = get_l_offset(parent_rect) - 1
end

-- --------------------------------
-- WarmDampDigConfigScreen
--

WarmDampDigConfigScreen = defclass(WarmDampDigConfigScreen, gui.ZScreen)
WarmDampDigConfigScreen.ATTRS {
    focus_path='WarmDampDigConfigScreen',
    defocusable=false,
    pass_movement_keys=true,
}

function WarmDampDigConfigScreen:init()
    self:addviews{WarmDampDigConfig{}}
end

function WarmDampDigConfigScreen:onDismiss()
    view = nil
end

local function launch_warm_damp_dig_config()
    view = view and view:raise() or WarmDampDigConfigScreen{}:show()
end

-- --------------------------------
-- WarmDampToolbarOverlay
--

WarmDampToolbarOverlay = defclass(WarmDampToolbarOverlay, overlay.OverlayWidget)
WarmDampToolbarOverlay.ATTRS{
    desc='Adds widgets to the dig interface to allow uninterrupted digging through warm and damp tiles.',
    default_pos={x=BASELINE_OFFSET, y=-4},
    default_enabled=true,
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
    frame={w=26, h=11},
}

function WarmDampToolbarOverlay:init()
    local button_chars = {
        {218, 196, 196, 191},
        {179, '~', '~', 179},
        {192, 196, 196, 217},
    }

    self:addviews{
        widgets.Panel{
            frame={t=0, r=0, w=26, h=7},
            frame_style=gui.FRAME_PANEL,
            frame_background=gui.CLEAR_PEN,
            frame_inset={l=1, r=1},
            visible=function() return not not self.subviews.icon:getMousePos() end,
            subviews={
                widgets.Label{
                    text={
                        'Mark for uninterrupted', NEWLINE,
                        'digging through damp', NEWLINE,
                        'or warm tiles.', NEWLINE,
                        NEWLINE,
                        {text='Hotkey: ', pen=COLOR_GRAY}, {key='CUSTOM_CTRL_D'},
                    },
                },
            },
        },
        widgets.Panel{
            view_id='icon',
            frame={b=0, r=22, w=4, h=3},
            subviews={
                widgets.Label{
                    text=widgets.makeButtonLabelText{
                        chars=button_chars,
                        pens=COLOR_GRAY,
                        tileset=toolbar_textures,
                        tileset_offset=1,
                        tileset_stride=8,
                    },
                    on_click=launch_warm_damp_dig_config,
                    visible=function() return not getWarmPaintEnabled() and not getDampPaintEnabled() end,
                },
                widgets.Label{
                    text=widgets.makeButtonLabelText{
                        chars=button_chars,
                        pens={
                            {COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE},
                            {COLOR_WHITE, COLOR_RED,   COLOR_GRAY,  COLOR_WHITE},
                            {COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE},
                        },
                        tileset=toolbar_textures,
                        tileset_offset=25,
                        tileset_stride=8,
                    },
                    on_click=launch_warm_damp_dig_config,
                    visible=function() return getWarmPaintEnabled() and not getDampPaintEnabled() end,
                },
                widgets.Label{
                    text=widgets.makeButtonLabelText{
                        chars=button_chars,
                        pens={
                            {COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE},
                            {COLOR_WHITE, COLOR_GRAY,  COLOR_BLUE,  COLOR_WHITE},
                            {COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE},
                        },
                        tileset=toolbar_textures,
                        tileset_offset=5,
                        tileset_stride=8,
                    },
                    on_click=launch_warm_damp_dig_config,
                    visible=function() return not getWarmPaintEnabled() and getDampPaintEnabled() end,
                },
                widgets.Label{
                    text=widgets.makeButtonLabelText{
                        chars=button_chars,
                        pens={
                            {COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE},
                            {COLOR_WHITE, COLOR_RED,   COLOR_BLUE,  COLOR_WHITE},
                            {COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE},
                        },
                        tileset=toolbar_textures,
                        tileset_offset=29,
                        tileset_stride=8,
                    },
                    on_click=launch_warm_damp_dig_config,
                    visible=function() return getWarmPaintEnabled() and getDampPaintEnabled() end,
                },
            },
        },
    }
end

function WarmDampToolbarOverlay:preUpdateLayout(parent_rect)
    self.frame.w = get_l_offset(parent_rect) - BASELINE_OFFSET + 26
end

function WarmDampToolbarOverlay:onInput(keys)
    if keys.CUSTOM_CTRL_D then
        launch_warm_damp_dig_config()
        return true
    end
    return WarmDampToolbarOverlay.super.onInput(self, keys)
end

-- --------------------------------
-- WarmDampOverlay
--

WarmDampOverlay = defclass(WarmDampOverlay, overlay.OverlayWidget)
WarmDampOverlay.ATTRS{
    desc='Visibility improvements for warm and damp map tiles.',
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
        'dwarfmode/Designate/ERASE',
        'dwarfmode/Designate/SMOOTH',
        'dwarfmode/Designate/ENGRAVE',
        'dwarfmode/Designate/TRACK',
        'dwarfmode/Designate/FORTIFY',
    },
    default_enabled=true,
    frame={w=0, h=0},
}

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

function WarmDampOverlay:onInput(keys)
    if WarmDampOverlay.super.onInput(self, keys) then
        return true
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

function WarmDampOverlay:onRenderFrame(dc, rect)
    WarmDampOverlay.super.onRenderFrame(self, dc, rect)
    if self.pending_fn then
        self.pending_fn()
        self.pending_fn = nil
    end
    paintScreenWarmDamp()
end

-- --------------------------------
-- DesignatedOverlay
--

DesignatedOverlay = defclass(DesignatedOverlay, overlay.OverlayWidget)
DesignatedOverlay.ATTRS{
    desc='Makes existing carving designations visible when in ASCII mode.',
    viewscreens={
        'dwarfmode/Designate/DIG_DIG',
        'dwarfmode/Designate/DIG_REMOVE_STAIRS_RAMPS',
        'dwarfmode/Designate/DIG_STAIR_UPDOWN',
        'dwarfmode/Designate/DIG_RAMP',
        'dwarfmode/Designate/DIG_CHANNEL',
        'dwarfmode/Designate/SMOOTH',
        'dwarfmode/Designate/ENGRAVE',
        'dwarfmode/Designate/TRACK',
        'dwarfmode/Designate/FORTIFY',
        'dwarfmode/Designate/ERASE',
    },
    default_enabled=true,
    frame={w=0, h=0},
}

function DesignatedOverlay:onRenderFrame()
    paintScreenDesignated()
end

-- --------------------------------
-- Exported symbols
--

OVERLAY_WIDGETS = {
    asciidesignated=DesignatedOverlay,
    warmdamp=WarmDampOverlay,
    warmdamptoolbar=WarmDampToolbarOverlay,
}

return _ENV
