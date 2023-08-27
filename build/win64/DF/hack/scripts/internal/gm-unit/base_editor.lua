-- Editor base class for gui/gm-unit. Every other editor should inherit from this.
--@ module = true

local widgets = require('gui.widgets')

Editor = defclass(Editor, widgets.Panel)
Editor.ATTRS{
    target_unit = DEFAULT_NIL,
    frame_title = DEFAULT_NIL,
}
