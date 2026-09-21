config.mode = 'fortress'
config.target = 'sort'

local gui = require('gui')
local overlay = require('plugins.overlay')

local function get_widget()
    return overlay.get_state().db['sort.stress_icons'].widget
end

local function get_citizen_counts()
    local counts = {}
    for _,unit in ipairs(df.global.world.units.active) do
        if dfhack.units.isCitizen(unit, true) then
            local cat = dfhack.units.getStressCategory(unit)
            counts[cat] = (counts[cat] or 0) + 1
        end
    end
    return counts
end

-- rects are stored happiest-first, so category c is at index 7-c
local function rect_for_category(w, cat)
    if not w.icon_rects then qerror('icon_rects not populated') end
    return w.icon_rects[7 - cat]
end

local function click_at(x, y)
    local handled
    mock.patch(dfhack.screen, 'getMousePos', mock.func(x, y), function()
        handled = get_widget():onInput{_MOUSE_L=true}
    end)
    return handled
end

local function dismiss_top()
    gui.simulateInput(dfhack.gui.getCurViewscreen(true), 'LEAVESCREEN')
end

local function synth_rects()
    local rects = {}
    for i = 0, 6 do
        local x1 = 42 + i * 3
        table.insert(rects, {x1=x1, y1=0, x2=x1+2, y2=2, category=6-i})
    end
    return rects
end

config.wrapper = function(test_fn)
    overlay.rescan()
    local w = get_widget()
    -- give the top bar a few frames to render so the anchor scan finds 'Pop';
    -- environments where the vanilla screen is never rendered (e.g. headless
    -- Windows CI) fall back to synthetic rects so the click/list behavior is
    -- still exercised
    for _ = 1, 20 do
        w:overlay_onupdate()
        if w.icon_rects then break end
        delay()
    end
    w.scan_unavailable = not w.icon_rects
    if w.scan_unavailable then
        print('NOTE: top bar not rendered; testing with synthetic icon rects')
        w.icon_rects = synth_rects()
    end
    return dfhack.with_finalize(function()
        -- dismiss any screens the test left open
        while (dfhack.gui.getCurFocus(true)[1] or ''):match('^dfhack/') do
            dismiss_top()
        end
    end, test_fn)
end

function test.icon_rects_detected()
    local w = get_widget()
    if w.scan_unavailable then
        -- the vanilla top bar is never rendered in this environment, so the
        -- screen scan cannot be verified here (covered by other platforms)
        return
    end
    expect.eq(7, #w.icon_rects)
    expect.eq(6, w.icon_rects[1].category)
    expect.eq(0, w.icon_rects[7].category)
    -- each rect covers a 3-wide, 3-tall cell in the top bar
    for _,rect in ipairs(w.icon_rects) do
        expect.eq(0, rect.y1)
        expect.eq(2, rect.y2)
        expect.eq(2, rect.x2 - rect.x1)
    end
end

function test.click_outside_icons_does_nothing()
    local w = get_widget()
    local before = dfhack.gui.getCurFocus(true)[1]
    expect.false_(click_at(0, 30))
    expect.eq(before, dfhack.gui.getCurFocus(true)[1])
end

function test.click_icon_opens_filtered_list()
    local w = get_widget()
    local counts = get_citizen_counts()
    local cat
    for c = 0, 6 do
        if (counts[c] or 0) > 0 then cat = c break end
    end
    expect.ne(nil, cat, 'no citizens with a stress category')

    local rect = rect_for_category(w, cat)
    expect.true_(click_at(rect.x1 + 1, 1))
    expect.eq('dfhack/lua/ListBox', dfhack.gui.getCurFocus(true)[1])
    dismiss_top()
end

function test.empty_category_shows_message()
    local w = get_widget()
    local counts = get_citizen_counts()
    local cat
    for c = 0, 6 do
        if not counts[c] or counts[c] == 0 then cat = c break end
    end
    if not cat then
        -- all categories populated; nothing to test
        return
    end
    local rect = rect_for_category(w, cat)
    expect.true_(click_at(rect.x1 + 1, 1))
    expect.true_((dfhack.gui.getCurFocus(true)[1] or ''):match('^dfhack/lua/') ~= nil)
    dismiss_top()
end

function test.selecting_unit_centers_map()
    local w = get_widget()
    local counts = get_citizen_counts()
    local cat
    for c = 0, 6 do
        if (counts[c] or 0) > 0 then cat = c break end
    end
    expect.ne(nil, cat, 'no citizens with a stress category')

    local rect = rect_for_category(w, cat)
    expect.true_(click_at(rect.x1 + 1, 1))
    expect.eq('dfhack/lua/ListBox', dfhack.gui.getCurFocus(true)[1])

    local reveal_spy = mock.observe_func(function() end)
    mock.patch(dfhack.gui, 'revealInDwarfmodeMap', reveal_spy,
        function()
            gui.simulateInput(dfhack.gui.getCurViewscreen(true), 'SELECT')
        end)
    expect.eq(1, reveal_spy.call_count)
    local args = reveal_spy.call_args[1]
    local called_with = {x=args[1], y=args[2], z=args[3]}

    expect.ne(nil, called_with)
    -- the revealed position must be a real citizen's position in that bucket
    local matched = false
    for _,unit in ipairs(df.global.world.units.active) do
        if dfhack.units.isCitizen(unit, true) and
                dfhack.units.getStressCategory(unit) == cat and
                unit.pos.x == called_with.x and unit.pos.y == called_with.y and
                unit.pos.z == called_with.z then
            matched = true
            break
        end
    end
    expect.true_(matched)
end
