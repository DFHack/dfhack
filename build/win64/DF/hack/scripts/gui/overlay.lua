-- overlay plugin gui config
--@ module = true

local gui = require('gui')
local guidm = require('gui.dwarfmode')
local widgets = require('gui.widgets')

local overlay = require('plugins.overlay')

local DIALOG_WIDTH = 59
local LIST_HEIGHT = 14
local HIGHLIGHT_TILE = df.global.init.load_bar_texpos[1]

local SHADOW_FRAME = copyall(gui.PANEL_FRAME)
SHADOW_FRAME.signature_pen = false

local to_pen = dfhack.pen.parse

local HIGHLIGHT_FRAME = {
    t_frame_pen = to_pen{tile=df.global.init.texpos_border_n, ch=205, fg=COLOR_GREEN, bg=COLOR_BLACK, tile_fg=COLOR_LIGHTGREEN},
    l_frame_pen = to_pen{tile=df.global.init.texpos_border_w, ch=186, fg=COLOR_GREEN, bg=COLOR_BLACK, tile_fg=COLOR_LIGHTGREEN},
    b_frame_pen = to_pen{tile=df.global.init.texpos_border_s, ch=205, fg=COLOR_GREEN, bg=COLOR_BLACK, tile_fg=COLOR_LIGHTGREEN},
    r_frame_pen = to_pen{tile=df.global.init.texpos_border_e, ch=186, fg=COLOR_GREEN, bg=COLOR_BLACK, tile_fg=COLOR_LIGHTGREEN},
    lt_frame_pen = to_pen{tile=df.global.init.texpos_border_nw, ch=201, fg=COLOR_GREEN, bg=COLOR_BLACK, tile_fg=COLOR_LIGHTGREEN},
    lb_frame_pen = to_pen{tile=df.global.init.texpos_border_sw, ch=200, fg=COLOR_GREEN, bg=COLOR_BLACK, tile_fg=COLOR_LIGHTGREEN},
    rt_frame_pen = to_pen{tile=df.global.init.texpos_border_ne, ch=187, fg=COLOR_GREEN, bg=COLOR_BLACK, tile_fg=COLOR_LIGHTGREEN},
    rb_frame_pen = to_pen{tile=df.global.init.texpos_border_se, ch=188, fg=COLOR_GREEN, bg=COLOR_BLACK, tile_fg=COLOR_LIGHTGREEN},
    signature_pen=false,
}

local function make_highlight_frame_style(frame)
    local frame_style = copyall(HIGHLIGHT_FRAME)
    local fg, bg = COLOR_GREEN, COLOR_LIGHTGREEN
    if frame.t then
        frame_style.t_frame_pen = to_pen{tile=HIGHLIGHT_TILE, ch=205, fg=fg, bg=bg}
    elseif frame.b then
        frame_style.b_frame_pen = to_pen{tile=HIGHLIGHT_TILE, ch=205, fg=fg, bg=bg}
    end
    if frame.l then
        frame_style.l_frame_pen = to_pen{tile=HIGHLIGHT_TILE, ch=186, fg=fg, bg=bg}
    elseif frame.r then
        frame_style.r_frame_pen = to_pen{tile=HIGHLIGHT_TILE, ch=186, fg=fg, bg=bg}
    end
    return frame_style
end

--------------------
-- DraggablePanel --
--------------------

DraggablePanel = defclass(DraggablePanel, widgets.Panel)
DraggablePanel.ATTRS{
    on_click=DEFAULT_NIL,
    name=DEFAULT_NIL,
    draggable=true,
    drag_anchors={frame=true, body=true},
    drag_bound='body',
}

function DraggablePanel:onInput(keys)
    if keys._MOUSE_L_DOWN then
        local rect = self.frame_rect
        local x,y = self:getMousePos(gui.ViewRect{rect=rect})
        if x then
            self.on_click()
        end
    end
    return DraggablePanel.super.onInput(self, keys)
end

