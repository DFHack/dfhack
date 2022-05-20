-- test -dhack/scripts/devel/tests -twidgets.Label

--reload('gui')
--reload('gui.widgets')

local gui = require('gui')
local widgets = require('gui.widgets')

local xtest = {} -- use to temporarily disable tests (change `function xtest.somename` to `function xxtest.somename`)
local wait = function(n)
    delay(n or 30) -- enable for debugging the tests
end

function test.Label_correct_frame_body_with_scroll_icons()
    local fs = defclass(fs, gui.FramedScreen)
    fs.ATTRS = {
        frame_style = gui.GREY_LINE_FRAME,
        frame_title = 'TestFramedScreen',
        frame_width = 10,
        frame_height = 10,
        frame_inset = 0,
        focus_path = 'test-framed-screen',
    }

    t = {}
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
                --show_scroll_icons = 'right',
            },
        }
    end

    local o = fs{}
    --o:show()
    --wait()
    expect.eq(o.subviews.text.frame_body.width, 9, "Label's frame_body.x2 and .width should be one smaller because of show_scroll_icons.")
    --o:dismiss()
end

function test.Label_correct_frame_body_with_few_text_lines()
    local fs = defclass(fs, gui.FramedScreen)
    fs.ATTRS = {
        frame_style = gui.GREY_LINE_FRAME,
        frame_title = 'TestFramedScreen',
        frame_width = 10,
        frame_height = 10,
        frame_inset = 0,
        focus_path = 'test-framed-screen',
    }

    t = {}
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
                --show_scroll_icons = 'right',
            },
        }
    end

    local o = fs{}
    --o:show()
    --wait()
    expect.eq(o.subviews.text.frame_body.width, 10, "Label's frame_body.x2 and .width should not change with show_scroll_icons = false.")
    --o:dismiss()
end

function test.Label_correct_frame_body_without_show_scroll_icons()
    local fs = defclass(fs, gui.FramedScreen)
    fs.ATTRS = {
        frame_style = gui.GREY_LINE_FRAME,
        frame_title = 'TestFramedScreen',
        frame_width = 10,
        frame_height = 10,
        frame_inset = 0,
        focus_path = 'test-framed-screen',
    }

    t = {}
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
    --o:show()
    --wait()
    expect.eq(o.subviews.text.frame_body.width, 10, "Label's frame_body.x2 and .width should not change with show_scroll_icons = false.")
    --o:dismiss()
end
