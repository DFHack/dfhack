local textures = require('gui.textures')
local GraphicButton = require('gui.widgets.buttons.graphic_button')

local help_pen_center = dfhack.pen.parse{
    tile=curry(textures.tp_control_panel, 9) or nil, ch=string.byte('?')}

----------------
-- HelpButton --
----------------

---@class widgets.HelpButton.attrs: widgets.GraphicButton.attrs
---@field command? string

---@class widgets.HelpButton.attrs.partial: widgets.HelpButton.attrs

---@class widgets.HelpButton: widgets.GraphicButton, widgets.HelpButton.attrs
---@field super widgets.GraphicButton
---@field ATTRS widgets.HelpButton.attrs|fun(attributes: widgets.HelpButton.attrs.partial)
---@overload fun(init_table: widgets.HelpButton.attrs.partial): self
HelpButton = defclass(HelpButton, GraphicButton)

HelpButton.ATTRS{
    frame={t=0, r=1, w=3, h=1},
    command=DEFAULT_NIL,
    pen_center=help_pen_center,
}

function HelpButton:init()
    local command = self.command .. ' '

    self.on_click = function() dfhack.run_command('gui/launcher', command) end
    self:refresh()
end

return HelpButton
