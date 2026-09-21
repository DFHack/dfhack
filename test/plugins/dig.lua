config.mode = 'fortress'
config.target = 'dig'

local gui = require('gui')
local overlay = require('plugins.overlay')

local TestScreen = defclass(TestScreen, gui.ZScreen)
TestScreen.ATTRS{focus_path='quickfort'}

function test.ascii_designations_visible_in_quickfort()
    require('plugins.dig')
    overlay.rescan()
    local widget = overlay.get_state().db['dig.asciidesignated'].widget
    expect.eq('dwarfmode/dfhack/lua/quickfort',
              widget.viewscreens[#widget.viewscreens])

    local render = mock.func()
    local screen = TestScreen{}
    screen:show()
    dfhack.with_finalize(
        function() screen:dismiss() end,
        function()
            mock.patch(widget, 'render', render, function()
                overlay.render_viewscreen_widgets(
                    'viewscreen_dwarfmodest',
                    dfhack.gui.getViewscreenByType(df.viewscreen_dwarfmodest, 0))
            end)
        end)
    expect.eq(1, render.call_count)
end
