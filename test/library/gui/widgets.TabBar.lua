config.target = 'core'

local gui = require('gui')
local widgets = require('gui.widgets')
local mock = require('test_util.mock')

local fs = defclass(nil, gui.FramedScreen)
fs.ATTRS = {
    frame_style = gui.GREY_LINE_FRAME,
    frame_title = 'TestFramedScreen',
    frame_inset = 0,
    focus_path = 'test-framed-screen',
}

---@class SetupArgs
---@field frame_width number | nil
---@field frame_height number | nil
---@field wrap boolean | nil
---@field selected number | nil
---@field labels string[] | nil

local DEFAULT_FRAME_WIDTH = 20
local DEFAULT_FRAME_HEIGHT = 20
local DEFAULT_WRAP = false
local DEFAULT_LABELS = {'Foo', 'Bar', 'Baz', 'Qux'}
local DEFAULT_SELECTED = 1

---@param args SetupArgs | nil
local function setup(args)
    args = args or {
        frame_width = DEFAULT_FRAME_WIDTH,
        frame_height = DEFAULT_FRAME_HEIGHT,
        wrap = DEFAULT_WRAP,
        labels = DEFAULT_LABELS,
        selected = DEFAULT_SELECTED,
    }

    local wrap = args.wrap or DEFAULT_WRAP
    local frame_width = args.frame_width or DEFAULT_FRAME_WIDTH
    local frame_height = args.frame_height or DEFAULT_FRAME_HEIGHT
    local labels = args.labels or DEFAULT_LABELS
    local selected = args.selected or DEFAULT_SELECTED

    local panel = Panel{
        frame = {t=0},
        subviews={
            widgets.TabBar {
                frame = {t=0},
                labels = labels,
                wrap = wrap,
                key = 'CUSTOM_ALT_S',
                key_back = 'CUSTOM_ALT_A',
                on_select=function(idx) selected = idx end,
                get_cur_page=function() return selected end,
            },
        }
    }

    function fs:init()
        self:addviews{
            panel
        }
    end

    local framed_screen = fs{
        frame_width = frame_width,
        frame_height = frame_height,
    }
    local tab_bar = framed_screen.subviews[1].subviews[1]
    return tab_bar, framed_screen
end

function test.tabsElement()
    local tb = setup()
    local tabsElement = tb:tabsElement()

    local tabsElementSubviewCount = #tabsElement.subviews

    expect.eq(tabsElementSubviewCount, 4, 'tabsElement should have 4 subviews (one for each Tab)')

    for i = 1, tabsElementSubviewCount do
        expect.eq(tabsElement.subviews[i].label, tb.labels[i], 'tabsElement subview text should match label')
    end
end

function test.scroll_elements_do_not_exist_when_wrap_is_true()
    local tb = setup({wrap = true})

    expect.eq(tb:scrollLeftElement(), nil, 'scroll elements should exist when wrap is true')
    expect.eq(tb:scrollRightElement(), nil, 'scroll elements should exist when wrap is true')
end

function test.tabs_element_frame_height_increases_with_number_of_tab_rows()
    local tb = setup({wrap = true})

    expect.eq(tb:tabsElement().frame.h, 4, 'tabsElement height should be equal to the number of rows to fit all tabs * 2')

    tb = setup({
        wrap = true,
        labels = {'Foo', 'Bar', 'Baz', 'Qux', 'Foo2', 'Bar2', 'Baz2', 'Qux2', 'Foo3', 'Bar3', 'Baz3', 'Qux3'},
    })

    expect.eq(tb:tabsElement().frame.h, 12, 'tabsElement height should be equal to the number of rows to fit all tabs * 2')
end

function test.tab_on_select_called_when_tab_is_clicked()
    local tb = setup()
    local secondTab = tb:tabsElement().subviews[2]

    secondTab.on_select = mock.observe_func(secondTab.on_select)
    secondTab.getMousePos = function() return {x=0, y=0} end

    secondTab:onInput({_MOUSE_L = true, _MOUSE_L_DOWN = true})

    expect.eq(secondTab.on_select.call_count, 1, 'on_select should be called when tab is clicked')
