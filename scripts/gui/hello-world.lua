-- Test lua viewscreens.

local gui = require 'gui'

local text = 'Woohoo, lua viewscreen :)'

local screen = gui.FramedScreen{
    frame_style = gui.GREY_LINE_FRAME,
    frame_title = 'Hello World',
    frame_width = #text,
    frame_height = 1,
    frame_inset = 1,
}

function screen:onRenderBody(dc)
    dc:string(text, COLOR_LIGHTGREEN)
end

function screen:onInput(keys)
    if keys.LEAVESCREEN or keys.SELECT then
        self:dismiss()
    end
end

screen:show()
