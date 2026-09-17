config.target = 'core'

local gui = require 'gui'

function test.mkdims_xy()
    local dims = gui.mkdims_xy(10, 20, 30, 40)
    expect.eq(dims.x1, 10)
    expect.eq(dims.y1, 20)
    expect.eq(dims.x2, 30)
    expect.eq(dims.y2, 40)
    expect.eq(dims.width, 21) -- 30-10+1
    expect.eq(dims.height, 21) -- 40-20+1
end

function test.mkdims_wh()
    local dims = gui.mkdims_wh(10, 20, 15, 25)
    expect.eq(dims.x1, 10)
    expect.eq(dims.y1, 20)
    expect.eq(dims.x2, 24) -- 10+15-1
    expect.eq(dims.y2, 44) -- 20+25-1
    expect.eq(dims.width, 15)
    expect.eq(dims.height, 25)
end

function test.parse_inset()
    -- Test with table
    local inset = gui.parse_inset({l = 5, r = 10, t = 3, b = 7})
    expect.eq(inset, {5, 3, 10, 7})

    -- Test with single value
    local inset2 = gui.parse_inset(8)
    expect.eq(inset2, {8, 8, 8, 8})

    -- Test with x/y shorthand
    local inset3 = gui.parse_inset({x = 4, y = 6})
    expect.eq(inset3, {4, 6, 4, 6})
end

function test.inset_frame()
    local rect = gui.mkdims_wh(0, 0, 100, 50)
    local inset = gui.inset_frame(rect, {l = 10, r = 10, t = 5, b = 5})

    expect.eq(inset.x1, 10)
    expect.eq(inset.y1, 5)
    expect.eq(inset.x2, 89) -- 100-10-1
    expect.eq(inset.y2, 44) -- 50-5-1
    expect.eq(inset.width, 80)
    expect.eq(inset.height, 40)
end

function test.is_in_rect()
    local rect = gui.mkdims_wh(10, 10, 20, 20)

    expect.true_(gui.is_in_rect(rect, 15, 15))
    expect.true_(gui.is_in_rect(rect, 10, 10))
    expect.true_(gui.is_in_rect(rect, 29, 29))
    expect.false_(gui.is_in_rect(rect, 9, 15))
    expect.false_(gui.is_in_rect(rect, 15, 9))
    expect.false_(gui.is_in_rect(rect, 30, 15))
    expect.false_(gui.is_in_rect(rect, 15, 30))
end

function test.ViewRect_basic()
    local view_rect = gui.ViewRect{rect = gui.mkdims_wh(0, 0, 50, 30)}

    expect.eq(view_rect.x1, 0)
    expect.eq(view_rect.y1, 0)
    expect.eq(view_rect.x2, 49)
    expect.eq(view_rect.y2, 29)
    expect.eq(view_rect.width, 50)
    expect.eq(view_rect.height, 30)
end

function test.ViewRect_isDefunct()
    local valid_rect = gui.ViewRect{rect = gui.mkdims_wh(0, 0, 50, 30)}
    expect.false_(valid_rect:isDefunct())

    local defunct_rect = gui.ViewRect{rect = gui.mkdims_wh(0, 0, 0, 0)}
    expect.true_(defunct_rect:isDefunct())
end

function test.ViewRect_inClipGlobalXY()
    local view_rect = gui.ViewRect{rect = gui.mkdims_wh(10, 10, 20, 20)}

    expect.true_(view_rect:inClipGlobalXY(15, 15))
    expect.true_(view_rect:inClipGlobalXY(10, 10))
    expect.true_(view_rect:inClipGlobalXY(29, 29))
    expect.false_(view_rect:inClipGlobalXY(9, 15))
    expect.false_(view_rect:inClipGlobalXY(15, 30))
end

function test.ViewRect_inClipLocalXY()
    local view_rect = gui.ViewRect{rect = gui.mkdims_wh(10, 10, 20, 20)}

    expect.true_(view_rect:inClipLocalXY(5, 5)) -- Global 15,15
    expect.true_(view_rect:inClipLocalXY(0, 0)) -- Global 10,10
    expect.false_(view_rect:inClipLocalXY(-1, 5)) -- Global 9,15
    expect.false_(view_rect:inClipLocalXY(5, 20)) -- Global 15,30
end

function test.ViewRect_localXY()
    local view_rect = gui.ViewRect{rect = gui.mkdims_wh(10, 10, 20, 20)}

    local lx, ly = view_rect:localXY(15, 25)
    expect.eq(lx, 5)
    expect.eq(ly, 15)
end

function test.ViewRect_globalXY()
    local view_rect = gui.ViewRect{rect = gui.mkdims_wh(10, 10, 20, 20)}

    local gx, gy = view_rect:globalXY(5, 15)
    expect.eq(gx, 15)
    expect.eq(gy, 25)
