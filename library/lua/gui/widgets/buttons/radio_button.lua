-- A 3x1 tile button that toggles state when clicked

local textures = require('gui.textures')
local ConfigureButton = require('gui.widgets.buttons.configure_button')

local to_pen = dfhack.pen.parse

local enabled_pen_left = to_pen{fg=COLOR_CYAN,
    tile=curry(textures.tp_control_panel, 1) or nil, ch=string.byte('[')}
local enabled_pen_center = to_pen{fg=COLOR_LIGHTGREEN,
    tile=curry(textures.tp_control_panel, 2) or nil, ch=251} -- check mark
local enabled_pen_right = to_pen{fg=COLOR_CYAN,
    tile=curry(textures.tp_control_panel, 3) or nil, ch=string.byte(']')}
local disabled_pen_left = to_pen{fg=COLOR_CYAN,
    tile=curry(textures.tp_control_panel, 4) or nil, ch=string.byte('[')}
local disabled_pen_center = to_pen{fg=COLOR_RED,
    tile=curry(textures.tp_control_panel, 5) or nil, ch=string.byte('x')}
local disabled_pen_right = to_pen{fg=COLOR_CYAN,
    tile=curry(textures.tp_control_panel, 6) or nil, ch=string.byte(']')}

-----------------
-- RadioButton --
-----------------

---@class widgets.RadioButton.attrs: widgets.ConfigureButton.attrs
---@field initial_state boolean
---@field on_change? fun(val: boolean)

---@class widgets.RadioButton.attrs.partial: widgets.RadioButton.attrs

---@class widgets.RadioButton: widgets.ConfigureButton, widgets.RadioButton.attrs
---@field super widgets.ConfigureButton
---@field ATTRS widgets.RadioButton.attrs|fun(attributes: widgets.RadioButton.attrs.partial)
---@overload fun(init_table: widgets.RadioButton.attrs.partial): self
RadioButton = defclass(RadioButton, ConfigureButton)

RadioButton.ATTRS{
    initial_state=true,
    on_change=DEFAULT_NIL,
}

function RadioButton:setState(val)
    self.toggle_state = not not val

    if self.on_change then
        self.on_change(self.toggle_state)
    end
end

function RadioButton:init()
    self.on_click = function() self:setState(not self.toggle_state) end
    self.pen_left = function() return self.toggle_state and enabled_pen_left or disabled_pen_left end
    self.pen_center = function() return self.toggle_state and enabled_pen_center or disabled_pen_center end
    self.pen_right = function() return self.toggle_state and enabled_pen_right or disabled_pen_right end

    self:setState(self.initial_state)
end

return RadioButton
