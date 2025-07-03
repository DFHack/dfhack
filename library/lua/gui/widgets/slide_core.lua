local Widget = require('gui.widgets.widget')

local to_pen = dfhack.pen.parse

--------------------------------
-- slide_core
--------------------------------

---@class widgets.slide_core.attrs: widgets.Widget.attrs
---@field num_stops integer
---@field is_single boolean
---@field w integer

---@class widgets.slide_core.attrs.partial: widgets.slide_core.attrs

---@class widgets.slide_core.initTable: widgets.slide_core.attrs
---@field num_stops integer

---@class widgets.slide_core: widgets.Widget, widgets.slide_core.attrs
---@field super widgets.Widget
---@field ATTRS widgets.slide_core.attrs|fun(attributes: widgets.slide_core.attrs.partial)
---@overload fun(init_table: widgets.slide_core.initTable): self
slide_core = defclass(slide_core, Widget)
slide_core.ATTRS{
    num_stops=DEFAULT_NIL,
    is_single=DEFAULT_NIL,
    w=DEFAULT_NIL
}

function slide_core:preinit(init_table)
    init_table.frame = init_table.frame or {}
    init_table.frame.h = init_table.frame.h or 1
end

function slide_core:init()
    local min_stops = self:get_min_stops()
    if self.num_stops < min_stops then error(('too few stops, expected at least %s'):format(min_stops)) end
    self.is_dragging_target = nil -- 'left', 'right', or 'both'
    self.is_dragging_idx = nil -- offset from leftmost dragged tile
end

function slide_core:get_min_stops()
    return self.is_single and 1 or 2
end

local function do_drag(self, width_per_idx)
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
            if self.get_left_idx_fn ~= nil then
                new_right_idx = new_left_idx + self.get_right_idx_fn() - self.get_left_idx_fn()
            else
                new_right_idx = new_left_idx
            end
            if new_right_idx > self.num_stops then
                return
            end
        end
    end
    if self.get_idx_fn == nil then
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
    else
        if new_idx and new_idx ~= self.get_idx_fn() then
            self.on_change(new_idx)
        end
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

function slide_core:onRenderBody(dc, rect)
    local left_idx, right_idx
    if self.get_idx_fn ~= nil then
        left_idx = self.get_idx_fn()
        right_idx = left_idx
    else
        left_idx, right_idx = self.get_left_idx_fn(), self.get_right_idx_fn()
    end
    local width_per_idx = self:get_width_per_idx()
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

    -- Draw tab(s)
    if self.is_single then
        -- Single slider: Draw one centered tab
        dc:seek(width_per_idx * (left_idx-1) + 2)  -- Center the tab
        dc:char(nil, SLIDER_TAB_LEFT)
        dc:char(nil, SLIDER_TAB_CENTER)
        dc:char(nil, SLIDER_TAB_RIGHT)
    else
        -- Dual slider: Draw left and right tabs separately
        dc:seek(width_per_idx * (left_idx-1))
        dc:char(nil, SLIDER_TAB_LEFT)
        dc:char(nil, SLIDER_TAB_CENTER)
        dc:char(nil, SLIDER_TAB_RIGHT)
        dc:seek(width_per_idx*(right_idx-1)+4)
        dc:char(nil, SLIDER_TAB_LEFT)
        dc:char(nil, SLIDER_TAB_CENTER)
        dc:char(nil, SLIDER_TAB_RIGHT)
    end

    -- Manage dragging
    if self.is_dragging_target then
        do_drag(self, width_per_idx)
    end
    if df.global.enabler.mouse_lbut_down == 0 then
        self.is_dragging_target = nil
        self.is_dragging_idx = nil
    end
end

return slide_core
