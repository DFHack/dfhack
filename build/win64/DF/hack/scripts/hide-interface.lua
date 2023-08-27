local gui = require('gui')
local widgets = require('gui.widgets')

TransparentScreen = defclass(TransparentScreen, gui.ZScreen)
TransparentScreen.ATTRS {
    focus_path='hide-interface',
    pass_movement_keys=true,
    pass_mouse_clicks=false,
    defocusable=false,
}

function TransparentScreen:init()
    self:addviews{
        widgets.Panel{
            frame_background=gui.TRANSPARENT_PEN,
            visible=function() return dfhack.screen.inGraphicsMode() end,
        },
        widgets.Panel{
            frame={h=5, w=50},
            frame_background=gui.CLEAR_PEN,
            frame_style=gui.FRAME_PANEL,
            visible=function() return not dfhack.screen.inGraphicsMode() end,
            subviews={
                widgets.Label{
                    auto_width=true,
                    text='Interface cannot be hidden in ASCII mode.',
                }
            },
        },
    }
end

function TransparentScreen:onDismiss()
    view = nil
end

view = view and view:raise() or TransparentScreen{}:show()
