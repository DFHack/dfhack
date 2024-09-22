local CycleHotkeyLabel = require('gui.widgets.labels.cycle_hotkey_label')

-----------------------
-- ToggleHotkeyLabel --
-----------------------

---@class widgets.ToggleHotkeyLabel: widgets.CycleHotkeyLabel
---@field super widgets.CycleHotkeyLabel
ToggleHotkeyLabel = defclass(ToggleHotkeyLabel, CycleHotkeyLabel)
ToggleHotkeyLabel.ATTRS{
    options={{label='On', value=true, pen=COLOR_GREEN},
             {label='Off', value=false}},
}

return ToggleHotkeyLabel
