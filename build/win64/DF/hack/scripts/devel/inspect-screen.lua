-- Read from the screen and display info about the tiles

local gui = require('gui')
local utils = require('utils')
local widgets = require('gui.widgets')
local overlay = require('plugins.overlay')

Inspect = defclass(Inspect, widgets.Window)
Inspect.ATTRS{
    frame={w=40, h=20},
    resizable=true,
    frame_title='Screen Inspector',
}

function Inspect:init()
    local scr_name = overlay.simplify_viewscreen_name(
            getmetatable(dfhack.gui.getDFViewscreen(true)))

    self:addviews{
        widgets.Label{
            frame={t=0, l=0},
            text={'Current screen: ', {text=scr_name, pen=COLOR_CYAN}}},
        widgets.CycleHotkeyLabel{
            view_id='layer',
            frame={t=2, l=0},
            key='CUSTOM_CTRL_A',
            label='Inspect layer:',
            options={{label='UI', value='ui'}, 'map'},
            enabled=self:callback('is_unfrozen')},
        widgets.CycleHotkeyLabel{
            view_id='empties',
            frame={t=3, l=0},
            key='CUSTOM_CTRL_E',
            label='Empty elements:',
            options={'hide', 'show'}},
        widgets.ToggleHotkeyLabel{
            view_id='freeze',
            frame={t=4, l=0},
            key='CUSTOM_CTRL_F',
            label='Freeze current tile:',
            initial_option=false},
        widgets.Label{
            frame={t=6},
            text={{text=self:callback('get_mouse_pos')}}},
        widgets.Label{
            view_id='report',
            frame={t=8},},
    }
end

function Inspect:is_unfrozen()
    return not self.subviews.freeze:getOptionValue()
end

function Inspect:do_refresh()
    return self:is_unfrozen() and not self:getMouseFramePos()
end

local cur_mouse_pos = {x=-1, y=-1}
function Inspect:get_mouse_pos()
    local pos, text = cur_mouse_pos, ''
    if self.subviews.layer:getOptionValue() == 'ui' then
        if self:do_refresh() then
            pos = xy2pos(dfhack.screen.getMousePos())
        end
        text = ('UI grid coords: %s, %s'):format(pos.x, pos.y)
    else
        if self:do_refresh() then
            pos = dfhack.gui.getMousePos()
        end
        if pos then
            text = ('Map coords: %s, %s, %s'):format(pos.x, pos.y, pos.z)
        else
            text = 'Mouse is not on the map'
        end
    end
    cur_mouse_pos = pos or cur_mouse_pos
    return text
end

local function add_screen_report_line(report, base, vec, index, show_empty)
    local ch = base[vec][index]
    if ch <= 0 and not show_empty then return end
    table.insert(report, '[' .. string.char(ch) .. ']')
    table.insert(report, (' (%s) - %s'):format(ch, vec))
    table.insert(report, NEWLINE)
    table.insert(report, ('  fg: %d, %d, %d')
        :format(base[vec][index+1], base[vec][index+2], base[vec][index+3]))
    table.insert(report, NEWLINE)
    table.insert(report, ('  bg: %d, %d, %d')
        :format(base[vec][index+4], base[vec][index+5], base[vec][index+6]))
    table.insert(report, NEWLINE)
end

local function add_texpos_report_line(report, base, vec, index, show_empty)
    local texpos = base[vec][index]
    if texpos <= 0 and not show_empty then return end
    table.insert(report, '[')
    table.insert(report, {tile=texpos})
    table.insert(report, ']')
    table.insert(report, (' (%s) - %s'):format(texpos, vec))
    table.insert(report, NEWLINE)
end

local function pop_flag(report, flag, name, show_empty, dim)
    dim = dim or 2
    local bit = flag % dim
    if bit ~= 0 or show_empty then
        table.insert(report, ('  %s = %d'):format(name, bit))
        table.insert(report, NEWLINE)
    end
    return flag // dim
end

local function add_texpos_flag_report_line(report, base, vec, index, flags, show_empty)
    local val = base[vec][index]
    if val == 0 and not show_empty then return end
    table.insert(report, vec)
    table.insert(report, NEWLINE)
    for _,f in ipairs(flags or {}) do
        val = pop_flag(report, val, f.name, show_empty, f.dim)
    end
end

local gps = df.global.gps

