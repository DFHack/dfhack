local Widget = require('gui.widgets.widget')

local to_pen = dfhack.pen.parse

--------------------------------
-- Slider
--------------------------------

---@class widgets.Slider.attrs: widgets.Widget.attrs
---@field num_stops integer
---@field get_idx_fn? function
---@field on_change? fun(index: integer)

---@class widgets.Slider.attrs.partial: widgets.Slider.attrs

---@class widgets.Slider.initTable: widgets.Slider.attrs
---@field num_stops integer

---@class widgets.Slider: widgets.Widget, widgets.Slider.attrs
---@field super widgets.Widget
---@field ATTRS widgets.Slider.attrs|fun(attributes: widgets.Slider.attrs.partial)
---@overload fun(init_table: widgets.Slider.initTable): self
Slider = defclass(Slider, Widget)
Slider.ATTRS{
    num_stops=DEFAULT_NIL,
    get_idx_fn=DEFAULT_NIL,
    on_change=DEFAULT_NIL,
}

function Slider:preinit(init_table)
    init_table.frame = init_table.frame or {}
    init_table.frame.h = init_table.frame.h or 1
end

function Slider:init()
    if self.num_stops < 2 then error('too few Slider stops') end
    self.is_dragging_target = nil -- 'left', 'right', or 'both'
    self.is_dragging_idx = nil -- offset from leftmost dragged tile
end

local function Slider_get_width_per_idx(self)
    return math.max(3, (self.frame_body.width-7) // (self.num_stops-1))
end

function Slider:onInput(keys)
    if not keys._MOUSE_L then return false end
    local x = self:getMousePos()
    if not x then return false end
    local left_idx = self.get_idx_fn()
    local width_per_idx = Slider_get_width_per_idx(self)
    local left_pos = width_per_idx*(left_idx-1)
    local right_pos = width_per_idx*(left_idx-1) + 4
    if x < left_pos then
        self.on_change(self.get_idx_fn() - 1)
    else
        self.is_dragging_target = 'both'
        self.is_dragging_idx = x - right_pos
    end
    return true
end

local function Slider_do_drag(self, width_per_idx)
    local x = self.frame_body:localXY(dfhack.screen.getMousePos())
    local cur_pos = x - self.is_dragging_idx
    cur_pos = math.max(0, cur_pos)
    cur_pos = math.min(width_per_idx*(self.num_stops-1)+7, cur_pos)
    local offset = 1
    local new_idx = math.max(0, cur_pos+offset)//width_per_idx + 1
    if self.is_dragging_target == 'both' then
        if new_idx > self.num_stops then
            return
        end
    end
    if new_idx and new_idx ~= self.get_idx_fn() then
        self.on_change(new_idx)
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

function Slider:onRenderBody(dc, rect)
    local left_idx = self.get_idx_fn()
    local width_per_idx = Slider_get_width_per_idx(self)
    -- draw track
    dc:seek(1,0)
    dc:char(nil, SLIDER_LEFT_END)
    dc:char(nil, SLIDER_TRACK)
    for stop_idx=1,self.num_stops-1 do
        local track_stop_pen = SLIDER_TRACK_STOP_SELECTED
        local track_pen = SLIDER_TRACK_SELECTED
        if left_idx ~= stop_idx then
            track_stop_pen = SLIDER_TRACK_STOP
            track_pen = SLIDER_TRACK
        elseif left_idx == stop_idx then
            track_pen = SLIDER_TRACK
        end
        dc:char(nil, track_stop_pen)
        for i=2,width_per_idx do
            dc:char(nil, track_pen)
        end
    end
    if left_idx >= self.num_stops then
        dc:char(nil, SLIDER_TRACK_STOP_SELECTED)
    else
        dc:char(nil, SLIDER_TRACK_STOP)
    end
    dc:char(nil, SLIDER_TRACK)
    dc:char(nil, SLIDER_RIGHT_END)
    -- draw tab
    dc:seek(width_per_idx*(left_idx-1)+2)
    dc:char(nil, SLIDER_TAB_LEFT)
    dc:char(nil, SLIDER_TAB_CENTER)
    dc:char(nil, SLIDER_TAB_RIGHT)
    -- manage dragging
    if self.is_dragging_target then
        Slider_do_drag(self, width_per_idx)
    end
    if df.global.enabler.mouse_lbut_down == 0 then
        self.is_dragging_target = nil
        self.is_dragging_idx = nil
    end
end

return Slider