end

function test.tab_on_select_not_called_without_mouse_pos()
    local tb = setup()
    local secondTab = tb:tabsElement().subviews[2]

    secondTab.on_select = mock.observe_func(secondTab.on_select)
    secondTab.getMousePos = function() return nil end

    secondTab:onInput({_MOUSE_L = true, _MOUSE_L_DOWN = true})

    expect.eq(secondTab.on_select.call_count, 0, 'on_select should not be called when getMousePos returns nil')
end

local TO_THE_RIGHT = string.char(16)
local TO_THE_LEFT = string.char(17)

function test.scrollLeftElement()
    local tb = setup()
    local scrollLeftElement = tb:scrollLeftElement()

    expect.eq(scrollLeftElement.text, TO_THE_LEFT, 'scrollLeftElement should have text "' .. TO_THE_LEFT .. '"')
end

function test.scrollRightElement()
    local tb = setup()
    local scrollRightElement = tb:scrollRightElement()

    expect.eq(scrollRightElement.text, TO_THE_RIGHT, 'scrollRightElement should have text "' .. TO_THE_RIGHT .. '"')
end

function test.leftScrollVisible()
    local tb = setup()

    tb.scroll_offset = 0

    expect.eq(tb:leftScrollVisible(), false, 'leftScrollVisible should return false when scroll_offset is 0')

    tb.scroll_offset = -1
    expect.eq(tb:leftScrollVisible(), true, 'leftScrollVisible should return true when scroll_offset is less than 0')
end

function test.rightScrollVisible()
    local tb = setup()

    tb.scroll_offset = 0
    tb.offset_to_show_last_tab = 50

    expect.eq(tb:rightScrollVisible(), false, 'rightScrollVisible should return false when scroll_offset is 0')

    tb.scroll_offset = 51
    expect.eq(tb:rightScrollVisible(), true, 'rightScrollVisible should return true when scroll_offset is greater than offset_to_show_last_tab')
end

function test.showScrollRight()
    local tb = setup()

    tb.scroll_offset = 0
    tb.offset_to_show_last_tab = 50
    tb:showScrollRight()

    expect.eq(tb:scrollRightElement().visible, false, 'scroll right element should not be visible')

    tb.scroll_offset = 51
    tb:showScrollRight()

    expect.eq(tb:scrollRightElement().visible, true, 'scroll right element should be visible')
end

function test.showScrollLeft()
    local tb = setup()

    tb.scroll_offset = 0
    tb:showScrollLeft()

    expect.eq(tb:scrollLeftElement().visible, false, 'scroll left element should not be visible')

    tb.scroll_offset = -1
    tb:showScrollLeft()

    expect.eq(tb:scrollLeftElement().visible, true, 'scroll left element should be visible')
end

function test.updateTabPanelPosition()
    local tb = setup()

    tb.scroll_offset = 0
    tb:tabsElement().updateLayout = mock.observe_func(tb:tabsElement().updateLayout)
    tb:updateTabPanelPosition()

    expect.eq(tb:tabsElement().frame_inset.l, tb.scroll_offset, 'frame_inset.l should be equal to scroll_offset after updateTabPanelPosition')
    expect.eq(tb:tabsElement().updateLayout.call_count, 1, 'updateTabPanelPosition should call updateLayout')

    tb.scroll_offset = -50
    tb:tabsElement().updateLayout = mock.observe_func(tb:tabsElement().updateLayout)
    tb:updateTabPanelPosition()

    expect.eq(tb:tabsElement().frame_inset.l, tb.scroll_offset, 'frame_inset.l should be equal to scroll_offset after updateTabPanelPosition')
    expect.eq(tb:tabsElement().updateLayout.call_count, 1, 'updateTabPanelPosition should call updateLayout')
end

