config.target = 'core'

-- hints for the typechecker
expect = expect or require('test_util.expect')
test = test or {}

local layout = require('gui.dflayout')
local ftb = layout.fort.toolbars

local function combine_comment(comment, suffix)
    if comment and suffix then
        return comment .. ': ' .. suffix
    end
    return comment or suffix
end

------ BEGIN MAGIC NUMBERS ------

local gui = require('gui')

local flush_sizes = {
    layout.MINIMUM_INTERFACE_SIZE, -- 114x46; from 912x552 with 8x12 UI tiles
    gui.mkdims_wh(0, 0, 180, 90),  -- 75% interface in 1920x1080 with 8x12 UI tiles
    gui.mkdims_wh(0, 0, 240, 90),  -- 100% interface in 1920x1080 with 8x12 UI tiles
    gui.mkdims_wh(0, 0, 480, 180), -- 100% interface in 3840x2160 with 8x12 UI tiles
}

local MINIMUM_INTERFACE_WIDTH = layout.MINIMUM_INTERFACE_SIZE.width
local LARGEST_CHECKED_INTERFACE_WIDTH = 210 -- traffic is the last to start "tracking" its center button at 196 interface width

local function for_all_checked_interface_widths(fn)
    for w = MINIMUM_INTERFACE_WIDTH, LARGEST_CHECKED_INTERFACE_WIDTH do
        local interface_size = gui.mkdims_wh(0, 0, w, 46)
        fn(interface_size)
    end
end

-- Most of the magic constants listed below are related to these values, so warn
-- if these differ from the baseline values.
function test.toolbar_positions_baseline()
    expect.eq(ftb.left.width, 32, 'unexpected fort left toolbar width; many tests will probably fail')
    expect.eq(ftb.center.width, 53, 'unexpected fort center toolbar width; many tests will probably fail')
end

local function no_growth()
    return 0
end
local function one_for_one_growth(delta)
    return delta
end
local function odd_one_for_two_growth(delta) -- used when starting on an odd width
    return delta // 2
end
local function even_one_for_two_growth(delta) -- used when starting on an even width
    return (delta+1) // 2
end

-- Fort mode center/secondary toolbar movement can be described in phases.
-- - The first phase can be "no growth" (toolbar is already in sync with a
--   not-yet-moving center toolbar), or "one for one" (toolbar is catching to up
--   the center toolbar).
-- - If it starts in "one for one", it may transition to "no growth" if it
--   catches up to the center toolbar before it starts moving.
-- - The final phase should be a "one for two" growth phase (in sync with center
--   toolbar).

-- The offset values can observed as the UI x-coord (0 based) that
-- `devel/inspect-screen` shows while the mouse is positioned over the left edge
-- of the toolbar in question while using a 100% interface size and adjusting
-- the window width.

local FORT_CENTER_MOVES_WIDTH = 131

local fort_center_phases = {
    { starting_width = MINIMUM_INTERFACE_WIDTH, offset = 39, growth = no_growth },
    { starting_width = FORT_CENTER_MOVES_WIDTH,    offset = 40, growth = odd_one_for_two_growth },
}

local fort_center_secondary_phases = {
    {
        name = 'dig',
        { starting_width = MINIMUM_INTERFACE_WIDTH, offset = 1,  growth = one_for_one_growth },
        { starting_width = 178,                     offset = 64, growth = even_one_for_two_growth },
    },
    {
        name = 'chop',
        { starting_width = MINIMUM_INTERFACE_WIDTH, offset = 37, growth = one_for_one_growth },
        { starting_width = 121,                     offset = 44, growth = no_growth },
        { starting_width = FORT_CENTER_MOVES_WIDTH, offset = 45, growth = odd_one_for_two_growth },
    },
    {
        name = 'gather',
        { starting_width = MINIMUM_INTERFACE_WIDTH, offset = 37, growth = one_for_one_growth },
        { starting_width = 126,                     offset = 48, growth = no_growth },
        { starting_width = FORT_CENTER_MOVES_WIDTH, offset = 49, growth = odd_one_for_two_growth },
    },
    {
        name = 'smooth',
        { starting_width = MINIMUM_INTERFACE_WIDTH, offset = 25, growth = one_for_one_growth },
        { starting_width = 153,                     offset = 64, growth = odd_one_for_two_growth },
    },
    {
        name = 'erase',
        { starting_width = MINIMUM_INTERFACE_WIDTH, offset = 56, growth = no_growth },
        { starting_width = FORT_CENTER_MOVES_WIDTH, offset = 57, growth = odd_one_for_two_growth },
    },
    {
        name = 'stockpile',
        { starting_width = MINIMUM_INTERFACE_WIDTH, offset = 65, growth = no_growth },
        { starting_width = FORT_CENTER_MOVES_WIDTH, offset = 66, growth = odd_one_for_two_growth },
    },
    {
        name = 'stockpile_paint',
        { starting_width = MINIMUM_INTERFACE_WIDTH, offset = 65, growth = no_growth },
        { starting_width = FORT_CENTER_MOVES_WIDTH, offset = 66, growth = odd_one_for_two_growth },
    },
    {
        name = 'burrow_paint',
        { starting_width = MINIMUM_INTERFACE_WIDTH, offset = 74, growth = no_growth },
        { starting_width = FORT_CENTER_MOVES_WIDTH, offset = 75, growth = odd_one_for_two_growth },
    },
    {
        name = 'traffic',
        { starting_width = MINIMUM_INTERFACE_WIDTH, offset = 33,  growth = one_for_one_growth },
        { starting_width = 198,                     offset = 116, growth = even_one_for_two_growth },
    },
    {
        name = 'mass_designation',
        { starting_width = MINIMUM_INTERFACE_WIDTH, offset = 65, growth = one_for_one_growth },
        { starting_width = 144,                     offset = 94, growth = even_one_for_two_growth },
    },
}

