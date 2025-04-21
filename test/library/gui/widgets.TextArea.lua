local gui = require('gui')
local widgets = require('gui.widgets')

config.target = 'core'

local CP437_NEW_LINE = '◙'

local function simulate_input_keys(...)
    local keys = {...}
    for _,key in ipairs(keys) do
        gui.simulateInput(dfhack.gui.getCurViewscreen(true), key)
    end
end

local function simulate_input_text(text)
    local screen = dfhack.gui.getCurViewscreen(true)

    for i = 1, #text do
        local charcode = string.byte(text:sub(i,i))
        local code_key = string.format('STRING_A%03d', charcode)

        gui.simulateInput(screen, { [code_key]=true })
    end
end

local function simulate_mouse_click(element, x, y)
    local screen = dfhack.gui.getCurViewscreen(true)

    local g_x, g_y = element.frame_body:globalXY(x, y)
    df.global.gps.mouse_x = g_x
    df.global.gps.mouse_y = g_y

    if not element.frame_body:inClipGlobalXY(g_x, g_y) then
        print('--- Click outside provided element area, re-check the test')
        return
    end

    gui.simulateInput(screen, {
        _MOUSE_L=true,
        _MOUSE_L_DOWN=true,
    })
    gui.simulateInput(screen, '_MOUSE_L_DOWN')
end

local function simulate_mouse_drag(element, x_from, y_from, x_to, y_to)
    local g_x_from, g_y_from = element.frame_body:globalXY(x_from, y_from)
    local g_x_to, g_y_to = element.frame_body:globalXY(x_to, y_to)

    df.global.gps.mouse_x = g_x_from
    df.global.gps.mouse_y = g_y_from

    gui.simulateInput(dfhack.gui.getCurViewscreen(true), {
        _MOUSE_L=true,
        _MOUSE_L_DOWN=true,
    })
    gui.simulateInput(dfhack.gui.getCurViewscreen(true), '_MOUSE_L_DOWN')

    df.global.gps.mouse_x = g_x_to
    df.global.gps.mouse_y = g_y_to
    gui.simulateInput(dfhack.gui.getCurViewscreen(true), '_MOUSE_L_DOWN')
end

local function arrange_textarea(options)
    options = options or {}

    local window_width = 50
    local window_height = 50

    if options.w then
        local border_width = 2
        local scrollbar_width = options.one_line_mode and 0 or 3
        local cursor_buffor = options.one_line_mode and 0 or 1
        window_width = options.w + border_width + scrollbar_width + cursor_buffor
    end

    if options.h then
        local border_width = 2
        window_height = options.h + border_width
    end

    local screen = gui.ZScreen{}

    screen:addviews({
        widgets.Window{
            view_id='window',
            resizable=true,
            frame={w=window_width, h=window_height},
            frame_inset=0,
            subviews={
                widgets.TextArea{
                    view_id='text_area_widget',
                    init_text=options.text or '',
                    init_cursor=options.cursor or 1,
                    frame={l=0,r=0,t=0,b=0},
                    one_line_mode=options.one_line_mode,
                    on_cursor_change=options.on_cursor_change,
                    on_text_change=options.on_text_change,
                }
            }
        }
    })

    local window = screen.subviews.window
    local text_area = screen.subviews.text_area_widget.text_area
    text_area.enable_cursor_blink = false

    screen:show()
    screen:onRender()

    return text_area, screen, window, screen.subviews.text_area_widget
end

local function read_rendered_text(text_area)
    text_area.parent_view.parent_view.parent_view:onRender()

    local pen = nil
    local text = ''

    local frame_body = text_area.frame_body

    for y=frame_body.clip_y1,frame_body.clip_y2 do

        for x=frame_body.clip_x1,frame_body.clip_x2 do
            pen = dfhack.screen.readTile(x, y)

            if pen == nil or pen.ch == nil or pen.ch == 0 or pen.fg == 0 then
                break
            else
                text = text .. (pen.ch == 10 and CP437_NEW_LINE or string.char(pen.ch))
            end
        end

        text = text .. '\n'
    end

    return text:gsub("\n+$", "")
end

local function read_selected_text(text_area)
    text_area.parent_view.parent_view.parent_view:onRender()

    local pen = nil
    local text = ''

    for y=0,text_area.frame_body.height do
        local has_sel = false

        for x=0,text_area.frame_body.width do
            local g_x, g_y = text_area.frame_body:globalXY(x, y)
            pen = dfhack.screen.readTile(g_x, g_y)

            local pen_char = string.char(pen.ch)
            if pen == nil or pen.ch == nil or pen.ch == 0 then
                break
            elseif pen.bg == COLOR_CYAN then
                has_sel = true
                text = text .. pen_char
            end
        end
        if has_sel then
            text = text .. '\n'
        end
    end

    return text:gsub("\n+$", "")
end

function test.load()
    local text_area, screen = arrange_textarea()

    expect.eq(read_rendered_text(text_area), '_')

    screen:dismiss()
end

function test.load_input_multiline_text()
    local text_area, screen, window = arrange_textarea({w=80})

    local text = table.concat({
        'Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        'Pellentesque dignissim volutpat orci, sed molestie metus elementum vel.',
        'Donec sit amet mattis ligula, ac vestibulum lorem.',
    }, '\n')

    simulate_input_text(text)

    expect.eq(read_rendered_text(text_area), text .. '_')

    screen:dismiss()
end

function test.handle_numpad_numbers_as_text()
    local text_area, screen, window = arrange_textarea({w=80})

    local text = table.concat({
        'Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n')
    simulate_input_text(text)

    simulate_input_keys({
        STANDARDSCROLL_LEFT      = true,
        KEYBOARD_CURSOR_LEFT     = true,
        _STRING                  = 52,
        STRING_A052              = true,
    })

    expect.eq(read_rendered_text(text_area), text .. '4_')

    simulate_input_keys({
        STRING_A054              = true,
        STANDARDSCROLL_RIGHT     = true,
        KEYBOARD_CURSOR_RIGHT    = true,
        _STRING                  = 54,
    })

    expect.eq(read_rendered_text(text_area), text .. '46_')

    simulate_input_keys({
        KEYBOARD_CURSOR_DOWN     = true,
        STRING_A050              = true,
        _STRING                  = 50,
        STANDARDSCROLL_DOWN      = true,
    })

    expect.eq(read_rendered_text(text_area), text .. '462_')

    simulate_input_keys({
        KEYBOARD_CURSOR_UP       = true,
        STRING_A056              = true,
        STANDARDSCROLL_UP        = true,
        _STRING                  = 56,
    })

    expect.eq(read_rendered_text(text_area), text .. '4628_')
    screen:dismiss()
end

function test.wrap_text_to_available_width()
    local text_area, screen, window = arrange_textarea({w=55})

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '41: Etiam id congue urna, vel aliquet mi.',
        '45: Nam dignissim libero a interdum porttitor.',
        '73: Proin dignissim euismod augue, laoreet porttitor est pellentesque ac.',
    }, '\n')

    simulate_input_text(text)

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero.',
        '41: Etiam id congue urna, vel aliquet mi.',
        '45: Nam dignissim libero a interdum porttitor.',
        '73: Proin dignissim euismod augue, laoreet porttitor ',
        'est pellentesque ac._',
    }, '\n'));

    screen:dismiss()
end

function test.submit_new_line()
    local text_area, screen, window = arrange_textarea({w=55})

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
    }, '\n')

    simulate_input_text(text)

    simulate_input_keys('SELECT')

    simulate_input_keys('SELECT')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero.',
        '',
        '_',
    }, '\n'));

    text_area:setCursor(58)

    simulate_input_keys('SELECT')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'el',
        '_t.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero.',
        -- empty end lines are not rendered
    }, '\n'));

    text_area:setCursor(84)

    simulate_input_keys('SELECT')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'el',
        'it.',
        '112: Sed consectetur,',
        -- wrapping changed
        '_urna sit amet aliquet egestas, ante nibh porttitor ',
        'mi, vitae rutrum eros metus nec libero.',
        -- empty end lines are not rendered
    }, '\n'));

    screen:dismiss()
end

function test.submit_new_line_callbacks()
    local cursor_change = {old=nil, new=nil}
    local text_change = {old=nil, new=nil}
    local text_area, screen, window = arrange_textarea({
        w=55,
        on_cursor_change=function (_cursor, _old_cursor)
            cursor_change = {old=_old_cursor, new=_cursor}
        end,
        on_text_change=function (_text, _old_text)
            text_change = {old=_old_text, new=_text}
        end,
    })

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
    }, '\n')

    simulate_input_text(text)
    expect.table_eq(cursor_change, {new=#text + 1, old=#text})

    simulate_input_keys('SELECT')
    expect.table_eq(cursor_change, {new=#text + 2, old=#text + 1})

    expect.table_eq(text_change, {
        new=table.concat({
            '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
            '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
            ''
        }, '\n'),
        old=text
    })

    screen:dismiss()
end

function test.keyboard_arrow_up_navigation()
    local text_area, screen, window = arrange_textarea({w=55})

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '41: Etiam id congue urna, vel aliquet mi.',
        '45: Nam dignissim libero a interdum porttitor.',
        '73: Proin dignissim euismod augue, laoreet porttitor est pellentesque ac.',
    }, '\n')

    simulate_input_text(text)

    simulate_input_keys('KEYBOARD_CURSOR_UP')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero.',
        '41: Etiam id congue urna, vel aliquet mi.',
        '45: Nam dignissim libero a interdum porttitor.',
        '73: Proin dignissim _uismod augue, laoreet porttitor ',
        'est pellentesque ac.',
    }, '\n'));

    simulate_input_keys('KEYBOARD_CURSOR_UP')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero.',
        '41: Etiam id congue urna, vel aliquet mi.',
        '45: Nam dignissim li_ero a interdum porttitor.',
        '73: Proin dignissim euismod augue, laoreet porttitor ',
        'est pellentesque ac.',
    }, '\n'));

    simulate_input_keys('KEYBOARD_CURSOR_UP')
    simulate_input_keys('KEYBOARD_CURSOR_UP')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero._',
        '41: Etiam id congue urna, vel aliquet mi.',
        '45: Nam dignissim libero a interdum porttitor.',
        '73: Proin dignissim euismod augue, laoreet porttitor ',
        'est pellentesque ac.',
    }, '\n'));

    simulate_input_keys('KEYBOARD_CURSOR_UP')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor _i, vitae rutrum eros metus nec ',
        'libero.',
        '41: Etiam id congue urna, vel aliquet mi.',
        '45: Nam dignissim libero a interdum porttitor.',
        '73: Proin dignissim euismod augue, laoreet porttitor ',
        'est pellentesque ac.',
    }, '\n'));

    simulate_input_keys('KEYBOARD_CURSOR_UP')
    simulate_input_keys('KEYBOARD_CURSOR_UP')
    simulate_input_keys('KEYBOARD_CURSOR_UP')
    simulate_input_keys('KEYBOARD_CURSOR_UP')

    expect.eq(read_rendered_text(text_area), table.concat({
        '_0: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero.',
        '41: Etiam id congue urna, vel aliquet mi.',
        '45: Nam dignissim libero a interdum porttitor.',
        '73: Proin dignissim euismod augue, laoreet porttitor ',
        'est pellentesque ac.',
    }, '\n'));

    simulate_input_keys('KEYBOARD_CURSOR_DOWN')
    simulate_input_keys('KEYBOARD_CURSOR_DOWN')
    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112: Sed consectetur_ urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero.',
        '41: Etiam id congue urna, vel aliquet mi.',
        '45: Nam dignissim libero a interdum porttitor.',
        '73: Proin dignissim euismod augue, laoreet porttitor ',
        'est pellentesque ac.',
    }, '\n'));

    screen:dismiss()
