local textures = require('gui.textures')
local GraphicButton = require('gui.widgets.buttons.graphic_button')

local to_pen = dfhack.pen.parse

local enabled_pen_left = to_pen{fg=COLOR_CYAN,
    tile=curry(textures.tp_control_panel, 1), ch=string.byte('[')}
local enabled_pen_center = to_pen{fg=COLOR_LIGHTGREEN,
    tile=curry(textures.tp_control_panel, 2) or nil, ch=251} -- check
local enabled_pen_right = to_pen{fg=COLOR_CYAN,
    tile=curry(textures.tp_control_panel, 3) or nil, ch=string.byte(']')}
local disabled_pen_left = to_pen{fg=COLOR_CYAN,
    tile=curry(textures.tp_control_panel, 4) or nil, ch=string.byte('[')}
local disabled_pen_center = to_pen{fg=COLOR_RED,
    tile=curry(textures.tp_control_panel, 5) or nil, ch=string.byte('x')}
local disabled_pen_right = to_pen{fg=COLOR_CYAN,
    tile=curry(textures.tp_control_panel, 6) or nil, ch=string.byte(']')}

------------------
-- ToggleButton --
------------------

---@class widgets.ToggleButton.attrs: widgets.GraphicButton.attrs
---@field initial_state boolean

---@class widgets.ToggleButton.attrs.partial: widgets.ToggleButton.attrs

---@class widgets.ToggleButton: widgets.GraphicButton, widgets.ToggleButton.attrs
---@field super widgets.GraphicButton
---@field ATTRS widgets.ToggleButton.attrs|fun(attributes: widgets.ToggleButton.attrs.partial)
---@overload fun(init_table: widgets.ToggleButton.attrs.partial): self
ToggleButton = defclass(ToggleButton, GraphicButton)

ToggleButton.ATTRS{
    initial_state=true,
}

function ToggleButton:init()
    self.toggle_state = self.initial_state

    self.on_click = function() self.toggle_state = not self.toggle_state end
    self.pen_left = function() return self.toggle_state and enabled_pen_left or disabled_pen_left end
    self.pen_center = function() return self.toggle_state and enabled_pen_center or disabled_pen_center end
    self.pen_right = function() return self.toggle_state and enabled_pen_right or disabled_pen_right end

    self:refresh()
end

return ToggleButton
