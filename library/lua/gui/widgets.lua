-- Simple widgets for screens

local _ENV = mkmodule('gui.widgets')

local gui = require('gui')
local guidm = require('gui.dwarfmode')
local textures = require('gui.textures')
local utils = require('utils')

local getval = utils.getval
local to_pen = dfhack.pen.parse

---@param view gui.View
---@param vis boolean
local function show_view(view,vis)
    if view then
        view.visible = vis
    end
end

---@param tab? table
---@param idx integer
---@return any|integer
local function map_opttab(tab,idx)
    if tab then
        return tab[idx]
    else
        return idx
    end
end

---@enum STANDARDSCROLL
STANDARDSCROLL = {
    STANDARDSCROLL_UP = -1,
    KEYBOARD_CURSOR_UP = -1,
    STANDARDSCROLL_DOWN = 1,
    KEYBOARD_CURSOR_DOWN = 1,
    STANDARDSCROLL_PAGEUP = '-page',
    KEYBOARD_CURSOR_UP_FAST = '-page',
    STANDARDSCROLL_PAGEDOWN = '+page',
    KEYBOARD_CURSOR_DOWN_FAST = '+page',
}

------------
-- Widget --
------------

---@class widgets.Widget.frame
---@field l? integer Gap between the left edge of the frame and the parent.
---@field t? integer Gap between the top edge of the frame and the parent.
---@field r? integer Gap between the right edge of the frame and the parent.
---@field b? integer Gap between the bottom edge of the frame and the parent.
---@field w? integer Desired width
---@field h? integer Desired height

---@class widgets.Widget.inset
---@field l? integer Left margin
---@field t? integer Top margin
---@field r? integer Right margin
---@field b? integer Bottom margin
---@field x? integer Left/right margin (if `l` and/or `r` are ommited)
---@field y? integer Top/bottom margin (if `t` and/or `b` are ommited)

---@class widgets.Widget.attrs: gui.View.attrs
---@field frame? widgets.Widget.frame
---@field frame_inset? widgets.Widget.inset|integer
---@field frame_background? dfhack.pen

---@class widgets.Widget.attrs.partial: widgets.Widget.attrs

---@class widgets.Widget: gui.View
---@field super gui.View
---@field ATTRS widgets.Widget.attrs|fun(attributes: widgets.Widget.attrs.partial)
---@overload fun(init_table: widgets.Widget.attrs.partial): self
Widget = defclass(Widget, gui.View)

Widget.ATTRS {
    frame = DEFAULT_NIL,
    frame_inset = DEFAULT_NIL,
    frame_background = DEFAULT_NIL,
}

---@param parent_rect { width: integer, height: integer }
---@return any
function Widget:computeFrame(parent_rect)
    local sw, sh = parent_rect.width, parent_rect.height
    return gui.compute_frame_body(sw, sh, self.frame, self.frame_inset)
end

---@param dc gui.Painter
---@param rect { x1: integer, y1: integer, x2: integer, y2: integer }
function Widget:onRenderFrame(dc, rect)
    if self.frame_background then
        dc:fill(rect, self.frame_background)
    end
end

-------------
-- Divider --
-------------

---@class widgets.Divider.attrs: widgets.Widget.attrs
---@field frame_style gui.Frame|fun(): gui.Frame
---@field interior boolean
---@field frame_style_t? false|gui.Frame|fun(): gui.Frame
---@field frame_style_b? false|gui.Frame|fun(): gui.Frame
---@field frame_style_l? false|gui.Frame|fun(): gui.Frame
---@field frame_style_r? false|gui.Frame|fun(): gui.Frame
---@field interior_t? boolean
---@field interior_b? boolean
---@field interior_l? boolean
---@field interior_r? boolean

---@class widgets.Divider.attrs.partial: widgets.Divider.attrs

---@class widgets.Divider:  widgets.Widget, widgets.Divider.attrs
---@field super widgets.Widget
---@field ATTRS widgets.Divider.attrs|fun(attributes: widgets.Divider.attrs.partial)
---@overload fun(init_table: widgets.Divider.attrs.partial): self
Divider = defclass(Divider, Widget)

Divider.ATTRS{
    frame_style=gui.FRAME_THIN,
    interior=false,
    frame_style_t=DEFAULT_NIL,
    interior_t=DEFAULT_NIL,
    frame_style_b=DEFAULT_NIL,
    interior_b=DEFAULT_NIL,
    frame_style_l=DEFAULT_NIL,
    interior_l=DEFAULT_NIL,
    frame_style_r=DEFAULT_NIL,
    interior_r=DEFAULT_NIL,
}

local function divider_get_val(self, base_name, edge_name)
    local val = self[base_name..'_'..edge_name]
    if val ~= nil then return val end
    return self[base_name]
end

local function divider_get_junction_pen(self, edge_name)
    local interior = divider_get_val(self, 'interior', edge_name)
    local pen_name = ('%sT%s_frame_pen'):format(edge_name, interior and 'i' or 'e')
    local frame_style = divider_get_val(self, 'frame_style', edge_name)
    if type(frame_style) == 'function' then
        frame_style = frame_style()
    end
    return frame_style[pen_name]
end

---@param dc gui.Painter
function Divider:onRenderBody(dc)
    local rect, style = self.frame_rect, self.frame_style
    if type(style) == 'function' then
        style = style()
    end

    if rect.height == 1 and rect.width == 1 then
        dc:seek(0, 0):char(nil, style.x_frame_pen)
    elseif rect.width == 1 then
        local fill_start, fill_end = 0, rect.height-1
        if self.frame_style_t ~= false then
            fill_start = 1
            dc:seek(0, 0):char(nil, divider_get_junction_pen(self, 't'))
        end
        if self.frame_style_b ~= false then
            fill_end = rect.height-2
            dc:seek(0, rect.height-1):char(nil, divider_get_junction_pen(self, 'b'))
        end
        dc:fill(0, fill_start, 0, fill_end, style.v_frame_pen)
    else
        local fill_start, fill_end = 0, rect.width-1
        if self.frame_style_l ~= false then
            fill_start = 1
            dc:seek(0, 0):char(nil, divider_get_junction_pen(self, 'l'))
        end
        if self.frame_style_r ~= false then
            fill_end = rect.width-2
            dc:seek(rect.width-1, 0):char(nil, divider_get_junction_pen(self, 'r'))
        end
        dc:fill(fill_start, 0, fill_end, 0, style.h_frame_pen)
    end
end

-----------
-- Panel --
-----------

DOUBLE_CLICK_MS = 500

---@class widgets.Panel.attrs: widgets.Widget.attrs
---@field frame_style? gui.Frame|fun(): gui.Frame
---@field frame_title? string
---@field on_render? fun(painter: gui.Painter) Called from `onRenderBody`.
---@field on_layout? fun(frame_body: any) Called from `postComputeFrame`.
---@field draggable boolean
---@field drag_anchors? { title: boolean, frame: boolean, body: boolean }
---@field drag_bound 'frame'|'body'
---@field on_drag_begin? fun()
---@field on_drag_end? fun(success: boolean, new_frame: gui.Frame)
---@field resizable boolean
---@field resize_anchors? { t: boolean, l: boolean, r: boolean, b: boolean }
---@field resize_min? { w: integer, h: integer }
---@field on_resize_begin? fun()
---@field on_resize_end? fun(success: boolean, new_frame: gui.Frame)
---@field autoarrange_subviews boolean
---@field autoarrange_gap integer
---@field kbd_get_pos? fun(): df.coord2d
---@field saved_frame? table
---@field saved_frame_rect? table
---@field drag_offset? table
---@field resize_edge? string
---@field last_title_click_ms number

---@class widgets.Panel.attrs.partial: widgets.Panel.attrs

---@class widgets.Panel.initTable: widgets.Panel.attrs.partial
---@field subviews? gui.View[]

---@class widgets.Panel: widgets.Widget, widgets.Panel.attrs
---@field super widgets.Widget
---@field ATTRS widgets.Panel.attrs|fun(attributes: widgets.Panel.attrs.partial)
---@overload fun(init_table: widgets.Panel.initTable): self
Panel = defclass(Panel, Widget)

Panel.ATTRS {
    frame_style = DEFAULT_NIL, -- as in gui.FramedScreen
    frame_title = DEFAULT_NIL, -- as in gui.FramedScreen
    on_render = DEFAULT_NIL,
    on_layout = DEFAULT_NIL,
    draggable = false,
    drag_anchors = DEFAULT_NIL,
    drag_bound = 'frame', -- or 'body'
    on_drag_begin = DEFAULT_NIL,
    on_drag_end = DEFAULT_NIL,
    resizable = false,
    resize_anchors = DEFAULT_NIL,
    resize_min = DEFAULT_NIL,
    on_resize_begin = DEFAULT_NIL,
    on_resize_end = DEFAULT_NIL,
    no_force_pause_badge = false,
    autoarrange_subviews = false, -- whether to automatically lay out subviews
    autoarrange_gap = 0, -- how many blank lines to insert between widgets
}

---@param self widgets.Panel
---@param args widgets.Panel.initTable
function Panel:init(args)
    if not self.drag_anchors then
        self.drag_anchors = {title=true, frame=not self.resizable, body=true}
    end
    if not self.resize_anchors then
        self.resize_anchors = {t=false, l=true, r=true, b=true}
    end
    self.resize_min = self.resize_min or {}
    self.resize_min.w = self.resize_min.w or (self.frame or {}).w or 5
    self.resize_min.h = self.resize_min.h or (self.frame or {}).h or 5

    self.kbd_get_pos = nil -- fn when we are in keyboard dragging mode
    self.saved_frame = nil -- copy of frame when dragging started
    self.saved_frame_rect = nil -- copy of frame_rect when dragging started
    self.drag_offset = nil -- relative pos of held panel tile
    self.resize_edge = nil -- which dimension is being resized?

    self.last_title_click_ms = 0 -- used to track double-clicking on the title
    self:addviews(args.subviews)
end

local function Panel_update_frame(self, frame, clear_state)
    if clear_state then
        self.kbd_get_pos = nil
        self.saved_frame = nil
        self.saved_frame_rect = nil
        self.drag_offset = nil
        self.resize_edge = nil
    end
    if not frame then return end
    if self.frame.l == frame.l and self.frame.r == frame.r
            and self.frame.t == frame.t and self.frame.b == frame.b
            and self.frame.w == frame.w and self.frame.h == frame.h then
        return
    end
    self.frame = frame
    self:updateLayout()
end

-- dim: the name of the dimension var (i.e. 'h' or 'w')
-- anchor: the name of the anchor var (i.e. 't', 'b', 'l', or 'r')
-- opposite_anchor: the name of the anchor var for the opposite edge
-- max_dim: how big this panel can get from its current pos and fit in parent
-- wanted_dim: how big the player is trying to make the panel
-- max_anchor: max value of the frame anchor for the edge that is being resized
-- wanted_anchor: how small the player is trying to make the anchor value
local function Panel_resize_edge_base(frame, resize_min, dim, anchor,
                                      opposite_anchor, max_dim, wanted_dim,
                                      max_anchor, wanted_anchor)
    frame[dim] = math.max(resize_min[dim], math.min(max_dim, wanted_dim))
    if frame[anchor] or not frame[opposite_anchor] then
        frame[anchor] = math.max(0, math.min(max_anchor, wanted_anchor))
    end
end

local function Panel_resize_edge(frame, resize_min, dim, anchor,
                                 opposite_anchor, dim_base, dim_ref, anchor_ref,
                                 dim_far, mouse_ref)
    local dim_sign = (anchor == 't' or anchor == 'l') and 1 or -1
    local max_dim = dim_base - dim_ref + 1
    local wanted_dim = dim_sign * (dim_far - mouse_ref) + 1
    local max_anchor = dim_base - resize_min[dim] - dim_ref + 1
    local wanted_anchor = dim_sign * (mouse_ref - anchor_ref)
    Panel_resize_edge_base(frame, resize_min, dim, anchor, opposite_anchor,
                           max_dim, wanted_dim, max_anchor, wanted_anchor)
end

local function Panel_resize_frame(self, mouse_pos)
    local frame, resize_min = copyall(self.frame), self.resize_min
    local parent_rect = self.frame_parent_rect
    local ref_rect = self.saved_frame_rect
    if self.resize_edge:find('t') then
        Panel_resize_edge(frame, resize_min, 'h', 't', 'b', ref_rect.y2,
                    parent_rect.y1, parent_rect.y1, ref_rect.y2, mouse_pos.y)
    end
    if self.resize_edge:find('b') then
        Panel_resize_edge(frame, resize_min, 'h', 'b', 't', parent_rect.y2,
                          ref_rect.y1, parent_rect.y2, ref_rect.y1, mouse_pos.y)
    end
    if self.resize_edge:find('l') then
        Panel_resize_edge(frame, resize_min, 'w', 'l', 'r', ref_rect.x2,
                    parent_rect.x1, parent_rect.x1, ref_rect.x2, mouse_pos.x)
    end
    if self.resize_edge:find('r') then
        Panel_resize_edge(frame, resize_min, 'w', 'r', 'l', parent_rect.x2,
                          ref_rect.x1, parent_rect.x2, ref_rect.x1, mouse_pos.x)
    end
    return frame
end

local function Panel_drag_frame(self, mouse_pos)
    local frame = copyall(self.frame)
    local parent_rect = self.frame_parent_rect
    local frame_rect = gui.mkdims_wh(
        self.frame_rect.x1+parent_rect.x1,
        self.frame_rect.y1+parent_rect.y1,
        self.frame_rect.width,
        self.frame_rect.height
    )
    local bound_rect = self.drag_bound == 'body' and self.frame_body
            or frame_rect
    local offset = self.drag_offset
    local max_width = parent_rect.width - (bound_rect.x2-frame_rect.x1+1)
    local max_height = parent_rect.height - (bound_rect.y2-frame_rect.y1+1)
    if frame.t or not frame.b then
        local min_pos = frame_rect.y1 - bound_rect.y1
        local requested_pos = mouse_pos.y - parent_rect.y1 - offset.y
        frame.t = math.max(min_pos, math.min(max_height, requested_pos))
    end
    if frame.b or not frame.t then
        local min_pos = bound_rect.y2 - frame_rect.y2
        local requested_pos = parent_rect.y2 - mouse_pos.y + offset.y -
                (frame_rect.y2 - frame_rect.y1)
        frame.b = math.max(min_pos, math.min(max_height, requested_pos))
    end
    if frame.l or not frame.r then
        local min_pos = frame_rect.x1 - bound_rect.x1
        local requested_pos = mouse_pos.x - parent_rect.x1 - offset.x
        frame.l = math.max(min_pos, math.min(max_width, requested_pos))
    end
    if frame.r or not frame.l then
        local min_pos = bound_rect.x2 - frame_rect.x2
        local requested_pos = parent_rect.x2 - mouse_pos.x + offset.x -
                (frame_rect.x2 - frame_rect.x1)
        frame.r = math.max(min_pos, math.min(max_width, requested_pos))
    end
    return frame