end

function test.keyboard_arrow_up_navigation_callbacks()
    local cursor_change = {old=nil, new=nil}
    local text_change = {old=nil, new=nil}
    local text_area, screen, window = arrange_textarea({
        w=55,
        on_cursor_change=function (_cursor, _old_cursor)
            cursor_change = {old=_old_cursor, new=_cursor}
        end,
        on_text_change=function (_text, _old_text)
            text_change = {old=_old_text, new=_text}
        end,
    })

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '41: Etiam id congue urna, vel aliquet mi.',
        '45: Nam dignissim libero a interdum porttitor.',
        '73: Proin dignissim euismod augue, laoreet porttitor est pellentesque ac.',
    }, '\n')

    simulate_input_text(text)
    expect.table_eq(cursor_change, {new=#text + 1, old=#text})

    text_change = {old=nil, new=nil}
    simulate_input_keys('KEYBOARD_CURSOR_UP')
    expect.table_eq(text_change, {new=nil, old=nil})

    expect.table_eq(cursor_change, {new=284, old=#text + 1})

    screen:dismiss()
end

function test.keyboard_arrow_down_navigation()
    local text_area, screen, window = arrange_textarea({w=55})

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '41: Etiam id congue urna, vel aliquet mi.',
        '45: Nam dignissim libero a interdum porttitor.',
        '73: Proin dignissim euismod augue, laoreet porttitor est pellentesque ac.',
    }, '\n')

    simulate_input_text(text)
    text_area:setCursor(11)

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem _psum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero.',
        '41: Etiam id congue urna, vel aliquet mi.',
        '45: Nam dignissim libero a interdum porttitor.',
        '73: Proin dignissim euismod augue, laoreet porttitor ',
        'est pellentesque ac.',
    }, '\n'));

    simulate_input_keys('KEYBOARD_CURSOR_DOWN')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit._',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero.',
        '41: Etiam id congue urna, vel aliquet mi.',
        '45: Nam dignissim libero a interdum porttitor.',
        '73: Proin dignissim euismod augue, laoreet porttitor ',
        'est pellentesque ac.',
    }, '\n'));

    simulate_input_keys('KEYBOARD_CURSOR_DOWN')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112: Sed c_nsectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero.',
        '41: Etiam id congue urna, vel aliquet mi.',
        '45: Nam dignissim libero a interdum porttitor.',
        '73: Proin dignissim euismod augue, laoreet porttitor ',
        'est pellentesque ac.',
    }, '\n'));

    simulate_input_keys('KEYBOARD_CURSOR_DOWN')
    simulate_input_keys('KEYBOARD_CURSOR_DOWN')
    simulate_input_keys('KEYBOARD_CURSOR_DOWN')
    simulate_input_keys('KEYBOARD_CURSOR_DOWN')
    simulate_input_keys('KEYBOARD_CURSOR_DOWN')
    simulate_input_keys('KEYBOARD_CURSOR_DOWN')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero.',
        '41: Etiam id congue urna, vel aliquet mi.',
        '45: Nam dignissim libero a interdum porttitor.',
        '73: Proin dignissim euismod augue, laoreet porttitor ',
        'est pellen_esque ac.',
    }, '\n'));

    simulate_input_keys('KEYBOARD_CURSOR_DOWN')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero.',
        '41: Etiam id congue urna, vel aliquet mi.',
        '45: Nam dignissim libero a interdum porttitor.',
        '73: Proin dignissim euismod augue, laoreet porttitor ',
        'est pellentesque ac._',
    }, '\n'));

    simulate_input_keys('KEYBOARD_CURSOR_UP')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero.',
        '41: Etiam id congue urna, vel aliquet mi.',
        '45: Nam dignissim libero a interdum porttitor.',
        '73: Proin _ignissim euismod augue, laoreet porttitor ',
        'est pellentesque ac.',
    }, '\n'));

    screen:dismiss()
end

function test.keyboard_arrow_down_navigation_callbacks()
    local cursor_change = {old=nil, new=nil}
    local text_change = {old=nil, new=nil}
    local text_area, screen, window = arrange_textarea({
        w=55,
        on_cursor_change=function (_cursor, _old_cursor)
            cursor_change = {old=_old_cursor, new=_cursor}
        end,
        on_text_change=function (_text, _old_text)
            text_change = {old=_old_text, new=_text}
        end,
    })

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '41: Etiam id congue urna, vel aliquet mi.',
        '45: Nam dignissim libero a interdum porttitor.',
        '73: Proin dignissim euismod augue, laoreet porttitor est pellentesque ac.',
    }, '\n')

    simulate_input_text(text)

    text_area:setCursor(11)

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem _psum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero.',
        '41: Etiam id congue urna, vel aliquet mi.',
        '45: Nam dignissim libero a interdum porttitor.',
        '73: Proin dignissim euismod augue, laoreet porttitor ',
        'est pellentesque ac.',
    }, '\n'));

    text_change = {old=nil, new=nil}

    simulate_input_keys('KEYBOARD_CURSOR_DOWN')

    expect.table_eq(cursor_change, {new=61, old=11})
    expect.table_eq(text_change, {new=nil, old=nil})

    screen:dismiss()
end

function test.keyboard_arrow_left_navigation()
    local text_area, screen, window = arrange_textarea({w=55})

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
    }, '\n')

    simulate_input_text(text)

    simulate_input_keys('KEYBOARD_CURSOR_LEFT')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero_',
    }, '\n'));

    for i=1,6 do
        simulate_input_keys('KEYBOARD_CURSOR_LEFT')
    end

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        '_ibero.',
    }, '\n'));

    simulate_input_keys('KEYBOARD_CURSOR_LEFT')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec_',
        'libero.',
    }, '\n'));

    for i=1,105 do
        simulate_input_keys('KEYBOARD_CURSOR_LEFT')
    end

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit._',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero.',
    }, '\n'));

    for i=1,60 do
        simulate_input_keys('KEYBOARD_CURSOR_LEFT')
    end

    expect.eq(read_rendered_text(text_area), table.concat({
        '_0: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero.',
    }, '\n'));

    simulate_input_keys('KEYBOARD_CURSOR_LEFT')

    expect.eq(read_rendered_text(text_area), table.concat({
        '_0: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero.',
    }, '\n'));

    screen:dismiss()
end

function test.keyboard_arrow_left_navigation_callbacks()
    local cursor_change = {old=nil, new=nil}
    local text_change = {old=nil, new=nil}
    local text_area, screen, window = arrange_textarea({
        w=55,
        on_cursor_change=function (_cursor, _old_cursor)
            cursor_change = {old=_old_cursor, new=_cursor}
        end,
        on_text_change=function (_text, _old_text)
            text_change = {old=_old_text, new=_text}
        end,
    })

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
    }, '\n')

    simulate_input_text(text)
    expect.table_eq(cursor_change, {new=#text + 1, old=#text})

    text_change = {old=nil, new=nil}

    simulate_input_keys('KEYBOARD_CURSOR_LEFT')
    expect.table_eq(cursor_change, {new=#text, old=#text + 1})
    simulate_input_keys('KEYBOARD_CURSOR_LEFT')
    expect.table_eq(cursor_change, {new=#text - 1, old=#text})

    expect.table_eq(text_change, {new=nil, old=nil})

    screen:dismiss()
end

function test.keyboard_arrow_right_navigation()
    local text_area, screen, window = arrange_textarea({w=55})

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
    }, '\n')

    simulate_input_text(text)

    text_area:setCursor(1)

    simulate_input_keys('KEYBOARD_CURSOR_RIGHT')

    expect.eq(read_rendered_text(text_area), table.concat({
        '6_: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero.',
    }, '\n'));

    for i=1,53 do
        simulate_input_keys('KEYBOARD_CURSOR_RIGHT')
    end

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing_',
        'elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero.',
    }, '\n'));

    simulate_input_keys('KEYBOARD_CURSOR_RIGHT')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        '_lit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero.',
    }, '\n'));

    for i=1,5 do
        simulate_input_keys('KEYBOARD_CURSOR_RIGHT')
    end

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit._',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero.',
    }, '\n'));

    for i=1,113 do
        simulate_input_keys('KEYBOARD_CURSOR_RIGHT')
    end

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero._',
    }, '\n'));

    simulate_input_keys('KEYBOARD_CURSOR_RIGHT')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero._',
    }, '\n'));

    screen:dismiss()
