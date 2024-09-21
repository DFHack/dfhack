local textures = require('gui.textures')
local Panel = require('gui.widgets.panel')
local Label = require('gui.widgets.label')

local to_pen = dfhack.pen.parse

local button_pen_left = to_pen{fg=COLOR_CYAN,
    tile=curry(textures.tp_control_panel, 7) or nil, ch=string.byte('[')}
local button_pen_right = to_pen{fg=COLOR_CYAN,
    tile=curry(textures.tp_control_panel, 8) or nil, ch=string.byte(']')}
local configure_pen_center = to_pen{
    tile=curry(textures.tp_control_panel, 10) or nil, ch=15} -- gear/masterwork symbol

---------------------
-- ConfigureButton --
---------------------

---@class widgets.ConfigureButton.attrs: widgets.Panel.attrs
---@field on_click? function

---@class widgets.ConfigureButton.attrs.partial: widgets.ConfigureButton.attrs

---@class widgets.ConfigureButton: widgets.Panel, widgets.ConfigureButton.attrs
---@field super widgets.Panel
---@field ATTRS widgets.ConfigureButton.attrs|fun(attributes: widgets.ConfigureButton.attrs.partial)
---@overload fun(init_table: widgets.ConfigureButton.attrs.partial): self
ConfigureButton = defclass(ConfigureButton, Panel)

ConfigureButton.ATTRS{
    on_click=DEFAULT_NIL,
}

function ConfigureButton:preinit(init_table)
    init_table.frame = init_table.frame or {}
    init_table.frame.h = init_table.frame.h or 1
    init_table.frame.w = init_table.frame.w or 3
end

function ConfigureButton:init()
    self:addviews{
        Label{
            frame={t=0, l=0, w=3, h=1},
            text={
                {tile=button_pen_left},
                {tile=configure_pen_center},
                {tile=button_pen_right},
            },
            on_click=self.on_click,
        },
    }
end

return ConfigureButton
