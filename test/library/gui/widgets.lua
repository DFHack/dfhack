config.target = 'core'

local widgets = require('gui.widgets')

function test.hotkeylabel_click()
    local func = mock.func()
    local l = widgets.HotkeyLabel{key='SELECT', on_activate=func}

    mock.patch(l, 'getMousePos', mock.func(0), function()
            l:onInput{_MOUSE_L=true}
            expect.eq(1, func.call_count)
        end)
end

function test.togglehotkeylabel()
    local toggle = widgets.ToggleHotkeyLabel{}
    expect.true_(toggle:getOptionValue())
    toggle:cycle()
    expect.false_(toggle:getOptionValue())
    toggle:cycle()
    expect.true_(toggle:getOptionValue())
end

function test.togglehotkeylabel_default_value()
    local toggle = widgets.ToggleHotkeyLabel{initial_option=2}
    expect.false_(toggle:getOptionValue())

    toggle = widgets.ToggleHotkeyLabel{initial_option=false}
    expect.false_(toggle:getOptionValue())
end

function test.togglehotkeylabel_click()
    local l = widgets.ToggleHotkeyLabel{}
    expect.true_(l:getOptionValue())
    mock.patch(l, 'getMousePos', mock.func(0), function()
            l:onInput{_MOUSE_L=true}
            expect.false_(l:getOptionValue())
        end)
end