function DraggablePanel:postUpdateLayout()
    if not self.is_selected then return end
    local frame = self.frame
    local matcher = {t=not not frame.t, b=not not frame.b,
                     l=not not frame.l, r=not not frame.r}
    local parent_rect, frame_rect = self.frame_parent_rect, self.frame_rect
    if frame_rect.y1 <= parent_rect.y1 then
        frame.t, frame.b = frame_rect.y1-parent_rect.y1, nil
    elseif frame_rect.y2 >= parent_rect.y2 then
        frame.t, frame.b = nil, parent_rect.y2-frame_rect.y2
    end
    if frame_rect.x1 <= parent_rect.x1 then
        frame.l, frame.r = frame_rect.x1-parent_rect.x1, nil
    elseif frame_rect.x2 >= parent_rect.x2 then
        frame.l, frame.r = nil, parent_rect.x2-frame_rect.x2
    end
    self.frame_style = make_highlight_frame_style(self.frame)
    if not not frame.t ~= matcher.t or not not frame.b ~= matcher.b
            or not not frame.l ~= matcher.l or not not frame.r ~= matcher.r then
        -- we've changed edge affinity, recalculate our frame
        self:updateLayout()
    end
end

function DraggablePanel:onRenderFrame(dc, rect)
    if self:getMousePos(gui.ViewRect{rect=self.frame_rect}) then
        self.frame_background = to_pen{
                ch=32, fg=COLOR_LIGHTGREEN, bg=COLOR_LIGHTGREEN}
    else
        self.frame_background = nil
    end
    DraggablePanel.super.onRenderFrame(self, dc, rect)
end

-------------------
-- OverlayConfig --
-------------------

OverlayConfig = defclass(OverlayConfig, gui.Screen)

function OverlayConfig:init()
    -- prevent hotspot widgets from reacting
    overlay.register_trigger_lock_screen(self)

    self.scr_name = overlay.simplify_viewscreen_name(
            getmetatable(dfhack.gui.getDFViewscreen(true)))

    local main_panel = widgets.Window{
        frame={w=DIALOG_WIDTH, h=LIST_HEIGHT+15},
        resizable=true,
        resize_min={h=20},
        frame_title='Reposition overlay widgets',
    }
    main_panel:addviews{
        widgets.Label{
            frame={t=0, l=0},
            text={'Current screen: ', {text=self.scr_name, pen=COLOR_CYAN}}},
        widgets.CycleHotkeyLabel{
            view_id='filter',
            frame={t=2, l=0},
            key='CUSTOM_CTRL_O',
            label='Showing:',
            options={{label='overlays for the current screen', value='cur'},
                     {label='all overlays', value='all'}},
            on_change=self:callback('refresh_list')},
        widgets.FilteredList{
            view_id='list',
            frame={t=4, b=7},
            on_select=self:callback('highlight_selected'),
        },
        widgets.HotkeyLabel{
            frame={b=5, l=0},
            key='SELECT',
            key_sep=' or drag the on-screen widget to reposition ',
            on_activate=function() self:reposition(self.subviews.list:getSelected()) end,
            scroll_keys={},
        },
        widgets.HotkeyLabel{
            frame={b=3, l=0},
            key='CUSTOM_CTRL_D',
            scroll_keys={},
            label='reset selected widget to its default position',
            on_activate=self:callback('reset'),
        },
        widgets.WrappedLabel{
            frame={b=0, l=0},
            scroll_keys={},
            text_to_wrap='When repositioning a widget, touch an edge of the'..
                ' screen to anchor the widget to that edge.',
        },
    }
    self:addviews{main_panel}
    self:refresh_list()
end

local function make_highlight_frame(widget_frame)
    local frame = {h=widget_frame.h+2, w=widget_frame.w+2}
    if widget_frame.l then frame.l = widget_frame.l - 1
    else frame.r = widget_frame.r - 1 end
    if widget_frame.t then frame.t = widget_frame.t - 1
    else frame.b = widget_frame.b - 1 end
    return frame
end

