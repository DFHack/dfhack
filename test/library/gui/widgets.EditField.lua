config.target = 'core'

local widgets = require('gui.widgets')

function test.editfield_cursor()
    local e = widgets.EditField{}
    e:setFocus(true)
    expect.eq(1, e.cursor, 'cursor should be after the empty string')

    e:onInput{_STRING=string.byte('a')}
    expect.eq('a', e.text)
    expect.eq(2, e.cursor)

    e:setText('one two three')
    expect.eq(14, e.cursor, 'cursor should be after the last char')
    e:onInput{_STRING=string.byte('s')}
    expect.eq('one two threes', e.text)
    expect.eq(15, e.cursor)

    e:setCursor(4)
    e:onInput{_STRING=string.byte('s')}
    expect.eq('ones two threes', e.text)
    expect.eq(5, e.cursor)

    e:onInput{KEYBOARD_CURSOR_LEFT=true}
    expect.eq(4, e.cursor)
    e:onInput{KEYBOARD_CURSOR_RIGHT=true}
    expect.eq(5, e.cursor)
    -- e:onInput{A_CARE_MOVE_W=true}
    -- expect.eq(1, e.cursor, 'interpret alt-left as home') -- uncomment when we have a home key
    e:onInput{CUSTOM_CTRL_F=true}
    expect.eq(6, e.cursor, 'interpret Ctrl-f as goto beginning of next word')
    e:onInput{CUSTOM_CTRL_E=true}
    expect.eq(16, e.cursor, 'interpret Ctrl-e as end')
    e:onInput{CUSTOM_CTRL_B=true}
    expect.eq(9, e.cursor, 'interpret Ctrl-b as goto end of previous word')
end

function test.editfield_click()
    local e = widgets.EditField{text='word'}
    e:setFocus(true)
    expect.eq(5, e.cursor)

    mock.patch(e, 'getMousePos', mock.func(0), function()
            e:onInput{_MOUSE_L_DOWN=true}
            expect.eq(1, e.cursor)
        end)

    mock.patch(e, 'getMousePos', mock.func(20), function()
            e:onInput{_MOUSE_L_DOWN=true}
            expect.eq(5, e.cursor, 'should only seek to end of text')
        end)

    mock.patch(e, 'getMousePos', mock.func(2), function()
            e:onInput{_MOUSE_L_DOWN=true}
            expect.eq(3, e.cursor)
        end)
end

function test.editfield_ignore_keys()
    local e = widgets.EditField{ignore_keys={'CUSTOM_B', 'CUSTOM_C'}}
    e:setFocus(true)

    e:onInput{_STRING=string.byte('a'), CUSTOM_A=true}
    expect.eq('a', e.text, '"a" should be accepted')
    e:onInput{_STRING=string.byte('b'), CUSTOM_B=true}
    expect.eq('a', e.text, '"b" should be rejected')
    e:onInput{_STRING=string.byte('c'), CUSTOM_C=true}
    expect.eq('a', e.text, '"c" should be rejected')
    e:onInput{_STRING=string.byte('d'), CUSTOM_D=true}
    expect.eq('ad', e.text, '"d" should be accepted')

end
