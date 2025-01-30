-- Simple widgets for screens

local _ENV = mkmodule('gui.widgets')

local common = require('gui.widgets.common')

Widget = require('gui.widgets.widget')
Divider = require('gui.widgets.divider')
Panel = require('gui.widgets.containers.panel')
Window = require('gui.widgets.containers.window')
ResizingPanel = require('gui.widgets.containers.resizing_panel')
Pages = require('gui.widgets.containers.pages')
EditField = require('gui.widgets.edit_field')
HotkeyLabel = require('gui.widgets.labels.hotkey_label')
Label = require('gui.widgets.labels.label')
Scrollbar = require('gui.widgets.scrollbar')
WrappedLabel = require('gui.widgets.labels.wrapped_label')
TooltipLabel = require('gui.widgets.labels.tooltip_label')
HelpButton = require('gui.widgets.buttons.help_button')
ConfigureButton = require('gui.widgets.buttons.configure_button')
BannerPanel = require('gui.widgets.containers.banner_panel')
TextButton = require('gui.widgets.buttons.text_button')
CycleHotkeyLabel = require('gui.widgets.labels.cycle_hotkey_label')
ButtonGroup = require('gui.widgets.buttons.button_group')
ToggleHotkeyLabel = require('gui.widgets.labels.toggle_hotkey_label')
List = require('gui.widgets.list')
FilteredList = require('gui.widgets.filtered_list')
TabBar = require('gui.widgets.tab_bar')
RangeSlider = require('gui.widgets.range_slider')
DimensionsTooltip = require('gui.widgets.dimensions_tooltip')
TextArea = require('gui.widgets.text_area')

Tab = TabBar.Tab
makeButtonLabelText = Label.makeButtonLabelText
STANDARDSCROLL = common.STANDARDSCROLL

---@return boolean
function getDoubleClickMs()
    return common.DOUBLE_CLICK_MS
end
function setDoubleClickMs(value)
    common.DOUBLE_CLICK_MS = value
end

---@return boolean
function getScrollInitialDelayMs()
    return common.SCROLL_INITIAL_DELAY_MS
end
function setScrollInitialDelayMs(value)
    common.SCROLL_INITIAL_DELAY_MS = value
end

---@return boolean
function getScrollDelayMs()
    return common.SCROLL_DELAY_MS
end
function setScrollDelayMs(value)
    common.SCROLL_DELAY_MS = value
end

return _ENV
