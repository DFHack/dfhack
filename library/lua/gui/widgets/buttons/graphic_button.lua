local textures = require('gui.textures')
local Panel = require('gui.widgets.containers.panel')
local Label = require('gui.widgets.labels.label')

local to_pen = dfhack.pen.parse

local button_pen_left = to_pen{fg=COLOR_CYAN,
    tile=curry(textures.tp_control_panel, 7) or nil, ch=string.byte('[')}
local button_pen_center = to_pen{fg=COLOR_CYAN,
    tile=curry(textures.tp_control_panel, 10) or nil, ch=string.byte('=')}
local button_pen_right = to_pen{fg=COLOR_CYAN,
    tile=curry(textures.tp_control_panel, 8) or nil, ch=string.byte(']')}

-------------------
-- GraphicButton --
-------------------

---@class widgets.GraphicButton.attrs: widgets.Panel.attrs
---@field on_click? function
---@field pen_left dfhack.pen|fun(): dfhack.pen
---@field pen_center dfhack.pen|fun(): dfhack.pen
---@field pen_right dfhack.pen|fun(): dfhack.pen

---@class widgets.GraphicButton.attrs.partial: widgets.GraphicButton.attrs

---@class widgets.GraphicButton: widgets.Panel, widgets.GraphicButton.attrs
---@field super widgets.Panel
---@field ATTRS widgets.GraphicButton.attrs|fun(attributes: widgets.GraphicButton.attrs.partial)
---@overload fun(init_table: widgets.GraphicButton.attrs.partial): self
GraphicButton = defclass(GraphicButton, Panel)

GraphicButton.ATTRS{
    on_click=DEFAULT_NIL,
    pen_left=button_pen_left,
    pen_center=button_pen_center,
    pen_right=button_pen_right,
}

function GraphicButton:init()
    self.frame.w = self.frame.w or 3
    self.frame.h = self.frame.h or 1

    self:addviews{
        Label{
            view_id='label',
            frame={t=0, l=0, w=3, h=1},
            text={
                {tile=self.pen_left},
                {tile=self.pen_center},
                {tile=self.pen_right},
            },
            on_click=self.on_click,
        },
    }
end

function GraphicButton:refresh()
    local l = self.subviews.label

    l.on_click = self.on_click
    l.pen_left = self.pen_left
    l.pen_center = self.pen_center
    l.pen_right = self.pen_right

    l:setText({{tile=self.pen_left}, {tile=self.pen_center}, {tile=self.pen_right}})
end

return GraphicButton
