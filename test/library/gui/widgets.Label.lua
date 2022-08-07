local gui = require('gui')
local widgets = require('gui.widgets')

local fs = defclass(fs, gui.FramedScreen)
fs.ATTRS = {
    frame_style = gui.GREY_LINE_FRAME,
    frame_title = 'TestFramedScreen',
    frame_width = 10,
    frame_height = 10,
    frame_inset = 0,
    focus_path = 'test-framed-screen',
}

function test.correct_frame_body_with_scroll_icons()
    local t = {}
    for i = 1, 12 do
        t[#t+1] = tostring(i)
        t[#t+1] = NEWLINE
    end

    function fs:init(args)
        self:addviews{
            widgets.Label{
                view_id = 'text',
                frame_inset = 0,
                text = t,
            },
        }
    end

    local o = fs{}
    expect.eq(o.subviews.text.frame_body.width, 9, "Label's frame_body.x2 and .width should be one smaller because of show_scroll_icons.")
end

function test.correct_frame_body_with_few_text_lines()
    local t = {}
    for i = 1, 10 do
        t[#t+1] = tostring(i)
        t[#t+1] = NEWLINE
    end

    function fs:init(args)
        self:addviews{
            widgets.Label{
                view_id = 'text',
                frame_inset = 0,
                text = t,
            },
        }
    end

    local o = fs{}
    expect.eq(o.subviews.text.frame_body.width, 10, "Label's frame_body.x2 and .width should not change with show_scroll_icons = false.")
end

function test.correct_frame_body_without_show_scroll_icons()
    local t = {}
    for i = 1, 12 do
        t[#t+1] = tostring(i)
        t[#t+1] = NEWLINE
    end

    function fs:init(args)
        self:addviews{
            widgets.Label{
                view_id = 'text',
                frame_inset = 0,
                text = t,
                show_scroll_icons = false,
            },
        }
    end

    local o = fs{}
    expect.eq(o.subviews.text.frame_body.width, 10, "Label's frame_body.x2 and .width should not change with show_scroll_icons = false.")
end
