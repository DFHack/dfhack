local gui = require('gui')
local Panel = require('gui.widgets.containers.panel')

------------
-- Window --
------------

---@class widgets.Window.attrs: widgets.Panel.attrs
---@field frame_style gui.Frame|fun(): gui.Frame
---@field frame_background dfhack.color|dfhack.pen
---@field frame_inset integer

---@class widgets.Window.attrs.partial: widgets.Window.attrs

---@class widgets.Window: widgets.Panel, widgets.Window.attrs
---@field super widgets.Panel
---@field ATTRS widgets.Window.attrs|fun(attributes: widgets.Window.attrs.partial)
---@overload fun(init_table: widgets.Window.attrs.partial): self
Window = defclass(Window, Panel)

Window.ATTRS {
    frame_style = gui.WINDOW_FRAME,
    frame_background = gui.CLEAR_PEN,
    frame_inset = 1,
    draggable = true,
}

return Window