end

function test.keyboard_arrow_right_navigation_callbacks()
    local cursor_change = {old=nil, new=nil}
    local text_change = {old=nil, new=nil}
    local text_area, screen, window = arrange_textarea({
        w=55,
        on_cursor_change=function (_cursor, _old_cursor)
            cursor_change = {old=_old_cursor, new=_cursor}
        end,
        on_text_change=function (_text, _old_text)
            text_change = {old=_old_text, new=_text}
        end,
    })

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
    }, '\n')

    simulate_input_text(text)
    expect.table_eq(cursor_change, {new=#text + 1, old=#text})

    text_area:setCursor(1)
    expect.table_eq(cursor_change, {new=1, old=#text + 1})

    text_change = {old=nil, new=nil}

    simulate_input_keys('KEYBOARD_CURSOR_RIGHT')
    expect.table_eq(cursor_change, {new=2, old=1})

    expect.table_eq(text_change, {new=nil, old=nil})

    screen:dismiss()
end

function test.handle_backspace()
    local text_area, screen, window = arrange_textarea({w=55})

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
    }, '\n')

    simulate_input_text(text)

    simulate_input_keys('STRING_A000')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero_',
    }, '\n'));

    for i=1,3 do
        simulate_input_keys('STRING_A000')
    end

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec lib_',
    }, '\n'));

    text_area:setCursor(62)

    simulate_input_keys('STRING_A000')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit._12: Sed consectetur, urna sit amet aliquet ',
        'egestas, ante nibh porttitor mi, vitae rutrum eros ',
        'metus nec lib',
    }, '\n'));

    text_area:setCursor(2)

    simulate_input_keys('STRING_A000')

    expect.eq(read_rendered_text(text_area), table.concat({
        '_: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.112: Sed consectetur, urna sit amet aliquet ',
        'egestas, ante nibh porttitor mi, vitae rutrum eros ',
        'metus nec lib',
    }, '\n'));

    simulate_input_keys('STRING_A000')

    expect.eq(read_rendered_text(text_area), table.concat({
        '_: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.112: Sed consectetur, urna sit amet aliquet ',
        'egestas, ante nibh porttitor mi, vitae rutrum eros ',
        'metus nec lib',
    }, '\n'));

    screen:dismiss()
end

