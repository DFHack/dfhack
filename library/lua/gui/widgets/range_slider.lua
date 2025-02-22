local Widget = require('gui.widgets.widget')

local to_pen = dfhack.pen.parse

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
    is_single=DEFAULT_NIL
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
    local min_value = (self.is_single) and 3 or 5  -- Single slider = 3, else 5
    return math.max(min_value, (self.frame_body.width-7) // (self.num_stops-1))
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
        rangeslider_do_drag(self, width_per_idx)
    end
    if df.global.enabler.mouse_lbut_down == 0 then
        self.is_dragging_target = nil
        self.is_dragging_idx = nil
    end
end

return RangeSlider
