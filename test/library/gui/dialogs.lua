config.mode = 'fortress'

local gui = require('gui')
local function send_keys(...)
    local keys = {...}
    for _,key in ipairs(keys) do
        gui.simulateInput(dfhack.gui.getCurViewscreen(true), key)
    end
end

local xtest = {} -- use to temporarily disable tests (change `function test.somename` to `function xtest.somename`)
local wait = function()
    --delay(30) -- enable for debugging the tests
end

local dialogs = require('gui.dialogs')

function test.ListBox_opens_and_closes()
    local before_scr = dfhack.gui.getCurViewscreen(true)
    local choices = {{
        text = 'ListBox_opens_and_closes'
    }}
    local lb = dialogs.ListBox({choices = choices})
    expect.eq(before_scr, dfhack.gui.getCurViewscreen(true), "creating a ListBox object should not change CurViewscreen")
    lb:show()
    wait()
    expect.ne(before_scr, dfhack.gui.getCurViewscreen(true), "ListBox:show should change CurViewscreen")
    send_keys('LEAVESCREEN')
    expect.eq(before_scr, dfhack.gui.getCurViewscreen(true), "Pressing LEAVESCREEN should return us to previous screen")
end

function test.ListBox_closes_on_select()
    local before_scr = dfhack.gui.getCurViewscreen(true)

    local args = {}
    local mock_cb = mock.func()
    local mock_cb2 = mock.func()
    args.on_select = mock_cb
    args.on_select2 = mock_cb2
    args.choices = {{
        text = 'ListBox_closes_on_select'
    }}

    local lb = dialogs.ListBox(args)
    lb:show()
    wait()
    send_keys('SELECT')

    expect.eq(1, mock_cb.call_count)
    expect.eq(0, mock_cb2.call_count)

    expect.eq(before_scr, dfhack.gui.getCurViewscreen(true), "Selecting an item should return us to previous screen")
end

function test.ListBox_closes_on_select2()
    local before_scr = dfhack.gui.getCurViewscreen(true)

    local args = {}
    local mock_cb = mock.func()
    local mock_cb2 = mock.func()
    args.on_select = mock_cb
    args.on_select2 = mock_cb2
    args.choices = {{
        text = 'ListBox_closes_on_select2'
    }}

    local lb = dialogs.ListBox(args)
    lb:show()
    wait()
    send_keys('SEC_SELECT')
    
    expect.eq(0, mock_cb.call_count)
    expect.eq(1, mock_cb2.call_count)

    expect.eq(before_scr, dfhack.gui.getCurViewscreen(true), "Selecting an item should return us to previous screen")
end

function test.ListBox_stays_open_with_multi_select()
    local before_scr = dfhack.gui.getCurViewscreen(true)

    local args = {}
    local mock_cb = mock.func()
    local mock_cb2 = mock.func()
    args.on_select = mock_cb
    args.on_select2 = mock_cb2
    args.dismiss_on_select = false
    args.choices = {{
        text = 'ListBox_stays_open_with_multi_select'
    }}

    local lb = dialogs.ListBox(args)
    lb:show()
    local lb_scr = dfhack.gui.getCurViewscreen(true)
    wait()
    send_keys('SELECT')

    expect.eq(lb_scr, dfhack.gui.getCurViewscreen(true), "Selecting an item should NOT close the ListBox")

    send_keys('SEC_SELECT')
    expect.eq(before_scr, dfhack.gui.getCurViewscreen(true), "With default dismiss_on_select2 it should return us to previous screen")
    
    expect.eq(1, mock_cb.call_count)
    expect.eq(1, mock_cb2.call_count)
end

function test.ListBox_stays_open_with_multi_select2()
    local before_scr = dfhack.gui.getCurViewscreen(true)

    local args = {}
    local mock_cb = mock.func()
    local mock_cb2 = mock.func()
    args.on_select = mock_cb
    args.on_select2 = mock_cb2
    args.dismiss_on_select2 = false
    args.choices = {{
        text = 'ListBox_stays_open_with_multi_select2'
    }}

    local lb = dialogs.ListBox(args)
    lb:show()
    local lb_scr = dfhack.gui.getCurViewscreen(true)
    wait()
    send_keys('SEC_SELECT')

    expect.eq(lb_scr, dfhack.gui.getCurViewscreen(true), "Sec-selecting an item should NOT close the ListBox")

    send_keys('SELECT')
    expect.eq(before_scr, dfhack.gui.getCurViewscreen(true), "With default dismiss_on_select it should return us to previous screen")
    
    expect.eq(1, mock_cb.call_count)
    expect.eq(1, mock_cb2.call_count)
end

function test.ListBox_with_multi_select()
    local before_scr = dfhack.gui.getCurViewscreen(true)

    local args = {}
    local mock_cb = mock.func()
    local mock_cb2 = mock.func()
    args.on_select = mock_cb
    args.on_select2 = mock_cb2
    args.dismiss_on_select = false
    args.dismiss_on_select2 = false
    args.choices = {{
        text = 'ListBox_with_multi_select'
    },{
        text = 'item2'
    },{
        text = 'item3'
    }
    }

    local lb = dialogs.ListBox(args)
    lb:show()
    local lb_scr = dfhack.gui.getCurViewscreen(true)
    wait()
    send_keys('SELECT')
    send_keys('STANDARDSCROLL_DOWN')
    wait()
    send_keys('SEC_SELECT')
    send_keys('STANDARDSCROLL_DOWN')
    wait()
    send_keys('SELECT')
    
    expect.eq(2, mock_cb.call_count)
    expect.eq(1, mock_cb2.call_count)
    
    expect.eq(lb_scr, dfhack.gui.getCurViewscreen(true), "With both dismiss_on_select and dismiss_on_select2 false the ListBox should stay open")
    
    send_keys('LEAVESCREEN')
    expect.eq(before_scr, dfhack.gui.getCurViewscreen(true), "Pressing LEAVESCREEN should still return us to previous screen")
end