end

local function Panel_make_frame(self, mouse_pos)
    mouse_pos = mouse_pos or xy2pos(dfhack.screen.getMousePos())
    return self.resize_edge and Panel_resize_frame(self, mouse_pos)
            or Panel_drag_frame(self, mouse_pos)
end

local function Panel_begin_drag(self, drag_offset, resize_edge)
    Panel_update_frame(self, nil, true)
    self.drag_offset = drag_offset or {x=0, y=0}
    self.resize_edge = resize_edge
    self.saved_frame = copyall(self.frame)
    self.saved_frame_rect = gui.mkdims_wh(
        self.frame_rect.x1+self.frame_parent_rect.x1,
        self.frame_rect.y1+self.frame_parent_rect.y1,
        self.frame_rect.width,
        self.frame_rect.height)
    self.prev_focus_owner = self.focus_group.cur
    self:setFocus(true)
    if self.resize_edge then
        self:onResizeBegin()
    else
        self:onDragBegin()
    end
end

local function Panel_end_drag(self, frame, success)
    success = not not success
    if self.prev_focus_owner then
        self.prev_focus_owner:setFocus(true)
    else
        self:setFocus(false)
    end
    local resize_edge = self.resize_edge
    Panel_update_frame(self, frame, true)
    if resize_edge then
        self:onResizeEnd(success, self.frame)
    else
        self:onDragEnd(success, self.frame)
    end
end

local function Panel_on_double_click(self)
    local a = self.resize_anchors
    local can_vert, can_horiz = a.t or a.b, a.l or a.r
    if not can_vert and not can_horiz then return false end
    local f, rmin = self.frame, self.resize_min
    local maximized = f.t == 0 and f.b == 0 and f.l == 0 and f.r == 0
    local frame
    if maximized then
        frame =  {
            t=not can_vert and f.t or nil,
            l=not can_horiz and f.l or nil,
            b=not can_vert and f.b or nil,
            r=not can_horiz and f.r or nil,
            w=can_vert and rmin.w or f.w,
            h=can_horiz and rmin.h or f.h,
        }
    else
        frame = {
            t=can_vert and 0 or f.t,
            l=can_horiz and 0 or f.l,
            b=can_vert and 0 or f.b,
            r=can_horiz and 0 or f.r
        }
    end
    Panel_update_frame(self, frame, true)
end

---@alias widgets.Keys
---| '_STRING'
---| '_MOUSE_L'
---| '_MOUSE_L_DOWN'
---| '_MOUSE_R'
---| '_MOUSE_R_DOWN'
---| '_MOUSE_M'
---| '_MOUSE_M_DOWN'

---@param keys table<string|integer|widgets.Keys, boolean>
---@return boolean|nil
function Panel:onInput(keys)
    if self.kbd_get_pos then
        if keys.SELECT or keys.LEAVESCREEN or keys._MOUSE_R then
            Panel_end_drag(self, not keys.SELECT and self.saved_frame or nil,
                           not not keys.SELECT)
            return true
        end
        for code in pairs(keys) do
            local dx, dy = guidm.get_movement_delta(code, 1, 10)
            if dx then
                local kbd_pos = self.kbd_get_pos()
                kbd_pos.x = kbd_pos.x + dx
                kbd_pos.y = kbd_pos.y + dy
                Panel_update_frame(self, Panel_make_frame(self, kbd_pos))
                return true
            end
        end
        return
    end
    if self.drag_offset then
        if keys._MOUSE_R then
            Panel_end_drag(self, self.saved_frame)
        elseif keys._MOUSE_L_DOWN then
            Panel_update_frame(self, Panel_make_frame(self))
        end
        return true
    end
    if Panel.super.onInput(self, keys) then
        return true
    end
    if not keys._MOUSE_L then return end
    local x,y = self:getMouseFramePos()
    if not x then return end

    if self.resizable and y == 0 then
        local now_ms = dfhack.getTickCount()
        if now_ms - self.last_title_click_ms <= DOUBLE_CLICK_MS then
            self.last_title_click_ms = 0
            if Panel_on_double_click(self) then return true end
        else
            self.last_title_click_ms = now_ms
        end
    end

    local resize_edge = nil
    if self.resizable then
        local rect = gui.mkdims_wh(
            self.frame_rect.x1+self.frame_parent_rect.x1,
            self.frame_rect.y1+self.frame_parent_rect.y1,
            self.frame_rect.width,
            self.frame_rect.height)
        if self.resize_anchors.r and self.resize_anchors.b
                and x == rect.x2 - rect.x1 and y == rect.y2 - rect.y1 then
            resize_edge = 'rb'
        elseif self.resize_anchors.l and self.resize_anchors.b
                and x == 0 and y == rect.y2 - rect.y1 then
            resize_edge = 'lb'
        elseif self.resize_anchors.r and self.resize_anchors.t
                and x == rect.x2 - rect.x1 and y == 0 then
            resize_edge = 'rt'
        elseif self.resize_anchors.r and self.resize_anchors.t
                and x == 0 and y == 0 then
            resize_edge = 'lt'
        elseif self.resize_anchors.r and x == rect.x2 - rect.x1 then
            resize_edge = 'r'
        elseif self.resize_anchors.l and x == 0 then
            resize_edge = 'l'
        elseif self.resize_anchors.b and y == rect.y2 - rect.y1 then
            resize_edge = 'b'
        elseif self.resize_anchors.t and y == 0 then
            resize_edge = 't'
        end
    end

    local is_dragging = false
    if not resize_edge and self.draggable then
        local on_body = self:getMousePos()
        is_dragging = (self.drag_anchors.title and self.frame_style and y == 0)
                or (self.drag_anchors.frame and not on_body) -- includes inset
                or (self.drag_anchors.body and on_body)
    end

    if resize_edge or is_dragging then
        Panel_begin_drag(self, {x=x, y=y}, resize_edge)
        return true
    end
end

---@param enabled boolean
function Panel:setKeyboardDragEnabled(enabled)
    if (enabled and self.kbd_get_pos)
            or (not enabled and not self.kbd_get_pos) then
        return
    end
    if enabled then
        local kbd_get_pos = function()
            return {
                x=self.frame_rect.x1+self.frame_parent_rect.x1,
                y=self.frame_rect.y1+self.frame_parent_rect.y1
            }
        end
        Panel_begin_drag(self)
        self.kbd_get_pos = kbd_get_pos
    else
        Panel_end_drag(self)
    end
end