function test.handle_backspace_callbacks()
    local cursor_change = {old=nil, new=nil}
    local text_change = {old=nil, new=nil}
    local text_area, screen, window = arrange_textarea({
        w=55,
        on_cursor_change=function (_cursor, _old_cursor)
            cursor_change = {old=_old_cursor, new=_cursor}
        end,
        on_text_change=function (_text, _old_text)
            text_change = {old=_old_text, new=_text}
        end,
    })

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
    }, '\n')

    simulate_input_text(text)
    expect.table_eq(cursor_change, {new=#text + 1, old=#text})

    simulate_input_keys('STRING_A000')
    expect.table_eq(cursor_change, {new=#text, old=#text + 1})

    expect.table_eq(text_change, {new=text:sub(1, -2), old=text})

    screen:dismiss()
end

function test.handle_delete()
    local text_area, screen, window = arrange_textarea({w=65})

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n')

    simulate_input_text(text)

    text_area:setCursor(1)

    simulate_input_keys('CUSTOM_DELETE')

    expect.eq(read_rendered_text(text_area), table.concat({
        '_: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    text_area:setCursor(124)
    simulate_input_keys('CUSTOM_DELETE')

    expect.eq(read_rendered_text(text_area), table.concat({
        '0: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        '_rttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    text_area:setCursor(123)
    simulate_input_keys('CUSTOM_DELETE')

    expect.eq(read_rendered_text(text_area), table.concat({
        '0: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante ',
        'nibh_rttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    text_area:setCursor(171)
    simulate_input_keys('CUSTOM_DELETE')

    expect.eq(read_rendered_text(text_area), table.concat({
        '0: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante ',
        'nibhorttitor mi, vitae rutrum eros metus nec libero._0: Lorem ',
        'ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    for i=1,59 do
        simulate_input_keys('CUSTOM_DELETE')
    end

    expect.eq(read_rendered_text(text_area), table.concat({
        '0: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante ',
        'nibhorttitor mi, vitae rutrum eros metus nec libero._',
    }, '\n'));

    simulate_input_keys('CUSTOM_DELETE')

    expect.eq(read_rendered_text(text_area), table.concat({
        '0: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante ',
        'nibhorttitor mi, vitae rutrum eros metus nec libero._',
    }, '\n'));

    screen:dismiss()
end

function test.handle_delete_callbacks()
    local cursor_change = {old=nil, new=nil}
    local text_change = {old=nil, new=nil}
    local text_area, screen, window = arrange_textarea({
        w=65,
        on_cursor_change=function (_cursor, _old_cursor)
            cursor_change = {old=_old_cursor, new=_cursor}
        end,
        on_text_change=function (_text, _old_text)
            text_change = {old=_old_text, new=_text}
        end,
    })

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n')

    simulate_input_text(text)

    text_area:setCursor(1)

    cursor_change = {old=nil, new=nil}

    simulate_input_keys('CUSTOM_DELETE')

    expect.table_eq(cursor_change, {new=nil, old=nil})
    expect.table_eq(text_change, {new=text:sub(2), old=text})

    screen:dismiss()
end

function test.line_end()
    local text_area, screen, window = arrange_textarea({w=65})

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n')

    simulate_input_text(text)
    text_area:setCursor(1)

    simulate_input_keys('CUSTOM_END')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit._',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    text_area:setCursor(70)

    simulate_input_keys('CUSTOM_END')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero._',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    text_area:setCursor(200)

    simulate_input_keys('CUSTOM_END')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit._',
    }, '\n'));

    simulate_input_keys('CUSTOM_END')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit._',
    }, '\n'));

    screen:dismiss()
end

function test.line_end_callbacks()
    local cursor_change = {old=nil, new=nil}
    local text_change = {old=nil, new=nil}
    local text_area, screen, window = arrange_textarea({
        w=65,
        on_cursor_change=function (_cursor, _old_cursor)
            cursor_change = {old=_old_cursor, new=_cursor}
        end,
        on_text_change=function (_text, _old_text)
            text_change = {old=_old_text, new=_text}
        end,
    })

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n')

    simulate_input_text(text)
    text_area:setCursor(1)
    expect.table_eq(cursor_change, {new=1, old=#text + 1})

    text_change = {old=nil, new=nil}

    simulate_input_keys('CUSTOM_END')
    expect.table_eq(cursor_change, {new=61, old=1})
    expect.table_eq(text_change, {new=nil, old=nil})

    screen:dismiss()
end

function test.line_beging()
    local text_area, screen, window = arrange_textarea({w=65})

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n')

    simulate_input_text(text)

    simulate_input_keys('CUSTOM_HOME')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '_0: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    text_area:setCursor(173)

    simulate_input_keys('CUSTOM_HOME')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '_12: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    text_area:setCursor(1)

    simulate_input_keys('CUSTOM_HOME')

    expect.eq(read_rendered_text(text_area), table.concat({
        '_0: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    screen:dismiss()
end

function test.line_beging_callbacks()
    local cursor_change = {old=nil, new=nil}
    local text_change = {old=nil, new=nil}
    local text_area, screen, window = arrange_textarea({
        w=65,
        on_cursor_change=function (_cursor, _old_cursor)
            cursor_change = {old=_old_cursor, new=_cursor}
        end,
        on_text_change=function (_text, _old_text)
            text_change = {old=_old_text, new=_text}
        end,
    })

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n')

    simulate_input_text(text)
    expect.table_eq(cursor_change, {new=#text + 1, old=#text})

    text_change = {old=nil, new=nil}

    simulate_input_keys('CUSTOM_HOME')
    expect.table_eq(cursor_change, {new=#text + 1 - 60, old=#text + 1})
    expect.table_eq(text_change, {new=nil, old=nil})

    screen:dismiss()
end

function test.line_delete()
    local text_area, screen, window = arrange_textarea({w=65})

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n')

    simulate_input_text(text)

    text_area:setCursor(65)

    simulate_input_keys('CUSTOM_CTRL_U')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '_0: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    simulate_input_keys('CUSTOM_CTRL_U')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '_'
    }, '\n'));

    text_area:setCursor(1)

    simulate_input_keys('CUSTOM_CTRL_U')

    expect.eq(read_rendered_text(text_area), table.concat({
        '_'
    }, '\n'));

    simulate_input_keys('CUSTOM_CTRL_U')

    expect.eq(read_rendered_text(text_area), table.concat({
        '_'
    }, '\n'));

    screen:dismiss()
end

function test.line_delete_callbacks()
    local cursor_change = {old=nil, new=nil}
    local text_change = {old=nil, new=nil}
    local text_area, screen, window = arrange_textarea({
        w=65,
        on_cursor_change=function (_cursor, _old_cursor)
            cursor_change = {old=_old_cursor, new=_cursor}
        end,
        on_text_change=function (_text, _old_text)
            text_change = {old=_old_text, new=_text}
        end,
    })

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n')

    simulate_input_text(text)

    text_area:setCursor(65)
    expect.table_eq(cursor_change, {new=65, old=#text + 1})

    simulate_input_keys('CUSTOM_CTRL_U')
    expect.table_eq(cursor_change, {new=62, old=65})

    expect.table_eq(text_change, {
        new=table.concat({
            '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
            '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        }, '\n'),
        old=text
    })

    screen:dismiss()
end

function test.line_delete_to_end()
    local text_area, screen, window = arrange_textarea({w=65})

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n')

    simulate_input_text(text)

    text_area:setCursor(70)

    simulate_input_keys('CUSTOM_CTRL_K')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed_',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    simulate_input_keys('CUSTOM_CTRL_K')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed_0: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.',
    }, '\n'));

    screen:dismiss()
end

function test.line_delete_to_end_callbacks()
    local cursor_change = {old=nil, new=nil}
    local text_change = {old=nil, new=nil}
    local text_area, screen, window = arrange_textarea({
        w=65,
        on_cursor_change=function (_cursor, _old_cursor)
            cursor_change = {old=_old_cursor, new=_cursor}
        end,
        on_text_change=function (_text, _old_text)
            text_change = {old=_old_text, new=_text}
        end,
    })

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n')

    simulate_input_text(text)

    text_area:setCursor(70)

    cursor_change = {old=nil, new=nil}

    simulate_input_keys('CUSTOM_CTRL_K')
    printall(cursor_change)
    expect.table_eq(cursor_change, {new=nil, old=nil})

    expect.table_eq(text_change, {
        new=table.concat({
            '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
            '112: Sed',
            '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        }, '\n'),
        old=text
    })

    screen:dismiss()
end

function test.delete_last_word()
    local text_area, screen, window = arrange_textarea({w=65})

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n')

    simulate_input_text(text)

    simulate_input_keys('CUSTOM_CTRL_W')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing _',
    }, '\n'));

    simulate_input_keys('CUSTOM_CTRL_W')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur _',
    }, '\n'));

    text_area:setCursor(82)

    simulate_input_keys('CUSTOM_CTRL_W')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed _ urna sit amet aliquet egestas, ante nibh porttitor ',
        'mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur ',
    }, '\n'));

    text_area:setCursor(37)

    simulate_input_keys('CUSTOM_CTRL_W')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, _ctetur adipiscing elit.',
        '112: Sed , urna sit amet aliquet egestas, ante nibh porttitor ',
        'mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur ',
    }, '\n'));

    for i=1,6 do
        simulate_input_keys('CUSTOM_CTRL_W')
    end

    expect.eq(read_rendered_text(text_area), table.concat({
        '_ctetur adipiscing elit.',
        '112: Sed , urna sit amet aliquet egestas, ante nibh porttitor ',
        'mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur ',
    }, '\n'));

    simulate_input_keys('CUSTOM_CTRL_W')

    expect.eq(read_rendered_text(text_area), table.concat({
        '_ctetur adipiscing elit.',
        '112: Sed , urna sit amet aliquet egestas, ante nibh porttitor ',
        'mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur ',
    }, '\n'));

    screen:dismiss()
end

function test.delete_last_word_callbacks()
    local cursor_change = {old=nil, new=nil}
    local text_change = {old=nil, new=nil}
    local text_area, screen, window = arrange_textarea({
        w=65,
        on_cursor_change=function (_cursor, _old_cursor)
            cursor_change = {old=_old_cursor, new=_cursor}
        end,
        on_text_change=function (_text, _old_text)
            text_change = {old=_old_text, new=_text}
        end,
    })

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n')

    simulate_input_text(text)
    expect.table_eq(cursor_change, {new=#text + 1, old=#text})

    simulate_input_keys('CUSTOM_CTRL_W')
    expect.table_eq(cursor_change, {new=#text + 1 - 5, old=#text + 1})

    expect.table_eq(text_change, {
        new=text:sub(1, -6),
        old=text
    })

    screen:dismiss()
end

function test.jump_to_text_end()
    local text_area, screen, window = arrange_textarea({w=65})

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n')

    simulate_input_text(text)

    text_area:setCursor(1)

    simulate_input_keys('CUSTOM_CTRL_END')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit._',
    }, '\n'));

    simulate_input_keys('CUSTOM_CTRL_END')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit._',
    }, '\n'));

    screen:dismiss()
end

function test.jump_to_text_end_callbacks()
    local cursor_change = {old=nil, new=nil}
    local text_change = {old=nil, new=nil}
    local text_area, screen, window = arrange_textarea({
        w=65,
        on_cursor_change=function (_cursor, _old_cursor)
            cursor_change = {old=_old_cursor, new=_cursor}
        end,
        on_text_change=function (_text, _old_text)
            text_change = {old=_old_text, new=_text}
        end,
    })

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n')

    simulate_input_text(text)

    text_area:setCursor(1)
    expect.table_eq(cursor_change, {new=1, old=#text + 1})

    text_change = {old=nil, new=nil}

    simulate_input_keys('CUSTOM_CTRL_END')
    expect.table_eq(cursor_change, {new=#text +1, old=1})
    expect.table_eq(text_change, {new=nil, old=nil})

    screen:dismiss()
end

function test.jump_to_text_begin()
    local text_area, screen, window = arrange_textarea({w=65})

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n')

    simulate_input_text(text)

    simulate_input_keys('CUSTOM_CTRL_HOME')

    expect.eq(read_rendered_text(text_area), table.concat({
        '_0: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    simulate_input_keys('CUSTOM_CTRL_HOME')

    expect.eq(read_rendered_text(text_area), table.concat({
        '_0: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    screen:dismiss()
end

function test.jump_to_text_begin_callbacks()
    local cursor_change = {old=nil, new=nil}
    local text_change = {old=nil, new=nil}
    local text_area, screen, window = arrange_textarea({
        w=65,
        on_cursor_change=function (_cursor, _old_cursor)
            cursor_change = {old=_old_cursor, new=_cursor}
        end,
        on_text_change=function (_text, _old_text)
            text_change = {old=_old_text, new=_text}
        end,
    })

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n')

    simulate_input_text(text)
    expect.table_eq(cursor_change, {new=#text + 1, old=#text})

    text_change = {old=nil, new=nil}

    simulate_input_keys('CUSTOM_CTRL_HOME')
    expect.table_eq(cursor_change, {new=1, old=#text + 1})
    expect.table_eq(text_change, {new=nil, old=nil})

    screen:dismiss()
end

function test.select_all()
    local text_area, screen, window = arrange_textarea({w=65})

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n')

    simulate_input_text(text)

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit._',
    }, '\n'));

    simulate_input_keys('CUSTOM_CTRL_A')

    expect.eq(read_selected_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    screen:dismiss()
end

function test.text_key_replace_selection()
    local text_area, screen, window = arrange_textarea({w=65})

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '51: Sed consectetur, urna sit amet aliquet egestas.',
    }, '\n')

    simulate_input_text(text)

    simulate_mouse_drag(text_area, 4, 0, 9, 0)

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '51: Sed consectetur, urna sit amet aliquet egestas.',
    }, '\n'));

    expect.eq(read_selected_text(text_area), 'Lorem ');

    simulate_input_text('+')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: +_psum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '51: Sed consectetur, urna sit amet aliquet egestas.',
    }, '\n'));

    simulate_mouse_drag(text_area, 6, 1, 6, 2)

    simulate_input_text('!')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: +ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: S!_r mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '51: Sed consectetur, urna sit amet aliquet egestas.',
    }, '\n'));

    simulate_mouse_drag(text_area, 3, 1, 6, 2)

    simulate_input_text('@')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: +ipsum dolor sit amet, consectetur adipiscing elit.',
        '112@_m ipsum dolor sit amet, consectetur adipiscing elit.',
        '51: Sed consectetur, urna sit amet aliquet egestas.',
    }, '\n'));

    screen:dismiss()
end

function test.arrows_reset_selection()
    local text_area, screen, window = arrange_textarea({w=65})

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero._',
    }, '\n')

    simulate_input_text(text)

    simulate_input_keys('CUSTOM_CTRL_A')

    expect.eq(read_rendered_text(text_area), text);

    expect.eq(read_selected_text(text_area), text);

    simulate_input_keys('KEYBOARD_CURSOR_LEFT')
    expect.eq(read_selected_text(text_area), '')
    expect.eq(read_rendered_text(text_area), '_' .. text:sub(2))

    simulate_input_keys('CUSTOM_CTRL_A')

    simulate_input_keys('KEYBOARD_CURSOR_RIGHT')
    expect.eq(read_selected_text(text_area), '')
    expect.eq(read_rendered_text(text_area), text:sub(1, #text) .. '_')

    simulate_input_keys('CUSTOM_CTRL_A')

    simulate_input_keys('KEYBOARD_CURSOR_UP')
    expect.eq(read_selected_text(text_area), '')

    simulate_input_keys('CUSTOM_CTRL_A')

    simulate_input_keys('KEYBOARD_CURSOR_DOWN')
    expect.eq(read_selected_text(text_area), '')

    screen:dismiss()
end

function test.click_reset_selection()
    local text_area, screen, window = arrange_textarea({w=65})

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
    }, '\n')

    simulate_input_text(text)

    simulate_input_keys('CUSTOM_CTRL_A')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
    }, '\n'));

    expect.eq(read_selected_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
    }, '\n'));

    simulate_mouse_click(text_area, 4, 0)
    expect.eq(read_selected_text(text_area), '')

    simulate_input_keys('CUSTOM_CTRL_A')

    simulate_mouse_click(text_area, 4, 8)
    expect.eq(read_selected_text(text_area), '')

    screen:dismiss()
end

function test.line_navigation_reset_selection()
    local text_area, screen, window = arrange_textarea({w=65})

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
    }, '\n')

    simulate_input_text(text)

    simulate_input_keys('CUSTOM_CTRL_A')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
    }, '\n'));

    expect.eq(read_selected_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
    }, '\n'));

    simulate_input_keys('CUSTOM_HOME')
    expect.eq(read_selected_text(text_area), '')

    simulate_input_keys('CUSTOM_END')
    expect.eq(read_selected_text(text_area), '')

    screen:dismiss()
end

function test.jump_begin_or_end_reset_selection()
    local text_area, screen, window = arrange_textarea({w=65})

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
    }, '\n')

    simulate_input_text(text)

    simulate_input_keys('CUSTOM_CTRL_A')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
    }, '\n'));

    expect.eq(read_selected_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
    }, '\n'));

    simulate_input_keys('CUSTOM_CTRL_HOME')
    expect.eq(read_selected_text(text_area), '')

    simulate_input_keys('CUSTOM_CTRL_END')
    expect.eq(read_selected_text(text_area), '')

    screen:dismiss()