local screentexpos_flags = {
    {name='grayscale'},
    {name='addcolor'},
    {name='anchor_subordinate'},
    {name='top_of_text'},
    {name='bottom_of_text'},
    {name='anchor_use_screen_color'},
    {name='anchor_x_coord', dim=64},
    {name='anchor_y_coord', dim=64},
}

local function get_ui_report(show_empty)
    local report = {}

    local pos = cur_mouse_pos
    if pos.x < 0 or pos.y < 0 then return report end

    local index = (pos.x * gps.dimy) + pos.y

    add_screen_report_line(report, gps, 'screen', index*8, show_empty)
    add_texpos_report_line(report, gps, 'screentexpos_lower', index, show_empty)
    add_texpos_report_line(report, gps, 'screentexpos', index, show_empty)
    add_texpos_report_line(report, gps, 'screentexpos_anchored', index, show_empty)
    add_texpos_flag_report_line(report, gps, 'screentexpos_flag', index,
                                screentexpos_flags, show_empty)

    if gps.top_in_use then
        add_screen_report_line(report, gps, 'screen_top', index*8, show_empty)
        add_texpos_report_line(report, gps, 'screentexpos_top_lower', index, show_empty)
        add_texpos_report_line(report, gps, 'screentexpos_top', index, show_empty)
        add_texpos_report_line(report, gps, 'screentexpos_top_anchored', index, show_empty)
        add_texpos_flag_report_line(report, gps, 'screentexpos_top_flag', index,
                                    screentexpos_flags, show_empty)
    end

    return report
end

local viewport_floor_flags = {
    {name='s_edging', dim=256},
    {name='w_edging', dim=256},
    {name='e_edging', dim=256},
    {name='n_edging', dim=256},
    {name='special_texture', dim=8},
}

local viewport_liquid_flags = {
    {name='center_animation', dim=4},
    {name='center_level', dim=8},
    {name='center_type', dim=16},
    {name='n_edge_type', dim=16},
    {name='s_edge_type', dim=16},
    {name='w_edge_type', dim=16},
    {name='e_edge_type', dim=16},
}

local viewport_spatter_flags = {
    {name='shape_type', dim=64},
    {name='material_type', dim=32},
    {name='color_index', dim=256},
    {name='derived_plus', dim=512},
    {name='fire', dim=8},
    {name='accepts_spatter'},
}

local viewport_ramp_flags = {
    {name='type', dim=256},
    {name='wall_n'},
    {name='wall_w'},
    {name='wall_e'},
    {name='wall_s'},
    {name='wall_nw'},
    {name='wall_ne'},
    {name='wall_sw'},
    {name='wall_se'},
    {name='n_is_dark_corner'},
    {name='s_is_dark_corner'},
    {name='w_is_dark_corner'},
    {name='e_is_dark_corner'},
    {name='n_is_open_air'},
    {name='s_is_open_air'},
    {name='w_is_open_air'},
    {name='e_is_open_air'},
    {name='show_up_arrow'},
    {name='show_down_arrow'},
    {name='color_index', dim=256},
}

local viewport_shadow_flags = {
    {name='accepts_shadow'},
    {name='is_shadow_wall'},
    {name='shadow_wall_to_n'},
    {name='shadow_wall_to_s'},
    {name='shadow_wall_to_w'},
    {name='shadow_wall_to_e'},
    {name='shadow_wall_to_nw'},
    {name='shadow_wall_to_ne'},
    {name='shadow_wall_to_sw'},
    {name='shadow_wall_to_se'},
    {name='ramp_shadow_on_floor_nw_of_corner_se'},
    {name='ramp_shadow_on_floor_n_of_corner_se'},
    {name='ramp_shadow_on_floor_n_of_s'},
    {name='ramp_shadow_on_floor_n_of_corner_sw'},
    {name='ramp_shadow_on_floor_ne_of_corner_sw'},
    {name='ramp_shadow_on_floor_w_of_corner_se'},
    {name='ramp_shadow_on_floor_e_of_corner_sw'},
    {name='ramp_shadow_on_floor_w_of_e'},
    {name='ramp_shadow_on_floor_e_of_w'},
    {name='ramp_shadow_on_floor_w_of_corner_ne'},
    {name='ramp_shadow_on_floor_e_of_corner_nw'},
    {name='ramp_shadow_on_floor_sw_of_corner_ne'},
    {name='ramp_shadow_on_floor_s_of_corner_ne'},
    {name='ramp_shadow_on_floor_s_of_n'},
    {name='ramp_shadow_on_floor_s_of_corner_nw'},
    {name='ramp_shadow_on_floor_se_of_corner_nw'},
}

