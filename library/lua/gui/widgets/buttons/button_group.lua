local CycleHotkeyLabel = require('gui.widgets.labels.cycle_hotkey_label')
local Label = require('gui.widgets.labels.label')

-----------------
-- ButtonGroup --
-----------------

---@class widgets.ButtonGroup.attrs: widgets.CycleHotkeyLabel.attrs

---@class widgets.ButtonGroup.initTable: widgets.ButtonGroup.attrs
---@field button_specs widgets.ButtonLabelSpec[]
---@field button_specs_selected widgets.ButtonLabelSpec[]

---@class widgets.ButtonGroup: widgets.ButtonGroup.attrs, widgets.CycleHotkeyLabel
---@field super widgets.CycleHotkeyLabel
---@overload fun(init_table: widgets.ButtonGroup.initTable): self
ButtonGroup = defclass(ButtonGroup, CycleHotkeyLabel)

ButtonGroup.ATTRS{
    auto_height=false,
}

function ButtonGroup:init(info)
    ensure_key(self, 'frame').h = #info.button_specs[1].chars + 2

    local start = 0
    for idx in ipairs(self.options) do
        local opt_val = self:getOptionValue(idx)
        local width = #info.button_specs[idx].chars[1]
        self:addviews{
            Label{
                frame={t=2, l=start, w=width},
                text=Label.makeButtonLabelText(info.button_specs[idx]),
                on_click=function() self:setOption(opt_val, true) end,
                visible=function() return self:getOptionValue() ~= opt_val end,
            },
            Label{
                frame={t=2, l=start, w=width},
                text=Label.makeButtonLabelText(info.button_specs_selected[idx]),
                on_click=function() self:setOption(opt_val, false) end,
                visible=function() return self:getOptionValue() == opt_val end,
            },
        }
        start = start + width
    end
end

return ButtonGroup
