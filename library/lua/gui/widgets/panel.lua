local gui = require('gui')
local utils = require('utils')
local guidm = require('gui.dwarfmode')
local Widget = require('gui.widgets.widget')

local getval = utils.getval
local to_pen = dfhack.pen.parse

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

Panel.DOUBLE_CLICK_MS = DOUBLE_CLICK_MS

return Panel