end

function test.new_line_override_selection()
    local text_area, screen, window = arrange_textarea({w=65})

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n')

    simulate_input_text(text)

    simulate_mouse_drag(text_area, 4, 0, 29, 2)

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    expect.eq(read_selected_text(text_area), table.concat({
        'Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
         '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum ero',
    }, '\n'));

    simulate_input_keys('SELECT')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: ',
        '_ metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    screen:dismiss()
end

function test.backspace_delete_selection()
    local text_area, screen, window = arrange_textarea({w=65})

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n')

    simulate_input_text(text)

    simulate_mouse_drag(text_area, 4, 0, 29, 2)

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    expect.eq(read_selected_text(text_area), table.concat({
        'Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
         '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum ero',
    }, '\n'));

    simulate_input_keys('STRING_A000')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: _ metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    screen:dismiss()
end

function test.delete_char_delete_selection()
    local text_area, screen, window = arrange_textarea({w=65})

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n')

    simulate_input_text(text)

    simulate_mouse_drag(text_area, 4, 0, 29, 2)

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    expect.eq(read_selected_text(text_area), table.concat({
        'Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
         '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum ero',
    }, '\n'));

    simulate_input_keys('CUSTOM_DELETE')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: _ metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    screen:dismiss()
end

function test.delete_line_delete_selection_lines()
    local text_area, screen, window = arrange_textarea({w=65})

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '51: Sed consectetur, urna sit amet aliquet egestas.',
    }, '\n')

    simulate_input_text(text)

    simulate_mouse_drag(text_area, 4, 0, 9, 0)

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '51: Sed consectetur, urna sit amet aliquet egestas.',
    }, '\n'));

    expect.eq(read_selected_text(text_area), 'Lorem ');

    simulate_input_keys('CUSTOM_CTRL_U')

    expect.eq(read_rendered_text(text_area), table.concat({
        '_12: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '51: Sed consectetur, urna sit amet aliquet egestas.',
    }, '\n'));

    simulate_mouse_drag(text_area, 4, 1, 29, 2)

    simulate_input_keys('CUSTOM_CTRL_U')

    expect.eq(read_rendered_text(text_area), table.concat({
        '_1: Sed consectetur, urna sit amet aliquet egestas.',
    }, '\n'));

    screen:dismiss()
end

function test.delete_line_rest_delete_selection_lines()
    local text_area, screen, window = arrange_textarea({w=65})

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '51: Sed consectetur, urna sit amet aliquet egestas.',
    }, '\n')

    simulate_input_text(text)

    simulate_mouse_drag(text_area, 4, 0, 9, 0)

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '51: Sed consectetur, urna sit amet aliquet egestas.',
    }, '\n'));

    expect.eq(read_selected_text(text_area), 'Lorem ');

    simulate_input_keys('CUSTOM_CTRL_K')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: _',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '51: Sed consectetur, urna sit amet aliquet egestas.',
    }, '\n'));

    simulate_mouse_drag(text_area, 6, 1, 6, 2)

    simulate_input_keys('CUSTOM_CTRL_K')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: ',
        '112: S_',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '51: Sed consectetur, urna sit amet aliquet egestas.',
    }, '\n'));

    simulate_mouse_drag(text_area, 3, 1, 6, 2)

    simulate_input_keys('CUSTOM_CTRL_K')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: ',
        '112_',
        '51: Sed consectetur, urna sit amet aliquet egestas.',
    }, '\n'));

    screen:dismiss()
end

function test.delete_last_word_delete_selection()
        local text_area, screen, window = arrange_textarea({w=65})

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '51: Sed consectetur, urna sit amet aliquet egestas.',
    }, '\n')

    simulate_input_text(text)

    simulate_mouse_drag(text_area, 4, 0, 9, 0)

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '51: Sed consectetur, urna sit amet aliquet egestas.',
    }, '\n'));

    expect.eq(read_selected_text(text_area), 'Lorem ');

    simulate_input_keys('CUSTOM_CTRL_W')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: _psum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '51: Sed consectetur, urna sit amet aliquet egestas.',
    }, '\n'));

    simulate_mouse_drag(text_area, 6, 1, 6, 2)

    simulate_input_keys('CUSTOM_CTRL_W')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: S_r mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '51: Sed consectetur, urna sit amet aliquet egestas.',
    }, '\n'));

    simulate_mouse_drag(text_area, 3, 1, 6, 2)

    simulate_input_keys('CUSTOM_CTRL_W')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: ipsum dolor sit amet, consectetur adipiscing elit.',
        '112_m ipsum dolor sit amet, consectetur adipiscing elit.',
        '51: Sed consectetur, urna sit amet aliquet egestas.',
    }, '\n'));

    screen:dismiss()
end

function test.single_mouse_click_set_cursor()
    local text_area, screen, window = arrange_textarea({w=65})

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n')

    simulate_input_text(text)

    simulate_mouse_click(text_area, 4, 0)

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: _orem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    simulate_mouse_click(text_area, 40, 2)

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus ne_ libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    simulate_mouse_click(text_area, 49, 2)

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero._',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    simulate_mouse_click(text_area, 60, 2)

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero._',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    simulate_mouse_click(text_area, 0, 10)

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '_0: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    simulate_mouse_click(text_area, 21, 10)

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor_sit amet, consectetur adipiscing elit.',
    }, '\n'));

    simulate_mouse_click(text_area, 63, 10)

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit._',
    }, '\n'));

    screen:dismiss()
end

function test.double_mouse_click_select_word()
    local text_area, screen, window = arrange_textarea({w=65})

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n')

    simulate_input_text(text)

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit._',
    }, '\n'));

    simulate_mouse_click(text_area, 0, 0)
    simulate_mouse_click(text_area, 0, 0)

    expect.eq(read_selected_text(text_area), '60:')

    simulate_mouse_click(text_area, 4, 0)
    simulate_mouse_click(text_area, 4, 0)

    expect.eq(read_selected_text(text_area), 'Lorem')

    simulate_mouse_click(text_area, 40, 2)
    simulate_mouse_click(text_area, 40, 2)

    expect.eq(read_selected_text(text_area), 'nec')

    simulate_mouse_click(text_area, 58, 3)
    simulate_mouse_click(text_area, 58, 3)
    expect.eq(read_selected_text(text_area), 'elit')

    simulate_mouse_click(text_area, 60, 3)
    simulate_mouse_click(text_area, 60, 3)
    expect.eq(read_selected_text(text_area), '.')

    screen:dismiss()
end

function test.double_mouse_click_select_white_spaces()
    local text_area, screen, window = arrange_textarea({w=65})

    local text = 'Lorem ipsum dolor sit amet,     consectetur elit.'
    simulate_input_text(text)

    expect.eq(read_rendered_text(text_area), text .. '_')

    simulate_mouse_click(text_area, 29, 0)
    simulate_mouse_click(text_area, 29, 0)

    expect.eq(read_selected_text(text_area), '     ')

    screen:dismiss()
end

function test.triple_mouse_click_select_line()
    local text_area, screen, window = arrange_textarea({w=65})

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n')

    simulate_input_text(text)

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit._',
    }, '\n'));

    simulate_mouse_click(text_area, 0, 0)
    simulate_mouse_click(text_area, 0, 0)
    simulate_mouse_click(text_area, 0, 0)

    expect.eq(
        read_selected_text(text_area),
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.'
    )

    simulate_mouse_click(text_area, 4, 0)
    simulate_mouse_click(text_area, 4, 0)
    simulate_mouse_click(text_area, 4, 0)

    expect.eq(
        read_selected_text(text_area),
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.'
    )

    simulate_mouse_click(text_area, 40, 2)
    simulate_mouse_click(text_area, 40, 2)
    simulate_mouse_click(text_area, 40, 2)

    expect.eq(read_selected_text(text_area), table.concat({
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
    }, '\n'));

    simulate_mouse_click(text_area, 58, 3)
    simulate_mouse_click(text_area, 58, 3)
    simulate_mouse_click(text_area, 58, 3)

    expect.eq(
        read_selected_text(text_area),
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.'
    )

    simulate_mouse_click(text_area, 60, 3)
    simulate_mouse_click(text_area, 60, 3)
    simulate_mouse_click(text_area, 60, 3)

    expect.eq(
        read_selected_text(text_area),
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.'
    )

    screen:dismiss()
end

function test.mouse_selection_control()
    local text_area, screen, window = arrange_textarea({w=65})

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n')

    simulate_input_text(text)

    simulate_mouse_drag(text_area, 4, 0, 29, 0)

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    expect.eq(read_selected_text(text_area), 'Lorem ipsum dolor sit amet')

    simulate_mouse_drag(text_area, 0, 0, 29, 0)

    expect.eq(read_selected_text(text_area), '60: Lorem ipsum dolor sit amet')

    simulate_mouse_drag(text_area, 32, 0, 32, 1)

    expect.eq(read_selected_text(text_area), table.concat({
        'consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit am'
    }, '\n'));

    simulate_mouse_drag(text_area, 32, 1, 48, 2)

    expect.eq(read_selected_text(text_area), table.concat({
        'met aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
    }, '\n'));

    simulate_mouse_drag(text_area, 42, 2, 59, 3)

    expect.eq(read_selected_text(text_area), table.concat({
        'libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.'
    }, '\n'));

    simulate_mouse_drag(text_area, 42, 2, 65, 3)

    expect.eq(read_selected_text(text_area), table.concat({
        'libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.'
    }, '\n'));

    simulate_mouse_drag(text_area, 42, 2, 65, 6)

    expect.eq(read_selected_text(text_area), table.concat({
        'libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.'
    }, '\n'));

    simulate_mouse_drag(text_area, 42, 2, 42, 6)

    expect.eq(read_selected_text(text_area), table.concat({
        'libero.',
        '60: Lorem ipsum dolor sit amet, consectetur'
    }, '\n'));

    screen:dismiss()
