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
    e:onInput{CUSTOM_HOME=true}
    expect.eq(1, e.cursor, 'cursor should be at beginning of string')
    e:onInput{CUSTOM_CTRL_RIGHT=true}
    expect.eq(5, e.cursor, 'goto end of current word')
    e:onInput{CUSTOM_END=true}
    expect.eq(16, e.cursor, 'cursor should be at end of string')
    e:onInput{CUSTOM_CTRL_LEFT=true}
    expect.eq(10, e.cursor, 'goto beginning of current word')
end

function test.editfield_click()
    local e = widgets.EditField{text='word'}
    e:setFocus(true)
    expect.eq(5, e.cursor)

    local text_area_content = e.text_area.text_area

    mock.patch(text_area_content, 'getMousePos', mock.func(0, 0), function()
        e:onInput{_MOUSE_L_DOWN=true, _MOUSE_L=true}
        e:onInput{_MOUSE_L_DOWN=true}
        expect.eq(1, e.cursor)
    end)

    mock.patch(text_area_content, 'getMousePos', mock.func(20, 0), function()
        e:onInput{_MOUSE_L_DOWN=true, _MOUSE_L=true}
        e:onInput{_MOUSE_L_DOWN=true}
        expect.eq(5, e.cursor, 'should only seek to end of text')
    end)

    mock.patch(text_area_content, 'getMousePos', mock.func(2, 0), function()
        e:onInput{_MOUSE_L_DOWN=true, _MOUSE_L=true}
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