function OverlayConfig:refresh_list(filter)
    local choices = {}
    local state = overlay.get_state()
    local list = self.subviews.list
    local make_on_click_fn = function(idx)
        return function() list.list:setSelected(idx) end
    end
    for _,name in ipairs(state.index) do
        local db_entry = state.db[name]
        local widget = db_entry.widget
        if widget.overlay_only then goto continue end
        if not widget.hotspot and filter ~= 'all' then
            local matched = false
            for _,scr in ipairs(overlay.normalize_list(widget.viewscreens)) do
                if overlay.simplify_viewscreen_name(scr):startswith(self.scr_name) then
                    matched = true
                    break
                end
            end
            if not matched then goto continue end
        end
        local panel = nil
        panel = DraggablePanel{
                frame=make_highlight_frame(widget.frame),
                frame_style=SHADOW_FRAME,
                on_click=make_on_click_fn(#choices+1),
                name=name}
        panel.on_drag_end = function(success)
            if (success) then
                local frame = panel.frame
                local posx = frame.l and tostring(frame.l+2)
                        or tostring(-(frame.r+2))
                local posy = frame.t and tostring(frame.t+2)
                        or tostring(-(frame.b+2))
                overlay.overlay_command({'position', name, posx, posy},true)
            end
            self.reposition_panel = nil
        end
        local cfg = state.config[name]
        local tokens = {}
        table.insert(tokens, name)
        table.insert(tokens, {text=function()
                if self.reposition_panel and self.reposition_panel == panel then
                    return ' (repositioning with keyboard)'
                end
                return ''
            end})
        table.insert(choices,
                {text=tokens, enabled=cfg.enabled, name=name, panel=panel,
                 search_key=name})
        ::continue::
    end
    local old_filter = list:getFilter()
    list:setChoices(choices)
    list:setFilter(old_filter)
    if self.frame_parent_rect then
        self:postUpdateLayout()
    end
end

function OverlayConfig:highlight_selected(_, obj)
    if self.selected_panel then
        self.selected_panel.frame_style = SHADOW_FRAME
        self.selected_panel.is_selected = false
        self.selected_panel = nil
    end
    if self.reposition_panel then
        self.reposition_panel:setKeyboardDragEnabled(false)
        self.reposition_panel = nil
    end
    if not obj or not obj.panel then return end
    local panel = obj.panel
    panel.is_selected = true
    panel.frame_style = make_highlight_frame_style(panel.frame)
    self.selected_panel = panel
end

function OverlayConfig:reposition(_, obj)
    if not obj then return end
    self.reposition_panel = obj.panel
    if self.reposition_panel then
        self.reposition_panel:setKeyboardDragEnabled(true)
    end
end

function OverlayConfig:reset()
    local idx,obj = self.subviews.list:getSelected()
    if not obj or not obj.panel then return end
    overlay.overlay_command({'position', obj.panel.name, 'default'}, true)
    self:refresh_list(self.subviews.filter:getOptionValue())
end

function OverlayConfig:onDismiss()
    view = nil
end

function OverlayConfig:postUpdateLayout()
    for _,choice in ipairs(self.subviews.list:getChoices()) do
        if choice.panel then
            choice.panel:updateLayout(self.frame_parent_rect)
        end
    end
end

function OverlayConfig:onInput(keys)
    if self.reposition_panel then
        if self.reposition_panel:onInput(keys) then
            return true
        end
    end
    if keys.LEAVESCREEN or keys._MOUSE_R_DOWN then
        self:dismiss()
        return true
    end
    if self.selected_panel then
        if self.selected_panel:onInput(keys) then
            return true
        end
    end
    for _,choice in ipairs(self.subviews.list:getVisibleChoices()) do
        if choice.panel and choice.panel:onInput(keys) then
            return true
        end
    end
    return self:inputToSubviews(keys)
end

function OverlayConfig:onRenderFrame(dc, rect)
    self:renderParent()
    for _,choice in ipairs(self.subviews.list:getVisibleChoices()) do
        local panel = choice.panel
        if panel and panel ~= self.selected_panel then
            panel:render(dc)
        end
    end
    if self.selected_panel then
        self.render_selected_panel = function()
            self.selected_panel:render(dc)
        end
    else
        self.render_selected_panel = nil
    end
end

function OverlayConfig:renderSubviews(dc)
    OverlayConfig.super.renderSubviews(self, dc)
    if self.render_selected_panel then
        self.render_selected_panel()
    end
end

if dfhack_flags.module then
    return
end

view = view or OverlayConfig{}:show()
