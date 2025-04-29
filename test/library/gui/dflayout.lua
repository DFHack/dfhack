config.target = 'core'

-- hints for the typechecker
expect = expect or require('test_util.expect')
test = test or {}

local layout = require('gui.dflayout')
local ftb_layouts = layout.element_layouts.fort.toolbars
local ftb_elements = layout.elements.fort.toolbars

local function combine_comment(comment, suffix)
    if comment and suffix then
        return comment .. ': ' .. suffix
    end
    return comment or suffix
end

------ BEGIN MAGIC NUMBERS ------

local gui = require('gui')

-- 114x46; from 912x552 with 8x12 UI tiles
local MIN_INTERFACE = gui.mkdims_wh(0, 0, layout.MINIMUM_INTERFACE_SIZE.width, layout.MINIMUM_INTERFACE_SIZE.height)
-- 75% interface in 1920x1080 with 8x12 UI tiles
local BIG_PARTIAL_INTERFACE = gui.mkdims_wh(30, 0, 180, 90)
-- 100% interface in 1920x1080 with 8x12 UI tiles
local BIG_INTERFACE = gui.mkdims_wh(0, 0, 240, 90)

local flush_sizes = {
    MIN_INTERFACE,
    BIG_PARTIAL_INTERFACE,
    BIG_INTERFACE,
    gui.mkdims_wh(0, 0, 480, 180), -- 100% interface in 3840x2160 with 8x12 UI tiles
}

local MINIMUM_INTERFACE_WIDTH = layout.MINIMUM_INTERFACE_SIZE.width
local LARGEST_CHECKED_INTERFACE_WIDTH = 210 -- traffic is the last to start "tracking" its center button at 196 interface width

local function for_all_checked_interface_sizes(fn)
    for _, size in ipairs(flush_sizes) do
        local interface_size = gui.mkdims_wh(0, 0, size.width, size.height)
        fn(interface_size)
    end
    for w = MINIMUM_INTERFACE_WIDTH, LARGEST_CHECKED_INTERFACE_WIDTH do
        local interface_size = gui.mkdims_wh(0, 0, w, 46)
        fn(interface_size)
    end
end

-- Most of the magic constants listed below are related to these values, so warn
-- if these differ from the baseline values.
function test.toolbar_positions_baseline()
    expect.eq(ftb_layouts.left.width, 32, 'unexpected fort left toolbar width; many tests will probably fail')
    expect.eq(ftb_layouts.center.width, 53, 'unexpected fort center toolbar width; many tests will probably fail')
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
    return (delta + 1) // 2
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
    { starting_width = FORT_CENTER_MOVES_WIDTH, offset = 40, growth = odd_one_for_two_growth },
}

