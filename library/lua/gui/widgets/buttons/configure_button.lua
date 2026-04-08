local textures = require('gui.textures')
local GraphicButton = require('gui.widgets.buttons.graphic_button')

local configure_pen_center = dfhack.pen.parse{
    tile=curry(textures.tp_control_panel, 10) or nil, ch=15} -- gear/masterwork symbol

---------------------
-- ConfigureButton --
---------------------

---@class widgets.ConfigureButton.attrs: widgets.GraphicButton.attrs
---@field super widgets.GraphicButton
ConfigureButton = defclass(ConfigureButton, GraphicButton)

ConfigureButton.ATTRS{
    pen_center=configure_pen_center,
}

return ConfigureButton