function test.updateScrollElements()
    local tb = setup()

    tb.showScrollLeft = mock.observe_func(tb.showScrollLeft)
    tb.showScrollRight = mock.observe_func(tb.showScrollRight)
    tb.updateTabPanelPosition = mock.observe_func(tb.updateTabPanelPosition)

    tb:updateScrollElements()

    expect.eq(tb.showScrollLeft.call_count, 1, 'updateScrollElements should call showScrollLeft')
    expect.eq(tb.showScrollRight.call_count, 1, 'updateScrollElements should call showScrollRight')
    expect.eq(tb.updateTabPanelPosition.call_count, 1, 'updateScrollElements should call updateTabPanelPosition')
end

function test.scrollLeft()
    local tb = setup()
    tb.offset_to_show_last_tab = -100
    tb.scroll_step = 25

    tb.scroll_offset = -50
    tb:showScrollLeft()

    tb.updateScrollElements = mock.observe_func(tb.updateScrollElements)
    tb:scrollLeft()

    expect.eq(tb.scroll_offset, -25, 'scroll left should increase scroll offset by scroll step')
    expect.eq(tb.updateScrollElements.call_count, 1, 'scroll left should call updateScrollElements')
end

function test.scrollRight()
    local tb = setup()
    tb.offset_to_show_last_tab = -100
    tb.scroll_step = 25

    tb.scroll_offset = -50
    tb:updateScrollElements()

    tb.updateScrollElements = mock.observe_func(tb.updateScrollElements)
    tb.updateTabPanelPosition = mock.observe_func(tb.updateTabPanelPosition)
    tb:scrollRight()

    expect.eq(tb.scroll_offset, -75, 'scroll right should decrease scroll offset by scroll step')
    expect.eq(tb.updateScrollElements.call_count, 1, 'scroll right should call updateScrollElements')
end

function test.capScrollOffset()
    local tb = setup()
    tb.offset_to_show_last_tab = -100

    tb.scroll_offset = -50
    tb:capScrollOffset()

    expect.eq(tb.scroll_offset, -50, 'capScrollOffset should not change scroll offset when it is within bounds')

    tb.scroll_offset = -101
    tb:capScrollOffset()

    expect.eq(tb.scroll_offset, -100, 'capScrollOffset should set scroll offset to -100 when it is less than -100')

    tb.scroll_offset = 0
    tb:capScrollOffset()

    expect.eq(tb.scroll_offset, 0, 'capScrollOffset should not change scroll offset when it is 0')

    tb.scroll_offset = 1
    tb:capScrollOffset()

    expect.eq(tb.scroll_offset, 0, 'capScrollOffset should set scroll offset to 0 when it is greater than 0')
end

function test.scrollTabIntoView()
    local tb = setup()

    tb.scroll_step = 10

    tb:scrollTabIntoView(1)
    expect.eq(tb.scroll_offset, 0, 'scrollTabIntoView should not change scroll offset when tab is already in view')

    tb:scrollTabIntoView(2)
    expect.eq(tb.scroll_offset, 0, 'scrollTabIntoView should not change scroll offset when tab is already in view')

    tb:scrollTabIntoView(4)
    expect.eq(tb.scroll_offset, tb.offset_to_show_last_tab, 'scrollTabIntoView should scroll to the right when tab is to the right of the view')

    tb:scrollTabIntoView(3)
    expect.eq(tb.scroll_offset, tb.offset_to_show_last_tab, 'scrollTabIntoView should not change scroll offset when tab is already in view')

    tb:scrollTabIntoView(1)
    expect.eq(tb.scroll_offset, 0, 'scrollTabIntoView should scroll to the left when tab is to the left of the view')
end

