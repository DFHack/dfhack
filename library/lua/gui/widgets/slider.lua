-- slider.lua
local RangeSlider = require('gui.widgets.range_slider')

local Slider = defclass(Slider, RangeSlider)
Slider.ATTRS {
    get_idx_fn = DEFAULT_NIL, -- Function to get the current index
    on_change = DEFAULT_NIL, -- Function to handle index change
    is_single = true
}

function Slider:init()
    self.get_left_idx_fn = self.get_idx_fn
    self.get_right_idx_fn = self.get_idx_fn
    self.on_left_change = function(index)
        if self.on_change then self.on_change(index) end
    end
    self.on_right_change = function(index)
        if self.on_change then self.on_change(index) end
    end
end

return Slider
