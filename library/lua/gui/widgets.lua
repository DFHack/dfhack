-- Simple widgets for screens

local _ENV = mkmodule('gui.widgets')

Widget = require('gui.widgets.widget')
Divider = require('gui.widgets.divider')
Panel = require('gui.widgets.panel')
Window = require('gui.widgets.window')
ResizingPanel = require('gui.widgets.resizing_panel')
Pages = require('gui.widgets.pages')
EditField = require('gui.widgets.edit_field')
HotkeyLabel = require('gui.widgets.hotkey_label')
Label = require('gui.widgets.label')
Scrollbar = require('gui.widgets.scrollbar')
WrappedLabel = require('gui.widgets.wrapped_label')
TooltipLabel = require('gui.widgets.tooltip_label')
HelpButton = require('gui.widgets.help_button')
ConfigureButton = require('gui.widgets.configure_button')
BannerPanel = require('gui.widgets.banner_panel')
TextButton = require('gui.widgets.text_button')
CycleHotkeyLabel = require('gui.widgets.cycle_hotkey_label')
ButtonGroup = require('gui.widgets.button_group')
ToggleHotkeyLabel = require('gui.widgets.toggle_hotkey_label')
List = require('gui.widgets.list')
FilteredList = require('gui.widgets.filtered_list')
TabBar = require('gui.widgets.tab_bar')
RangeSlider = require('gui.widgets.range_slider')
DimensionsTooltip = require('gui.widgets.dimensions_tooltip')

Tab = TabBar.Tab
makeButtonLabelText = Label.makeButtonLabelText
parse_label_text = Label.parse_label_text
render_text = Label.render_text
check_text_keys = Label.check_text_keys

DOUBLE_CLICK_MS = Panel.DOUBLE_CLICK_MS
STANDARDSCROLL = Scrollbar.STANDARDSCROLL
SCROLL_INITIAL_DELAY_MS = Scrollbar.SCROLL_INITIAL_DELAY_MS
SCROLL_DELAY_MS = Scrollbar.SCROLL_DELAY_MS

return _ENV
