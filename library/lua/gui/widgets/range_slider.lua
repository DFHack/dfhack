local core = require('gui.widgets.slide_core')

--------------------------------
-- RangeSlider
--------------------------------

---@class widgets.RangeSlider.attrs: widgets.Widget.attrs
---@field get_left_idx_fn? function
---@field get_right_idx_fn? function
---@field on_left_change? fun(index: integer)
---@field on_right_change? fun(index: integer)
---@field is_single boolean

---@class widgets.RangeSlider.attrs.partial: widgets.RangeSlider.attrs

---@class widgets.RangeSlider.initTable: widgets.RangeSlider.attrs
---@field num_stops integer

---@class widgets.RangeSlider: widgets.Widget, widgets.RangeSlider.attrs
---@field super widgets.Widget
---@field ATTRS widgets.RangeSlider.attrs|fun(attributes: widgets.RangeSlider.attrs.partial)
---@overload fun(init_table: widgets.RangeSlider.initTable): self
RangeSlider = defclass(RangeSlider, slide_core)
RangeSlider.ATTRS{
    get_left_idx_fn=DEFAULT_NIL,
    get_right_idx_fn=DEFAULT_NIL,
    on_left_change=DEFAULT_NIL,
    on_right_change=DEFAULT_NIL,
    num_stops=DEFAULT_NIL,
    is_single=false,
}

function RangeSlider:get_width_per_idx()
    local min_value = (self.is_single) and 3 or 5  -- Single slider = 3, else 5
    return math.max(min_value, (self.frame_body.width-7) // (self.num_stops-1))
end

function RangeSlider:onInput(keys)
    if not keys._MOUSE_L then return false end
    local x = self:getMousePos()
    if not x then return false end
    local left_idx, right_idx = self.get_left_idx_fn(), self.get_right_idx_fn()
    local width_per_idx = self:get_width_per_idx()
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

return RangeSlider