end

function test.copy_and_paste_text_line()
    local text_area, screen, window = arrange_textarea({w=65})

    local text = table.concat({
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n')

    simulate_input_text(text)

    expect.eq(read_rendered_text(text_area), table.concat({
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit._',
    }, '\n'));

    simulate_input_keys('CUSTOM_CTRL_C')
    simulate_input_keys('CUSTOM_CTRL_V')

    expect.eq(read_rendered_text(text_area), table.concat({
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit._',
    }, '\n'));

    simulate_mouse_click(text_area, 15, 3)
    simulate_input_keys('CUSTOM_CTRL_C')
    simulate_input_keys('CUSTOM_CTRL_V')

    expect.eq(read_rendered_text(text_area), table.concat({
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '60: Lorem ipsum_dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    simulate_mouse_click(text_area, 5, 0)
    simulate_input_keys('CUSTOM_CTRL_C')
    simulate_input_keys('CUSTOM_CTRL_V')

    expect.eq(read_rendered_text(text_area), table.concat({
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '112: _ed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    simulate_mouse_click(text_area, 6, 0)
    simulate_input_keys('CUSTOM_CTRL_C')
    simulate_mouse_click(text_area, 5, 6)
    simulate_input_keys('CUSTOM_CTRL_V')

    expect.eq(read_rendered_text(text_area), table.concat({
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: L_rem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    screen:dismiss()
end

function test.copy_and_paste_selected_text()
        local text_area, screen, window = arrange_textarea({w=65})

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n')

    simulate_input_text(text)

    simulate_mouse_drag(text_area, 4, 0, 8, 0)

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    expect.eq(read_selected_text(text_area), 'Lorem')

    simulate_input_keys('CUSTOM_CTRL_C')
    simulate_input_keys('CUSTOM_CTRL_V')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem_ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    simulate_mouse_click(text_area, 4, 2)

    simulate_input_keys('CUSTOM_CTRL_V')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'portLorem_itor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    simulate_mouse_click(text_area, 0, 0)

    simulate_input_keys('CUSTOM_CTRL_V')

    expect.eq(read_rendered_text(text_area), table.concat({
        'Lorem_0: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'portLoremtitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    simulate_mouse_click(text_area, 60, 4)

    simulate_input_keys('CUSTOM_CTRL_V')

    expect.eq(read_rendered_text(text_area), table.concat({
        'Lorem60: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'portLoremtitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.Lorem_',
    }, '\n'));

    screen:dismiss()
end

function test.cut_and_paste_text_line()
    local text_area, screen, window = arrange_textarea({w=65})

    local text = table.concat({
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n')

    simulate_input_text(text)

    expect.eq(read_rendered_text(text_area), table.concat({
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit._',
    }, '\n'));

    simulate_input_keys('CUSTOM_CTRL_X')
    simulate_input_keys('CUSTOM_CTRL_V')

    expect.eq(read_rendered_text(text_area), table.concat({
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '_',
    }, '\n'));

    simulate_mouse_click(text_area, 0, 0)
    simulate_input_keys('CUSTOM_CTRL_X')
    simulate_input_keys('CUSTOM_CTRL_V')

    expect.eq(read_rendered_text(text_area), table.concat({
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '_0: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    simulate_mouse_click(text_area, 60, 2)
    simulate_input_keys('CUSTOM_CTRL_X')
    simulate_input_keys('CUSTOM_CTRL_V')

    expect.eq(read_rendered_text(text_area), table.concat({
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '_',
    }, '\n'));

    screen:dismiss()
end

function test.cut_and_paste_selected_text()
    local text_area, screen, window = arrange_textarea({w=65})

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n')

    simulate_input_text(text)

    simulate_mouse_drag(text_area, 4, 0, 8, 0)

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    expect.eq(read_selected_text(text_area), 'Lorem')

    simulate_input_keys('CUSTOM_CTRL_X')
    simulate_input_keys('CUSTOM_CTRL_V')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem_ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    simulate_mouse_drag(text_area, 4, 0, 8, 0)
    simulate_input_keys('CUSTOM_CTRL_X')

    simulate_mouse_click(text_area, 4, 2)

    simulate_input_keys('CUSTOM_CTRL_V')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60:  ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'portLorem_itor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    simulate_mouse_drag(text_area, 5, 2, 8, 2)
    simulate_input_keys('CUSTOM_CTRL_X')

    simulate_mouse_click(text_area, 0, 0)
    simulate_input_keys('CUSTOM_CTRL_V')

    expect.eq(read_rendered_text(text_area), table.concat({
        'orem_0:  ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'portLtitor mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
    }, '\n'));

    simulate_mouse_drag(text_area, 5, 2, 8, 2)
    simulate_input_keys('CUSTOM_CTRL_X')

    simulate_mouse_click(text_area, 60, 4)
    simulate_input_keys('CUSTOM_CTRL_V')

    expect.eq(read_rendered_text(text_area), table.concat({
        'orem60:  ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'portLr mi, vitae rutrum eros metus nec libero.',
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.tito_',
    }, '\n'));

    screen:dismiss()
end

function test.scroll_long_text()
    local text_area, screen, window, widget = arrange_textarea({w=100, h=10})
    local scrollbar = widget.scrollbar

    local text = table.concat({
        'Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        'Nulla ut lacus ut tortor semper consectetur.',
        'Nam scelerisque ligula vitae magna varius, vel porttitor tellus egestas.',
        'Suspendisse aliquet dolor ac velit maximus, ut tempor lorem tincidunt.',
        'Ut eu orci non nibh hendrerit posuere.',
        'Sed euismod odio eu fringilla bibendum.',
        'Etiam dignissim diam nec aliquet facilisis.',
        'Integer tristique purus at tellus luctus, vel aliquet sapien sollicitudin.',
        'Fusce ornare est vitae urna feugiat, vel interdum quam vestibulum.',
        '10: Vivamus id felis scelerisque, lobortis diam ut, mollis nisi.',
        'Donec quis lectus ac erat placerat eleifend.',
        'Aenean non orci id erat malesuada pharetra.',
        'Nunc in lectus et metus finibus venenatis.',
        'Morbi id mauris dignissim, suscipit metus nec, auctor odio.',
        'Sed in libero eget velit condimentum lacinia ut quis dui.',
        'Praesent sollicitudin dui ac mollis lacinia.',
        'Ut gravida tortor ac accumsan suscipit.',
        '18: Vestibulum at ante ut dui hendrerit pellentesque ut eu ex.',
    }, '\n')

    simulate_input_text(text)

    expect.eq(read_rendered_text(text_area), table.concat({
        'Fusce ornare est vitae urna feugiat, vel interdum quam vestibulum.',
        '10: Vivamus id felis scelerisque, lobortis diam ut, mollis nisi.',
        'Donec quis lectus ac erat placerat eleifend.',
        'Aenean non orci id erat malesuada pharetra.',
        'Nunc in lectus et metus finibus venenatis.',
        'Morbi id mauris dignissim, suscipit metus nec, auctor odio.',
        'Sed in libero eget velit condimentum lacinia ut quis dui.',
        'Praesent sollicitudin dui ac mollis lacinia.',
        'Ut gravida tortor ac accumsan suscipit.',
        '18: Vestibulum at ante ut dui hendrerit pellentesque ut eu ex._',
    }, '\n'))

    simulate_mouse_click(scrollbar, 0, 0)

    expect.eq(read_rendered_text(text_area), table.concat({
        'Integer tristique purus at tellus luctus, vel aliquet sapien sollicitudin.',
        'Fusce ornare est vitae urna feugiat, vel interdum quam vestibulum.',
        '10: Vivamus id felis scelerisque, lobortis diam ut, mollis nisi.',
        'Donec quis lectus ac erat placerat eleifend.',
        'Aenean non orci id erat malesuada pharetra.',
        'Nunc in lectus et metus finibus venenatis.',
        'Morbi id mauris dignissim, suscipit metus nec, auctor odio.',
        'Sed in libero eget velit condimentum lacinia ut quis dui.',
        'Praesent sollicitudin dui ac mollis lacinia.',
        'Ut gravida tortor ac accumsan suscipit.',
    }, '\n'))

    simulate_mouse_click(scrollbar, 0, 0)
    simulate_mouse_click(scrollbar, 0, 0)

    expect.eq(read_rendered_text(text_area), table.concat({
        'Sed euismod odio eu fringilla bibendum.',
        'Etiam dignissim diam nec aliquet facilisis.',
        'Integer tristique purus at tellus luctus, vel aliquet sapien sollicitudin.',
        'Fusce ornare est vitae urna feugiat, vel interdum quam vestibulum.',
        '10: Vivamus id felis scelerisque, lobortis diam ut, mollis nisi.',
        'Donec quis lectus ac erat placerat eleifend.',
        'Aenean non orci id erat malesuada pharetra.',
        'Nunc in lectus et metus finibus venenatis.',
        'Morbi id mauris dignissim, suscipit metus nec, auctor odio.',
        'Sed in libero eget velit condimentum lacinia ut quis dui.',
    }, '\n'))

    simulate_mouse_click(scrollbar, 0, scrollbar.frame_body.height - 2)

    expect.eq(read_rendered_text(text_area), table.concat({
        'Fusce ornare est vitae urna feugiat, vel interdum quam vestibulum.',
        '10: Vivamus id felis scelerisque, lobortis diam ut, mollis nisi.',
        'Donec quis lectus ac erat placerat eleifend.',
        'Aenean non orci id erat malesuada pharetra.',
        'Nunc in lectus et metus finibus venenatis.',
        'Morbi id mauris dignissim, suscipit metus nec, auctor odio.',
        'Sed in libero eget velit condimentum lacinia ut quis dui.',
        'Praesent sollicitudin dui ac mollis lacinia.',
        'Ut gravida tortor ac accumsan suscipit.',
        '18: Vestibulum at ante ut dui hendrerit pellentesque ut eu ex._',
    }, '\n'))

    simulate_mouse_click(scrollbar, 0, 2)

    expect.eq(read_rendered_text(text_area), table.concat({
        'Suspendisse aliquet dolor ac velit maximus, ut tempor lorem tincidunt.',
        'Ut eu orci non nibh hendrerit posuere.',
        'Sed euismod odio eu fringilla bibendum.',
        'Etiam dignissim diam nec aliquet facilisis.',
        'Integer tristique purus at tellus luctus, vel aliquet sapien sollicitudin.',
        'Fusce ornare est vitae urna feugiat, vel interdum quam vestibulum.',
        '10: Vivamus id felis scelerisque, lobortis diam ut, mollis nisi.',
        'Donec quis lectus ac erat placerat eleifend.',
        'Aenean non orci id erat malesuada pharetra.',
        'Nunc in lectus et metus finibus venenatis.',
    }, '\n'))

    screen:dismiss()
end

function test.scroll_follows_cursor()
    local text_area, screen, window = arrange_textarea({w=100, h=10})
    local scrollbar = window.subviews.text_area_scrollbar

    local text = table.concat({
        'Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        'Nulla ut lacus ut tortor semper consectetur.',
        'Nam scelerisque ligula vitae magna varius, vel porttitor tellus egestas.',
        'Suspendisse aliquet dolor ac velit maximus, ut tempor lorem tincidunt.',
        'Ut eu orci non nibh hendrerit posuere.',
        'Sed euismod odio eu fringilla bibendum.',
        'Etiam dignissim diam nec aliquet facilisis.',
        'Integer tristique purus at tellus luctus, vel aliquet sapien sollicitudin.',
        'Fusce ornare est vitae urna feugiat, vel interdum quam vestibulum.',
        '10: Vivamus id felis scelerisque, lobortis diam ut, mollis nisi.',
        'Donec quis lectus ac erat placerat eleifend.',
        'Aenean non orci id erat malesuada pharetra.',
        'Nunc in lectus et metus finibus venenatis.',
        'Morbi id mauris dignissim, suscipit metus nec, auctor odio.',
        'Sed in libero eget velit condimentum lacinia ut quis dui.',
        'Praesent sollicitudin dui ac mollis lacinia.',
        'Ut gravida tortor ac accumsan suscipit.',
        '18: Vestibulum at ante ut dui hendrerit pellentesque ut eu ex.',
    }, '\n')

    simulate_input_text(text)

    expect.eq(read_rendered_text(text_area), table.concat({
        'Fusce ornare est vitae urna feugiat, vel interdum quam vestibulum.',
        '10: Vivamus id felis scelerisque, lobortis diam ut, mollis nisi.',
        'Donec quis lectus ac erat placerat eleifend.',
        'Aenean non orci id erat malesuada pharetra.',
        'Nunc in lectus et metus finibus venenatis.',
        'Morbi id mauris dignissim, suscipit metus nec, auctor odio.',
        'Sed in libero eget velit condimentum lacinia ut quis dui.',
        'Praesent sollicitudin dui ac mollis lacinia.',
        'Ut gravida tortor ac accumsan suscipit.',
        '18: Vestibulum at ante ut dui hendrerit pellentesque ut eu ex._',
    }, '\n'))

    simulate_mouse_click(text_area, 0, 8)
    simulate_input_keys('KEYBOARD_CURSOR_UP')

    expect.eq(read_rendered_text(text_area), table.concat({
        '_nteger tristique purus at tellus luctus, vel aliquet sapien sollicitudin.',
        'Fusce ornare est vitae urna feugiat, vel interdum quam vestibulum.',
        '10: Vivamus id felis scelerisque, lobortis diam ut, mollis nisi.',
        'Donec quis lectus ac erat placerat eleifend.',
        'Aenean non orci id erat malesuada pharetra.',
        'Nunc in lectus et metus finibus venenatis.',
        'Morbi id mauris dignissim, suscipit metus nec, auctor odio.',
        'Sed in libero eget velit condimentum lacinia ut quis dui.',
        'Praesent sollicitudin dui ac mollis lacinia.',
        'Ut gravida tortor ac accumsan suscipit.',
    }, '\n'))

    simulate_input_keys('CUSTOM_CTRL_HOME')

    simulate_mouse_click(text_area, 0, 9)
    simulate_input_keys('KEYBOARD_CURSOR_DOWN')

    expect.eq(read_rendered_text(text_area), table.concat({
        'Nulla ut lacus ut tortor semper consectetur.',
        'Nam scelerisque ligula vitae magna varius, vel porttitor tellus egestas.',
        'Suspendisse aliquet dolor ac velit maximus, ut tempor lorem tincidunt.',
        'Ut eu orci non nibh hendrerit posuere.',
        'Sed euismod odio eu fringilla bibendum.',
        'Etiam dignissim diam nec aliquet facilisis.',
        'Integer tristique purus at tellus luctus, vel aliquet sapien sollicitudin.',
        'Fusce ornare est vitae urna feugiat, vel interdum quam vestibulum.',
        '10: Vivamus id felis scelerisque, lobortis diam ut, mollis nisi.',
        '_onec quis lectus ac erat placerat eleifend.',
    }, '\n'))

    simulate_mouse_click(text_area, 44, 10)
    simulate_input_keys('KEYBOARD_CURSOR_RIGHT')

    expect.eq(read_rendered_text(text_area), table.concat({
        'Nam scelerisque ligula vitae magna varius, vel porttitor tellus egestas.',
        'Suspendisse aliquet dolor ac velit maximus, ut tempor lorem tincidunt.',
        'Ut eu orci non nibh hendrerit posuere.',
        'Sed euismod odio eu fringilla bibendum.',
        'Etiam dignissim diam nec aliquet facilisis.',
        'Integer tristique purus at tellus luctus, vel aliquet sapien sollicitudin.',
        'Fusce ornare est vitae urna feugiat, vel interdum quam vestibulum.',
        '10: Vivamus id felis scelerisque, lobortis diam ut, mollis nisi.',
        'Donec quis lectus ac erat placerat eleifend.',
        '_enean non orci id erat malesuada pharetra.',
    }, '\n'))

    simulate_mouse_click(text_area, 0, 2)
    simulate_input_keys('KEYBOARD_CURSOR_LEFT')

    expect.eq(read_rendered_text(text_area), table.concat({
        'Nulla ut lacus ut tortor semper consectetur._',
        'Nam scelerisque ligula vitae magna varius, vel porttitor tellus egestas.',
        'Suspendisse aliquet dolor ac velit maximus, ut tempor lorem tincidunt.',
        'Ut eu orci non nibh hendrerit posuere.',
        'Sed euismod odio eu fringilla bibendum.',
        'Etiam dignissim diam nec aliquet facilisis.',
        'Integer tristique purus at tellus luctus, vel aliquet sapien sollicitudin.',
        'Fusce ornare est vitae urna feugiat, vel interdum quam vestibulum.',
        '10: Vivamus id felis scelerisque, lobortis diam ut, mollis nisi.',
        'Donec quis lectus ac erat placerat eleifend.',
    }, '\n'))

    screen:dismiss()
end

function test.fast_rewind_words_right()
    local text_area, screen, window = arrange_textarea({w=55})

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
    }, '\n')

    simulate_input_text(text)
    text_area:setCursor(1)

    simulate_input_keys('CUSTOM_CTRL_RIGHT')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60:_Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero.',
    }, '\n'));

    simulate_input_keys('CUSTOM_CTRL_RIGHT')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem_ipsum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero.',
    }, '\n'));

    for i=1,6 do
        simulate_input_keys('CUSTOM_CTRL_RIGHT')
    end

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing_',
        'elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero.',
    }, '\n'));

    simulate_input_keys('CUSTOM_CTRL_RIGHT')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit._',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero.',
    }, '\n'));

    simulate_input_keys('CUSTOM_CTRL_RIGHT')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112:_Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero.',
    }, '\n'));

    for i=1,17 do
        simulate_input_keys('CUSTOM_CTRL_RIGHT')
    end

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero._',
    }, '\n'));

    simulate_input_keys('CUSTOM_CTRL_RIGHT')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero._',
    }, '\n'));

    screen:dismiss()
