-- A basic example to start your own gui script from.
--@ module = true

local gui = require('gui')
local widgets = require('gui.widgets')

local HIGHLIGHT_PEN = dfhack.pen.parse{
    ch=string.byte(' '),
    fg=COLOR_LIGHTGREEN,
    bg=COLOR_LIGHTGREEN,
}

HelloWorldWindow = defclass(HelloWorldWindow, widgets.Window)
HelloWorldWindow.ATTRS{
    frame={w=20, h=14},
    frame_title='Hello World',
    autoarrange_subviews=true,
    autoarrange_gap=1,
}

function HelloWorldWindow:init()
    self:addviews{
        widgets.Label{text={{text='Hello, world!', pen=COLOR_LIGHTGREEN}}},
        widgets.HotkeyLabel{
            frame={l=0, t=0},
            label='Click me',
            key='CUSTOM_CTRL_A',
            on_activate=self:callback('toggleHighlight'),
        },
        widgets.Panel{
            view_id='highlight',
            frame={w=10, h=5},
            frame_style=gui.INTERIOR_FRAME,
        },
    }
end

function HelloWorldWindow:toggleHighlight()
    local panel = self.subviews.highlight
    panel.frame_background = not panel.frame_background and HIGHLIGHT_PEN or nil
end

HelloWorldScreen = defclass(HelloWorldScreen, gui.ZScreen)
HelloWorldScreen.ATTRS{
    focus_path='hello-world',
}

function HelloWorldScreen:init()
    self:addviews{HelloWorldWindow{}}
end

function HelloWorldScreen:onDismiss()
    view = nil
end

if dfhack_flags.module then
    return
end

view = view and view:raise() or HelloWorldScreen{}:show()