local function Panel_get_resize_data(self)
    local resize_anchors = self.resize_anchors
    local frame_rect = gui.mkdims_wh(
        self.frame_rect.x1+self.frame_parent_rect.x1,
        self.frame_rect.y1+self.frame_parent_rect.y1,
        self.frame_rect.width,
        self.frame_rect.height)
    if resize_anchors.r and resize_anchors.b then
        return 'rb', function()
            return {x=frame_rect.x2, y=frame_rect.y2} end
    elseif resize_anchors.l and resize_anchors.b then
        return 'lb', function()
            return {x=frame_rect.x1, y=frame_rect.y2} end
    elseif resize_anchors.r and resize_anchors.t then
        return 'rt', function()
            return {x=frame_rect.x2, y=frame_rect.y1} end
    elseif resize_anchors.l and resize_anchors.t then
        return 'lt', function()
            return {x=frame_rect.x1, y=frame_rect.y1} end
    elseif resize_anchors.b then
        return 'b', function()
            return {x=(frame_rect.x1+frame_rect.x2)//2,
                    y=frame_rect.y2} end
    elseif resize_anchors.r then
        return 'r', function()
            return {x=frame_rect.x2,
                    y=(frame_rect.y1+frame_rect.y2)//2} end
    elseif resize_anchors.l then
        return 'l', function()
            return {x=frame_rect.x1,
                    y=(frame_rect.y1+frame_rect.y2)//2} end
    elseif resize_anchors.t then
        return 't', function()
            return {x=(frame_rect.x1+frame_rect.x2)//2,
                    y=frame_rect.y1} end
    end
end

---@param enabled boolean
function Panel:setKeyboardResizeEnabled(enabled)
    if (enabled and self.kbd_get_pos)
            or (not enabled and not self.kbd_get_pos) then
        return
    end
    if enabled then
        local resize_edge, kbd_get_pos = Panel_get_resize_data(self)
        if not resize_edge then
            dfhack.printerr('cannot resize window: no anchors are enabled')
        else
            Panel_begin_drag(self, kbd_get_pos(), resize_edge)
            self.kbd_get_pos = kbd_get_pos
        end
    else
        Panel_end_drag(self)
    end
end

function Panel:onRenderBody(dc)
    if self.on_render then self.on_render(dc) end
end

function Panel:computeFrame(parent_rect)
    local sw, sh = parent_rect.width, parent_rect.height
    if self.frame then
        if self.frame.t and self.frame.h and self.frame.t + self.frame.h > sh then
            self.frame.t = math.max(0, sh - self.frame.h)
        end
        if self.frame.b and self.frame.h and self.frame.b + self.frame.h > sh then
            self.frame.b = math.max(0, sh - self.frame.h)
        end
        if self.frame.l and self.frame.w and self.frame.l + self.frame.w > sw then
            self.frame.l = math.max(0, sw - self.frame.w)
        end
        if self.frame.r and self.frame.w and self.frame.r + self.frame.w > sw then
            self.frame.r = math.max(0, sw - self.frame.w)
        end
    end
    return gui.compute_frame_body(sw, sh, self.frame, self.frame_inset,
                                  self.frame_style and 1 or 0)
end

function Panel:postComputeFrame(body)
    if self.on_layout then self.on_layout(body) end
end

-- if self.autoarrange_subviews is true, lay out visible subviews vertically,
-- adding gaps between widgets according to self.autoarrange_gap.
function Panel:postUpdateLayout()
    -- don't leave artifacts behind on the parent screen when we move
    gui.Screen.request_full_screen_refresh = true

    if not self.autoarrange_subviews then return end

    local gap = self.autoarrange_gap
    local y = 0
    for _,subview in ipairs(self.subviews) do
        if not subview.frame then goto continue end
        subview.frame.t = y
        if getval(subview.visible) then
            y = y + (subview.frame.h or 0) + gap
        end
        ::continue::
    end

    -- let widgets adjust to their new positions
    self:updateSubviewLayout()
end

function Panel:onRenderFrame(dc, rect)
    Panel.super.onRenderFrame(self, dc, rect)
    if not self.frame_style then return end
    local inactive = self.parent_view and self.parent_view.hasFocus
            and not self.parent_view:hasFocus()
    local pause_forced = not self.no_force_pause_badge and safe_index(self.parent_view, 'force_pause')
    gui.paint_frame(dc, rect, self.frame_style, self.frame_title, inactive,
            pause_forced, self.resizable)
    if self.kbd_get_pos then
        local pos = self.kbd_get_pos()
        local pen = to_pen{fg=COLOR_GREEN, bg=COLOR_BLACK}
        dc:seek(pos.x, pos.y):pen(pen):char(string.char(0xDB))
    end
    if self.drag_offset and not self.kbd_get_pos
            and df.global.enabler.mouse_lbut_down == 0 then
        Panel_end_drag(self, nil, true)
    end
end

function Panel:onDragBegin()
    if self.on_drag_begin then self.on_drag_begin() end
end

function Panel:onDragEnd(success, new_frame)
    if self.on_drag_end then self.on_drag_end(success, new_frame) end
end

function Panel:onResizeBegin()
    if self.on_resize_begin then self.on_resize_begin() end
end

function Panel:onResizeEnd(success, new_frame)
    if self.on_resize_end then self.on_resize_end(success, new_frame) end
end

------------
-- Window --
------------

---@class widgets.Window.attrs: widgets.Panel.attrs
---@field frame_style gui.Frame|fun(): gui.Frame
---@field frame_background dfhack.color|dfhack.pen
---@field frame_inset integer

---@class widgets.Window.attrs.partial: widgets.Window.attrs

---@class widgets.Window: widgets.Panel, widgets.Window.attrs
---@field super widgets.Panel
---@field ATTRS widgets.Window.attrs|fun(attributes: widgets.Window.attrs.partial)
---@overload fun(init_table: widgets.Window.attrs.partial): self
Window = defclass(Window, Panel)

Window.ATTRS {
    frame_style = gui.WINDOW_FRAME,
    frame_background = gui.CLEAR_PEN,
    frame_inset = 1,
    draggable = true,
}

-------------------
-- ResizingPanel --
-------------------

---@class widgets.ResizingPanel.attrs: widgets.Panel.attrs
---@field auto_height boolean
---@field auto_width boolean

---@class widgets.ResizingPanel.attrs.partial: widgets.ResizingPanel.attrs

---@class widgets.ResizingPanel: widgets.Panel, widgets.ResizingPanel.attrs
---@field super widgets.Panel
---@field ATTRS widgets.ResizingPanel.attrs|fun(attributes: widgets.ResizingPanel.attrs.partial)
---@overload fun(init_table: widgets.ResizingPanel.attrs.partial): self
ResizingPanel = defclass(ResizingPanel, Panel)

ResizingPanel.ATTRS{
    auto_height = true,
    auto_width = false,
}

-- adjust our frame dimensions according to positions and sizes of our subviews
function ResizingPanel:postUpdateLayout(frame_body)
    local w, h = 0, 0
    for _,s in ipairs(self.subviews) do
        if getval(s.visible) then
            w = math.max(w, (s.frame and s.frame.l or 0) +
                            (s.frame and s.frame.w or frame_body.width))
            h = math.max(h, (s.frame and s.frame.t or 0) +
                            (s.frame and s.frame.h or frame_body.height))
        end
    end
    local l,t,r,b = gui.parse_inset(self.frame_inset)
    w = w + l + r
    h = h + t + b
    if self.frame_style then
        w = w + 2
        h = h + 2
    end
    if not self.frame then self.frame = {} end
    local oldw, oldh = self.frame.w, self.frame.h
    if not self.auto_height then h = oldh end
    if not self.auto_width then w = oldw end
    self.frame.w, self.frame.h = w, h
    if not self._updateLayoutGuard and (oldw ~= w or oldh ~= h) then
        self._updateLayoutGuard = true -- protect against infinite loops
        self:updateLayout() -- our frame has changed, we need to fully refresh
    end
    self._updateLayoutGuard = nil
end

-----------
-- Pages --
-----------

---@class widgets.Pages.initTable: widgets.Panel.attrs
---@field selected? integer|string

---@class widgets.Pages: widgets.Panel
---@field super widgets.Panel
---@overload fun(attributes: widgets.Pages.initTable): self
Pages = defclass(Pages, Panel)

---@param self widgets.Pages
---@param args widgets.Pages.initTable
function Pages:init(args)
    for _,v in ipairs(self.subviews) do
        v.visible = false
    end
    self:setSelected(args.selected or 1)
end

---@param idx integer|string
function Pages:setSelected(idx)
    if type(idx) ~= 'number' then
        local key = idx
        if type(idx) == 'string' then
            key = self.subviews[key]
        end
        idx = utils.linear_index(self.subviews, key)
        if not idx then
            error('Unknown page: '..tostring(key))
        end
    end

    show_view(self.subviews[self.selected], false)
    self.selected = math.min(math.max(1, idx), #self.subviews)
    show_view(self.subviews[self.selected], true)
end

---@return integer index
---@return gui.View child
function Pages:getSelected()
    return self.selected, self.subviews[self.selected]
end

---@return gui.View child
function Pages:getSelectedPage()
    return self.subviews[self.selected]
end

----------------
-- Edit field --
----------------

---@class widgets.EditField.attrs: widgets.Widget.attrs
---@field label_text? string
---@field text string
---@field text_pen? dfhack.color|dfhack.pen
---@field on_char? function
---@field on_change? function
---@field on_submit? function
---@field on_submit2? function
---@field key? string
---@field key_sep? string
---@field modal boolean
---@field ignore_keys? string[]

---@class widgets.EditField.attrs.partial: widgets.EditField.attrs

---@class widgets.EditField: widgets.Widget, widgets.EditField.attrs
---@field super widgets.Widget
---@field ATTRS widgets.EditField.attrs|fun(attributes: widgets.EditField.attrs.partial)
---@overload fun(init_table: widgets.EditField.attrs.partial): self
EditField = defclass(EditField, Widget)

EditField.ATTRS{
    label_text = DEFAULT_NIL,
    text = '',
    text_pen = DEFAULT_NIL,
    on_char = DEFAULT_NIL,
    on_change = DEFAULT_NIL,
    on_submit = DEFAULT_NIL,
    on_submit2 = DEFAULT_NIL,
    key = DEFAULT_NIL,
    key_sep = DEFAULT_NIL,
    modal = false,
    ignore_keys = DEFAULT_NIL,
}

---@param init_table widgets.EditField.attrs
function EditField:preinit(init_table)
    init_table.frame = init_table.frame or {}
    init_table.frame.h = init_table.frame.h or 1
end

function EditField:init()
    local function on_activate()
        self.saved_text = self.text
        self:setFocus(true)
    end

    self.start_pos = 1
    self.cursor = #self.text + 1

    self:addviews{HotkeyLabel{frame={t=0,l=0},
                              key=self.key,
                              key_sep=self.key_sep,
                              label=self.label_text,
                              on_activate=self.key and on_activate or nil}}
end

function EditField:getPreferredFocusState()
    return not self.key
end

function EditField:setCursor(cursor)
    if not cursor or cursor > #self.text then
        self.cursor = #self.text + 1
        return
    end
    self.cursor = math.max(1, cursor)
end

function EditField:setText(text, cursor)
    local old = self.text
    self.text = text
    self:setCursor(cursor)
    if self.on_change and text ~= old then
        self.on_change(self.text, old)
    end
end

function EditField:postUpdateLayout()
    self.text_offset = self.subviews[1]:getTextWidth()
end

---@param dc gui.Painter
function EditField:onRenderBody(dc)
    dc:pen(self.text_pen or COLOR_LIGHTCYAN)

    local cursor_char = '_'
    if not getval(self.active) or not self.focus or gui.blink_visible(300) then
        cursor_char = (self.cursor > #self.text) and ' ' or
                                        self.text:sub(self.cursor, self.cursor)
    end
    local txt = self.text:sub(1, self.cursor - 1) .. cursor_char ..
                                                self.text:sub(self.cursor + 1)
    local max_width = dc.width - self.text_offset
    self.start_pos = 1
    if #txt > max_width then
        -- get the substring in the vicinity of the cursor
        max_width = max_width - 2
        local half_width = math.floor(max_width/2)
        local start_pos = math.max(1, self.cursor-half_width)
        local end_pos = math.min(#txt, self.cursor+half_width-1)
        if self.cursor + half_width > #txt then
            start_pos = #txt - (max_width - 1)
        end
        if self.cursor - half_width <= 1 then
            end_pos = max_width + 1
        end
        self.start_pos = start_pos > 1 and start_pos - 1 or start_pos
        txt = ('%s%s%s'):format(start_pos == 1 and '' or string.char(27),
                                txt:sub(start_pos, end_pos),
                                end_pos == #txt and '' or string.char(26))
    end
    dc:advance(self.text_offset):string(txt)
end

function EditField:insert(text)
    local old = self.text
    self:setText(old:sub(1,self.cursor-1)..text..old:sub(self.cursor),
                 self.cursor + #text)
end

function EditField:onInput(keys)
    if not self.focus then
        -- only react to our hotkey
        return self:inputToSubviews(keys)
    end

    if self.ignore_keys then
        for _,ignore_key in ipairs(self.ignore_keys) do
            if keys[ignore_key] then return false end
        end
    end

    if self.key and (keys.LEAVESCREEN or keys._MOUSE_R) then
        self:setText(self.saved_text)
        self:setFocus(false)
        return true
    end

    if keys.SELECT or keys.SELECT_ALL then
        if self.key then
            self:setFocus(false)
        end
        if keys.SELECT_ALL then
            if self.on_submit2 then
                self.on_submit2(self.text)
                return true
            end
        else
            if self.on_submit then
                self.on_submit(self.text)
                return true
            end
        end
        return not not self.key
    elseif keys._STRING then
        local old = self.text
        if keys._STRING == 0 then
            -- handle backspace
            local del_pos = self.cursor - 1
            if del_pos > 0 then
                self:setText(old:sub(1, del_pos-1) .. old:sub(del_pos+1),
                             del_pos)
            end
        else
            local cv = string.char(keys._STRING)
            if not self.on_char or self.on_char(cv, old) then
                self:insert(cv)
            elseif self.on_char then
                return self.modal
            end
        end
        return true
    elseif keys.KEYBOARD_CURSOR_LEFT then
        self:setCursor(self.cursor - 1)
        return true
    elseif keys.CUSTOM_CTRL_B then -- back one word
        local _, prev_word_end = self.text:sub(1, self.cursor-1):
                                               find('.*[%w_%-][^%w_%-]')
        self:setCursor(prev_word_end or 1)
        return true
    -- commented out until we get HOME key support from DF
    -- elseif keys.CUSTOM_CTRL_A then -- home
    --     self:setCursor(1)
    --     return true
    elseif keys.KEYBOARD_CURSOR_RIGHT then
        self:setCursor(self.cursor + 1)
        return true
    elseif keys.CUSTOM_CTRL_F then -- forward one word
        local _,next_word_start = self.text:find('[^%w_%-][%w_%-]', self.cursor)
        self:setCursor(next_word_start)
        return true
    elseif keys.CUSTOM_CTRL_E then -- end
        self:setCursor()
        return true
    elseif keys.CUSTOM_CTRL_C then
        dfhack.internal.setClipboardTextCp437(self.text)
        return true
    elseif keys.CUSTOM_CTRL_X then
        dfhack.internal.setClipboardTextCp437(self.text)
        self:setText('')
        return true
    elseif keys.CUSTOM_CTRL_V then
        self:insert(dfhack.internal.getClipboardTextCp437())
        return true
    elseif keys._MOUSE_L_DOWN then
        local mouse_x = self:getMousePos()
        if mouse_x then
            self:setCursor(self.start_pos + mouse_x - (self.text_offset or 0))
            return true
        end
    end

    -- if we're modal, then unconditionally eat all the input
    return self.modal
end

---------------
-- Scrollbar --
---------------

SCROLL_INITIAL_DELAY_MS = 300
SCROLL_DELAY_MS = 20

---@class widgets.Scrollbar.attrs: widgets.Widget.attrs
---@field on_scroll? fun(new_top_elem?: integer)

---@class widgets.Scrollbar.attrs.partial: widgets.Scrollbar.attrs

---@class widgets.Scrollbar: widgets.Widget, widgets.Scrollbar.attrs
---@field super widgets.Widget
---@field ATTRS widgets.Scrollbar.attrs|fun(attributes: widgets.Scrollbar.attrs.partial)
---@overload fun(init_table: widgets.Scrollbar.attrs.partial): self
Scrollbar = defclass(Scrollbar, Widget)

Scrollbar.ATTRS{
    on_scroll = DEFAULT_NIL,
}

---@param init_table widgets.Scrollbar.attrs.partial
function Scrollbar:preinit(init_table)
    init_table.frame = init_table.frame or {}
    init_table.frame.w = init_table.frame.w or 2
end

function Scrollbar:init()
    self.last_scroll_ms = 0
    self.is_first_click = false
    self.scroll_spec = nil
    self.is_dragging = false -- index of the scrollbar tile that we're dragging
    self:update(1, 1, 1)
end

local function scrollbar_get_max_pos_and_height(scrollbar)
    local frame_body = scrollbar.frame_body
    local scrollbar_body_height = (frame_body and frame_body.height or 3) - 2

    local height = math.max(2, math.floor(
        (math.min(scrollbar.elems_per_page, scrollbar.num_elems) * scrollbar_body_height) /
        scrollbar.num_elems))

    return scrollbar_body_height - height, height
end

-- calculate and cache the number of tiles of empty space above the top of the
-- scrollbar and the number of tiles the scrollbar should occupy to represent
-- the percentage of text that is on the screen.
-- if elems_per_page or num_elems are not specified, the last values passed to
-- Scrollbar:update() are used.
function Scrollbar:update(top_elem, elems_per_page, num_elems)
    if not top_elem then error('must specify index of new top element') end
    elems_per_page = elems_per_page or self.elems_per_page
    num_elems = num_elems or self.num_elems
    self.top_elem = top_elem
    self.elems_per_page, self.num_elems = elems_per_page, num_elems

    local max_pos, height = scrollbar_get_max_pos_and_height(self)
    local pos = (num_elems == elems_per_page) and 0 or
            math.ceil(((top_elem-1) * max_pos) /
                      (num_elems - elems_per_page))

    self.bar_offset, self.bar_height = pos, height
end

local function scrollbar_do_drag(scrollbar)
    local x,y = dfhack.screen.getMousePos()
    if not y then return end
    x,y = scrollbar.frame_body:localXY(x, y)
    local cur_pos = y - scrollbar.is_dragging
    local max_top = scrollbar.num_elems - scrollbar.elems_per_page + 1
    local max_pos = scrollbar_get_max_pos_and_height(scrollbar)
    local new_top_elem = math.floor(cur_pos * max_top / max_pos) + 1
    new_top_elem = math.max(1, math.min(new_top_elem, max_top))
    if new_top_elem ~= scrollbar.top_elem then
        scrollbar.on_scroll(new_top_elem)
    end
end

local function scrollbar_is_visible(scrollbar)
    return scrollbar.elems_per_page < scrollbar.num_elems
end

local SBSO = df.global.init.scrollbar_texpos[0] --Scroll Bar Spritesheet Offset / change this to point to a different spritesheet (ui themes, anyone? :p)
local SCROLLBAR_UP_LEFT_PEN = to_pen{tile=SBSO+0, ch=47, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_UP_RIGHT_PEN = to_pen{tile=SBSO+1, ch=92, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_DOWN_LEFT_PEN = to_pen{tile=SBSO+24, ch=92, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_DOWN_RIGHT_PEN = to_pen{tile=SBSO+25, ch=47, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_UP_LEFT_PEN = to_pen{tile=SBSO+6, ch=219, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_UP_RIGHT_PEN = to_pen{tile=SBSO+7, ch=219, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_LEFT_PEN = to_pen{tile=SBSO+30, ch=219, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_RIGHT_PEN = to_pen{tile=SBSO+31, ch=219, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_CENTER_UP_LEFT_PEN = to_pen{tile=SBSO+10, ch=219, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_CENTER_UP_RIGHT_PEN = to_pen{tile=SBSO+11, ch=219, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_CENTER_DOWN_LEFT_PEN = to_pen{tile=SBSO+22, ch=219, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_CENTER_DOWN_RIGHT_PEN = to_pen{tile=SBSO+23, ch=219, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_CENTER_LEFT_PEN = to_pen{tile=SBSO+18, ch=219, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_CENTER_RIGHT_PEN = to_pen{tile=SBSO+19, ch=219, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_DOWN_LEFT_PEN = to_pen{tile=SBSO+42, ch=219, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_DOWN_RIGHT_PEN = to_pen{tile=SBSO+43, ch=219, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_2TALL_UP_LEFT_PEN = to_pen{tile=SBSO+26, ch=219, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_2TALL_UP_RIGHT_PEN = to_pen{tile=SBSO+27, ch=219, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_2TALL_DOWN_LEFT_PEN = to_pen{tile=SBSO+38, ch=219, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_2TALL_DOWN_RIGHT_PEN = to_pen{tile=SBSO+39, ch=219, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_UP_LEFT_HOVER_PEN = to_pen{tile=SBSO+2, ch=47, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_UP_RIGHT_HOVER_PEN = to_pen{tile=SBSO+3, ch=92, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_DOWN_LEFT_HOVER_PEN = to_pen{tile=SBSO+14, ch=92, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_DOWN_RIGHT_HOVER_PEN = to_pen{tile=SBSO+15, ch=47, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_UP_LEFT_HOVER_PEN = to_pen{tile=SBSO+8, ch=219, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_UP_RIGHT_HOVER_PEN = to_pen{tile=SBSO+9, ch=219, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_LEFT_HOVER_PEN = to_pen{tile=SBSO+32, ch=219, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_RIGHT_HOVER_PEN = to_pen{tile=SBSO+33, ch=219, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_CENTER_UP_LEFT_HOVER_PEN = to_pen{tile=SBSO+34, ch=219, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_CENTER_UP_RIGHT_HOVER_PEN = to_pen{tile=SBSO+35, ch=219, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_CENTER_DOWN_LEFT_HOVER_PEN = to_pen{tile=SBSO+46, ch=219, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_CENTER_DOWN_RIGHT_HOVER_PEN = to_pen{tile=SBSO+47, ch=219, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_CENTER_LEFT_HOVER_PEN = to_pen{tile=SBSO+20, ch=219, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_CENTER_RIGHT_HOVER_PEN = to_pen{tile=SBSO+21, ch=219, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_DOWN_LEFT_HOVER_PEN = to_pen{tile=SBSO+44, ch=219, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_DOWN_RIGHT_HOVER_PEN = to_pen{tile=SBSO+45, ch=219, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_2TALL_UP_LEFT_HOVER_PEN = to_pen{tile=SBSO+28, ch=219, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_2TALL_UP_RIGHT_HOVER_PEN = to_pen{tile=SBSO+29, ch=219, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_2TALL_DOWN_LEFT_HOVER_PEN = to_pen{tile=SBSO+40, ch=219, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_2TALL_DOWN_RIGHT_HOVER_PEN = to_pen{tile=SBSO+41, ch=219, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_BG_LEFT_PEN = to_pen{tile=SBSO+12, ch=176, fg=COLOR_DARKGREY, bg=COLOR_BLACK}
local SCROLLBAR_BAR_BG_RIGHT_PEN = to_pen{tile=SBSO+13, ch=176, fg=COLOR_DARKGREY, bg=COLOR_BLACK}

function Scrollbar:onRenderBody(dc)
    -- don't draw if all elements are visible
    if not scrollbar_is_visible(self) then
        return
    end
    -- determine which elements should be highlighted
    local _,hover_y = self:getMousePos()
    local hover_up, hover_down, hover_bar = false, false, false
    if hover_y == 0 then
        hover_up = true
    elseif hover_y == dc.height-1 then
        hover_down = true
    elseif hover_y then
        hover_bar = true
    end
    -- render up arrow
    dc:seek(0, 0)
    dc:char(nil, hover_up and SCROLLBAR_UP_LEFT_HOVER_PEN or SCROLLBAR_UP_LEFT_PEN)
    dc:char(nil, hover_up and SCROLLBAR_UP_RIGHT_HOVER_PEN or SCROLLBAR_UP_RIGHT_PEN)
    -- render scrollbar body
    local starty = self.bar_offset + 1
    local endy = self.bar_offset + self.bar_height
    local midy = (starty + endy)/2
    for y=1,dc.height-2 do
        dc:seek(0, y)
        if y >= starty and y <= endy then
            if y == starty and y <= midy - 1 then
                dc:char(nil, hover_bar and SCROLLBAR_BAR_UP_LEFT_HOVER_PEN or SCROLLBAR_BAR_UP_LEFT_PEN)
                dc:char(nil, hover_bar and SCROLLBAR_BAR_UP_RIGHT_HOVER_PEN or SCROLLBAR_BAR_UP_RIGHT_PEN)
            elseif y == midy - 0.5 and self.bar_height == 2 then
                dc:char(nil, hover_bar and SCROLLBAR_BAR_2TALL_UP_LEFT_HOVER_PEN or SCROLLBAR_BAR_2TALL_UP_LEFT_PEN)
                dc:char(nil, hover_bar and SCROLLBAR_BAR_2TALL_UP_RIGHT_HOVER_PEN or SCROLLBAR_BAR_2TALL_UP_RIGHT_PEN)
            elseif y == midy + 0.5 and self.bar_height == 2 then
                dc:char(nil, hover_bar and SCROLLBAR_BAR_2TALL_DOWN_LEFT_HOVER_PEN or SCROLLBAR_BAR_2TALL_DOWN_LEFT_PEN)
                dc:char(nil, hover_bar and SCROLLBAR_BAR_2TALL_DOWN_RIGHT_HOVER_PEN or SCROLLBAR_BAR_2TALL_DOWN_RIGHT_PEN)
            elseif y == midy - 0.5 then
                dc:char(nil, hover_bar and SCROLLBAR_BAR_CENTER_UP_LEFT_HOVER_PEN or SCROLLBAR_BAR_CENTER_UP_LEFT_PEN)
                dc:char(nil, hover_bar and SCROLLBAR_BAR_CENTER_UP_RIGHT_HOVER_PEN or SCROLLBAR_BAR_CENTER_UP_RIGHT_PEN)
            elseif y == midy + 0.5 then
                dc:char(nil, hover_bar and SCROLLBAR_BAR_CENTER_DOWN_LEFT_HOVER_PEN or SCROLLBAR_BAR_CENTER_DOWN_LEFT_PEN)
                dc:char(nil, hover_bar and SCROLLBAR_BAR_CENTER_DOWN_RIGHT_HOVER_PEN or SCROLLBAR_BAR_CENTER_DOWN_RIGHT_PEN)
            elseif y == midy then
                dc:char(nil, hover_bar and SCROLLBAR_BAR_CENTER_LEFT_HOVER_PEN or SCROLLBAR_BAR_CENTER_LEFT_PEN)
                dc:char(nil, hover_bar and SCROLLBAR_BAR_CENTER_RIGHT_HOVER_PEN or SCROLLBAR_BAR_CENTER_RIGHT_PEN)
            elseif y == endy and y >= midy + 1 then
                dc:char(nil, hover_bar and SCROLLBAR_BAR_DOWN_LEFT_HOVER_PEN or SCROLLBAR_BAR_DOWN_LEFT_PEN)
                dc:char(nil, hover_bar and SCROLLBAR_BAR_DOWN_RIGHT_HOVER_PEN or SCROLLBAR_BAR_DOWN_RIGHT_PEN)
            else
                dc:char(nil, hover_bar and SCROLLBAR_BAR_LEFT_HOVER_PEN or SCROLLBAR_BAR_LEFT_PEN)
                dc:char(nil, hover_bar and SCROLLBAR_BAR_RIGHT_HOVER_PEN or SCROLLBAR_BAR_RIGHT_PEN)
            end
        else
            dc:char(nil, SCROLLBAR_BAR_BG_LEFT_PEN)
            dc:char(nil, SCROLLBAR_BAR_BG_RIGHT_PEN)
        end
    end
    -- render down arrow
    dc:seek(0, dc.height-1)
    dc:char(nil, hover_down and SCROLLBAR_DOWN_LEFT_HOVER_PEN or SCROLLBAR_DOWN_LEFT_PEN)
    dc:char(nil, hover_down and SCROLLBAR_DOWN_RIGHT_HOVER_PEN or SCROLLBAR_DOWN_RIGHT_PEN)
    if not self.on_scroll then return end
    -- manage state for dragging and continuous scrolling
    if self.is_dragging then
        scrollbar_do_drag(self)
    end
    if df.global.enabler.mouse_lbut_down == 0 then
        self.last_scroll_ms = 0
        self.is_dragging = false
        self.scroll_spec = nil
        return
    end
    if self.last_scroll_ms == 0 then return end
    local now = dfhack.getTickCount()
    local delay = self.is_first_click and
            SCROLL_INITIAL_DELAY_MS or SCROLL_DELAY_MS
    if now - self.last_scroll_ms >= delay then
        self.is_first_click = false
        self.on_scroll(self.scroll_spec)
        self.last_scroll_ms = now
    end
end

function Scrollbar:onInput(keys)
    if not self.on_scroll or not scrollbar_is_visible(self) then
        return false
    end

    if self.parent_view and self.parent_view:getMousePos() then
        if keys.CONTEXT_SCROLL_UP then
            self.on_scroll('up_small')
            return true
        elseif keys.CONTEXT_SCROLL_DOWN then
            self.on_scroll('down_small')
            return true
        elseif keys.CONTEXT_SCROLL_PAGEUP then
            self.on_scroll('up_large')
            return true
        elseif keys.CONTEXT_SCROLL_PAGEDOWN then
            self.on_scroll('down_large')
            return true
        end
    end
    if not keys._MOUSE_L then return false end
    local _,y = self:getMousePos()
    if not y then return false end
    local scroll_spec = nil
    if y == 0 then scroll_spec = 'up_small'
    elseif y == self.frame_body.height - 1 then scroll_spec = 'down_small'
    elseif y <= self.bar_offset then scroll_spec = 'up_large'
    elseif y > self.bar_offset + self.bar_height then scroll_spec = 'down_large'
    else
        self.is_dragging = y - self.bar_offset
        return true
    end
    self.scroll_spec = scroll_spec
    self.on_scroll(scroll_spec)
    -- reset continuous scroll state
    self.is_first_click = true
    self.last_scroll_ms = dfhack.getTickCount()
    return true
end

-----------
-- Label --
-----------

function parse_label_text(obj)
    local text = obj.text or {}
    if type(text) ~= 'table' then
        text = { text }
    end
    local curline = nil
    local out = { }
    local active = nil
    local idtab = nil
    for _,v in ipairs(text) do
        local vv
        if type(v) == 'string' then
            vv = v:split(NEWLINE)
        else
            vv = { v }
        end

        for i = 1,#vv do
            local cv = vv[i]
            if i > 1 then
                if not curline then
                    table.insert(out, {})
                end
                curline = nil
            end
            if cv ~= '' then
                if not curline then
                    curline = {}
                    table.insert(out, curline)
                end

                if type(cv) == 'string' then
                    table.insert(curline, { text = cv })
                else
                    table.insert(curline, cv)

                    if cv.on_activate then
                        active = active or {}
                        table.insert(active, cv)
                    end

                    if cv.id then
                        idtab = idtab or {}
                        idtab[cv.id] = cv
                    end
                end
            end
        end
    end
    obj.text_lines = out
    obj.text_active = active
    obj.text_ids = idtab
end

local function is_disabled(token)
    return (token.disabled ~= nil and getval(token.disabled)) or
           (token.enabled ~= nil and not getval(token.enabled))
end

-- Make the hover pen -- that is a pen that should render elements that has the
-- mouse hovering over it. if hpen is specified, it just checks the fields and
-- returns it (in parsed pen form)
local function make_hpen(pen, hpen)
    if not hpen then
        pen = to_pen(pen)

        -- Swap the foreground and background
        hpen = dfhack.pen.make(pen.bg, nil, pen.fg + (pen.bold and 8 or 0))
    end

    -- text_hpen needs a character in order to paint the background using
    -- Painter:fill(), so let's make it paint a space to show the background
    -- color
    local hpen_parsed = to_pen(hpen)
    hpen_parsed.ch = string.byte(' ')
    return hpen_parsed
end

---@param obj any
---@param dc gui.Painter
---@param x0 integer
---@param y0 integer
---@param pen dfhack.pen|dfhack.color|fun(): dfhack.pen|dfhack.color
---@param dpen dfhack.pen|dfhack.color|fun(): dfhack.pen|dfhack.color
---@param disabled boolean
---@param hpen dfhack.pen|dfhack.color|fun(): dfhack.pen|dfhack.color
---@param hovered boolean
function render_text(obj,dc,x0,y0,pen,dpen,disabled,hpen,hovered)
    pen, dpen, hpen = getval(pen), getval(dpen), getval(hpen)
    local width = 0
    for iline = dc and obj.start_line_num or 1, #obj.text_lines do
        local x, line = 0, obj.text_lines[iline]
        if dc then
            local offset = (obj.start_line_num or 1) - 1
            local y = y0 + iline - offset - 1
            -- skip text outside of the containing frame
            if y > dc.height - 1 then break end
            dc:seek(x+x0, y)
        end
        for _,token in ipairs(line) do
            token.line = iline
            token.x1 = x

            if token.gap then
                x = x + token.gap
                if dc then
                    dc:advance(token.gap)
                end
            end

            if token.tile or (hovered and token.htile) then
                x = x + 1
                if dc then
                    local tile = hovered and getval(token.htile or token.tile) or getval(token.tile)
                    local tile_pen = tonumber(tile) and to_pen{tile=tile} or tile
                    dc:char(nil, tile_pen)
                    if token.width then
                        dc:advance(token.width-1)
                    end
                end
            end

            if token.text or token.key then
                local text = ''..(getval(token.text) or '')
                local keypen = to_pen(token.key_pen or COLOR_LIGHTGREEN)

                if dc then
                    local tpen = getval(token.pen)
                    local dcpen = to_pen(tpen or pen)

                    -- If disabled, figure out which dpen to use
                    if disabled or is_disabled(token) then
                        dcpen = to_pen(getval(token.dpen) or tpen or dpen)
                        if keypen.fg ~= COLOR_BLACK then
                            keypen.bold = false
                        end

                        -- if hovered *and* disabled, combine both effects
                        if hovered then
                            dcpen = make_hpen(dcpen)
                        end
                    elseif hovered then
                        dcpen = make_hpen(dcpen, getval(token.hpen) or hpen)
                    end

                    dc:pen(dcpen)
                end
                local width = getval(token.width)
                local padstr
                if width then
                    x = x + width
                    if #text > width then
                        text = string.sub(text,1,width)
                    else
                        if token.pad_char then
                            padstr = string.rep(token.pad_char,width-#text)
                        end
                        if dc and token.rjustify then
                            if padstr then dc:string(padstr) else dc:advance(width-#text) end
                        end
                    end
                else
                    x = x + #text
                end

                if token.key then
                    if type(token.key) == 'string' and not df.interface_key[token.key] then
                        error('Invalid interface_key: ' .. token.key)
                    end
                    local keystr = gui.getKeyDisplay(token.key)
                    local sep = token.key_sep or ''

                    x = x + #keystr

                    if sep:startswith('()') then
                        if dc then
                            dc:string(text)
                            dc:string(' ('):string(keystr,keypen)
                            dc:string(sep:sub(2))
                        end
                        x = x + 1 + #sep
                    else
                        if dc then
                            dc:string(keystr,keypen):string(sep):string(text)
                        end
                        x = x + #sep
                    end
                else
                    if dc then
                        dc:string(text)
                    end
                end

                if width and dc and not token.rjustify then
                    if padstr then dc:string(padstr) else dc:advance(width-#text) end
                end
            end

            token.x2 = x
        end
        width = math.max(width, x)
    end
    obj.text_width = width
end

function check_text_keys(self, keys)
    if self.text_active then
        for _,item in ipairs(self.text_active) do
            if item.key and keys[item.key] and not is_disabled(item) then
                item.on_activate()
                return true
            end
        end
    end
end

---@class widgets.LabelToken
---@field text string|fun(): string
---@field gap? integer
---@field tile? integer|dfhack.pen
---@field htile? integer|dfhack.pen
---@field width? integer|fun(): integer
---@field pad_char? string
---@field key? string
---@field key_sep? string
---@field disabled? boolean|fun(): boolean
---@field enabled? boolean|fun(): boolean
---@field pen? dfhack.color|dfhack.pen
---@field dpen? dfhack.color|dfhack.pen
---@field hpen? dfhack.color|dfhack.pen
---@field on_activiate? fun()
---@field id? string
---@field line? any Internal use only
---@field x1? any Internal use only
---@field x2? any Internal use only

---@class widgets.Label.attrs: widgets.Widget.attrs
---@field text? string|widgets.LabelToken[]
---@field text_pen dfhack.color|dfhack.pen
---@field text_dpen dfhack.color|dfhack.pen
---@field text_hpen? dfhack.color|dfhack.pen
---@field disabled? boolean|fun(): boolean
---@field enabled? boolean|fun(): boolean
---@field auto_height boolean
---@field auto_width boolean
---@field on_click? function
---@field on_rclick? function
---@field scroll_keys table<string, string|integer>

---@class widgets.Label.attrs.partial: widgets.Label.attrs

---@class widgets.Label: widgets.Widget, widgets.Label.attrs
---@field super widgets.Widget
---@field ATTRS widgets.Label.attrs|fun(attributes: widgets.Label.attrs.partial)
---@overload fun(init_table: widgets.Label.attrs.partial): self
Label = defclass(Label, Widget)

Label.ATTRS{
    text_pen = COLOR_WHITE,
    text_dpen = COLOR_DARKGREY, -- disabled
    text_hpen = DEFAULT_NIL, -- hover - default is to invert the fg/bg colors
    disabled = DEFAULT_NIL,
    enabled = DEFAULT_NIL,
    auto_height = true,
    auto_width = false,
    on_click = DEFAULT_NIL,
    on_rclick = DEFAULT_NIL,
    scroll_keys = STANDARDSCROLL,
}

---@param args widgets.Label.attrs.partial
function Label:init(args)
    self.scrollbar = Scrollbar{
        frame={r=0},
        on_scroll=self:callback('on_scrollbar')}

    self:addviews{self.scrollbar}

    self:setText(args.text or self.text)
end

local function update_label_scrollbar(label)
    local body_height = label.frame_body and label.frame_body.height or 1
    label.scrollbar:update(label.start_line_num, body_height,
                           label:getTextHeight())
end

function Label:setText(text)
    self.start_line_num = 1
    self.text = text
    parse_label_text(self)

    if self.auto_height then
        self.frame = self.frame or {}
        self.frame.h = self:getTextHeight()
    end

    update_label_scrollbar(self)
end

function Label:preUpdateLayout()
    if self.auto_width then
        self.frame = self.frame or {}
        self.frame.w = self:getTextWidth()
    end
end

function Label:postUpdateLayout()
    update_label_scrollbar(self)
end

function Label:itemById(id)
    if self.text_ids then
        return self.text_ids[id]
    end
end

function Label:getTextHeight()
    return #self.text_lines
end

function Label:getTextWidth()
    render_text(self)
    return self.text_width
end

-- Overridden by subclasses that also want to add new mouse handlers, see
-- HotkeyLabel.
function Label:shouldHover()
    return self.on_click or self.on_rclick
end

function Label:onRenderBody(dc)
    local text_pen = self.text_pen
    local hovered = self:getMousePos() and self:shouldHover()
    render_text(self,dc,0,0,text_pen,self.text_dpen,is_disabled(self), self.text_hpen, hovered)
end

function Label:on_scrollbar(scroll_spec)
    local v = 0
    if tonumber(scroll_spec) then
        v = scroll_spec - self.start_line_num
    elseif scroll_spec == 'down_large' then
        v = '+halfpage'
    elseif scroll_spec == 'up_large' then
        v = '-halfpage'
    elseif scroll_spec == 'down_small' then
        v = 1
    elseif scroll_spec == 'up_small' then
        v = -1
    end

    self:scroll(v)
end

function Label:scroll(nlines)
    if not nlines then return end
    local text_height = math.max(1, self:getTextHeight())
    if type(nlines) == 'string' then
        if nlines == '+page' then
            nlines = self.frame_body.height
        elseif nlines == '-page' then
            nlines = -self.frame_body.height
        elseif nlines == '+halfpage' then
            nlines = math.ceil(self.frame_body.height/2)
        elseif nlines == '-halfpage' then
            nlines = -math.ceil(self.frame_body.height/2)
        elseif nlines == 'home' then
            nlines = 1 - self.start_line_num
        elseif nlines == 'end' then
            nlines = text_height
        else
            error(('unhandled scroll keyword: "%s"'):format(nlines))
        end
    end
    local n = self.start_line_num + nlines
    n = math.min(n, text_height - self.frame_body.height + 1)
    n = math.max(n, 1)
    nlines = n - self.start_line_num
    self.start_line_num = n
    update_label_scrollbar(self)
    return nlines
end

function Label:onInput(keys)
    if is_disabled(self) then return false end
    if self:inputToSubviews(keys) then
        return true
    end
    if keys._MOUSE_L and self:getMousePos() and self.on_click then
        self.on_click()
        return true
    end
    if keys._MOUSE_R and self:getMousePos() and self.on_rclick then
        self.on_rclick()
        return true
    end
    for k,v in pairs(self.scroll_keys) do
        if keys[k] and 0 ~= self:scroll(v) then
            return true
        end
    end
    return check_text_keys(self, keys)
end

--------------------------------
-- makeButtonLabelText
--

local function get_button_token_hover_ch(spec, x, y, ch)
    local ch_hover = ch
    if spec.chars_hover then
        local row = spec.chars_hover[y]
        if type(row) == 'string' then
            ch_hover = row:sub(x, x)
        else
            ch_hover = row[x]
        end
    end
    return ch_hover
end

local function get_button_token_base_pens(spec, x, y)
    local pen, pen_hover = COLOR_GRAY, COLOR_WHITE
    if spec.pens then
        pen = type(spec.pens) == 'table' and (safe_index(spec.pens, y, x) or spec.pens[y]) or spec.pens
        if spec.pens_hover then
            pen_hover = type(spec.pens_hover) == 'table' and (safe_index(spec.pens_hover, y, x) or spec.pens_hover[y]) or spec.pens_hover
        else
            pen_hover = pen
        end
    end
    return pen, pen_hover
end

local function get_button_tileset_idx(spec, x, y, tileset_offset, tileset_stride)
    local idx = (tileset_offset or 1)
    idx = idx + (x - 1)
    idx = idx + (y - 1) * (tileset_stride or #spec.chars[1])
    return idx
end

local function get_asset_tile(asset, x, y)
    return dfhack.screen.findGraphicsTile(asset.page, asset.x+x-1, asset.y+y-1)
end

local function get_button_token_tiles(spec, x, y)
    local tile = safe_index(spec.tiles_override, y, x)
    local tile_hover = safe_index(spec.tiles_hover_override, y, x) or tile
    if not tile and spec.tileset then
        local tileset = spec.tileset
        local idx = get_button_tileset_idx(spec, x, y, spec.tileset_offset, spec.tileset_stride)
        tile = dfhack.textures.getTexposByHandle(tileset[idx])
        if spec.tileset_hover then
            local tileset_hover = spec.tileset_hover
            local idx_hover = get_button_tileset_idx(spec, x, y,
                spec.tileset_hover_offset or spec.tileset_offset,
                spec.tileset_hover_stride or spec.tileset_stride)
            tile_hover = dfhack.textures.getTexposByHandle(tileset_hover[idx_hover])
        else
            tile_hover = tile
        end
    end
    if not tile and spec.asset then
        tile = get_asset_tile(spec.asset, x, y)
        if spec.asset_hover then
            tile_hover = get_asset_tile(spec.asset_hover, x, y)
        else
            tile_hover = tile
        end
    end
    return tile, tile_hover
end

local function get_button_token_pen(base_pen, tile, ch)
    local pen = dfhack.pen.make(base_pen)
    pen.tile = tile
    pen.ch = ch
    return pen
end

local function get_button_token_pens(spec, x, y, ch, ch_hover)
    local base_pen, base_pen_hover = get_button_token_base_pens(spec, x, y)
    local tile, tile_hover = get_button_token_tiles(spec, x, y)
    return get_button_token_pen(base_pen, tile, ch), get_button_token_pen(base_pen_hover, tile_hover, ch_hover)
end

local function make_button_token(spec, x, y, ch)
    local ch_hover = get_button_token_hover_ch(spec, x, y, ch)
    local pen, pen_hover = get_button_token_pens(spec, x, y, ch, ch_hover)
    return {
        tile=pen,
        htile=pen_hover,
        width=1,
    }
end

---@class widgets.ButtonLabelSpec
---@field chars (string|string[])[]
---@field chars_hover? (string|string[])[]
---@field pens? dfhack.color|dfhack.color[][]
---@field pens_hover? dfhack.color|dfhack.color[][]
---@field tiles_override? integer[][]
---@field tiles_hover_override? integer[][]
---@field tileset? TexposHandle[]
---@field tileset_hover? TexposHandle[]
---@field tileset_offset? integer
---@field tileset_hover_offset? integer
---@field tileset_stride? integer
---@field tileset_hover_stride? integer
---@field asset? {page: string, x: integer, y: integer}
---@field asset_hover? {page: string, x: integer, y: integer}

---@nodiscard
---@param spec widgets.ButtonLabelSpec
---@return widgets.LabelToken[]
function makeButtonLabelText(spec)
    local tokens = {}
    for y, row in ipairs(spec.chars) do
        if type(row) == 'string' then
            local x = 1
            for ch in row:gmatch('.') do
                table.insert(tokens, make_button_token(spec, x, y, ch))
                x = x + 1
            end
        else
            for x, ch in ipairs(row) do
                table.insert(tokens, make_button_token(spec, x, y, ch))
            end
        end
        if y < #spec.chars then
            table.insert(tokens, NEWLINE)
        end
    end
    return tokens
end

------------------
-- WrappedLabel --
------------------

---@class widgets.WrappedLabel.attrs: widgets.Label.attrs
---@field text_to_wrap? string|string[]|fun(): string|string[]
---@field indent integer

---@class widgets.WrappedLabel.attrs.partial: widgets.WrappedLabel.attrs

---@class widgets.WrappedLabel: widgets.Label, widgets.WrappedLabel.attrs
---@field super widgets.Label
---@field ATTRS widgets.WrappedLabel.attrs|fun(attributes: widgets.WrappedLabel.attrs.partial)
---@overload fun(init_table: widgets.WrappedLabel.attrs.partial): self
WrappedLabel = defclass(WrappedLabel, Label)

WrappedLabel.ATTRS{
    text_to_wrap=DEFAULT_NIL,
    indent=0,
}

function WrappedLabel:getWrappedText(width)
    -- 0 width can happen if the parent has 0 width
    if not self.text_to_wrap or width <= 0 then return nil end
    local text_to_wrap = getval(self.text_to_wrap)
    if type(text_to_wrap) == 'table' then
        text_to_wrap = table.concat(text_to_wrap, NEWLINE)
    end
    return text_to_wrap:wrap(width - self.indent, {return_as_table=true})
end

function WrappedLabel:preUpdateLayout()
    self.saved_start_line_num = self.start_line_num
end

-- we can't set the text in init() since we may not yet have a frame that we
-- can get wrapping bounds from.
function WrappedLabel:postComputeFrame()
    local wrapped_text = self:getWrappedText(self.frame_body.width-3)
    if not wrapped_text then return end
    local text = {}
    for _,line in ipairs(wrapped_text) do
        table.insert(text, {gap=self.indent, text=line})
        -- a trailing newline will get ignored so we don't have to manually trim
        table.insert(text, NEWLINE)
    end
    self:setText(text)
    self:scroll(self.saved_start_line_num - 1)
end

------------------
-- TooltipLabel --
------------------

---@class widgets.TooltipLabel.attrs: widgets.WrappedLabel.attrs
---@field show_tooltip? boolean|fun(): boolean

---@class widgets.TooltipLabel.attrs.partial: widgets.TooltipLabel.attrs

---@class widgets.TooltipLabel: widgets.WrappedLabel, widgets.TooltipLabel.attrs
---@field super widgets.WrappedLabel
---@field ATTRS widgets.TooltipLabel.attrs|fun(attributes: widgets.TooltipLabel.attrs.partial)
---@overload fun(init_table: widgets.TooltipLabel.attrs.partial): self
TooltipLabel = defclass(TooltipLabel, WrappedLabel)

TooltipLabel.ATTRS{
    show_tooltip=DEFAULT_NIL,
    indent=2,
    text_pen=COLOR_GREY,
}

function TooltipLabel:init()
    self.visible = self.show_tooltip
end

-----------------
-- HotkeyLabel --
-----------------

---@class widgets.HotkeyLabel.attrs: widgets.Label.attrs
---@field key? string
---@field key_sep string
---@field label? string|fun(): string
---@field on_activate? function

---@class widgets.HotkeyLabel.attrs.partial: widgets.HotkeyLabel.attrs

---@class widgets.HotkeyLabel: widgets.Label, widgets.HotkeyLabel.attrs
---@field super widgets.Label
---@field ATTRS widgets.HotkeyLabel.attrs|fun(attributes: widgets.HotkeyLabel.attrs.partial)
---@overload fun(init_table: widgets.HotkeyLabel.attrs.partial): self
HotkeyLabel = defclass(HotkeyLabel, Label)

HotkeyLabel.ATTRS{
    key=DEFAULT_NIL,
    key_sep=': ',
    label=DEFAULT_NIL,
    on_activate=DEFAULT_NIL,
}

function HotkeyLabel:init()
    self:initializeLabel()
end

function HotkeyLabel:setOnActivate(on_activate)
    self.on_activate = on_activate
    self:initializeLabel()
end

function HotkeyLabel:setLabel(label)
    self.label = label
    self:initializeLabel()
end

function HotkeyLabel:shouldHover()
    -- When on_activate is set, text should also hover on mouseover
    return self.on_activate or HotkeyLabel.super.shouldHover(self)
end

function HotkeyLabel:initializeLabel()
    self:setText{{key=self.key, key_sep=self.key_sep, text=self.label,
                  on_activate=self.on_activate}}
end

function HotkeyLabel:onInput(keys)
    if HotkeyLabel.super.onInput(self, keys) then
        return true
    elseif keys._MOUSE_L and self:getMousePos() and self.on_activate
            and not is_disabled(self) then
        self.on_activate()
        return true
    end
end

----------------
-- HelpButton --
----------------

---@class widgets.HelpButton.attrs: widgets.Panel.attrs
---@field command? string

---@class widgets.HelpButton.attrs.partial: widgets.HelpButton.attrs

---@class widgets.HelpButton: widgets.Panel, widgets.HelpButton.attrs
---@field super widgets.Panel
---@field ATTRS widgets.HelpButton.attrs|fun(attributes: widgets.HelpButton.attrs.partial)
---@overload fun(init_table: widgets.HelpButton.attrs.partial): self
HelpButton = defclass(HelpButton, Panel)

HelpButton.ATTRS{
    frame={t=0, r=1, w=3, h=1},
    command=DEFAULT_NIL,
}

local button_pen_left = to_pen{fg=COLOR_CYAN,
    tile=curry(textures.tp_control_panel, 7) or nil, ch=string.byte('[')}
local button_pen_right = to_pen{fg=COLOR_CYAN,
    tile=curry(textures.tp_control_panel, 8) or nil, ch=string.byte(']')}
local help_pen_center = to_pen{
    tile=curry(textures.tp_control_panel, 9) or nil, ch=string.byte('?')}
local configure_pen_center = dfhack.pen.parse{
    tile=curry(textures.tp_control_panel, 10) or nil, ch=15} -- gear/masterwork symbol

function HelpButton:init()
    self.frame.w = self.frame.w or 3
    self.frame.h = self.frame.h or 1

    local command = self.command .. ' '

    self:addviews{
        Label{
            frame={t=0, l=0, w=3, h=1},
            text={
                {tile=button_pen_left},
                {tile=help_pen_center},
                {tile=button_pen_right},
            },
            on_click=function() dfhack.run_command('gui/launcher', command) end,
        },
    }
end

---------------------
-- ConfigureButton --
---------------------

---@class widgets.ConfigureButton.attrs: widgets.Panel.attrs
---@field on_click? function

---@class widgets.ConfigureButton.attrs.partial: widgets.ConfigureButton.attrs

---@class widgets.ConfigureButton: widgets.Panel, widgets.ConfigureButton.attrs
---@field super widgets.Panel
---@field ATTRS widgets.ConfigureButton.attrs|fun(attributes: widgets.ConfigureButton.attrs.partial)
---@overload fun(init_table: widgets.ConfigureButton.attrs.partial): self
ConfigureButton = defclass(ConfigureButton, Panel)

ConfigureButton.ATTRS{
    on_click=DEFAULT_NIL,
}

function ConfigureButton:preinit(init_table)
    init_table.frame = init_table.frame or {}
    init_table.frame.h = init_table.frame.h or 1
    init_table.frame.w = init_table.frame.w or 3
end

function ConfigureButton:init()
    self:addviews{
        Label{
            frame={t=0, l=0, w=3, h=1},
            text={
                {tile=button_pen_left},
                {tile=configure_pen_center},
                {tile=button_pen_right},
            },
            on_click=self.on_click,
        },
    }
end

-----------------
-- BannerPanel --
-----------------

---@class widgets.BannerPanel: widgets.Panel
---@field super widgets.Panel
BannerPanel = defclass(BannerPanel, Panel)

---@param dc gui.Painter
function BannerPanel:onRenderBody(dc)
    dc:pen(COLOR_RED)
    for y=0,self.frame_rect.height-1 do
        dc:seek(0, y):char('[')
        dc:seek(self.frame_rect.width-1):char(']')
    end
end

----------------
-- TextButton --
----------------

---@class widgets.TextButton.initTable: widgets.Panel.attrs.partial, widgets.HotkeyLabel.attrs.partial

---@class widgets.TextButton: widgets.BannerPanel
---@field super widgets.BannerPanel
---@overload fun(init_table: widgets.TextButton.initTable): self
TextButton = defclass(TextButton, BannerPanel)

---@param info widgets.TextButton.initTable
function TextButton:init(info)
    self.label = HotkeyLabel{
        frame={t=0, l=1, r=1},
        key=info.key,
        key_sep=info.key_sep,
        label=info.label,
        on_activate=info.on_activate,
        text_pen=info.text_pen,
        text_dpen=info.text_dpen,
        text_hpen=info.text_hpen,
        disabled=info.disabled,
        enabled=info.enabled,
        auto_height=info.auto_height,
        auto_width=info.auto_width,
        on_click=info.on_click,
        on_rclick=info.on_rclick,
        scroll_keys=info.scroll_keys,
    }

    self:addviews{self.label}
end

function TextButton:setLabel(label)
    self.label:setLabel(label)
end

----------------------
-- CycleHotkeyLabel --
----------------------

---@class widgets.CycleHotkeyLabel.attrs: widgets.Label.attrs
---@field key? string
---@field key_back? string
---@field key_sep string
---@field label? string|fun(): string
---@field label_width? integer
---@field label_below boolean
---@field option_gap integer
---@field options? { label: string, value: string, pen: dfhack.pen|nil }[]
---@field initial_option integer|string
---@field on_change? fun(new_option_value: integer|string, old_option_value: integer|string)

---@class widgets.CycleHotkeyLabel.attrs.partial: widgets.CycleHotkeyLabel.attrs

---@class widgets.CycleHotkeyLabel: widgets.Label, widgets.CycleHotkeyLabel.attrs
---@field super widgets.Label
---@field ATTRS widgets.CycleHotkeyLabel.attrs|fun(attributes: widgets.CycleHotkeyLabel.attrs.partial)
---@overload fun(init_table: widgets.CycleHotkeyLabel.attrs.partial)
CycleHotkeyLabel = defclass(CycleHotkeyLabel, Label)

CycleHotkeyLabel.ATTRS{
    key=DEFAULT_NIL,
    key_back=DEFAULT_NIL,
    key_sep=': ',
    option_gap=1,
    label=DEFAULT_NIL,
    label_width=DEFAULT_NIL,
    label_below=false,
    options=DEFAULT_NIL,
    initial_option=1,
    on_change=DEFAULT_NIL,
}

function CycleHotkeyLabel:init()
    self:setOption(self.initial_option)

    if self.label_below then
        self.option_gap = self.option_gap + (self.key_back and 1 or 0) + (self.key and 2 or 0)
    end

    self:setText{
        self.key_back ~= nil and {key=self.key_back, key_sep='', width=0, on_activate=self:callback('cycle', true)} or {},
        {key=self.key, key_sep=self.key_sep, text=self.label, width=self.label_width,
         on_activate=self:callback('cycle')},
        self.label_below and NEWLINE or '',
        {gap=self.option_gap, text=self:callback('getOptionLabel'),
         pen=self:callback('getOptionPen')},
    }
end

-- CycleHotkeyLabels are always clickable and therefore should always change
-- color when hovered.
function CycleHotkeyLabel:shouldHover()
    return true
end

function CycleHotkeyLabel:cycle(backwards)
    local old_option_idx = self.option_idx
    if self.option_idx == #self.options and not backwards then
        self.option_idx = 1
    elseif self.option_idx == 1 and backwards then
        self.option_idx = #self.options
    else
        self.option_idx = self.option_idx + (not backwards and 1 or -1)
    end
    if self.on_change then
        self.on_change(self:getOptionValue(),
                       self:getOptionValue(old_option_idx))
    end
end

function CycleHotkeyLabel:setOption(value_or_index, call_on_change)
    local option_idx = nil
    for i in ipairs(self.options) do
        if value_or_index == self:getOptionValue(i) then
            option_idx = i
            break
        end
    end
    if not option_idx then
        if self.options[value_or_index] then
            option_idx = value_or_index
        end
    end
    if not option_idx then
        option_idx = 1
    end
    local old_option_idx = self.option_idx
    self.option_idx = option_idx
    if call_on_change and self.on_change then
        self.on_change(self:getOptionValue(),
                       self:getOptionValue(old_option_idx))
    end
end

local function cyclehotkeylabel_getOptionElem(self, option_idx, key, require_key)
    option_idx = option_idx or self.option_idx
    local option = self.options[option_idx]
    if type(option) == 'table' then
        return option[key]
    end
    return not require_key and option or nil
end

function CycleHotkeyLabel:getOptionLabel(option_idx)
    return getval(cyclehotkeylabel_getOptionElem(self, option_idx, 'label'))
end

function CycleHotkeyLabel:getOptionValue(option_idx)
    return cyclehotkeylabel_getOptionElem(self, option_idx, 'value')
end

function CycleHotkeyLabel:getOptionPen(option_idx)
    local pen = getval(cyclehotkeylabel_getOptionElem(self, option_idx, 'pen', true))
    if type(pen) == 'string' then return nil end
    return pen
end

function CycleHotkeyLabel:onInput(keys)
    if CycleHotkeyLabel.super.onInput(self, keys) then
        return true
    elseif keys._MOUSE_L and not is_disabled(self) then
        local x = self:getMousePos()
        if x then
            self:cycle(self.key_back and x == 0)
            return true
        end
    end
end

-----------------
-- ButtonGroup --
-----------------

---@class widgets.ButtonGroup.attrs: widgets.CycleHotkeyLabel.attrs

---@class widgets.ButtonGroup.initTable: widgets.ButtonGroup.attrs
---@field button_specs widgets.ButtonLabelSpec[]
---@field button_specs_selected widgets.ButtonLabelSpec[]

---@class widgets.ButtonGroup: widgets.ButtonGroup.attrs, widgets.CycleHotkeyLabel
---@field super widgets.CycleHotkeyLabel
---@overload fun(init_table: widgets.ButtonGroup.initTable): self
ButtonGroup = defclass(ButtonGroup, CycleHotkeyLabel)

ButtonGroup.ATTRS{
    auto_height=false,
}

function ButtonGroup:init(info)
    ensure_key(self, 'frame').h = #info.button_specs[1].chars + 2

    local start = 0
    for idx in ipairs(self.options) do
        local opt_val = self:getOptionValue(idx)
        local width = #info.button_specs[idx].chars[1]
        self:addviews{
            Label{
                frame={t=2, l=start, w=width},
                text=makeButtonLabelText(info.button_specs[idx]),
                on_click=function() self:setOption(opt_val, true) end,
                visible=function() return self:getOptionValue() ~= opt_val end,
            },
            Label{
                frame={t=2, l=start, w=width},
                text=makeButtonLabelText(info.button_specs_selected[idx]),
                on_click=function() self:setOption(opt_val, false) end,
                visible=function() return self:getOptionValue() == opt_val end,
            },
        }
        start = start + width
    end
end

-----------------------
-- ToggleHotkeyLabel --
-----------------------

---@class widgets.ToggleHotkeyLabel: widgets.CycleHotkeyLabel
---@field super widgets.CycleHotkeyLabel
ToggleHotkeyLabel = defclass(ToggleHotkeyLabel, CycleHotkeyLabel)
ToggleHotkeyLabel.ATTRS{
    options={{label='On', value=true, pen=COLOR_GREEN},
             {label='Off', value=false}},
}

----------
-- List --
----------

---@class widgets.ListChoice
---@field text string|widgets.LabelToken[]
---@field key string
---@field search_key? string
---@field icon? string|dfhack.pen|fun(): string|dfhack.pen
---@field icon_pen? dfhack.pen

---@class widgets.List.attrs: widgets.Widget.attrs
---@field choices widgets.ListChoice[]
---@field selected integer
---@field text_pen dfhack.color|dfhack.pen
---@field text_hpen? dfhack.color|dfhack.pen
---@field cursor_pen dfhack.color|dfhack.pen
---@field inactive_pen? dfhack.color|dfhack.pen
---@field icon_pen? dfhack.color|dfhack.pen
---@field on_select? function
---@field on_submit? function
---@field on_submit2? function
---@field on_double_click? function
---@field on_double_click2? function
---@field row_height integer
---@field scroll_keys table<string, string|integer>
---@field icon_width? integer
---@field page_top integer
---@field page_size integer
---@field scrollbar widgets.Scrollbar
---@field last_select_click_ms integer

---@class widgets.List.attrs.partial: widgets.List.attrs

---@class widgets.List: widgets.Widget, widgets.List.attrs
---@field super widgets.Widget
---@field ATTRS widgets.List.attrs|fun(attributes: widgets.List.attrs.partial)
---@overload fun(init_table: widgets.List.attrs.partial): self
List = defclass(List, Widget)

List.ATTRS{
    text_pen = COLOR_CYAN,
    text_hpen = DEFAULT_NIL, -- hover color, defaults to inverting the FG/BG pens for each text object
    cursor_pen = COLOR_LIGHTCYAN,
    inactive_pen = DEFAULT_NIL,
    icon_pen = DEFAULT_NIL,
    on_select = DEFAULT_NIL,
    on_submit = DEFAULT_NIL,
    on_submit2 = DEFAULT_NIL,
    on_double_click = DEFAULT_NIL,
    on_double_click2 = DEFAULT_NIL,
    row_height = 1,
    scroll_keys = STANDARDSCROLL,
    icon_width = DEFAULT_NIL,
}

---@param self widgets.List
---@param info widgets.List.attrs.partial
function List:init(info)
    self.page_top = 1
    self.page_size = 1
    self.scrollbar = Scrollbar{
        frame={r=0},
        on_scroll=self:callback('on_scrollbar')}

    self:addviews{self.scrollbar}

    if info.choices then
        self:setChoices(info.choices, info.selected)
    else
        self.choices = {}
        self.selected = 1
    end

    self.last_select_click_ms = 0 -- used to track double-clicking on an item
end

function List:setChoices(choices, selected)
    self.choices = {}

    for i,v in ipairs(choices or {}) do
        local l = utils.clone(v);
        if type(v) ~= 'table' then
            l = { text = v }
        else
            l.text = v.text or v.caption or v[1]
        end
        parse_label_text(l)
        self.choices[i] = l
    end

    self:setSelected(selected)

    -- Check if page_top needs to be adjusted
    if #self.choices - self.page_size < 0 then
        self.page_top = 1
    elseif self.page_top > #self.choices - self.page_size + 1 then
        self.page_top = #self.choices - self.page_size + 1
    end
end

function List:setSelected(selected)
    self.selected = selected or self.selected or 1
    self:moveCursor(0, true)
    return self.selected
end

function List:getChoices()
    return self.choices
end

function List:getSelected()
    if #self.choices > 0 then
        return self.selected, self.choices[self.selected]
    end
end

function List:getContentWidth()
    local width = 0
    for i,v in ipairs(self.choices) do
        render_text(v)
        local roww = v.text_width
        if v.key then
            roww = roww + 3 + #gui.getKeyDisplay(v.key)
        end
        width = math.max(width, roww)
    end
    return width + (self.icon_width or 0)
end

function List:getContentHeight()
    return #self.choices * self.row_height
end

local function update_list_scrollbar(list)
    list.scrollbar:update(list.page_top, list.page_size, #list.choices)
end

function List:postComputeFrame(body)
    local row_count = body.height // self.row_height
    self.page_size = math.max(1, row_count)

    local num_choices = #self.choices
    if num_choices == 0 then
        self.page_top = 1
        update_list_scrollbar(self)
        return
    end

    if self.page_top > num_choices - self.page_size + 1 then
        self.page_top = math.max(1, num_choices - self.page_size + 1)
    end

    update_list_scrollbar(self)
end

function List:postUpdateLayout()
    update_list_scrollbar(self)
end

function List:moveCursor(delta, force_cb)
    local cnt = #self.choices

    if cnt < 1 then
        self.page_top = 1
        self.selected = 1
        update_list_scrollbar(self)
        if force_cb and self.on_select then
            self.on_select(nil,nil)
        end
        return
    end

    local off = self.selected+delta-1
    local ds = math.abs(delta)

    if ds > 1 then
        if off >= cnt+ds-1 then
            off = 0
        else
            off = math.min(cnt-1, off)
        end
        if off <= -ds then
            off = cnt-1
        else
            off = math.max(0, off)
        end
    end

    local buffer = 1 + math.min(4, math.floor(self.page_size/10))

    self.selected = 1 + off % cnt
    if (self.selected - buffer) < self.page_top then
        self.page_top = math.max(1, self.selected - buffer)
    elseif (self.selected + buffer + 1) > (self.page_top + self.page_size) then
        local max_page_top = cnt - self.page_size + 1
        self.page_top = math.max(1,
            math.min(max_page_top, self.selected - self.page_size + buffer + 1))
    end
    update_list_scrollbar(self)

    if (force_cb or delta ~= 0) and self.on_select then
        self.on_select(self:getSelected())
    end
end

function List:on_scrollbar(scroll_spec)
    local v = 0
    if tonumber(scroll_spec) then
        v = scroll_spec - self.page_top
    elseif scroll_spec == 'down_large' then
        v = math.ceil(self.page_size / 2)
    elseif scroll_spec == 'up_large' then
        v = -math.ceil(self.page_size / 2)
    elseif scroll_spec == 'down_small' then
        v = 1
    elseif scroll_spec == 'up_small' then
        v = -1
    end

    local max_page_top = math.max(1, #self.choices - self.page_size + 1)
    self.page_top = math.max(1, math.min(max_page_top, self.page_top + v))
    update_list_scrollbar(self)
end

function List:onRenderBody(dc)
    local choices = self.choices
    local top = self.page_top
    local iend = math.min(#choices, top+self.page_size-1)
    local iw = self.icon_width

    local function paint_icon(icon, obj)
        if type(icon) ~= 'string' then
            dc:char(nil,icon)
        else
            if current then
                dc:string(icon, obj.icon_pen or self.icon_pen or cur_pen)
            else
                dc:string(icon, obj.icon_pen or self.icon_pen or cur_dpen)
            end
        end
    end

    local hoveridx = self:getIdxUnderMouse()
    for i = top,iend do
        local obj = choices[i]
        local current = (i == self.selected)
        local hovered = (i == hoveridx)
        -- cur_pen and cur_dpen can't be integers or background colors get
        -- messed up in render_text for subsequent renders
        local cur_pen = to_pen(self.cursor_pen)
        local cur_dpen = to_pen(self.text_pen)
        local active_pen = (current and cur_pen or cur_dpen)

        if not getval(self.active) then
            cur_pen = self.inactive_pen or self.cursor_pen
        end

        local y = (i - top)*self.row_height
        local icon = getval(obj.icon)

        if iw and icon then
            dc:seek(0, y):pen(active_pen)
            paint_icon(icon, obj)
        end

        render_text(obj, dc, iw or 0, y, cur_pen, cur_dpen, not current, self.text_hpen, hovered)

        local ip = dc.width

        if obj.key then
            local keystr = gui.getKeyDisplay(obj.key)
            ip = ip-3-#keystr
            dc:seek(ip,y):pen(self.text_pen)
            dc:string('('):string(keystr,COLOR_LIGHTGREEN):string(')')
        end

        if icon and not iw then
            dc:seek(ip-1,y):pen(active_pen)
            paint_icon(icon, obj)
        end
    end
end

function List:getIdxUnderMouse()
    if self.scrollbar:getMousePos() then return end
    local _,mouse_y = self:getMousePos()
    if mouse_y and #self.choices > 0 and
            mouse_y < (#self.choices-self.page_top+1) * self.row_height then
        return self.page_top + math.floor(mouse_y/self.row_height)
    end
end

function List:submit()
    if self.on_submit and #self.choices > 0 then
        self.on_submit(self:getSelected())
        return true
    end
end

function List:submit2()
    if self.on_submit2 and #self.choices > 0 then
        self.on_submit2(self:getSelected())
        return true
    end
end

function List:double_click()
    if #self.choices == 0 then return end
    local cb = dfhack.internal.getModifiers().shift and
            self.on_double_click2 or self.on_double_click
    if cb then
        cb(self:getSelected())
        return true
    end
end

function List:onInput(keys)
    if self:inputToSubviews(keys) then
        return true
    end
    if keys.SELECT then
        return self:submit()
    elseif keys.SELECT_ALL then
        return self:submit2()
    elseif keys._MOUSE_L then
        local idx = self:getIdxUnderMouse()
        if idx then
            local now_ms = dfhack.getTickCount()
            if idx ~= self:getSelected() then
                self.last_select_click_ms = now_ms
            else
                if now_ms - self.last_select_click_ms <= DOUBLE_CLICK_MS then
                    self.last_select_click_ms = 0
                    if self:double_click() then return true end
                else
                    self.last_select_click_ms = now_ms
                end
            end

            self:setSelected(idx)
            if dfhack.internal.getModifiers().shift then
                self:submit2()
            else
                self:submit()
            end
            return true
        end
    else
        for k,v in pairs(self.scroll_keys) do
            if keys[k] then
                if v == '+page' then
                    v = self.page_size
                elseif v == '-page' then
                    v = -self.page_size
                end

                self:moveCursor(v)
                return true
            end
        end

        for i,v in ipairs(self.choices) do
            if v.key and keys[v.key] then
                self:setSelected(i)
                self:submit()
                return true
            end
        end

        local current = self.choices[self.selected]
        if current then
            return check_text_keys(current, keys)
        end
    end
end

-------------------
-- Filtered List --
-------------------


---@class widgets.FilteredList.attrs: widgets.Widget.attrs
---@field choices widgets.ListChoice[]
---@field selected? integer
---@field edit_pen dfhack.color|dfhack.pen
---@field edit_below boolean
---@field edit_key? string
---@field edit_ignore_keys? string[]
---@field edit_on_char? function
---@field edit_on_change? function
---@field list widgets.List
---@field edit widgets.EditField
---@field not_found widgets.Label

---@class widgets.FilteredList.attrs.partial: widgets.FilteredList.attrs

---@class widgets.FilteredList.initTable: widgets.FilteredList.attrs.partial, widgets.List.attrs.partial, widgets.EditField.attrs.partial
---@field not_found_label? string

---@class widgets.FilteredList: widgets.Widget, widgets.FilteredList.attrs
---@field super widgets.Widget
---@field ATTRS widgets.FilteredList.attrs|fun(attributes: widgets.FilteredList.attrs.partial)
---@overload fun(init_table: widgets.FilteredList.initTable): self
FilteredList = defclass(FilteredList, Widget)

FilteredList.ATTRS {
    edit_below = false,
    edit_key = DEFAULT_NIL,
    edit_ignore_keys = DEFAULT_NIL,
    edit_on_char = DEFAULT_NIL,
    edit_on_change = DEFAULT_NIL,
}

---@param self widgets.FilteredList
---@param info widgets.FilteredList.initTable
function FilteredList:init(info)
    local on_change = self:callback('onFilterChange')
    if self.edit_on_change then
        on_change = function(text)
            self.edit_on_change(text)
            self:onFilterChange(text)
        end
    end

    self.edit = EditField{
        text_pen = info.edit_pen or info.cursor_pen,
        frame = { l = info.icon_width, t = 0, h = 1 },
        on_change = on_change,
        on_char = self.edit_on_char,
        key = self.edit_key,
        ignore_keys = self.edit_ignore_keys,
    }
    self.list = List{
        frame = { t = 2 },
        text_pen = info.text_pen,
        cursor_pen = info.cursor_pen,
        inactive_pen = info.inactive_pen,
        icon_pen = info.icon_pen,
        row_height = info.row_height,
        scroll_keys = info.scroll_keys,
        icon_width = info.icon_width,
    }
    if self.edit_below then
        self.edit.frame = { l = info.icon_width, b = 0, h = 1 }
        self.list.frame = { t = 0, b = 2 }
    end
    if info.on_select then
        self.list.on_select = function()
            return info.on_select(self:getSelected())
        end
    end
    if info.on_submit then
        self.list.on_submit = function()
            return info.on_submit(self:getSelected())
        end
    end
    if info.on_submit2 then
        self.list.on_submit2 = function()
            return info.on_submit2(self:getSelected())
        end
    end
    if info.on_double_click then
        self.list.on_double_click = function()
            return info.on_double_click(self:getSelected())
        end
    end
    if info.on_double_click2 then
        self.list.on_double_click2 = function()
            return info.on_double_click2(self:getSelected())
        end
    end
    self.not_found = Label{
        visible = true,
        text = info.not_found_label or 'No matches',
        text_pen = COLOR_LIGHTRED,
        frame = { l = info.icon_width, t = self.list.frame.t },
    }
    self:addviews{ self.edit, self.list, self.not_found }
    if info.choices then
        self:setChoices(info.choices, info.selected)
    else
        self.choices = {}
    end
end

function FilteredList:getChoices()
    return self.choices
end

function FilteredList:getVisibleChoices()
    return self.list.choices
end

function FilteredList:setChoices(choices, pos)
    choices = choices or {}
    self.edit:setText('')
    self.list:setChoices(choices, pos)
    self.choices = self.list.choices
    self.not_found.visible = (#choices == 0)
end

function FilteredList:submit()
    return self.list:submit()
end

function FilteredList:submit2()
    return self.list:submit2()
end

function FilteredList:canSubmit()
    return not self.not_found.visible
end

function FilteredList:getSelected()
    local i,v = self.list:getSelected()
    if i then
        return map_opttab(self.choice_index, i), v
    end
end

function FilteredList:getContentWidth()
    return self.list:getContentWidth()
end

function FilteredList:getContentHeight()
    return self.list:getContentHeight() + 2
end

function FilteredList:getFilter()
    return self.edit.text, self.list.choices
end

function FilteredList:setFilter(filter, pos)
    local choices = self.choices
    local cidx = nil

    filter = filter or ''
    if filter ~= self.edit.text then
        self.edit:setText(filter)
    end

    if filter ~= '' then
        local tokens = filter:split()
        local ipos = pos

        choices = {}
        cidx = {}
        pos = nil

        for i,v in ipairs(self.choices) do
            local search_key = v.search_key
            if not search_key then
                if type(v.text) ~= 'table' then
                    search_key = v.text
                else
                    local texts = {}
                    for _,token in ipairs(v.text) do
                        table.insert(texts,
                            type(token) == 'string' and token
                                or getval(token.text) or '')
                    end
                    search_key = table.concat(texts, ' ')
                end
            end
            if utils.search_text(search_key, tokens) then
                table.insert(choices, v)
                cidx[#choices] = i
                if ipos == i then
                    pos = #choices
                end
            end
        end
    end

    self.choice_index = cidx
    self.list:setChoices(choices, pos)
    self.not_found.visible = (#choices == 0)
end

function FilteredList:onFilterChange(text)
    self:setFilter(text)
end

---@class widgets.TabPens
---@field text_mode_tab_pen dfhack.pen
---@field text_mode_label_pen dfhack.pen
---@field lt dfhack.pen
---@field lt2 dfhack.pen
---@field t dfhack.pen
---@field rt2 dfhack.pen
---@field rt dfhack.pen
---@field lb dfhack.pen
---@field lb2 dfhack.pen
---@field b dfhack.pen
---@field rb2 dfhack.pen
---@field rb dfhack.pen

local TSO = df.global.init.tabs_texpos[0] -- tab spritesheet offset
local DEFAULT_ACTIVE_TAB_PENS = {
    text_mode_tab_pen=to_pen{fg=COLOR_YELLOW},
    text_mode_label_pen=to_pen{fg=COLOR_WHITE},
    lt=to_pen{tile=TSO+5, write_to_lower=true},
    lt2=to_pen{tile=TSO+6, write_to_lower=true},
    t=to_pen{tile=TSO+7, fg=COLOR_BLACK, write_to_lower=true, top_of_text=true},
    rt2=to_pen{tile=TSO+8, write_to_lower=true},
    rt=to_pen{tile=TSO+9, write_to_lower=true},
    lb=to_pen{tile=TSO+15, write_to_lower=true},
    lb2=to_pen{tile=TSO+16, write_to_lower=true},
    b=to_pen{tile=TSO+17, fg=COLOR_BLACK, write_to_lower=true, bottom_of_text=true},
    rb2=to_pen{tile=TSO+18, write_to_lower=true},
    rb=to_pen{tile=TSO+19, write_to_lower=true},
}

local DEFAULT_INACTIVE_TAB_PENS = {
    text_mode_tab_pen=to_pen{fg=COLOR_BROWN},
    text_mode_label_pen=to_pen{fg=COLOR_DARKGREY},
    lt=to_pen{tile=TSO+0, write_to_lower=true},
    lt2=to_pen{tile=TSO+1, write_to_lower=true},
    t=to_pen{tile=TSO+2, fg=COLOR_WHITE, write_to_lower=true, top_of_text=true},
    rt2=to_pen{tile=TSO+3, write_to_lower=true},
    rt=to_pen{tile=TSO+4, write_to_lower=true},
    lb=to_pen{tile=TSO+10, write_to_lower=true},
    lb2=to_pen{tile=TSO+11, write_to_lower=true},
    b=to_pen{tile=TSO+12, fg=COLOR_WHITE, write_to_lower=true, bottom_of_text=true},
    rb2=to_pen{tile=TSO+13, write_to_lower=true},
    rb=to_pen{tile=TSO+14, write_to_lower=true},
}

---------
-- Tab --
---------

---@class widgets.Tab.attrs: widgets.Widget.attrs
---@field id? string|integer
---@field label string
---@field on_select? function
---@field get_pens? fun(): widgets.TabPens

---@class widgets.Tab.attrs.partial: widgets.Tab.attrs

---@class widgets.Tab.initTable: widgets.Tab.attrs
---@field label string

---@class widgets.Tab: widgets.Widget, widgets.Tab.attrs
---@field super widgets.Widget
---@field ATTRS widgets.Tab.attrs|fun(attributes: widgets.Tab.attrs.partial)
---@overload fun(init_table: widgets.Tab.initTable): self
Tab = defclass(Tabs, Widget)
Tab.ATTRS{
    id=DEFAULT_NIL,
    label=DEFAULT_NIL,
    on_select=DEFAULT_NIL,
    get_pens=DEFAULT_NIL,
}

function Tab:preinit(init_table)
    init_table.frame = init_table.frame or {}
    init_table.frame.w = #init_table.label + 4
    init_table.frame.h = 2
end

function Tab:onRenderBody(dc)
    local pens = self.get_pens()
    dc:seek(0, 0)
    if dfhack.screen.inGraphicsMode() then
        dc:char(nil, pens.lt):char(nil, pens.lt2)
        for i=1,#self.label do
            dc:char(self.label:sub(i,i), pens.t)
        end
        dc:char(nil, pens.rt2):char(nil, pens.rt)
        dc:seek(0, 1)
        dc:char(nil, pens.lb):char(nil, pens.lb2)
        for i=1,#self.label do
            dc:char(self.label:sub(i,i), pens.b)
        end
        dc:char(nil, pens.rb2):char(nil, pens.rb)
    else
        local tp = pens.text_mode_tab_pen
        dc:char(' ', tp):char('/', tp)
        for i=1,#self.label do
            dc:char('-', tp)
        end
        dc:char('\\', tp):char(' ', tp)
        dc:seek(0, 1)
        dc:char('/', tp):char('-', tp)
        dc:string(self.label, pens.text_mode_label_pen)
        dc:char('-', tp):char('\\', tp)
    end
end

function Tab:onInput(keys)
    if Tab.super.onInput(self, keys) then return true end
    if keys._MOUSE_L and self:getMousePos() then
        self.on_select(self.id)
        return true
    end
end

-------------
-- Tab Bar --
-------------

---@class widgets.TabBar.attrs: widgets.ResizingPanel.attrs
---@field labels string[]
---@field on_select? function
---@field get_cur_page? function
---@field active_tab_pens widgets.TabPens
---@field inactive_tab_pens widgets.TabPens
---@field get_pens? fun(index: integer, tabbar: self): widgets.TabPens
---@field key string
---@field key_back string

---@class widgets.TabBar.attrs.partial: widgets.TabBar.attrs

---@class widgets.TabBar.initTable: widgets.TabBar.attrs
---@field labels string[]

---@class widgets.TabBar: widgets.ResizingPanel, widgets.TabBar.attrs
---@field super widgets.ResizingPanel
---@field ATTRS widgets.TabBar.attrs|fun(attribute: widgets.TabBar.attrs.partial)
---@overload fun(init_table: widgets.TabBar.initTable): self
TabBar = defclass(TabBar, ResizingPanel)
TabBar.ATTRS{
    labels=DEFAULT_NIL,
    on_select=DEFAULT_NIL,
    get_cur_page=DEFAULT_NIL,
    active_tab_pens=DEFAULT_ACTIVE_TAB_PENS,
    inactive_tab_pens=DEFAULT_INACTIVE_TAB_PENS,
    get_pens=DEFAULT_NIL,
    key='CUSTOM_CTRL_T',
    key_back='CUSTOM_CTRL_Y',
}

---@param self widgets.TabBar
function TabBar:init()
    for idx,label in ipairs(self.labels) do
        self:addviews{
            Tab{
                frame={t=0, l=0},
                id=idx,
                label=label,
                on_select=self.on_select,
                get_pens=self.get_pens and function()
                    return self.get_pens(idx, self)
                end or function()
                    if self.get_cur_page() == idx then
                        return self.active_tab_pens
                    end

                    return self.inactive_tab_pens
                end,
            }
        }
    end
end

function TabBar:postComputeFrame(body)
    local t, l, width = 0, 0, body.width
    for _,tab in ipairs(self.subviews) do
        if l > 0 and l + tab.frame.w > width then
            t = t + 2
            l = 0
        end
        tab.frame.t = t
        tab.frame.l = l
        l = l + tab.frame.w
    end
end

function TabBar:onInput(keys)
    if TabBar.super.onInput(self, keys) then return true end
    if self.key and keys[self.key] then
        local zero_idx = self.get_cur_page() - 1
        local next_zero_idx = (zero_idx + 1) % #self.labels
        self.on_select(next_zero_idx + 1)
        return true
    end
    if self.key_back and keys[self.key_back] then
        local zero_idx = self.get_cur_page() - 1
        local prev_zero_idx = (zero_idx - 1) % #self.labels
        self.on_select(prev_zero_idx + 1)
        return true
    end
end

--------------------------------
-- RangeSlider
--------------------------------

---@class widgets.RangeSlider.attrs: widgets.Widget.attrs
---@field num_stops integer
---@field get_left_idx_fn? function
---@field get_right_idx_fn? function
---@field on_left_change? fun(index: integer)
---@field on_right_change? fun(index: integer)

---@class widgets.RangeSlider.attrs.partial: widgets.RangeSlider.attrs

---@class widgets.RangeSlider.initTable: widgets.RangeSlider.attrs
---@field num_stops integer

---@class widgets.RangeSlider: widgets.Widget, widgets.RangeSlider.attrs
---@field super widgets.Widget
---@field ATTRS widgets.RangeSlider.attrs|fun(attributes: widgets.RangeSlider.attrs.partial)
---@overload fun(init_table: widgets.RangeSlider.initTable): self
RangeSlider = defclass(RangeSlider, Widget)
RangeSlider.ATTRS{
    num_stops=DEFAULT_NIL,
    get_left_idx_fn=DEFAULT_NIL,
    get_right_idx_fn=DEFAULT_NIL,
    on_left_change=DEFAULT_NIL,
    on_right_change=DEFAULT_NIL,
}

function RangeSlider:preinit(init_table)
    init_table.frame = init_table.frame or {}
    init_table.frame.h = init_table.frame.h or 1
end

function RangeSlider:init()
    if self.num_stops < 2 then error('too few RangeSlider stops') end
    self.is_dragging_target = nil -- 'left', 'right', or 'both'
    self.is_dragging_idx = nil -- offset from leftmost dragged tile
end

local function rangeslider_get_width_per_idx(self)
    return math.max(5, (self.frame_body.width-7) // (self.num_stops-1))
end

function RangeSlider:onInput(keys)
    if not keys._MOUSE_L then return false end
    local x = self:getMousePos()
    if not x then return false end
    local left_idx, right_idx = self.get_left_idx_fn(), self.get_right_idx_fn()
    local width_per_idx = rangeslider_get_width_per_idx(self)
    local left_pos = width_per_idx*(left_idx-1)
    local right_pos = width_per_idx*(right_idx-1) + 4
    if x < left_pos then
        self.on_left_change(self.get_left_idx_fn() - 1)
    elseif x < left_pos+3 then
        self.is_dragging_target = 'left'
        self.is_dragging_idx = x - left_pos
    elseif x < right_pos then
        self.is_dragging_target = 'both'
        self.is_dragging_idx = x - left_pos
    elseif x < right_pos+3 then
        self.is_dragging_target = 'right'
        self.is_dragging_idx = x - right_pos
    else
        self.on_right_change(self.get_right_idx_fn() + 1)
    end
    return true
end

local function rangeslider_do_drag(self, width_per_idx)
    local x = self.frame_body:localXY(dfhack.screen.getMousePos())
    local cur_pos = x - self.is_dragging_idx
    cur_pos = math.max(0, cur_pos)
    cur_pos = math.min(width_per_idx*(self.num_stops-1)+7, cur_pos)
    local offset = self.is_dragging_target == 'right' and -2 or 1
    local new_idx = math.max(0, cur_pos+offset)//width_per_idx + 1
    local new_left_idx, new_right_idx
    if self.is_dragging_target == 'right' then
        new_right_idx = new_idx
    else
        new_left_idx = new_idx
        if self.is_dragging_target == 'both' then
            new_right_idx = new_left_idx + self.get_right_idx_fn() - self.get_left_idx_fn()
            if new_right_idx > self.num_stops then
                return
            end
        end
    end
    if new_left_idx and new_left_idx ~= self.get_left_idx_fn() then
        if not new_right_idx and new_left_idx > self.get_right_idx_fn() then
            self.on_right_change(new_left_idx)
        end
        self.on_left_change(new_left_idx)
    end
    if new_right_idx and new_right_idx ~= self.get_right_idx_fn() then
        if new_right_idx < self.get_left_idx_fn() then
            self.on_left_change(new_right_idx)
        end
        self.on_right_change(new_right_idx)
    end
end

local SLIDER_LEFT_END = to_pen{ch=198, fg=COLOR_GREY, bg=COLOR_BLACK}
local SLIDER_TRACK = to_pen{ch=205, fg=COLOR_GREY, bg=COLOR_BLACK}
local SLIDER_TRACK_SELECTED = to_pen{ch=205, fg=COLOR_LIGHTGREEN, bg=COLOR_BLACK}
local SLIDER_TRACK_STOP = to_pen{ch=216, fg=COLOR_GREY, bg=COLOR_BLACK}
local SLIDER_TRACK_STOP_SELECTED = to_pen{ch=216, fg=COLOR_LIGHTGREEN, bg=COLOR_BLACK}
local SLIDER_RIGHT_END = to_pen{ch=181, fg=COLOR_GREY, bg=COLOR_BLACK}
local SLIDER_TAB_LEFT = to_pen{ch=60, fg=COLOR_BLACK, bg=COLOR_YELLOW}
local SLIDER_TAB_CENTER = to_pen{ch=9, fg=COLOR_BLACK, bg=COLOR_YELLOW}
local SLIDER_TAB_RIGHT = to_pen{ch=62, fg=COLOR_BLACK, bg=COLOR_YELLOW}

function RangeSlider:onRenderBody(dc, rect)
    local left_idx, right_idx = self.get_left_idx_fn(), self.get_right_idx_fn()
    local width_per_idx = rangeslider_get_width_per_idx(self)
    -- draw track
    dc:seek(1,0)
    dc:char(nil, SLIDER_LEFT_END)
    dc:char(nil, SLIDER_TRACK)
    for stop_idx=1,self.num_stops-1 do
        local track_stop_pen = SLIDER_TRACK_STOP_SELECTED
        local track_pen = SLIDER_TRACK_SELECTED
        if left_idx > stop_idx or right_idx < stop_idx then
            track_stop_pen = SLIDER_TRACK_STOP
            track_pen = SLIDER_TRACK
        elseif right_idx == stop_idx then
            track_pen = SLIDER_TRACK
        end
        dc:char(nil, track_stop_pen)
        for i=2,width_per_idx do
            dc:char(nil, track_pen)
        end
    end
    if right_idx >= self.num_stops then
        dc:char(nil, SLIDER_TRACK_STOP_SELECTED)
    else
        dc:char(nil, SLIDER_TRACK_STOP)
    end
    dc:char(nil, SLIDER_TRACK)
    dc:char(nil, SLIDER_RIGHT_END)
    -- draw tabs
    dc:seek(width_per_idx*(left_idx-1))
    dc:char(nil, SLIDER_TAB_LEFT)
    dc:char(nil, SLIDER_TAB_CENTER)
    dc:char(nil, SLIDER_TAB_RIGHT)
    dc:seek(width_per_idx*(right_idx-1)+4)
    dc:char(nil, SLIDER_TAB_LEFT)
    dc:char(nil, SLIDER_TAB_CENTER)
    dc:char(nil, SLIDER_TAB_RIGHT)
    -- manage dragging
    if self.is_dragging_target then
        rangeslider_do_drag(self, width_per_idx)
    end
    if df.global.enabler.mouse_lbut_down == 0 then
        self.is_dragging_target = nil
        self.is_dragging_idx = nil
    end
end

--------------------------------
-- DimensionsTooltip
--------------------------------

---@class widgets.DimensionsTooltip.attrs: widgets.ResizingPanel.attrs
---@field display_offset? df.coord2d
---@field get_anchor_pos_fn fun(): df.coord?

---@class widgets.DimensionsTooltip: widgets.ResizingPanel
---@field super widgets.ResizingPanel
---@overload fun(init_table: widgets.DimensionsTooltip.attrs): self
DimensionsTooltip = defclass(DimensionsTooltip, ResizingPanel)

DimensionsTooltip.ATTRS{
    frame_style=gui.FRAME_THIN,
    frame_background=gui.CLEAR_PEN,
    no_force_pause_badge=true,
    auto_width=true,
    display_offset={x=3, y=3},
    get_anchor_pos_fn=DEFAULT_NIL,
}

local function get_cur_area_dims(anchor_pos)
    local mouse_pos = dfhack.gui.getMousePos(true)
    if not mouse_pos or not anchor_pos then return 1, 1, 1 end

    -- clamp to map edges (you can start a selection from out of bounds)
    mouse_pos = xyz2pos(
        math.max(0, math.min(df.global.world.map.x_count-1, mouse_pos.x)),
        math.max(0, math.min(df.global.world.map.y_count-1, mouse_pos.y)),
        math.max(0, math.min(df.global.world.map.z_count-1, mouse_pos.z)))
    anchor_pos = xyz2pos(
        math.max(0, math.min(df.global.world.map.x_count-1, anchor_pos.x)),
        math.max(0, math.min(df.global.world.map.y_count-1, anchor_pos.y)),
        math.max(0, math.min(df.global.world.map.z_count-1, anchor_pos.z)))

    return math.abs(mouse_pos.x - anchor_pos.x) + 1,
        math.abs(mouse_pos.y - anchor_pos.y) + 1,
        math.abs(mouse_pos.z - anchor_pos.z) + 1
end

function DimensionsTooltip:init()
    ensure_key(self, 'frame').w = 17
    self.frame.h = 4

    self.label = Label{
        frame={t=0},
        auto_width=true,
        text={
            {
                text=function()
                    local anchor_pos = self.get_anchor_pos_fn()
                    return ('%dx%dx%d'):format(get_cur_area_dims(anchor_pos))
                end
            }
        },
    }

    self:addviews{
        Panel{
            -- set minimum size for tooltip frame so the DFHack frame badge fits
            frame={t=0, l=0, w=7, h=2},
        },
        self.label,
    }
end

function DimensionsTooltip:render(dc)
    local x, y = dfhack.screen.getMousePos()
    if not x or not self.get_anchor_pos_fn() then return end
    local sw, sh = dfhack.screen.getWindowSize()
    local frame_width = math.max(9, self.label:getTextWidth() + 2)
    self.frame.l = math.min(x + self.display_offset.x, sw - frame_width)
    self.frame.t = math.min(y + self.display_offset.y, sh - self.frame.h)
    self:updateLayout()
    DimensionsTooltip.super.render(self, dc)
end

return _ENV