function test.selected_tab_scrolled_into_view_on_first_render()
    local tb = setup({selected=4})

    expect.eq(tb.scroll_offset, tb.offset_to_show_last_tab,
        'scroll offset should be set to offset_to_show_last_tab on first render to ensure current tab is visible'
    )

    tb = setup({selected=1})

    expect.eq(tb.scroll_offset, 0,
        'scroll offset should be set to 0 on first render to ensure current tab is visible'
    )

    tb = setup({selected=3})

    expect.gt(tb.scroll_offset, tb.offset_to_show_last_tab,
        'scroll offset should be greather than offset_to_show_last_tab'
    )
    expect.lt(tb.scroll_offset, 0,
        'scroll offset should be less than 0'
    )
end

function test.fastStep()
    local tb = setup()

    tb.scroll_step = 10
    tb.fast_scroll_modifier = 2
    expect.eq(tb:fastStep(), 20, 'fastStep should return scroll_step * fast_scroll_modifier')

    tb.scroll_step = 5
    tb.fast_scroll_modifier = 3
    expect.eq(tb:fastStep(), 15, 'fastStep should return scroll_step * fast_scroll_modifier')
end

function test.scroll_controls_do_nothing_when_wrap_true()
    local tb = setup({wrap = true})

    tb.scroll_offset = tb.offset_to_show_last_tab
    local current_scroll_offset = tb.scroll_offset

    tb:onInput({
        CONTEXT_SCROLL_UP = true,
        CONTEXT_SCROLL_DOWN = true,
        CONTEXT_SCROLL_PAGEUP= true,
        CONTEXT_SCROLL_PAGEDOWN = true,
        [tb.scroll_key] = true,
        [tb.scroll_key_back] = true,
    })

    expect.eq(tb.scroll_offset, current_scroll_offset, 'scroll offset should not change when wrap is true')
end

function test.mouse_scroll_up_requires_mouse_focus()
    local tb = setup()

    tb.scroll_offset = tb.offset_to_show_last_tab
    tb:updateScrollElements()
    local current_scroll_offset = tb.scroll_offset

    tb.isMouseOver = mock.func(false)
    tb:onInput({CONTEXT_SCROLL_UP = true})

    expect.eq(tb.scroll_offset, current_scroll_offset, 'scroll offset should not change if the mouse is not over the tab bar')

    tb.isMouseOver = mock.func(true)
    tb:onInput({CONTEXT_SCROLL_UP = true})

    expect.gt(tb.scroll_offset, current_scroll_offset, 'scroll offset should increase if the mouse is over the tab bar')
end

function test.shift_mouse_scroll_up_requires_mouse_focus()
    local tb = setup()

    tb.scroll_offset = tb.offset_to_show_last_tab
    tb:updateScrollElements()
    local current_scroll_offset = tb.scroll_offset

    tb.isMouseOver = mock.func(false)
    tb:onInput({CONTEXT_SCROLL_PAGEUP = true})

    expect.eq(tb.scroll_offset, current_scroll_offset, 'scroll offset should not change if the mouse is not over the tab bar')

    tb.isMouseOver = mock.func(true)
    tb:onInput({CONTEXT_SCROLL_PAGEUP = true})

    expect.gt(tb.scroll_offset, current_scroll_offset, 'scroll offset should increase if the mouse is over the tab bar')
end

function test.mouse_scroll_down_requires_mouse_focus()
    local tb = setup()

    tb.scroll_offset = 0
    tb:updateScrollElements()
    local current_scroll_offset = tb.scroll_offset

    tb.isMouseOver = mock.func(false)
    tb:onInput({CONTEXT_SCROLL_DOWN = true})

    expect.eq(tb.scroll_offset, current_scroll_offset, 'scroll offset should not change if the mouse is not over the tab bar')

    tb.isMouseOver = mock.func(true)
    tb:onInput({CONTEXT_SCROLL_DOWN = true})

    expect.lt(tb.scroll_offset, current_scroll_offset, 'scroll offset should decrease if the mouse is over the tab bar')
end