local function get_map_report(show_empty)
    local report = {}

    local pos = cur_mouse_pos
    if not pos or not pos.z then
        return report
    end

    if not dfhack.screen.inGraphicsMode() then
        table.insert(report, 'Please inspect the UI layer')
        table.insert(report, NEWLINE)
        table.insert(report, 'when not in graphics mode')
        return report
    end

    local vp = gps.main_viewport
    local index = ((pos.x - df.global.window_x) * vp.dim_y) + pos.y - df.global.window_y

    add_texpos_report_line(report, vp, 'screentexpos_background', index, show_empty)
    add_texpos_flag_report_line(report, vp, 'screentexpos_floor_flag', index,
                                viewport_floor_flags, show_empty)
    add_texpos_report_line(report, vp, 'screentexpos_background_two', index, show_empty)
    add_texpos_flag_report_line(report, vp, 'screentexpos_liquid_flag', index,
                                viewport_liquid_flags, show_empty)
    add_texpos_flag_report_line(report, vp, 'screentexpos_spatter_flag', index,
                                viewport_spatter_flags, show_empty)
    add_texpos_report_line(report, vp, 'screentexpos_spatter', index, show_empty)
    add_texpos_flag_report_line(report, vp, 'screentexpos_ramp_flag', index,
                                viewport_ramp_flags, show_empty)
    add_texpos_flag_report_line(report, vp, 'screentexpos_shadow_flag', index,
                                viewport_shadow_flags, show_empty)
    add_texpos_report_line(report, vp, 'screentexpos_building_one', index, show_empty)
    add_texpos_report_line(report, vp, 'screentexpos_item', index, show_empty)
    add_texpos_report_line(report, vp, 'screentexpos_vehicle', index, show_empty)
    add_texpos_report_line(report, vp, 'screentexpos_vermin', index, show_empty)
    add_texpos_report_line(report, vp, 'screentexpos_left_creature', index, show_empty)
    add_texpos_report_line(report, vp, 'screentexpos', index, show_empty)
    add_texpos_report_line(report, vp, 'screentexpos_right_creature', index, show_empty)
    add_texpos_report_line(report, vp, 'screentexpos_building_two', index, show_empty)
    add_texpos_report_line(report, vp, 'screentexpos_projectile', index, show_empty)
    add_texpos_report_line(report, vp, 'screentexpos_high_flow', index, show_empty)
    add_texpos_report_line(report, vp, 'screentexpos_top_shadow', index, show_empty)
    add_texpos_report_line(report, vp, 'screentexpos_signpost', index, show_empty)
    add_texpos_report_line(report, vp, 'screentexpos_upleft_creature', index, show_empty)
    add_texpos_report_line(report, vp, 'screentexpos_up_creature', index, show_empty)
    add_texpos_report_line(report, vp, 'screentexpos_upright_creature', index, show_empty)
    add_texpos_report_line(report, vp, 'screentexpos_designation', index, show_empty)
    add_texpos_report_line(report, vp, 'screentexpos_interface', index, show_empty)

    return report
end

function Inspect:onRenderBody()
    if self:do_refresh() then
        local show_empty = self.subviews.empties:getOptionValue() == 'show'
        local report = self.subviews.layer:getOptionValue() == 'ui' and
                get_ui_report(show_empty) or get_map_report(show_empty)
        self.subviews.report:setText(report)
        self:updateLayout()
    end
end

function Inspect:onInput(keys)
    if Inspect.super.onInput(self, keys) then
        return true
    end
    if keys._MOUSE_L_DOWN and not self:getMouseFramePos() then
        self.subviews.freeze:cycle()
        return true
    end
end

InspectScreen = defclass(InspectScreen, gui.ZScreen)
InspectScreen.ATTRS{
    focus_string='inspect-screen',
    force_pause=true,
    pass_mouse_clicks=false,
}

function InspectScreen:init()
    -- prevent hotspot widgets from reacting
    overlay.register_trigger_lock_screen(self)

    self:addviews{Inspect{view_id='main'}}
end

function InspectScreen:onDismiss()
    view = nil
end

view = view and view:raise() or InspectScreen{}:show()
