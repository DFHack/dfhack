-- Test lua viewscreens.

local gui = require 'gui'

local text = 'Woohoo, lua viewscreen :)'

local screen = mkinstance(gui.FramedScreen, {
    frame_style = gui.GREY_LINE_FRAME,
    frame_title = 'Hello World',
    frame_width = #text+6,
    frame_height = 3,
    onRenderBody = function(self, dc)
        dc:seek(3,1):string(text, COLOR_LIGHTGREEN)
    end,
    onInput = function(self,keys)
        if keys.LEAVESCREEN or keys.SELECT then
            self:dismiss()
        end
    end
})

screen:show()
