local WrappedLabel = require('gui.widgets.wrapped_label')

------------------
-- TooltipLabel --
------------------

---@class widgets.TooltipLabel.attrs: widgets.WrappedLabel.attrs
---@field show_tooltip? boolean|fun(): boolean

---@class widgets.TooltipLabel.attrs.partial: widgets.TooltipLabel.attrs

---@class widgets.TooltipLabel: widgets.WrappedLabel, widgets.TooltipLabel.attrs
---@field super widgets.WrappedLabel
---@field ATTRS widgets.TooltipLabel.attrs|fun(attributes: widgets.TooltipLabel.attrs.partial)
---@overload fun(init_table: widgets.TooltipLabel.attrs.partial): self
TooltipLabel = defclass(TooltipLabel, WrappedLabel)

TooltipLabel.ATTRS{
    show_tooltip=DEFAULT_NIL,
    indent=2,
    text_pen=COLOR_GREY,
}

function TooltipLabel:init()
    self.visible = self.show_tooltip
end

return TooltipLabel