end

function test.fast_rewind_words_left()
    local text_area, screen, window = arrange_textarea({w=55})

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
    }, '\n')

    simulate_input_text(text)

    simulate_input_keys('CUSTOM_CTRL_LEFT')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        '_ibero.',
    }, '\n'));

    simulate_input_keys('CUSTOM_CTRL_LEFT')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus _ec ',
        'libero.',
    }, '\n'));

    for i=1,8 do
        simulate_input_keys('CUSTOM_CTRL_LEFT')
    end

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        '_nte nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero.',
    }, '\n'));

    simulate_input_keys('CUSTOM_CTRL_LEFT')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112: Sed consectetur, urna sit amet aliquet _gestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero.',
    }, '\n'));

    for i=1,16 do
        simulate_input_keys('CUSTOM_CTRL_LEFT')
    end

    expect.eq(read_rendered_text(text_area), table.concat({
        '_0: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero.',
    }, '\n'));

    simulate_input_keys('CUSTOM_CTRL_LEFT')

    expect.eq(read_rendered_text(text_area), table.concat({
        '_0: Lorem ipsum dolor sit amet, consectetur adipiscing ',
        'elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ',
        'ante nibh porttitor mi, vitae rutrum eros metus nec ',
        'libero.',
    }, '\n'));

    screen:dismiss()
end