function test.shift_mouse_scroll_down_requires_mouse_focus()
    local tb = setup()

    tb.scroll_offset = 0
    tb:updateScrollElements()
    local current_scroll_offset = tb.scroll_offset

    tb.isMouseOver = mock.func(false)
    tb:onInput({CONTEXT_SCROLL_PAGEDOWN = true})

    expect.eq(tb.scroll_offset, current_scroll_offset, 'scroll offset should not change if the mouse is not over the tab bar')

    tb.isMouseOver = mock.func(true)
    tb:onInput({CONTEXT_SCROLL_PAGEDOWN = true})

    expect.lt(tb.scroll_offset, current_scroll_offset, 'scroll offset should decrease if the mouse is over the tab bar')
end

function test.scroll_key_should_scroll_right()
    local tb = setup()

    tb.scroll_offset = 0
    tb:updateScrollElements()
    local current_scroll_offset = tb.scroll_offset

    tb:onInput({[tb.scroll_key] = true})

    expect.lt(tb.scroll_offset, current_scroll_offset, 'scroll offset should decrease when scroll key is pressed')
end

function test.scroll_key_back_should_scroll_left()
    local tb = setup()

    tb.scroll_offset = tb.offset_to_show_last_tab
    tb:updateScrollElements()
    local current_scroll_offset = tb.scroll_offset

    tb:onInput({[tb.scroll_key_back] = true})

    expect.gt(tb.scroll_offset, current_scroll_offset, 'scroll offset should increase when scroll key back is pressed')
end

function test.scrollable_is_false_when_wrap_is_true()
    local tb = setup({wrap = true})

    expect.eq(tb.scrollable, false, 'scrollable should return false when wrap is true')
end

function test.scrollable_is_false_when_all_tabs_fit_and_wrap_is_false()
    local tb = setup({
        frame_width = 100,
        frame_height = 100,
    })

    expect.eq(tb.scrollable, false, 'scrollable should return false when all tabs fit in the frame')
end

function test.scrollable_is_true_when_tabs_do_not_fit_and_wrap_is_false()
    local tb = setup()

    expect.eq(tb.scrollable, true, 'scrollable should return true when tabs do not fit in the frame')
end

function test.key_should_select_next_tab()
    local tb = setup({wrap=false})

    tb:onInput({[tb.key] = true})

    expect.eq(tb:get_cur_page(), 2, 'key should select the next tab')
end

function test.key_back_should_select_previous_tab()
    local tb = setup({wrap=false})

    tb:onInput({[tb.key_back] = true})

    expect.eq(tb:get_cur_page(), 4, 'key back should select the previous tab')
end

function test.key_should_wrap_to_first_tab_when_on_last_tab()
    local tb = setup({wrap=false, selected=4})

    tb:onInput({[tb.key] = true})

    expect.eq(tb:get_cur_page(), 1, 'key should wrap to the first tab when on the last tab')
end

function test.key_back_should_wrap_to_last_tab_when_on_first_tab()
    local tb = setup({wrap=false, selected=1})

    tb:onInput({[tb.key_back] = true})

    expect.eq(tb:get_cur_page(), 4, 'key back should wrap to the last tab when on the first tab')
end

function test.key_should_scroll_next_tab_into_view_if_necessary_when_wrap_is_false()
    local tb = setup({
        wrap=false,
        frame_width=10,
    })

    local scroll_offset_before_input = tb.scroll_offset
    tb:onInput({[tb.key] = true})

    expect.eq(tb:get_cur_page(), 2, 'key should select the next tab')
    expect.lt(tb.scroll_offset, scroll_offset_before_input, 'key should scroll the next tab into view')
end

function test.key_back_should_scroll_previous_tab_into_view_if_necessary_when_wrap_is_false()
    local tb = setup({
        selected=4,
        wrap=false,
        frame_width=10,
    })

    local scroll_offset_before_input = tb.scroll_offset
    tb:onInput({[tb.key_back] = true})

    expect.eq(tb:get_cur_page(), 3, 'key back should select the previous tab')
    expect.gt(tb.scroll_offset, scroll_offset_before_input, 'key back should scroll the previous tab into view')
end
