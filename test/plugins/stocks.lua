config.mode = 'fortress'
config.target = 'overlay'

local gui = require('gui')
local overlay = require('plugins.overlay')

local stocks = df.global.game.main_interface.stocks

-- feed a key to the stocks overlay widget's input handler. This is the same
-- code path the viewscreen's interposed feed() takes; we invoke it directly
-- because the focus string cache only refreshes on game frames, which tests
-- cannot wait for.
local function send_key(key)
    overlay.get_state().db['stocks.overlay'].widget:onInput({[key]=true})
end

local function set_stocks_open(open)
    if stocks.open ~= open then
        gui.simulateInput(dfhack.gui.getDFViewscreen(true), 'D_STOCKS')
    end
end

local function expanded_count()
    local n = 0
    for i=0,#stocks.current_type_a_expanded-1 do
        if stocks.current_type_a_expanded[i] then n = n + 1 end
    end
    return n
end

local saved_open

config.wrapper = function(test_fn)
    saved_open = stocks.open
    -- the widget db can be empty if a rescan is still in progress
    overlay.rescan()
    return dfhack.with_finalize(function()
        -- close and reopen so the game repopulates any lists the test mutated
        set_stocks_open(false)
        set_stocks_open(saved_open)
    end, test_fn)
end

function test.collapse_hotkey_collapses_all()
    set_stocks_open(true)
    local num_sections = #stocks.current_type_a_expanded
    expect.true_(num_sections > 0)
    for i=0,num_sections-1 do
        stocks.current_type_a_expanded[i] = true
    end
    stocks.scroll_position_item = 5
    send_key('CUSTOM_CTRL_X')
    expect.eq(0, expanded_count())
    -- collapsing resets the item scroll position so it stays in bounds
    expect.eq(0, stocks.scroll_position_item)
end

function test.expand_hotkey_expands_all()
    set_stocks_open(true)
    for i=0,#stocks.current_type_a_expanded-1 do
        stocks.current_type_a_expanded[i] = false
    end
    send_key('CUSTOM_CTRL_Z')
    expect.eq(#stocks.current_type_a_expanded, expanded_count())
end

function test.remove_empty_keeps_type_scroll_in_bounds()
    set_stocks_open(true)
    stocks.scroll_position_type = #stocks.filtered_type_list + 10
    send_key('CUSTOM_CTRL_E')
    expect.true_(stocks.scroll_position_type >= 0)
    expect.true_(stocks.scroll_position_type < #stocks.filtered_type_list)
end

function test.closed_stocks_ignores_overlay_keys()
    set_stocks_open(false)
    -- the stocks.open guard must make every hotkey a no-op while the page is
    -- closed, even if an activation slips through as the page closes
    local n = expanded_count()
    send_key('CUSTOM_CTRL_X')
    send_key('CUSTOM_CTRL_Z')
    send_key('CUSTOM_CTRL_E')
    expect.eq(n, expanded_count())
end