-- find the most advanced phase that starts before interface_width and apply its growth rule
local function phased_offset(interface_width, phases)
    for i = #phases, 1, -1 do
        local phase = phases[i]
        local delta = interface_width - phase.starting_width
        if delta >= 0 then
            return phase.offset + phase.growth(delta)
        end
    end
end

------ END MAGIC NUMBERS ------

-- left toolbar is always flush to left and bottom (0 left- and bottom-offsets)
local function expect_bottom_left_frame(a, interface_size, w, h, comment)
    local c = curry(combine_comment, comment)
    return expect.eq(a.l, 0, c('not flush to left'))
        and expect.eq(a.b, 0, c('not flush to bottom'))
        and expect.eq(a.w, w, c('unexpected width'))
        and expect.eq(a.h, h, c('unexpected height'))
        and expect.eq(a.r, interface_size.width - (0 + a.w), c('right offset does not fill i/f width'))
        and expect.eq(a.t, interface_size.height - (a.h + 0), c('top offset does not fill i/f height'))
end

function test.fort_left_toolbar_positions()
    local left = ftb.left
    for _, size in ipairs(flush_sizes) do
        local size_str = ('%dx%d'):format(size.width, size.height)
        expect_bottom_left_frame(left.frame(size), size, left.width, layout.TOOLBAR_HEIGHT, size_str)
    end
    for_all_checked_interface_widths(function(size)
        local size_str = ('%dx%d'):format(size.width, size.height)
        expect_bottom_left_frame(left.frame(size), size, left.width, layout.TOOLBAR_HEIGHT, size_str)
    end)
end

-- right toolbar is always flush to right and bottom (0 right- and bottom-offsets)
local function expect_bottom_right_frame(a, interface_size, w, h, comment)
    local c = curry(combine_comment, comment)
    expect.eq(a.r, 0, c('not flush to right'))
    expect.eq(a.b, 0, c('not flush to bottom'))
    expect.eq(a.w, w, c('unexpected width'))
    expect.eq(a.h, h, c('unexpected height'))
    expect.eq(a.l, interface_size.width - (a.w + 0), c('left offset does not fill i/f width'))
    expect.eq(a.t, interface_size.height - (a.h + 0), c('top offset does not fill i/f height'))
end

function test.fort_right_toolbar_positions()
    local left = ftb.right
    for _, size in ipairs(flush_sizes) do
        local size_str = ('%dx%d'):format(size.width, size.height)
        expect_bottom_right_frame(left.frame(size), size, left.width, layout.TOOLBAR_HEIGHT, size_str)
    end
    for_all_checked_interface_widths(function(size)
        local size_str = ('%dx%d'):format(size.width, size.height)
        expect_bottom_right_frame(left.frame(size), size, left.width, layout.TOOLBAR_HEIGHT, size_str)
    end)
end

-- center toolbar is flush to bottom (0 bottom-offset) and has a phase-defined left-offset
local function expect_bottom_center_frame(a, interface_size, w, h, l, comment)
    local c = curry(combine_comment, comment)
    expect.eq(a.l, l, c('center left-offset'))
    expect.eq(a.b, 0, c('not flush to bottom'))
    expect.eq(a.w, w, c('unexpected width'))
    expect.eq(a.h, h, c('unexpected height'))
    expect.eq(a.r, interface_size.width - (a.l + a.w), c('right offset does not fill i/f width'))
    expect.eq(a.t, interface_size.height - (a.h + 0), c('top offset does not fill i/f height'))
end

function test.fort_center_toolbar_positions()
    local center = ftb.center
    for_all_checked_interface_widths(function(size)
        local size_str = ('%dx%d'):format(size.width, size.height)
        local expected_l = phased_offset(size.width, fort_center_phases)
        expect_bottom_center_frame(center.frame(size), size, center.width, layout.TOOLBAR_HEIGHT, expected_l, size_str)
    end)
end

-- secondary toolbars are just above the bottom toolbars (layout.TOOLBAR_HEIGHT bottom-offset) and have phase-defined left-offsets
local function expect_center_secondary_frame(a, interface_size, w, h, l, comment)
    local c = curry(combine_comment, comment)
    expect.eq(a.l, l, c('center left-offset'))
    expect.eq(a.b, layout.TOOLBAR_HEIGHT, c('not directly above bottom toolbar'))
    expect.eq(a.w, w, c('unexpected width'))
    expect.eq(a.h, h, c('unexpected height'))
    expect.eq(a.r, interface_size.width - (a.l + a.w), c('right offset does not fill i/f width'))
    expect.eq(a.t, interface_size.height - (a.h + a.b), c('top offset does not fill i/f height'))
end

for _, phases in ipairs(fort_center_secondary_phases) do
    local name = phases.name
    local toolbar = layout.fort.secondary_toolbars[name]
    test[('fort_secondary_%s_toolbar_positions'):format(name)] = function()
        for_all_checked_interface_widths(function(size)
            expect_center_secondary_frame(
                toolbar.frame(size), size,
                toolbar.width, layout.SECONDARY_TOOLBAR_HEIGHT,
                phased_offset(size.width, phases),
                ('%s: %dx%d'):format(name, size.width, size.height))
        end)
    end
end
