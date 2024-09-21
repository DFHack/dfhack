local textures = require('gui.textures')
local Panel = require('gui.widgets.panel')
local Label = require('gui.widgets.label')

local to_pen = dfhack.pen.parse

----------------
-- HelpButton --
----------------

---@class widgets.HelpButton.attrs: widgets.Panel.attrs
---@field command? string

---@class widgets.HelpButton.attrs.partial: widgets.HelpButton.attrs

---@class widgets.HelpButton: widgets.Panel, widgets.HelpButton.attrs
---@field super widgets.Panel
---@field ATTRS widgets.HelpButton.attrs|fun(attributes: widgets.HelpButton.attrs.partial)
---@overload fun(init_table: widgets.HelpButton.attrs.partial): self
HelpButton = defclass(HelpButton, Panel)

HelpButton.ATTRS{
    frame={t=0, r=1, w=3, h=1},
    command=DEFAULT_NIL,
}

local button_pen_left = to_pen{fg=COLOR_CYAN,
    tile=curry(textures.tp_control_panel, 7) or nil, ch=string.byte('[')}
local button_pen_right = to_pen{fg=COLOR_CYAN,
    tile=curry(textures.tp_control_panel, 8) or nil, ch=string.byte(']')}
local help_pen_center = to_pen{
    tile=curry(textures.tp_control_panel, 9) or nil, ch=string.byte('?')}

function HelpButton:init()
    self.frame.w = self.frame.w or 3
    self.frame.h = self.frame.h or 1

    local command = self.command .. ' '

    self:addviews{
        Label{
            frame={t=0, l=0, w=3, h=1},
            text={
                {tile=button_pen_left},
                {tile=help_pen_center},
                {tile=button_pen_right},
            },
            on_click=function() dfhack.run_command('gui/launcher', command) end,
        },
    }
end

return HelpButton