end

function test.ViewRect_viewport()
    local view_rect = gui.ViewRect{rect = gui.mkdims_wh(0, 0, 100, 100)}
    local viewport = view_rect:viewport(10, 10, 20, 20)

    expect.eq(viewport.x1, 10)
    expect.eq(viewport.y1, 10)
    expect.eq(viewport.x2, 29)
    expect.eq(viewport.y2, 29)
    expect.eq(viewport.width, 20)
    expect.eq(viewport.height, 20)
end

function test.Painter_basic()
    local painter = gui.Painter{rect = gui.mkdims_wh(0, 0, 50, 30)}

    expect.eq(painter.x, 0)
    expect.eq(painter.y, 0)
    expect.true_(painter:isValidPos())
end

function test.Painter_cursor()
    local painter = gui.Painter{rect = gui.mkdims_wh(10, 10, 50, 30)}

    local cx, cy = painter:cursor()
    expect.eq(cx, 0)
    expect.eq(cy, 0)

    painter:seek(5, 10)
    cx, cy = painter:cursor()
    expect.eq(cx, 5)
    expect.eq(cy, 10)
end

function test.Painter_seek()
    local painter = gui.Painter{rect = gui.mkdims_wh(0, 0, 50, 30)}

    painter:seek(10, 20)
    expect.eq(painter.x, 10)
    expect.eq(painter.y, 20)

    painter:seek(5) -- Only x
    expect.eq(painter.x, 5)
    expect.eq(painter.y, 20) -- y unchanged
end

function test.Painter_advance()
    local painter = gui.Painter{rect = gui.mkdims_wh(0, 0, 50, 30)}

    painter:advance(10, 5)
    expect.eq(painter.x, 10)
    expect.eq(painter.y, 5)

    painter:advance(5) -- Only x
    expect.eq(painter.x, 15)
    expect.eq(painter.y, 5) -- y unchanged
end

function test.Painter_newline()
    local painter = gui.Painter{rect = gui.mkdims_wh(0, 0, 50, 30)}

    painter:seek(10, 5)
    painter:newline(3)

    expect.eq(painter.x, 3)
    expect.eq(painter.y, 6)
end

function test.Painter_viewport()
    local painter = gui.Painter{rect = gui.mkdims_wh(0, 0, 100, 100)}
    local viewport_painter = painter:viewport(10, 10, 20, 20)

    expect.eq(viewport_painter.x, 10)
    expect.eq(viewport_painter.y, 10)
    expect.eq(viewport_painter.width, 20)
    expect.eq(viewport_painter.height, 20)
end

function test.View_basic()
    local view = gui.View{}

    expect.table_eq(view.subviews, {})
    expect.eq(view.focus, false)
    expect.eq(#view.focus_group, 1)
end

function test.View_addviews()
    local parent = gui.View{}
    local child1 = gui.View{view_id = 'child1'}
    local child2 = gui.View{view_id = 'child2'}

    parent:addviews({child1, child2})

    expect.eq(#parent.subviews, 2)
    expect.eq(parent.subviews.child1, child1)
    expect.eq(parent.subviews.child2, child2)
    expect.eq(child1.parent_view, parent)
    expect.eq(child2.parent_view, parent)
end

function test.View_getPreferredFocusState()
    local view = gui.View{}
    expect.false_(view:getPreferredFocusState())
end

function test.View_setFocus()
    local view = gui.View{}

    expect.false_(view.focus)
    view:setFocus(true)
    expect.true_(view.focus)
    view:setFocus(false)
    expect.false_(view.focus)
end

function test.View_assign()
    local view = gui.View{}
    view:assign({custom_field = 'value', another_field = 42})

    expect.eq(view.custom_field, 'value')
    expect.eq(view.another_field, 42)
end

function test.View_callback()
    local view = gui.View{}
    function view:test_method(arg)
        return arg * 2
    end

    local cb = view:callback('test_method')
    expect.eq(cb(5), 10)
end

function test.View_cb_getfield()
    local view = gui.View{test_field = 'test_value'}
    local getter = view:cb_getfield('test_field')

    expect.eq(getter(), 'test_value')
end

function test.View_cb_setfield()
    local view = gui.View{test_field = 'initial'}
    local setter = view:cb_setfield('test_field')

    setter('updated')
    expect.eq(view.test_field, 'updated')
end

function test.compute_frame_rect()
    local rect = gui.compute_frame_rect(100, 50, {w = 80, h = 40})

    expect.eq(rect.width, 80)
    expect.eq(rect.height, 40)
    expect.ge(rect.x1, 0)
    expect.ge(rect.y1, 0)
end

function test.blink_visible()
    -- Test that blink_visible returns boolean
    local result = gui.blink_visible(100)
    expect.eq(type(result), 'boolean')
end
