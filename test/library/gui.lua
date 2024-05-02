config.target = 'core'

local gui = require('gui')

function test.getKeyDisplay()
    expect.eq(gui.getKeyDisplay(df.interface_key.CUSTOM_A), 'a')
    expect.eq(gui.getKeyDisplay('CUSTOM_A'), 'a')
    expect.eq(gui.getKeyDisplay(df.interface_key._first_item - 1), '?')
    expect.eq(gui.getKeyDisplay(df.interface_key._last_item + 1), '?')
    expect.eq(gui.getKeyDisplay(df.interface_key.KEYBINDING_COMPLETE), '?')
end

function test.clear_pen()
    local expected_tile = dfhack.screen.inGraphicsMode() and
        df.global.init.texpos_border_interior or df.global.init.classic_texpos_border_interior

    expect.table_eq(gui.CLEAR_PEN, {
        tile = expected_tile,
        ch = string.byte(' '),
        fg = COLOR_BLACK,
        bg = COLOR_BLACK,
        bold = false,
        tile_color = false,
        write_to_lower = true,
    })
end

WantsFocusView = defclass(WantsFocusView, gui.View)
function WantsFocusView:getPreferredFocusState()
    return true
end

function test.view_wants_focus()
    local parent = gui.View()
    expect.false_(parent.focus)

    -- expect first (regular) child to not get focus
    local regular_child = gui.View()
    expect.false_(regular_child.focus)
    expect.ne(parent.focus_group, regular_child.focus_group)
    parent:addviews{regular_child}
    expect.false_(regular_child.focus)
    expect.eq(parent.focus_group, regular_child.focus_group)

    -- the first child who wants focus gets it
    local focus_child = WantsFocusView()
    expect.false_(focus_child.focus)
    parent:addviews{focus_child}
    expect.true_(focus_child.focus)
    expect.eq(parent.focus_group.cur, focus_child)

    -- the second child who wants focus doesn't
    local focus_child2 = WantsFocusView()
    parent:addviews{focus_child2}
    expect.false_(focus_child2.focus)
    expect.eq(parent.focus_group.cur, focus_child)
end

function test.inherit_focus_from_subview()
    local parent = gui.View()
    local regular_child = gui.View()
    local focus_child = WantsFocusView()
    regular_child:addviews{focus_child}
    expect.true_(focus_child.focus)
    parent:addviews{regular_child}
    expect.eq(parent.focus_group.cur, focus_child)
end

function test.subviews_negotiate_focus()
    local parent = gui.View()
    local regular_child = gui.View()
    local regular_child2 = gui.View()
    local focus_child = WantsFocusView()
    local focus_child2 = WantsFocusView()
    regular_child:addviews{focus_child}
    regular_child2:addviews{focus_child2}
    expect.true_(focus_child.focus)
    expect.true_(focus_child2.focus)
    expect.ne(regular_child.focus_group, regular_child2.focus_group)
    parent:addviews{regular_child}
    expect.eq(parent.focus_group.cur, focus_child)
    expect.true_(focus_child.focus)
    expect.true_(focus_child2.focus)
    parent:addviews{regular_child2}
    expect.eq(parent.focus_group.cur, focus_child)
    expect.eq(regular_child.focus_group, regular_child2.focus_group)
    expect.true_(focus_child.focus)
    expect.false_(focus_child2.focus)
end

MockInputView = defclass(MockInputView, gui.View)
function MockInputView:onInput(keys)
    self.mock(keys)
    MockInputView.super.onInput(self, keys)
    return true
end

local function reset_child_mocks(parent)
    for _,child in ipairs(parent.subviews) do
        child.mock = mock.func()
        reset_child_mocks(child)
    end
end

-- verify that input got routed as expected
local function test_children(expected, parent)
    local children = parent.subviews
    for i,val in ipairs(expected) do
        expect.eq(val, children[i].mock.call_count, 'child '..i)
    end
end

function test.keyboard_follows_focus()
    local parent = gui.View()
    local regular_child = MockInputView{}
    local regular_child2 = MockInputView{}
    local last_child = MockInputView{}
    parent:addviews{regular_child, regular_child2, last_child}

    reset_child_mocks(parent)
    parent:onInput({'a'})
    test_children({0,0,1}, parent)

    regular_child:setFocus(true)
    reset_child_mocks(parent)
    parent:onInput({'a'})
    test_children({1,0,0}, parent)

    regular_child2:setFocus(true)
    reset_child_mocks(parent)
    parent:onInput({'a'})
    test_children({0,1,0}, parent)

    regular_child2:setFocus(false)
    reset_child_mocks(parent)
    parent:onInput({'a'})
    test_children({0,0,1}, parent)
end

function test.one_callback_on_double_focus()
    local on_focus = mock.func()
    local view = gui.View{on_focus=on_focus}
    expect.eq(0, on_focus.call_count)
    view:setFocus(true)
    expect.eq(1, on_focus.call_count)
    view:setFocus(true)
    expect.eq(1, on_focus.call_count)
end

function test.one_callback_on_double_unfocus()
    local on_unfocus = mock.func()
    local view = gui.View{on_unfocus=on_unfocus}
    expect.eq(0, on_unfocus.call_count)
    view:setFocus(false)
    expect.eq(0, on_unfocus.call_count)
    view:setFocus(true)
    expect.eq(0, on_unfocus.call_count)
    view:setFocus(false)
    expect.eq(1, on_unfocus.call_count)
    view:setFocus(false)
    expect.eq(1, on_unfocus.call_count)
end

function test.no_input_when_focus_owner_is_hidden()
    local parent = gui.View()
    local child1 = MockInputView()
    local child2 = MockInputView()
    parent:addviews{child1, child2}
    child1:setFocus(true)
    child1.visible = false
    reset_child_mocks(parent)
    parent:onInput({'a'})
    test_children({0,1}, parent)
end

function test.no_input_when_ancestor_is_hidden()
    local grandparent = gui.View()
    local parent = MockInputView()
    local child1 = MockInputView()
    local child2 = MockInputView()
    grandparent:addviews{parent}
    parent:addviews{child1, child2}
    child1:setFocus(true)
    parent.visible = false
    reset_child_mocks(grandparent)
    grandparent:onInput({'a'})
    test_children({0}, grandparent)
    test_children({0,0}, parent)
end

function test.no_input_loop_in_children_of_focus_owner()
    local grandparent = gui.View()
    local parent = MockInputView()
    local child = MockInputView()
    grandparent:addviews{parent}
    parent:addviews{child}
    parent:setFocus(true)
    reset_child_mocks(grandparent)
    child:onInput({'a'})
    test_children({0}, grandparent)
    test_children({1}, parent)
end
