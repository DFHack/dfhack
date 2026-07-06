-- A 3x1 tile button with a question mark on it. Clicking on it will show help text for a command

local textures = require('gui.textures')
local ConfigureButton = require('gui.widgets.buttons.configure_button')

local help_pen_center = dfhack.pen.parse{
    tile=curry(textures.tp_control_panel, 9) or nil, ch=string.byte('?')}

----------------
-- HelpButton --
----------------

---@class widgets.HelpButton.attrs: widgets.ConfigureButton.attrs
---@field command? string

---@class widgets.HelpButton.attrs.partial: widgets.HelpButton.attrs

---@class widgets.HelpButton: widgets.ConfigureButton, widgets.HelpButton.attrs
---@field super widgets.ConfigureButton
---@field ATTRS widgets.HelpButton.attrs|fun(attributes: widgets.HelpButton.attrs.partial)
---@overload fun(init_table: widgets.HelpButton.attrs.partial): self
HelpButton = defclass(HelpButton, ConfigureButton)

HelpButton.ATTRS{
    frame={t=0, r=1, w=3, h=1},
    command=DEFAULT_NIL,
    pen_center=help_pen_center,
}

function HelpButton:init()
    local command = self.command .. ' '
    self.on_click = function() dfhack.run_command('gui/launcher', command) end
end

return HelpButton