local fort_center_secondary_phases = {
    {
        name = 'DIG',
        { starting_width = MINIMUM_INTERFACE_WIDTH, offset = 1,  growth = one_for_one_growth },
        { starting_width = 178,                     offset = 64, growth = even_one_for_two_growth },
    },
    {
        name = 'CHOP',
        { starting_width = MINIMUM_INTERFACE_WIDTH, offset = 37, growth = one_for_one_growth },
        { starting_width = 121,                     offset = 44, growth = no_growth },
        { starting_width = FORT_CENTER_MOVES_WIDTH, offset = 45, growth = odd_one_for_two_growth },
    },
    {
        name = 'GATHER',
        { starting_width = MINIMUM_INTERFACE_WIDTH, offset = 37, growth = one_for_one_growth },
        { starting_width = 126,                     offset = 48, growth = no_growth },
        { starting_width = FORT_CENTER_MOVES_WIDTH, offset = 49, growth = odd_one_for_two_growth },
    },
    {
        name = 'SMOOTH',
        { starting_width = MINIMUM_INTERFACE_WIDTH, offset = 25, growth = one_for_one_growth },
        { starting_width = 153,                     offset = 64, growth = odd_one_for_two_growth },
    },
    {
        name = 'ERASE',
        { starting_width = MINIMUM_INTERFACE_WIDTH, offset = 56, growth = no_growth },
        { starting_width = FORT_CENTER_MOVES_WIDTH, offset = 57, growth = odd_one_for_two_growth },
    },
    {
        name = 'MAIN_STOCKPILE_MODE',
        { starting_width = MINIMUM_INTERFACE_WIDTH, offset = 65, growth = no_growth },
        { starting_width = FORT_CENTER_MOVES_WIDTH, offset = 66, growth = odd_one_for_two_growth },
    },
    {
        name = 'STOCKPILE_NEW',
        { starting_width = MINIMUM_INTERFACE_WIDTH, offset = 65, growth = no_growth },
        { starting_width = FORT_CENTER_MOVES_WIDTH, offset = 66, growth = odd_one_for_two_growth },
    },
    {
        name = 'Add new burrow',
        { starting_width = MINIMUM_INTERFACE_WIDTH, offset = 74, growth = no_growth },
        { starting_width = FORT_CENTER_MOVES_WIDTH, offset = 75, growth = odd_one_for_two_growth },
    },
    {
        name = 'TRAFFIC',
        { starting_width = MINIMUM_INTERFACE_WIDTH, offset = 33,  growth = one_for_one_growth },
        { starting_width = 198,                     offset = 116, growth = even_one_for_two_growth },
    },
    {
        name = 'ITEM_BUILDING',
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

--- Test the toolbar frame functions: *_toolbar_positions ---

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
    local w = ftb_layouts.left.width
    local left_frame = ftb_elements.left.frame_fn
    for_all_checked_interface_sizes(function(size)
        local size_str = ('%dx%d'):format(size.width, size.height)
        expect_bottom_left_frame(left_frame(size), size, w, layout.TOOLBAR_HEIGHT, size_str)
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
    local w = ftb_layouts.right.width
    local right_frame = ftb_elements.right.frame_fn
    for_all_checked_interface_sizes(function(size)
        local size_str = ('%dx%d'):format(size.width, size.height)
        expect_bottom_right_frame(right_frame(size), size, w, layout.TOOLBAR_HEIGHT, size_str)
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
    local w = ftb_layouts.center.width
    local center_frame = ftb_elements.center.frame_fn
    for_all_checked_interface_sizes(function(size)
        local size_str = ('%dx%d'):format(size.width, size.height)
        local expected_l = phased_offset(size.width, fort_center_phases)
        expect_bottom_center_frame(center_frame(size), size, w, layout.TOOLBAR_HEIGHT, expected_l, size_str)
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
    local w = layout.element_layouts.fort.secondary_toolbars[name].width
    local frame = layout.elements.fort.secondary_toolbars[name].frame_fn
    test[('fort_secondary_%s_toolbar_positions'):format(name)] = function()
        for_all_checked_interface_sizes(function(size)
            expect_center_secondary_frame(
                frame(size), size,
                w, layout.SECONDARY_TOOLBAR_HEIGHT,
                phased_offset(size.width, phases),
                ('%s: %dx%d'):format(name, size.width, size.height))
        end)
    end
end

--- Test the button frame functions: *_toolbar_button_positions ---

for _, toolbar in ipairs{
    { 'left',                { 'fort', 'toolbars' },           { 'fort', 'toolbar_buttons' } },
    { 'center',              { 'fort', 'toolbars' },           { 'fort', 'toolbar_buttons' } },
    { 'right',               { 'fort', 'toolbars' },           { 'fort', 'toolbar_buttons' } },
    { 'DIG',                 { 'fort', 'secondary_toolbars' }, { 'fort', 'secondary_toolbar_buttons' } },
    { 'CHOP',                { 'fort', 'secondary_toolbars' }, { 'fort', 'secondary_toolbar_buttons' } },
    { 'GATHER',              { 'fort', 'secondary_toolbars' }, { 'fort', 'secondary_toolbar_buttons' } },
    { 'SMOOTH',              { 'fort', 'secondary_toolbars' }, { 'fort', 'secondary_toolbar_buttons' } },
    { 'ERASE',               { 'fort', 'secondary_toolbars' }, { 'fort', 'secondary_toolbar_buttons' } },
    { 'MAIN_STOCKPILE_MODE', { 'fort', 'secondary_toolbars' }, { 'fort', 'secondary_toolbar_buttons' } },
    { 'STOCKPILE_NEW',       { 'fort', 'secondary_toolbars' }, { 'fort', 'secondary_toolbar_buttons' } },
    { 'Add new burrow',      { 'fort', 'secondary_toolbars' }, { 'fort', 'secondary_toolbar_buttons' } },
    { 'TRAFFIC',             { 'fort', 'secondary_toolbars' }, { 'fort', 'secondary_toolbar_buttons' } },
    { 'ITEM_BUILDING',       { 'fort', 'secondary_toolbars' }, { 'fort', 'secondary_toolbar_buttons' } },
} do
    local toolbar_name = toolbar[1]
    local toolbar_path = copyall(toolbar[2])
    table.insert(toolbar_path, toolbar_name)
    local buttons_path = copyall(toolbar[3])
    table.insert(buttons_path, toolbar_name)
    local toolbar_el = safe_index(layout.elements, table.unpack(toolbar_path))
    local button_layouts = safe_index(layout.element_layouts, table.unpack(toolbar_path)).buttons
    local button_els = safe_index(layout.elements, table.unpack(buttons_path))
    test[('fort_%s_toolbar_button_positions'):format(toolbar_name)] = function()
        for_all_checked_interface_sizes(function(size)
            local toolbar_frame = toolbar_el.frame_fn(size)
            local function c(b, d)
                return ('%s %s: %dx%d'):format(b, d, size.width, size.height)
            end
            for button_name, button_spec in pairs(button_layouts) do
                local button_el = button_els[button_name]
                expect.true_(button_el, c(button_name, 'element should exist'))
                if button_el then
                    local frame = button_el.frame_fn(size)
                    local expected_l = toolbar_frame.l + button_spec.offset
                    local expected_r = toolbar_frame.w - (button_spec.offset + button_spec.width) + toolbar_frame.r
                    expect.eq(frame.w, button_spec.width, c(button_name, 'w'))
                    expect.eq(frame.h, toolbar_frame.h, c(button_name, 'h'))
                    expect.eq(frame.l, expected_l, c(button_name, 'l'))
                    expect.eq(frame.r, expected_r, c(button_name, 'r'))
                    expect.eq(frame.t, toolbar_frame.t, c(button_name, 't'))
                    expect.eq(frame.b, toolbar_frame.b, c(button_name, 'b'))
                end
            end
        end)
    end
end

--- Test the overlay helper: overlay_placement_info_* ---

-- 5x5 in the center of the interface area
local function get_centered_ui_el(size)
    local function frame_fn(interface_size)
        local l = (interface_size.width - size.w) // 2
        local b = (interface_size.height - size.h) // 2
        return {
            l = l,
            w = size.w,
            r = interface_size.width - size.w - l,

            t = interface_size.height - size.h - b,
            h = size.h,
            b = b,
        }
    end
    ---@type DFLayout.DynamicUIElement
    return {
        name = 'Centered UI element for testing',
        frame_fn = frame_fn,
    }
end

-- test alignment specification
function test.overlay_placement_info_alignments()
    ---@type { h: DFLayout.Placement.HorizontalAlignment, x: integer }[]
    local has = {
        { h = 'on left',           x = 52 },
        { h = 'align left edges',  x = 55 },
        { h = 'align right edges', x = 57 },
        { h = 'on right',          x = 60 },
    }
    ---@type { v: DFLayout.Placement.VerticalAlignment, y: integer }[]
    local vas = {
        { v = 'above',              y = -26 },
        { v = 'align top edges',    y = -23 },
        { v = 'align bottom edges', y = -21 },
        { v = 'below',              y = -18 },
    }
    local el = get_centered_ui_el{ w = 5, h = 5 }
    for _, ha in ipairs(has) do
        for _, va in ipairs(vas) do
            ---@type DFLayout.Placement.Spec
            local spec = {
                name = 'overlay placement spec for .{h,v}_placement testing',
                size = { w = 3, h = 3 },
                ui_element = el,
                h_placement = ha.h,
                v_placement = va.v,
            }
            local placement = layout.getOverlayPlacementInfo(spec)
            expect.table_eq(
                placement.default_pos,
                { x = ha.x, y = va.y },
                ha.h .. ', ' .. va.v .. ' default_pos')
        end
    end
end

local function sum(...)
    local s = { x = 0, y = 0 }
    for _, v in ipairs({...}) do
        s.x = s.x + (v.x or 0)
        s.y = s.y + (v.y or 0)
    end
    return s
end

-- test offset specification
function test.overlay_placement_info_offset()
    ---@type DFLayout.Placement.Spec
    local base_spec = {
        name = 'overlay placement spec for .offset testing',
        size = { w = 3, h = 3 },
        ui_element = get_centered_ui_el{ w = 5, h = 5 },
        h_placement = 'on left',
        v_placement = 'above',
    }
    local base_placement = layout.getOverlayPlacementInfo(base_spec)

    for _, dx in ipairs{ -1, 0, 2 } do
        for _, dy in ipairs{ -3, 0, 4 } do
            local spec = copyall(base_spec)
            spec.offset = { x = dx, y = dy }
            local placement = layout.getLeftOnlyOverlayPlacementInfo(spec)
            expect.table_eq(
                placement.default_pos,
                sum(base_placement.default_pos, spec.offset),
                'default_pos should incorporate offset')
        end
    end
end

local function size_to_ViewRect(size)
    return gui.ViewRect{ rect = gui.mkdims_wh(0, 0, size.width, size.height) }
end

-- test default_pos specification
function test.overlay_placement_info_default_pos()
    local el = get_centered_ui_el{ w = 5, h = 5 }
    local function spec(dp)
        ---@type DFLayout.Placement.Spec
        local new_spec = {
            name = 'overlay placement spec for .default_pos testing',
            size = { w = 3, h = 3 },
            ui_element = el,
            h_placement = 'align left edges',
            v_placement = 'align bottom edges',
        }
        new_spec.default_pos = dp
        return new_spec
    end

    -- check that insets reflect deltas from the default default_pos; and
    -- "deeper" positions are rejected
    local function test_around_default_pos(ddp)
        local depth = 10
        for _, dx in ipairs{ -depth, 0, depth } do
            for _, dy in ipairs{ -depth, 0, depth } do
                local s = ('(%d,%d) + (%d,%d)'):format(ddp.x, ddp.y, dx, dy)
                local new_dp = sum(ddp, { x = dx, y = dy })
                local new_spec = spec(new_dp)
                if math.abs(new_dp.x) > math.abs(ddp.x)
                    or math.abs(new_dp.y) > math.abs(ddp.y)
                then
                    expect.error(function()
                        layout.getOverlayPlacementInfo(new_spec)
                    end, s .. ': default_pos "deeper" than default should be rejected')
                    goto continue
                end
                local placement = layout.getOverlayPlacementInfo(new_spec)
                expect.table_eq(
                    placement.default_pos,
                    new_dp,
                    ('%s ~= (%d,%d)'):format(s, new_dp.x, new_dp.y))
                local fake_widget = { frame = {} }
                placement.preUpdateLayout_fn(fake_widget, size_to_ViewRect(MIN_INTERFACE))
                local expected_insets = {}
                if new_dp.x > 0 then
                    expected_insets.l = -dx
                    expected_insets.r = 0
                else
                    expected_insets.l = 0
                    expected_insets.r = dx
                end
                if new_dp.y > 0 then
                    expected_insets.t = -dy
                    expected_insets.b = 0
                else
                    expected_insets.t = 0
                    expected_insets.b = dy
                end
                expect.table_eq(
                    fake_widget.frame_inset,
                    expected_insets,
                    'insets should match delta')
                ::continue::
            end
        end
    end

    local l = 54
    local r = 57
    local t = 23
    local b = 20

    -- default_pos from upper-left
    test_around_default_pos{
        x = l + 1,
        y = t + 1,
    }
    -- default_pos from upper-right
    test_around_default_pos{
        x = -(r + 1),
        y = t + 1,
    }
    -- default_pos from lower-left
    test_around_default_pos{
        x = (l + 1),
        y = -(b + 1),
    }
    -- default_pos from lower-right
    test_around_default_pos{
        x = -(r + 1),
        y = -(b + 1),
    }
end

-- test default_pos anchor specification without specific values
function test.overlay_placement_info_default_pos_corner()
    local el = get_centered_ui_el{ w = 5, h = 5 }
    local function spec(dp)
        ---@type DFLayout.Placement.Spec
        local new_spec = {
            name = 'overlay placement spec for .default_pos testing',
            size = { w = 3, h = 3 },
            ui_element = el,
            h_placement = 'on right',
            v_placement = 'below',
        }
        new_spec.default_pos = dp
        return new_spec
    end

    for _, t in ipairs{
        { right = false, top = false, x = 60,  y = -18 },
        { right = true,  top = false, x = -53, y = -18 },
        { right = true,  top = true,  x = -53, y = 27 },
        { right = false, top = true,  x = 60,  y = 27 },
    } do
        local placement = layout.getOverlayPlacementInfo(spec{
            from_right = t.right,
            from_top = t.top,
        })
        local corner = (t.top and 'U' or 'L') .. (t.right and 'R' or 'L')
        expect.table_eq(placement.default_pos, { x = t.x, y = t.y }, corner .. '-anchored default_pos')
    end
end

-- test positioning that would nominal hang off the edge
function test.overlay_placement_info_spanning_edge()
    local function test_placements(gap, h_specs, v_specs)
        local el = get_centered_ui_el{
            w = MIN_INTERFACE.width - 2 * gap,
            h = MIN_INTERFACE.height - 2 * gap,
        }
        for _, h_spec in ipairs(h_specs) do
            for _, v_spec in ipairs(v_specs) do
                ---@type DFLayout.Placement.Spec
                local spec = {
                    name = 'overlay placement spec testing near edges',
                    size = { w = 10, h = 10 },
                    ui_element = el,
                    h_placement = h_spec.placement,
                    v_placement = v_spec.placement,
                }
                local placement = layout.getOverlayPlacementInfo(spec)
                expect.table_eq(
                    placement.default_pos,
                    { x = h_spec.x, y = v_spec.y },
                    ('%d gap %s, %s default_pos'):format(gap, h_spec.placement, v_spec.placement))
            end
        end
    end

    -- if the gaps are not greater than 10, the 10x10 is placed in the corners

    ---@type { h: DFLayout.Placement.HorizontalAlignment, x: integer }[]
    local corner_h_placement = {
        { placement = 'on left',  x = 1 },
        { placement = 'on right', x = 105 },
    }
    ---@type { v: DFLayout.Placement.VerticalAlignment, y: integer }[]
    local corner_v_placement = {
        { placement = 'above', y = -37 },
        { placement = 'below', y = -1 },
    }
    test_placements(0, corner_h_placement, corner_v_placement)
    test_placements(5, corner_h_placement, corner_v_placement)
    test_placements(10, corner_h_placement, corner_v_placement)

    -- once there is extra room, the 10x10 can move away from the corners

    test_placements(11, {
        { placement = 'on left',  x = 2 },
        { placement = 'on right', x = 104 },
    }, {
        { placement = 'above', y = -36 },
        { placement = 'below', y = -2 },
    })
end

-- oversized placement is limited to layout.MINIMUM_INTERFACE_SIZE
function test.overlay_placement_info_oversized()
    local placement = layout.getOverlayPlacementInfo{
        name = 'overlay placement spec for oversized testing',
        size = { w= MIN_INTERFACE.width + 1, h = MIN_INTERFACE.height + 1 },
        ui_element = layout.elements.fort.toolbars.left,
        h_placement = 'align right edges',
        v_placement = 'align bottom edges',
        offset = { x = 1 },
    }
    expect.table_eq(
        placement.frame,
        { w = MIN_INTERFACE.width, h = MIN_INTERFACE.height },
        'oversize area should be limited to MINIMUM_INTERFACE_SIZE')
    expect.table_eq(
        placement.default_pos,
        { x = 1, y = -1 },
        'oversize area should end up in corner')
end

-- test a "real" positioning across multiple interface sizes
function test.overlay_placement_info_DIG_button()
    -- normally, this would be specified as 'on right' of DIG_OPEN_RIGHT without
    -- an offset; but since this is a test, we are exercising an different
    -- horizontal placement
    ---@type DFLayout.Placement.Spec
    local spec = {
        name = 'overlay placement spec for DIG.DIG_MODE_ALL testing',
        size = { w = 26, h = 11 },
        ui_element = layout.elements.fort.secondary_toolbar_buttons.DIG.DIG_MODE_ALL,
        h_placement = 'align left edges',
        v_placement = 'align bottom edges',
        offset = { x = -4 },
    }
    local placement = layout.getOverlayPlacementInfo(spec)
    expect.table_eq(placement.default_pos, {
        x = 42,
        y = -4,
    }, 'default_pos')

    local min_if = size_to_ViewRect(MIN_INTERFACE)
    local big_partial_if = size_to_ViewRect(BIG_PARTIAL_INTERFACE)
    local big_if = size_to_ViewRect(BIG_INTERFACE)

    local frame = {}
    local fake_widget = { frame = frame }

    placement.preUpdateLayout_fn(fake_widget, min_if)
    expect.table_eq(fake_widget.frame_inset, {
        l = 0,
        r = 0,
        t = 0,
        b = 0,
    }, 'zero inset for minimum-size i/f')
    expect.eq(frame, fake_widget.frame, 'frame is same table')
    expect.table_eq(fake_widget.frame, { w = spec.size.w, h = spec.size.h }, 'frame w/h should be populated')

    placement.preUpdateLayout_fn(fake_widget, big_partial_if)
    local big_partial_inset = {
        l = 64,
        r = 2,
        t = 44,
        b = 0,
    }
    expect.table_eq(fake_widget.frame_inset, big_partial_inset, 'inset for big window with partial i/f')
    expect.eq(frame, fake_widget.frame, 'frame is same table')
    expect.table_eq(fake_widget.frame, {
        w = spec.size.w + big_partial_inset.l + big_partial_inset.r,
        h = spec.size.h + big_partial_inset.t + big_partial_inset.b,
    }, 'frame w/h should be populated')

    placement.preUpdateLayout_fn(fake_widget, big_if)
    local big_inset = {
        l = 94,
        r = 32,
        t = 44,
        b = 0,
    }
    expect.table_eq(fake_widget.frame_inset, big_inset, 'inset for big window with full i/f')
    expect.eq(frame, fake_widget.frame, 'frame is same table')
    expect.table_eq(fake_widget.frame, {
        w = spec.size.w + big_inset.l + big_inset.r,
        h = spec.size.h + big_inset.t + big_inset.b,
    }, 'frame w/h should be populated')

    placement.preUpdateLayout_fn(fake_widget, min_if)
    expect.table_eq(fake_widget.frame_inset, {
        l = 0,
        r = 0,
        t = 0,
        b = 0,
    }, 'zero inset for minimum-size i/f')
    expect.eq(frame, fake_widget.frame, 'frame is same table')
    expect.table_eq(fake_widget.frame, { w = spec.size.w, h = spec.size.h }, 'frame w/h should be populated')
end

-- test a "real" positioning with an off-nominal default_pos across multiple interface sizes
function test.overlay_placement_info_ERASE_toolbar()
    ---@type DFLayout.Placement.Spec
    local spec = {
        name = 'overlay placement spec for ERASE toolbar testing',
        size = { w = 26, h = 10 },
        ui_element = layout.elements.fort.secondary_toolbars.ERASE,
        h_placement = 'on right',
        v_placement = 'align bottom edges',
        offset = { x = 1 },
        default_pos = { x = 42 }, -- nominal is 66 == 42 + 24
    }
    local dx = 24 -- due to forced default_pos: shrink default_pos.x, grow inset.l
    local placement = layout.getOverlayPlacementInfo(spec)
    expect.table_eq(placement.default_pos, {
        x = 66 - dx,
        y = -4,
    }, 'requested default_pos')

    local min_if = size_to_ViewRect(MIN_INTERFACE) -- 114
    local big_partial_if = size_to_ViewRect(BIG_PARTIAL_INTERFACE) -- 180
    local big_if = size_to_ViewRect(BIG_INTERFACE) -- 240

    local frame = {}
    local fake_widget = { frame = frame }

    placement.preUpdateLayout_fn(fake_widget, min_if)
    local min_inset = {
        l = 0 + dx,
        r = 0,
        t = 0,
        b = 0,
    }
    expect.table_eq(fake_widget.frame_inset, min_inset, 'mostly zero inset for minimum-size i/f')
    expect.eq(frame, fake_widget.frame, 'frame is same table')
    expect.table_eq(fake_widget.frame, {
        w = min_inset.l + spec.size.w + min_inset.r,
        h = min_inset.t + spec.size.h + min_inset.b,
    }, 'frame w/h should be populated')

    placement.preUpdateLayout_fn(fake_widget, big_partial_if)
    local big_partial_inset = {
        l = 25 + dx,
        r = 41,
        t = 44,
        b = 0,
    }
    expect.table_eq(fake_widget.frame_inset, big_partial_inset, 'inset for big window with partial i/f')
    expect.eq(frame, fake_widget.frame, 'frame is same table')
    expect.table_eq(fake_widget.frame, {
        w = spec.size.w + big_partial_inset.l + big_partial_inset.r,
        h = spec.size.h + big_partial_inset.t + big_partial_inset.b,
    }, 'frame w/h should be populated')

    placement.preUpdateLayout_fn(fake_widget, big_if)
    local big_inset = {
        l = 55 + dx,
        r = 71,
        t = 44,
        b = 0,
    }
    expect.table_eq(fake_widget.frame_inset, big_inset, 'inset for big window with full i/f')
    expect.eq(frame, fake_widget.frame, 'frame is same table')
    expect.table_eq(fake_widget.frame, {
        w = spec.size.w + big_inset.l + big_inset.r,
        h = spec.size.h + big_inset.t + big_inset.b,
    }, 'frame w/h should be populated')

    placement.preUpdateLayout_fn(fake_widget, min_if)
    expect.table_eq(fake_widget.frame_inset, min_inset, 'mostly zero inset for minimum-size i/f')
    expect.eq(frame, fake_widget.frame, 'frame is same table')
    expect.table_eq(fake_widget.frame, {
        w = min_inset.l + spec.size.w + min_inset.r,
        h = min_inset.t + spec.size.h + min_inset.b,
    }, 'frame w/h should be populated')
end

--- Test the experimental overlay helper: overlay_experimental_placement_info_* ---

function test.overlay_experimental_placement_info_orders_list_element_positions()
    for _, t in ipairs{
        -- no scrollbar, wrapped info tab bar
        { w = 114, h = 46, c = 9,  p = { l = 6, w = 73, r = 35, t = 10, h = 27, b = 9 } },
        { w = 114, h = 47, c = 9,  p = { l = 6, w = 73, r = 35, t = 10, h = 27, b = 10 } },
        { w = 114, h = 48, c = 9,  p = { l = 6, w = 73, r = 35, t = 10, h = 27, b = 11 } },
        { w = 114, h = 49, c = 10, p = { l = 6, w = 73, r = 35, t = 10, h = 30, b = 9 } },

        -- no scrollbar, unwrapped info tab bar
        { w = 155, h = 46, c = 9,  p = { l = 6, w = 114, r = 35, t = 8, h = 27, b = 11 } },
        { w = 155, h = 47, c = 10, p = { l = 6, w = 114, r = 35, t = 8, h = 30, b = 9 } },
        { w = 155, h = 48, c = 10, p = { l = 6, w = 114, r = 35, t = 8, h = 30, b = 10 } },
        { w = 155, h = 49, c = 10, p = { l = 6, w = 114, r = 35, t = 8, h = 30, b = 11 } },
        { w = 155, h = 50, c = 11, p = { l = 6, w = 114, r = 35, t = 8, h = 33, b = 9 } },

        -- with scrollbar, wrapped info tab bar
        { w = 114, h = 46, c = 10, p = { l = 6, w = 71, r = 37, t = 10, h = 27, b = 9 } },
        { w = 114, h = 47, c = 10, p = { l = 6, w = 71, r = 37, t = 10, h = 27, b = 10 } },
        { w = 114, h = 48, c = 10, p = { l = 6, w = 71, r = 37, t = 10, h = 27, b = 11 } },
        { w = 114, h = 49, c = 11, p = { l = 6, w = 71, r = 37, t = 10, h = 30, b = 9 } },

        -- with scrollbar, unwrapped info tab bar
        { w = 155, h = 46, c = 10, p = { l = 6, w = 112, r = 37, t = 8, h = 27, b = 11 } },
        { w = 155, h = 47, c = 11, p = { l = 6, w = 112, r = 37, t = 8, h = 30, b = 9 } },
        { w = 155, h = 48, c = 11, p = { l = 6, w = 112, r = 37, t = 8, h = 30, b = 10 } },
        { w = 155, h = 49, c = 11, p = { l = 6, w = 112, r = 37, t = 8, h = 30, b = 11 } },
        { w = 155, h = 50, c = 12, p = { l = 6, w = 112, r = 37, t = 8, h = 33, b = 9 } },
    } do
        local f = layout.experimental_elements.orders.frame_fn({width = t.w, height = t.h}, {
            orders_needs_scrollbar = function(n)
                expect.eq(n, t.p.h // 3, 'displayable row count')
                return t.c > n
            end,
        })
        expect.table_eq(f, t.p, ('%dx%d w/%d orders'):format(t.w, t.h, t.c))
    end
end

function test.overlay_experimental_placement_info_zones_list_element_positions()
    for _, t in ipairs{
        -- no scrollbar, wrapped info tab bar
        { w = 114, h = 46, c = 9,  p = { l = 6, w = 78, r = 30, t = 12, h = 27, b = 7 } },
        { w = 114, h = 47, c = 10, p = { l = 6, w = 78, r = 30, t = 12, h = 30, b = 5 } },
        { w = 114, h = 48, c = 10, p = { l = 6, w = 78, r = 30, t = 12, h = 30, b = 6 } },
        { w = 114, h = 49, c = 10, p = { l = 6, w = 78, r = 30, t = 12, h = 30, b = 7 } },
        { w = 114, h = 50, c = 11, p = { l = 6, w = 78, r = 30, t = 12, h = 33, b = 5 } },

        -- no scrollbar, unwrapped info tab bar
        { w = 155, h = 46, c = 10, p = { l = 6, w = 119, r = 30, t = 10, h = 30, b = 6 } },
        { w = 155, h = 47, c = 10, p = { l = 6, w = 119, r = 30, t = 10, h = 30, b = 7 } },
        { w = 155, h = 48, c = 11, p = { l = 6, w = 119, r = 30, t = 10, h = 33, b = 5 } },
        { w = 155, h = 49, c = 11, p = { l = 6, w = 119, r = 30, t = 10, h = 33, b = 6 } },
        { w = 155, h = 50, c = 11, p = { l = 6, w = 119, r = 30, t = 10, h = 33, b = 7 } },
        { w = 155, h = 51, c = 12, p = { l = 6, w = 119, r = 30, t = 10, h = 36, b = 5 } },

        -- with scrollbar, wrapped info tab bar
        { w = 114, h = 46, c = 10, p = { l = 6, w = 76, r = 32, t = 12, h = 27, b = 7 } },
        { w = 114, h = 47, c = 11, p = { l = 6, w = 76, r = 32, t = 12, h = 30, b = 5 } },
        { w = 114, h = 48, c = 11, p = { l = 6, w = 76, r = 32, t = 12, h = 30, b = 6 } },
        { w = 114, h = 49, c = 11, p = { l = 6, w = 76, r = 32, t = 12, h = 30, b = 7 } },
        { w = 114, h = 50, c = 12, p = { l = 6, w = 76, r = 32, t = 12, h = 33, b = 5 } },

        -- with scrollbar, unwrapped info tab bar
        { w = 155, h = 46, c = 11, p = { l = 6, w = 117, r = 32, t = 10, h = 30, b = 6 } },
        { w = 155, h = 47, c = 11, p = { l = 6, w = 117, r = 32, t = 10, h = 30, b = 7 } },
        { w = 155, h = 48, c = 12, p = { l = 6, w = 117, r = 32, t = 10, h = 33, b = 5 } },
        { w = 155, h = 49, c = 12, p = { l = 6, w = 117, r = 32, t = 10, h = 33, b = 6 } },
        { w = 155, h = 50, c = 12, p = { l = 6, w = 117, r = 32, t = 10, h = 33, b = 7 } },
        { w = 155, h = 51, c = 13, p = { l = 6, w = 117, r = 32, t = 10, h = 36, b = 5 } },
    } do
        local f = layout.experimental_elements.zones.frame_fn({width = t.w, height = t.h}, {
            zones_needs_scrollbar = function(n)
                expect.eq(n, t.p.h // 3, 'displayable row count')
                return t.c > n
            end,
        })
        expect.table_eq(f, t.p, ('%dx%d w/%d zones'):format(t.w, t.h, t.c))
    end
end

local function fully_place(interface_size, frame)
    expect.true_(frame.w, 'should have .w')
    expect.true_(frame.h, 'should have .h')
    local l, r, t, b = frame.l, frame.r, frame.t, frame.b
    if l == nil and r ~= nil then
        l = interface_size.width - (frame.w + r)
    end
    if r == nil and l ~= nil then
        r = interface_size.width - (l + frame.w)
    end
    if t == nil and b ~= nil then
        t = interface_size.height - (frame.h + b)
    end
    if b == nil and t ~= nil then
        b = interface_size.height - (t + frame.h)
    end
    return {
        l = l, w = frame.w, r = r,
        t = t, h = frame.h, b = b,
    }
end

local function apply_insets(interface_size, frame, frame_inset)
    frame = fully_place(interface_size, frame)
    return {
        l = frame.l + frame_inset.l,
        r = frame.r + frame_inset.r,
        w = frame.w - (frame_inset.l + frame_inset.r),
        t = frame.t + frame_inset.t,
        b = frame.b + frame_inset.b,
        h = frame.h - (frame_inset.t + frame_inset.b),
    }
end

local function corner_name(left, top)
    if left then
        if top then
            return 'UL'
        else
            return 'LL'
        end
    else
        if top then
            return 'UR'
        else
            return 'LR'
        end
    end
end

local function check_corner(name, size, placement, fields)
    return function(interface_size, el_f, c)
        local frame = {
            w = placement.frame.w,
            h = placement.frame.h,
        }
        if placement.default_pos.x > 0 then
            frame.l = placement.default_pos.x - 1
        else
            frame.r = -placement.default_pos.x - 1
        end
        if placement.default_pos.y > 0 then
            frame.t = placement.default_pos.y - 1
        else
            frame.b = -placement.default_pos.y - 1
        end
        local fake_widget = { frame = frame }
        placement.preUpdateLayout_fn(fake_widget, interface_size)
        local frame_inset = fake_widget.frame_inset
        for _, d in ipairs{ 'l', 'r', 't', 'b' } do
            expect.ge(frame_inset[d], 0, c .. ' ' .. name .. ' .' .. d .. ' should be non-negative')
        end

        local base = { w = size.w, h = size.h }
        for _, f in ipairs(fields) do
            base[f] = el_f[f]
        end
        expect.table_eq(
            apply_insets(interface_size, frame, frame_inset),
            fully_place(interface_size, base),
            name .. ' corner: ' .. c)
    end
end

function test.overlay_experimental_placement_info_orders_list()
    local orders = layout.experimental_elements.orders
    local size = { w = 2, h = 2 }
    local item_count
    local feature_tests = {
        orders_needs_scrollbar = function(n)
            return item_count > n
        end,
    }

    local function placement(overlay_left, overlay_top, from_right, from_top)
        local corner = corner_name(overlay_left, overlay_top)
        local anchor = corner_name(not from_right, from_top)
        ---@type DFLayout.Placement.HorizontalAlignment
        local hp = overlay_left and 'align left edges' or 'align right edges'
        ---@type DFLayout.Placement.VerticalAlignment
        local vp = overlay_top and 'align top edges' or 'align bottom edges'
        local placement = layout.getOverlayPlacementInfo{
            name = anchor .. '-anchored overlay placement spec for Orders list (' .. corner .. ') testing',
            size = size,
            ui_element = orders,
            h_placement = hp,
            v_placement = vp,
            default_pos = {
                from_right = from_right,
                from_top = from_top,
            },
            feature_tests = feature_tests,
        }
        return placement
    end

    for _, from in ipairs{
        { right = false, top = false }, -- overlay positioned from left, bottom
        { right = false, top = true },  -- overlay positioned from left, top
        { right = true,  top = true },  -- overlay positioned from right, top
        { right = true,  top = false }, -- overlay positioned from right, bottom
    } do
        local anchor_corner = corner_name(not from.right, from.top) .. '-anchored'

        local UL = placement(true, true, from.right, from.top)
        local UR = placement(false, true, from.right, from.top)
        local LR = placement(false, false, from.right, from.top)
        local LL = placement(true, false, from.right, from.top)

        -- check default_pos

        local check_UL = check_corner(anchor_corner .. ' UL', size, UL, {'l', 't'})
        local check_UR = check_corner(anchor_corner .. ' UR', size, UR, {'r', 't'})
        local check_LR = check_corner(anchor_corner .. ' LR', size, LR, {'r', 'b'})
        local check_LL = check_corner(anchor_corner .. ' LL', size, LL, {'l', 'b'})

        for _, t in ipairs{
            -- no scrollbar, wrapped info tab bar
            { w = 114, h = 46, c = 9 },
            { w = 114, h = 47, c = 9 },
            { w = 114, h = 48, c = 9 },
            { w = 114, h = 49, c = 10 },

            -- no scrollbar, unwrapped info tab bar
            { w = 155, h = 46, c = 9 },
            { w = 155, h = 47, c = 10 },
            { w = 155, h = 48, c = 10 },
            { w = 155, h = 49, c = 10 },
            { w = 155, h = 50, c = 11 },

            -- with scrollbar, wrapped info tab bar
            { w = 114, h = 46, c = 10 },
            { w = 114, h = 47, c = 10 },
            { w = 114, h = 48, c = 10 },
            { w = 114, h = 49, c = 11 },

            -- with scrollbar, unwrapped info tab bar
            { w = 155, h = 46, c = 10 },
            { w = 155, h = 47, c = 11 },
            { w = 155, h = 48, c = 11 },
            { w = 155, h = 49, c = 11 },
            { w = 155, h = 50, c = 12 },
        } do
            local interface_size = { width = t.w, height = t.h }
            local c = ('%dx%d w/%d orders'):format(t.w, t.h, t.c)
            item_count = t.c
            local el_f = orders.frame_fn(interface_size, feature_tests)
            check_UL(interface_size, el_f, c)
            check_UR(interface_size, el_f, c)
            check_LR(interface_size, el_f, c)
            check_LL(interface_size, el_f, c)
        end
    end
end

function test.overlay_experimental_placement_info_zones_list()
    local zones = layout.experimental_elements.zones
    local size = { w = 2, h = 2 }
    local item_count
    local feature_tests = {
        zones_needs_scrollbar = function(n)
            return item_count > n
        end
    }

    local function placement(overlay_left, overlay_top, from_right, from_top)
        local corner = corner_name(overlay_left, overlay_top)
        local anchor = corner_name(not from_right, from_top)
        ---@type DFLayout.Placement.HorizontalAlignment
        local hp = overlay_left and 'align left edges' or 'align right edges'
        ---@type DFLayout.Placement.VerticalAlignment
        local vp = overlay_top and 'align top edges' or 'align bottom edges'
        local placement = layout.getOverlayPlacementInfo{
            name = anchor .. '-anchored overlay placement spec for Places/Zones list (' .. corner .. ') testing',
            size = size,
            ui_element = zones,
            h_placement = hp,
            v_placement = vp,
            default_pos = {
                from_right = from_right,
                from_top = from_top,
            },
            feature_tests = feature_tests,
        }
        return placement
    end

    for _, from in ipairs{
        { right = false, top = false }, -- overlay positioned from left, bottom
        { right = false, top = true },  -- overlay positioned from left, top
        { right = true,  top = true },  -- overlay positioned from right, top
        { right = true,  top = false }, -- overlay positioned from right, bottom
    } do
        local anchor_corner = corner_name(not from.right, from.top) .. '-anchored'

        local UL = placement(true, true, from.right, from.top)
        local UR = placement(false, true, from.right, from.top)
        local LR = placement(false, false, from.right, from.top)
        local LL = placement(true, false, from.right, from.top)

        -- check default_pos

        local check_UL = check_corner(anchor_corner .. ' UL', size, UL, {'l', 't'})
        local check_UR = check_corner(anchor_corner .. ' UR', size, UR, {'r', 't'})
        local check_LR = check_corner(anchor_corner .. ' LR', size, LR, {'r', 'b'})
        local check_LL = check_corner(anchor_corner .. ' LL', size, LL, {'l', 'b'})

        for _, t in ipairs{
            -- no scrollbar, wrapped info tab bar
            { w = 114, h = 46, c = 9 },
            { w = 114, h = 47, c = 10 },
            { w = 114, h = 48, c = 10 },
            { w = 114, h = 49, c = 10 },
            { w = 114, h = 50, c = 11 },

            -- no scrollbar, unwrapped info tab bar
            { w = 155, h = 46, c = 10 },
            { w = 155, h = 47, c = 10 },
            { w = 155, h = 48, c = 11 },
            { w = 155, h = 49, c = 11 },
            { w = 155, h = 50, c = 11 },
            { w = 155, h = 51, c = 12 },

            -- with scrollbar, wrapped info tab bar
            { w = 114, h = 46, c = 10 },
            { w = 114, h = 47, c = 11 },
            { w = 114, h = 48, c = 11 },
            { w = 114, h = 49, c = 11 },
            { w = 114, h = 50, c = 12 },

            -- with scrollbar, unwrapped info tab bar
            { w = 155, h = 46, c = 11 },
            { w = 155, h = 47, c = 11 },
            { w = 155, h = 48, c = 12 },
            { w = 155, h = 49, c = 12 },
            { w = 155, h = 50, c = 12 },
            { w = 155, h = 51, c = 13 },
        } do
            local interface_size = { width = t.w, height = t.h }
            local c = ('%dx%d w/%d zones'):format(t.w, t.h, t.c)
            item_count = t.c
            local el_f = zones.frame_fn(interface_size, feature_tests)
            check_UL(interface_size, el_f, c)
            check_UR(interface_size, el_f, c)
            check_LR(interface_size, el_f, c)
            check_LL(interface_size, el_f, c)
        end
    end
end