function test.fast_rewind_reset_selection()
    local text_area, screen, window = arrange_textarea({w=65})

    local text = table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh porttitor mi, vitae rutrum eros metus nec libero.',
    }, '\n')

    simulate_input_text(text)

    simulate_input_keys('CUSTOM_CTRL_A')

    expect.eq(read_rendered_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
    }, '\n'));

    expect.eq(read_selected_text(text_area), table.concat({
        '60: Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        '112: Sed consectetur, urna sit amet aliquet egestas, ante nibh ',
        'porttitor mi, vitae rutrum eros metus nec libero.',
    }, '\n'));

    simulate_input_keys('CUSTOM_CTRL_LEFT')
    expect.eq(read_selected_text(text_area), '')

    simulate_input_keys('CUSTOM_CTRL_A')

    simulate_input_keys('CUSTOM_CTRL_RIGHT')
    expect.eq(read_selected_text(text_area), '')

    screen:dismiss()
end

function test.render_text_set_by_api()
    local text_area, screen, window, widget = arrange_textarea({w=80})

    local text = table.concat({
        'Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        'Pellentesque dignissim volutpat orci, sed molestie metus elementum vel.',
        'Donec sit amet mattis ligula, ac vestibulum lorem.',
    }, '\n')

    widget:setText(text)
    widget:setCursor(#text + 1)

    expect.eq(read_rendered_text(text_area), text .. '_')

    screen:dismiss()
end

function test.undo_redo_keyboard_changes()
    local text_area, screen, window, widget = arrange_textarea({w=80})

    local text = table.concat({
        'Lorem ipsum dolor sit amet. ',
    }, '\n')

    function reset_text()
        text_area:setText(text)
        text_area:setCursor(#text + 1)
    end

    reset_text()

    -- undo single char
    simulate_input_text('A')

    expect.eq(read_rendered_text(text_area), text .. 'A_')

    simulate_input_keys('CUSTOM_CTRL_Z')

    expect.eq(read_rendered_text(text_area), text .. '_')

    simulate_input_keys('CUSTOM_CTRL_Y')

    expect.eq(read_rendered_text(text_area), text .. 'A_')

    -- undo fast written text as group
    reset_text()
    simulate_input_text('123')

    expect.eq(read_rendered_text(text_area), text .. '123_')

    simulate_input_keys('CUSTOM_CTRL_Z')

    expect.eq(read_rendered_text(text_area), text .. '_')

    simulate_input_keys('CUSTOM_CTRL_Y')

    expect.eq(read_rendered_text(text_area), text .. '123_')

    -- undo cut feature
    reset_text()
    simulate_input_text('123')

    expect.eq(read_rendered_text(text_area), text .. '123_')

    simulate_input_keys('CUSTOM_CTRL_A')
    simulate_input_keys('CUSTOM_CTRL_X')

    expect.eq(read_rendered_text(text_area), '_')

    simulate_input_keys('CUSTOM_CTRL_Z')

    expect.eq(read_rendered_text(text_area), text .. '123_')

    simulate_input_keys('CUSTOM_CTRL_Y')

    expect.eq(read_rendered_text(text_area), '_')

    -- undo paste feature
    reset_text()

    simulate_input_keys('CUSTOM_CTRL_V')

    expect.eq(read_rendered_text(text_area), text .. text .. '123_')

    simulate_input_keys('CUSTOM_CTRL_Z')

    expect.eq(read_rendered_text(text_area), text .. '_')

    simulate_input_keys('CUSTOM_CTRL_Y')

    expect.eq(read_rendered_text(text_area), text .. text .. '123_')

    -- undo enter
    reset_text()

    simulate_input_keys('SELECT')

    expect.eq(read_rendered_text(text_area), text .. '\n_')

    simulate_input_keys('CUSTOM_CTRL_Z')

    expect.eq(read_rendered_text(text_area), text .. '_')

    simulate_input_keys('CUSTOM_CTRL_Y')

    expect.eq(read_rendered_text(text_area), text .. '\n_')

    -- undo backspace
    reset_text()

    simulate_input_keys('STRING_A000')

    expect.eq(read_rendered_text(text_area), text:sub(1, #text - 1) .. '_')

    simulate_input_keys('CUSTOM_CTRL_Z')

    expect.eq(read_rendered_text(text_area), text .. '_')

    simulate_input_keys('CUSTOM_CTRL_Y')

    expect.eq(read_rendered_text(text_area), text:sub(1, #text - 1) .. '_')

    -- undo line delete
    reset_text()

    simulate_input_keys('CUSTOM_CTRL_U')

    expect.eq(read_rendered_text(text_area), '_')

    simulate_input_keys('CUSTOM_CTRL_Z')

    expect.eq(read_rendered_text(text_area), text .. '_')

    simulate_input_keys('CUSTOM_CTRL_Y')

    expect.eq(read_rendered_text(text_area), '_')

    -- undo delete rest of line
    reset_text()

    text_area:setCursor(5)
    local expected_text = text:sub(1, 4) .. '_' .. text:sub(6, #text)
    expect.eq(read_rendered_text(text_area), expected_text)

    simulate_input_keys('CUSTOM_CTRL_K')

    expect.eq(read_rendered_text(text_area), text:sub(1, 4) .. '_')

    simulate_input_keys('CUSTOM_CTRL_Z')

    expect.eq(read_rendered_text(text_area), expected_text)

    simulate_input_keys('CUSTOM_CTRL_Y')

    expect.eq(read_rendered_text(text_area), text:sub(1, 4) .. '_')

    -- undo delete char
    reset_text()

    text_area:setCursor(5)
    expect.eq(read_rendered_text(text_area), expected_text)

    simulate_input_keys('CUSTOM_DELETE')

    expect.eq(
        read_rendered_text(text_area),
        text:sub(1, 4) .. '_' .. text:sub(7, #text)
    )

    simulate_input_keys('CUSTOM_CTRL_Z')

    expect.eq(read_rendered_text(text_area), expected_text)

    simulate_input_keys('CUSTOM_CTRL_Y')

    expect.eq(
        read_rendered_text(text_area),
        text:sub(1, 4) .. '_' .. text:sub(7, #text)
    )

    -- undo delete last word
    reset_text()

    simulate_input_keys('CUSTOM_CTRL_W')
    expect.eq(read_rendered_text(text_area), 'Lorem ipsum dolor sit _')

    simulate_input_keys('CUSTOM_CTRL_Z')

    expect.eq(read_rendered_text(text_area), text .. '_')

    simulate_input_keys('CUSTOM_CTRL_Y')

    expect.eq(read_rendered_text(text_area), 'Lorem ipsum dolor sit _')

    -- undo API setText
    reset_text()

    widget:clearHistory()
    widget:setText('Random new text')
    widget:setCursor(1)
    expect.eq(read_rendered_text(text_area), '_andom new text')

    simulate_input_keys('CUSTOM_CTRL_Z')

    expect.eq(read_rendered_text(text_area), text .. '_')

    simulate_input_keys('CUSTOM_CTRL_Y')

    expect.eq(read_rendered_text(text_area), '_andom new text')

    screen:dismiss()
end

function test.clear_undo_redo_history()
    local text_area, screen, window, widget = arrange_textarea({w=80})

    local text = table.concat({
        'Lorem ipsum dolor sit amet. ',
    }, '\n')

    text_area:setText(text)
    text_area:setCursor(#text + 1)

    simulate_input_text('A')
    simulate_input_text(' ')
    simulate_input_text('longer text')

    expect.eq(read_rendered_text(text_area), text .. 'A longer text_')

    widget:clearHistory()
    simulate_input_keys('CUSTOM_CTRL_Z')

    expect.eq(read_rendered_text(text_area), text .. 'A longer text_')

    screen:dismiss()
end

function test.render_new_lines_in_one_line_mode()
    local text_area, screen, window, widget = arrange_textarea({
        w=80,
        one_line_mode=true
    })

    local text_table = {
        'Lorem ipsum dolor sit amet, ',
        'consectetur adipiscing elit.',
    }

    widget:setText(table.concat(text_table, '\n'))

    widget:setCursor(1)

    expect.eq(read_rendered_text(text_area), '_' .. table.concat(text_table, CP437_NEW_LINE):sub(2))

    widget:setText('')
    simulate_input_text(' test')
    simulate_input_text('\n')
    simulate_input_text(' test')

    expect.eq(
        read_rendered_text(text_area),
        ' test' .. CP437_NEW_LINE .. ' test' .. '_'
    )

    screen:dismiss()
end

function test.should_ignore_submit_in_one_line_mode()
    local text_area, screen, window, widget = arrange_textarea({
        w=80,
        one_line_mode=true
    })

    local text = 'Lorem ipsum dolor sit amet'

    widget:setText(text)

    widget:setCursor(1)

    simulate_input_keys('SELECT')

    expect.eq(read_rendered_text(text_area), '_' .. text:sub(2))

    screen:dismiss()
end

function test.should_scroll_horizontally_in_one_line_mode()
    local text_area, screen, window, widget = arrange_textarea({
        w=80,
        one_line_mode=true
    })

    local text = 'Lorem ipsum dolor sit amet, consectetur adipiscing elit. Pellentesque dignissim volutpat orci, sed'

    widget:setText(text)

    widget:setCursor(1)

    expect.eq(read_rendered_text(text_area), '_' .. text:sub(2, 80))

    widget:setCursor(81)

    expect.eq(read_rendered_text(text_area), text:sub(2, 80) .. '_')

    widget:setCursor(90)

    expect.eq(read_rendered_text(text_area), text:sub(11, 89) .. '_')

    widget:setCursor(2)

    expect.eq(read_rendered_text(text_area), '_' .. text:sub(3, 81))

    screen:dismiss()
end

function test.should_reset_horizontal_in_one_line_mode()
    local text_area, screen, window, widget = arrange_textarea({
        w=40,
        one_line_mode=true
    })

    local text = 'Lorem ipsum dolor sit amet, consectetur adipiscing elit. Pellentesque dignissim volutpat orci, sed'

    widget:setText(text)

    widget:setCursor(#text + 1)

    expect.eq(read_rendered_text(text_area), text:sub(-39) .. '_')

    widget:setText('Lorem ipsum')

    expect.eq(read_rendered_text(text_area), 'Lorem ipsum_')

    screen:dismiss()
end
