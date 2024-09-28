local BannerPanel = require('gui.widgets.containers.banner_panel')
local HotkeyLabel = require('gui.widgets.labels.hotkey_label')

----------------
-- TextButton --
----------------

---@class widgets.TextButton.initTable: widgets.Panel.attrs.partial, widgets.HotkeyLabel.attrs.partial

---@class widgets.TextButton: widgets.BannerPanel
---@field super widgets.BannerPanel
---@overload fun(init_table: widgets.TextButton.initTable): self
TextButton = defclass(TextButton, BannerPanel)

---@param info widgets.TextButton.initTable
function TextButton:init(info)
    self.label = HotkeyLabel{
        frame={t=0, l=1, r=1},
        key=info.key,
        key_sep=info.key_sep,
        label=info.label,
        on_activate=info.on_activate,
        text_pen=info.text_pen,
        text_dpen=info.text_dpen,
        text_hpen=info.text_hpen,
        disabled=info.disabled,
        enabled=info.enabled,
        auto_height=info.auto_height,
        auto_width=info.auto_width,
        on_click=info.on_click,
        on_rclick=info.on_rclick,
        scroll_keys=info.scroll_keys,
    }

    self:addviews{self.label}
end

function TextButton:setLabel(label)
    self.label:setLabel(label)
end

return TextButton
