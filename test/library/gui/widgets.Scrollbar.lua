config.target = 'core'

local gui = require('gui')
local widgets = require('gui.widgets')

function test.update()
    local s = widgets.Scrollbar{}
    s.frame_body = {height=100} -- give us some space to work with

    -- initial defaults
    expect.eq(1, s.top_elem)
    expect.eq(1, s.elems_per_page)
    expect.eq(1, s.num_elems)
    expect.eq(0, s.bar_offset)
    expect.eq(2, s.bar_height)

    -- top_elem, elems_per_page, num_elems
    s:update(1, 10, 0)
    expect.eq(1, s.top_elem)
    expect.eq(10, s.elems_per_page)
    expect.eq(0, s.num_elems)
    expect.eq(0, s.bar_offset)
    expect.eq(2, s.bar_height)

    -- first 10 of 50 shown
    s:update(1, 10, 50)
    expect.eq(1, s.top_elem)
    expect.eq(10, s.elems_per_page)
    expect.eq(50, s.num_elems)
    expect.eq(0, s.bar_offset)
    expect.eq(19, s.bar_height)

    -- bottom 10 of 50 shown
    s:update(41, 10, 50)
    expect.eq(41, s.top_elem)
    expect.eq(10, s.elems_per_page)
    expect.eq(50, s.num_elems)
    expect.eq(79, s.bar_offset)
    expect.eq(19, s.bar_height)

    -- ~middle 10 of 50 shown
    s:update(23, 10, 50)
    expect.eq(23, s.top_elem)
    expect.eq(10, s.elems_per_page)
    expect.eq(50, s.num_elems)
    expect.eq(44, s.bar_offset)
    expect.eq(19, s.bar_height)
end

function test.onInput()
    local spec = nil
    local mock_on_scroll = function(scroll_spec) spec = scroll_spec end
    local s = widgets.Scrollbar{on_scroll=mock_on_scroll}
    s.frame_body = {height=100} -- give us some space to work with
    local y = nil
    s.getMousePos = function() return 0, y end

    -- put scrollbar somewhere in the middle so we can click above and below it
    s:update(23, 10, 50)

    expect.false_(s:onInput{}, 'no mouse down')
    expect.false_(s:onInput{_MOUSE_L=true}, 'no y coord')

    spec, y = nil, 0
    expect.true_(s:onInput{_MOUSE_L=true})
    expect.eq('up_small', spec, 'on up arrow')

    spec, y = nil, 1
    expect.true_(s:onInput{_MOUSE_L=true})
    expect.eq('up_large', spec, 'on body above bar')

    spec, y = nil, 44
    expect.true_(s:onInput{_MOUSE_L=true})
    expect.eq('up_large', spec, 'on body just above bar')

    spec, y = nil, 45
    expect.true_(s:onInput{_MOUSE_L=true})
    expect.nil_(spec, 'on top of bar')

    spec, y = nil, 63
    expect.true_(s:onInput{_MOUSE_L=true})
    expect.nil_(spec, 'on bottom of bar')

    spec, y = nil, 64
    expect.true_(s:onInput{_MOUSE_L=true})
    expect.eq('down_large', spec, 'on body just below bar')

    spec, y = nil, 98
    expect.true_(s:onInput{_MOUSE_L=true})
    expect.eq('down_large', spec, 'on body below bar')

    spec, y = nil, 99
    expect.true_(s:onInput{_MOUSE_L=true})
    expect.eq('down_small', spec, 'on down arrow')
end
