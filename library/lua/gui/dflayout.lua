local _ENV = mkmodule('gui.dflayout')

local utils = require('utils')

-- Provide data-driven locations for the DF toolbars at the bottom of the
-- screen. Not quite as nice as getting the data from DF directly, but better
-- than hand-rolling calculations for each "interesting" button.

TOOLBAR_HEIGHT = 3
SECONDARY_TOOLBAR_HEIGHT = 3

-- Basic rectangular size class. Should structurally accept gui.dimension (e.g.,
-- get_interface_rect) and gui.ViewRect (e.g., updateLayout's parent_rect), but
-- the LuaLSP disagrees.
--
---@class DFLayout.Interface.Size.class
---@field width integer
---@field height integer

-- An alias that gathers a few types that we know are compatible with our
-- width/height size requirements.
--
---@alias DFLayout.Interface.Size
--- | DFLayout.Interface.Size.class basic width/height size
--- | gui.dimension                 e.g., gui.get_interface_rect()
--- | gui.ViewRect                  e.g., parent_rect supplied to updateLayout subsidiary methods

---@type DFLayout.Interface.Size
MINIMUM_INTERFACE_SIZE = { width = 114, height = 46 }

---@generic T
---@param sequences T[][]
---@return T[]
local function concat_sequences(sequences)
    local collected = {}
    for _, sequence in ipairs(sequences) do
        table.move(sequence, 1, #sequence, #collected + 1, collected)
    end
    return collected
end

---@alias DFLayout.Toolbar.Button { offset: integer, width: integer }
---@alias DFLayout.Toolbar.Buttons table<string,DFLayout.Toolbar.Button> -- multiple entries

---@class DFLayout.Toolbar.Layout
---@field buttons DFLayout.Toolbar.Buttons
---@field width integer

---@class DFLayout.Inset.partial
---@field l? integer Optional gap between the left edge of the frame and the parent.
---@field t? integer Optional gap between the top edge of the frame and the parent.
---@field r? integer Optional gap between the right edge of the frame and the parent.
---@field b? integer Optional gap between the bottom edge of the frame and the parent.

---@class DFLayout.Inset
---@field l integer Gap between the left edge of the frame and the parent.
---@field t integer Gap between the top edge of the frame and the parent.
---@field r integer Gap between the right edge of the frame and the parent.
---@field b integer Gap between the bottom edge of the frame and the parent.

-- Like widgets.Widget.frame, but no optional fields.
--
---@class DFLayout.Frame: DFLayout.Inset
---@field w integer Width
---@field h integer Height

---@alias DFLayout.FrameFn.FeatureTests table<string, fun(...): boolean>

---@alias DFLayout.FrameFn.Features table<string, boolean | integer | string>

-- Function that generates a "full placement" for a given interface size.
--
-- If the FrameFn tests for a feature (by calling a `feature_tests` field), a
-- true result should result in the relevant placement inset being larger
-- (further to the edge of the interface area) than for a false result.
--
---@alias DFLayout.FrameFn fun(interface_size: DFLayout.Interface.Size, feature_tests: DFLayout.FrameFn.FeatureTests): frame: DFLayout.Frame, active_features: DFLayout.FrameFn.Features?

---@alias DFLayout.DynamicUIElement.State boolean | integer | string | table<string, boolean | integer | string>

-- A name-tagged bundle of a DFLayout.FrameFn and its supporting values.
--
-- The `collapsable_inset` field should describe the amount `frame_fn`-reported
-- insets might shrink when computed for interfaces sizes other than
-- MINIMUM_INTERFACE_SIZE. It should also include inset shrinkage due to
-- positive "feature tests" done by `frame_fn`.
--
---@class DFLayout.DynamicUIElement
---@field name string
---@field frame_fn DFLayout.FrameFn function that describes where DF will draw the UI element in a given interface size
---@field collapsable_inset? DFLayout.Inset.partial amount of UI element inset on MINIMUM_INTERFACE_SIZE that might disappear with different sizes or different states
---@field state_fn? fun(): DFLayout.DynamicUIElement.State return non-size state values that affect the placement of the UI element

---@alias DFLayout.Placement.HorizontalAlignment
--- | 'on left'           overlay."right edge col" + 1 == reference_frame."left edge col"
--- | 'align left edges'  overlay."left edge col" == reference_frame."left edge col"
--- | 'align right edges' overlay."right edge col" == reference_frame."right edge col"
--- | 'on right'          overlay."left edge col" == reference_frame."right edge col" + 1
---@alias DFLayout.Placement.VerticalAlignment
--- | 'above'              overlay."bottom edge row" + 1 == reference_frame."top edge row"
--- | 'align top edges'    overlay."top edge row" == reference_frame."top edge row"
--- | 'align bottom edges' overlay."bottom edge row" == reference_frame."bottom edge row"
--- | 'below'              overlay."top edge row" == reference_frame."bottom edge row" + 1

---@alias DFLayout.Placement.Offset { x?: integer, y?: integer }

---@class DFLayout.Placement.DefaultPos
---@field x? integer directly specify a default_pos.x value
---@field y? integer directly specify a default_pos.y value
---@field from_right? boolean automatic default_pos.x should be based on right edge
---@field from_top? boolean automatic default_pos.y should be based on top edge

---@class DFLayout.Placement.Size
---@field w integer
---@field h integer

---@class DFLayout.Placement.Spec
---@field name string used in error messages
---@field size DFLayout.Placement.Size the static size of overlay
---@field ui_element DFLayout.DynamicUIElement a UI element value from the `elements` tree
---@field h_placement DFLayout.Placement.HorizontalAlignment how to align the overlay's horizontal position against the `ui_element`
---@field v_placement DFLayout.Placement.VerticalAlignment how to align the overlay's vertical position against the `ui_element`
---@field offset? DFLayout.Placement.Offset how far to move overlay after alignment with `ui_element`
---@field feature_tests? DFLayout.FrameFn.FeatureTests for internal/testing use

---@class DFLayout.OverlayPlacement.Spec: DFLayout.Placement.Spec
---@field default_pos? DFLayout.Placement.DefaultPos specify an overlay default_pos, or which edges it should be based on

---@alias DFLayout.Placement.GenericAlignment 'place before' | 'align start' | 'align end' | 'place after'

---@class DFLayout.OverlayPlacementInfo
---@field default_pos { x: integer, y: integer } use for the overlay's default_pos
---@field frame widgets.Widget.frame use for the overlay's initial frame
---@field preUpdateLayout_fn fun(self_overlay_widget: widgets.Widget, parent_rect: gui.ViewRect) use as the overlay's preUpdateLayout method (the "self" param is overlay.OverlayWidget)
---@field onRenderBody_fn fun(self_overlay_widget: widgets.Widget, painter: gui.Painter) use as the overlay's onRenderBody_fn method (the "self" param is overlay.OverlayWidget)

--- DF UI element definitions ---

---@alias DFLayout.Toolbar.Widths table<string,integer> -- single entry, value is width

---@param widths DFLayout.Toolbar.Widths[] single-name entries only!
---@return DFLayout.Toolbar.Layout
local function button_widths_to_toolbar_layout(widths)
    local buttons = {}
    local offset = 0
    for _, ww in ipairs(widths) do
        local name, w = next(ww)
        if name then
            if not name:startswith('_') then
                buttons[name] = { offset = offset, width = w }
            end
            offset = offset + w
        end
    end
    return { buttons = buttons, width = offset }
end

local BUTTON_WIDTH = 4

---@param buttons string[]
---@return DFLayout.Toolbar.Widths[]
local function buttons_to_widths(buttons)
    local widths = {}
    for _, button_name in ipairs(buttons) do
        table.insert(widths, { [button_name] = BUTTON_WIDTH })
    end
    return widths
end

---@param buttons string[]
---@return DFLayout.Toolbar.Layout
local function buttons_to_toolbar_layout(buttons)
    return button_widths_to_toolbar_layout(buttons_to_widths(buttons))
end

element_layouts = {
    fort = {
        toolbars = {
            ---@type DFLayout.Toolbar.Layout
            left = buttons_to_toolbar_layout{
                'MAIN_OPEN_CREATURES',
                'MAIN_OPEN_TASKS',
                'MAIN_OPEN_PLACES',
                'MAIN_OPEN_LABOR',
                'MAIN_OPEN_WORK_ORDERS',
                'MAIN_OPEN_NOBLES',
                'MAIN_OPEN_OBJECTS',
                'MAIN_OPEN_JUSTICE',
            },
            ---@type DFLayout.Toolbar.Layout
            center = button_widths_to_toolbar_layout(concat_sequences{
                { { _left_border = 1 } },
                buttons_to_widths{ 'DIG', 'CHOP', 'GATHER', 'SMOOTH', 'ERASE' },
                { { _divider = 1 } },
                buttons_to_widths{ 'MAIN_BUILDING_MODE', 'MAIN_STOCKPILE_MODE', 'MAIN_ZONE_MODE' },
                { { _divider = 1 } },
                buttons_to_widths{ 'MAIN_BURROW_MODE', 'MAIN_HAULING_MODE', 'TRAFFIC' },
                { { _divider = 1 } },
                buttons_to_widths{ 'ITEM_BUILDING' },
                { { _right_border = 1 } },
            }),
            ---@type DFLayout.Toolbar.Layout
            right = buttons_to_toolbar_layout{
                'MAIN_OPEN_SQUADS', 'MAIN_OPEN_WORLD',
            },
        },
        secondary_toolbars = {
            ---@type DFLayout.Toolbar.Layout
            DIG = buttons_to_toolbar_layout{
                'DIG_DIG',
                'DIG_STAIRS',
                'DIG_RAMP',
                'DIG_CHANNEL',
                'DIG_REMOVE_STAIRS_RAMPS', '_gap',
                'DIG_PAINT_RECTANGLE',
                'DIG_FREE_PAINT', '_gap',
                'DIG_OPEN_RIGHT', '_gap', -- also DIG_CLOSE_LEFT
                'DIG_MODE_ALL',
                'DIG_MODE_AUTO',
                'DIG_MODE_ONLY_ORE_GEM',
                'DIG_MODE_ONLY_GEM', '_gap',
                'DIG_PRIORITY_1',
                'DIG_PRIORITY_2',
                'DIG_PRIORITY_3',
                'DIG_PRIORITY_4',
                'DIG_PRIORITY_5',
                'DIG_PRIORITY_6',
                'DIG_PRIORITY_7', '_gap',
                'DIG_TO_BLUEPRINT', -- also DIG_TO_STANDARD
                'DIG_GO_FROM_BLUEPRINT',
                'DIG_GO_TO_BLUEPRINT',
            },
            ---@type DFLayout.Toolbar.Layout
            CHOP = buttons_to_toolbar_layout{
                'CHOP_REGULAR', '_gap',
                'CHOP_PAINT_RECTANGLE',
                'CHOP_FREE_PAINT', '_gap',
                'CHOP_OPEN_RIGHT', '_gap', -- also CHOP_CLOSE_LEFT
                'CHOP_PRIORITY_1',
                'CHOP_PRIORITY_2',
                'CHOP_PRIORITY_3',
                'CHOP_PRIORITY_4',
                'CHOP_PRIORITY_5',
                'CHOP_PRIORITY_6',
                'CHOP_PRIORITY_7', '_gap',
                'CHOP_TO_BLUEPRINT', -- also CHOP_TO_STANDARD
                'CHOP_GO_FROM_BLUEPRINT',
                'CHOP_GO_TO_BLUEPRINT',
            },
            ---@type DFLayout.Toolbar.Layout
            GATHER = buttons_to_toolbar_layout{
                'GATHER_REGULAR', '_gap',
                'GATHER_PAINT_RECTANGLE',
                'GATHER_FREE_PAINT', '_gap',
                'GATHER_OPEN_RIGHT', '_gap', -- also GATHER_CLOSE_LEFT
                'GATHER_PRIORITY_1',
                'GATHER_PRIORITY_2',
                'GATHER_PRIORITY_3',
                'GATHER_PRIORITY_4',
                'GATHER_PRIORITY_5',
                'GATHER_PRIORITY_6',
                'GATHER_PRIORITY_7', '_gap',
                'GATHER_TO_BLUEPRINT', -- also GATHER_TO_STANDARD
                'GATHER_GO_FROM_BLUEPRINT',
                'GATHER_GO_TO_BLUEPRINT',
            },
            ---@type DFLayout.Toolbar.Layout
            SMOOTH = buttons_to_toolbar_layout{
                'SMOOTH_SMOOTH',
                'SMOOTH_ENGRAVE',
                'SMOOTH_TRACK',
                'SMOOTH_FORTIFY', '_gap',
                'SMOOTH_PAINT_RECTANGLE',
                'SMOOTH_FREE_PAINT', '_gap',
                'SMOOTH_OPEN_RIGHT', '_gap', -- also SMOOTH_CLOSE_LEFT
                'SMOOTH_PRIORITY_1',
                'SMOOTH_PRIORITY_2',
                'SMOOTH_PRIORITY_3',
                'SMOOTH_PRIORITY_4',
                'SMOOTH_PRIORITY_5',
                'SMOOTH_PRIORITY_6',
                'SMOOTH_PRIORITY_7', '_gap',
                'SMOOTH_TO_BLUEPRINT', -- also SMOOTH_TO_STANDARD
                'SMOOTH_GO_FROM_BLUEPRINT',
                'SMOOTH_GO_TO_BLUEPRINT',
            },
            ---@type DFLayout.Toolbar.Layout
            ERASE = buttons_to_toolbar_layout{
                -- Note: The ERASE secondary toolbar re-uses main_hover_instruction values from ITEM_BUILDING
                'ITEM_BUILDING_PAINT_RECTANGLE',
                'ITEM_BUILDING_FREE_PAINT',
            },
            -- MAIN_BUILDING_MODE -- completely different and quite variable
            ---@type DFLayout.Toolbar.Layout
            MAIN_STOCKPILE_MODE = buttons_to_toolbar_layout{ 'STOCKPILE_NEW' },
            ---@type DFLayout.Toolbar.Layout
            STOCKPILE_NEW = buttons_to_toolbar_layout{
                'STOCKPILE_PAINT_RECTANGLE',
                'STOCKPILE_PAINT_FREE',
                'STOCKPILE_ERASE',
                'STOCKPILE_PAINT_REMOVE',
            },
            -- MAIN_ZONE_MODE -- no secondary toolbar
            -- MAIN_BURROW_MODE -- no direct secondary toolbar
            ---@type DFLayout.Toolbar.Layout
            ['Add new burrow'] = buttons_to_toolbar_layout{ -- "Add new burrow" is the text of button in burrows window; there is no main_hover_instruction for it
                'BURROW_PAINT_RECTANGLE',
                'BURROW_PAINT_FREE',
                'BURROW_ERASE',
                'BURROW_PAINT_REMOVE',
            },
            -- MAIN_HAULING_MODE -- no secondary toolbar
            ---@type DFLayout.Toolbar.Layout
            TRAFFIC = button_widths_to_toolbar_layout(
                concat_sequences{ buttons_to_widths{
                    'TRAFFIC_HIGH',
                    'TRAFFIC_NORMAL',
                    'TRAFFIC_LOW',
                    'TRAFFIC_RESTRICTED', '_gap',
                    'TRAFFIC_PAINT_RECTANGLE',
                    'TRAFFIC_FREE_PAINT', '_gap',
                    'TRAFFIC_OPEN_RIGHT', '_gap', -- also TRAFFIC_CLOSE_LEFT
                }, {
                    -- These last spans all use TRAFFIC_SLIDERS as the
                    -- main_hover_instruction, but have distinct interactions.
                    -- Note: The TRAFFIC secondary toolbar is taller (total of four
                    -- toolbar heights) in this region. Only the bottom row (in the
                    -- normal secondary toolbar area) is not currently represented
                    -- here.
                    { ['TRAFFIC_SLIDERS.which'] = 4 },
                    { ['TRAFFIC_SLIDERS.slider'] = 26 },
                    { ['TRAFFIC_SLIDERS.value'] = 6 },
                } }
            ),
            ---@type DFLayout.Toolbar.Layout
            ITEM_BUILDING = buttons_to_toolbar_layout{
                'ITEM_BUILDING_CLAIM',
                'ITEM_BUILDING_FORBID',
                'ITEM_BUILDING_DUMP',
                'ITEM_BUILDING_UNDUMP',
                'ITEM_BUILDING_MELT',
                'ITEM_BUILDING_UNMELT',
                'ITEM_BUILDING_UNHIDE',
                'ITEM_BUILDING_HIDE', '_gap',
                'ITEM_BUILDING_PAINT_RECTANGLE',
                'ITEM_BUILDING_FREE_PAINT',
            },
        },
    }
}

local fort_tb_layout = element_layouts.fort.toolbars
local fort_stb_layout = element_layouts.fort.secondary_toolbars

--- DF UI element "frame" calculation functions ---

---@type DFLayout.FrameFn
local function fort_left_tb_frame(interface_size)
    return {
        l = 0,
        w = fort_tb_layout.left.width,
        r = interface_size.width - fort_tb_layout.left.width,

        t = interface_size.height - TOOLBAR_HEIGHT,
        h = TOOLBAR_HEIGHT,
        b = 0,
    }
end

local FORT_LEFT_CENTER_TOOLBAR_GAP_MINIMUM = 7

---@type DFLayout.FrameFn
local function fort_center_tb_frame(interface_size)
    -- center toolbar is "centered" in interface area, but never closer to the
    -- left toolbar than FORT_LEFT_CENTER_TOOLBAR_GAP_MINIMUM

    local interface_offset_centered = (interface_size.width - fort_tb_layout.center.width) // 2 + 1
    local interface_offset_min = fort_tb_layout.left.width + FORT_LEFT_CENTER_TOOLBAR_GAP_MINIMUM
    local interface_offset = math.max(interface_offset_min, interface_offset_centered)

    return {
        l = interface_offset,
        w = fort_tb_layout.center.width,
        r = interface_size.width - interface_offset - fort_tb_layout.center.width,

        t = interface_size.height - TOOLBAR_HEIGHT,
        h = TOOLBAR_HEIGHT,
        b = 0,
    }
end

---@type DFLayout.FrameFn
local function fort_right_tb_frame(interface_size)
    return {
        l = interface_size.width - fort_tb_layout.right.width,
        w = fort_tb_layout.right.width,
        r = 0,

        t = interface_size.height - TOOLBAR_HEIGHT,
        h = TOOLBAR_HEIGHT,
        b = 0,
    }
end

---@alias DFLayout.Fort.SecondaryToolbar.CenterButton  'DIG' | 'CHOP' | 'GATHER' | 'SMOOTH' | 'ERASE' | 'MAIN_STOCKPILE_MODE'                   | 'MAIN_BURROW_MODE' | 'TRAFFIC' | 'ITEM_BUILDING'
---@alias DFLayout.Fort.SecondaryToolbar.Names         'DIG' | 'CHOP' | 'GATHER' | 'SMOOTH' | 'ERASE' | 'MAIN_STOCKPILE_MODE' | 'STOCKPILE_NEW' | 'Add new burrow'   | 'TRAFFIC' | 'ITEM_BUILDING'

-- Derive the frame_fn for a secondary toolbar that "wants to" align with the
-- specified center toolbar button.
--
---@param center_button_name DFLayout.Fort.SecondaryToolbar.CenterButton
---@param secondary_toolbar_layout DFLayout.Toolbar.Layout
---@return DFLayout.FrameFn
local function get_secondary_frame_fn(center_button_name, secondary_toolbar_layout)
    local toolbar_button = fort_tb_layout.center.buttons[center_button_name]
        or dfhack.error('bad center toolbar button name: ' .. tostring(center_button_name))
    return function(interface_size)
        local toolbar_offset = fort_center_tb_frame(interface_size).l

        -- Ideally, the secondary toolbar is positioned directly above the (main) toolbar button
        local ideal_offset = toolbar_offset + toolbar_button.offset

        -- In "narrow" interfaces conditions, a wide secondary toolbar (pretty much
        -- any tool that has "advanced" options) that was ideally positioned above
        -- its tool's button would extend past the right edge of the interface area.
        -- Such wide secondary toolbars are instead right justified with a bit of
        -- padding.

        -- padding necessary to line up width-constrained secondaries
        local secondary_padding = 5
        local width_constrained_offset = math.max(0, interface_size.width - (secondary_toolbar_layout.width + secondary_padding))

        -- Use whichever position is left-most.
        local l = math.min(ideal_offset, width_constrained_offset)
        return {
            l = l,
            w = secondary_toolbar_layout.width,
            r = interface_size.width - l - secondary_toolbar_layout.width,

            t = interface_size.height - TOOLBAR_HEIGHT - SECONDARY_TOOLBAR_HEIGHT,
            h = SECONDARY_TOOLBAR_HEIGHT,
            b = TOOLBAR_HEIGHT,
        }
    end
end

---@type table<DFLayout.Fort.SecondaryToolbar.Names, DFLayout.FrameFn>
local fort_secondary_tb_frames = {
    DIG = get_secondary_frame_fn('DIG', fort_stb_layout.DIG),
    CHOP = get_secondary_frame_fn('CHOP', fort_stb_layout.CHOP),
    GATHER = get_secondary_frame_fn('GATHER', fort_stb_layout.GATHER),
    SMOOTH = get_secondary_frame_fn('SMOOTH', fort_stb_layout.SMOOTH),
    ERASE = get_secondary_frame_fn('ERASE', fort_stb_layout.ERASE),
    -- MAIN_BUILDING_MODE -- completely different and quite variable
    MAIN_STOCKPILE_MODE = get_secondary_frame_fn('MAIN_STOCKPILE_MODE', fort_stb_layout.MAIN_STOCKPILE_MODE),
    STOCKPILE_NEW = get_secondary_frame_fn('MAIN_STOCKPILE_MODE', fort_stb_layout.STOCKPILE_NEW),
    -- MAIN_ZONE_MODE -- no secondary toolbar
    -- MAIN_BURROW_MODE -- no direct secondary toolbar
    ['Add new burrow'] = get_secondary_frame_fn('MAIN_BURROW_MODE', fort_stb_layout['Add new burrow']),
    -- MAIN_HAULING_MODE -- no secondary toolbar
    TRAFFIC = get_secondary_frame_fn('TRAFFIC', fort_stb_layout.TRAFFIC),
    ITEM_BUILDING = get_secondary_frame_fn('ITEM_BUILDING', fort_stb_layout.ITEM_BUILDING),
}

---@param name string
---@param frame_fn DFLayout.FrameFn
---@return DFLayout.DynamicUIElement
local function ui_el(name, frame_fn)
    return {
        name = name,
        frame_fn = frame_fn,
    }
end

-- Derive the DynamicUIElement for the named button given a toolbar's frame_fn, and its button button layout.
--
---@param toolbar_name string
---@param toolbar_frame_fn DFLayout.FrameFn
---@param toolbar_layout DFLayout.Toolbar.Layout
---@param button_name string
---@return DFLayout.DynamicUIElement
local function button_ui_el(toolbar_name, toolbar_frame_fn, toolbar_layout, button_name)
    local button = toolbar_layout.buttons[button_name]
        or dfhack.error('button not present in given toolbar layout: ' .. tostring(button_name))
    return ui_el(toolbar_name .. '.' .. button_name, function(interface_size)
        local toolbar_frame = toolbar_frame_fn(interface_size)
        local l = toolbar_frame.l + button.offset
        local r = interface_size.width - (l + button.width)
        return {
            l = l,
            w = button.width,
            r = r,

            t = toolbar_frame.t,
            h = toolbar_frame.h,
            b = toolbar_frame.b,
        }
    end)
end

local function left_button_ui_el(button_name)
    return button_ui_el('fort.toolbar_buttons.left', fort_left_tb_frame, fort_tb_layout.left, button_name)
end
local function center_button_ui_el(button_name)
    return button_ui_el('fort.toolbar_buttons.center', fort_center_tb_frame, fort_tb_layout.center, button_name)
end
local function center_close_button_ui_el(button_name)
    return button_ui_el('fort.toolbar_buttons.center', fort_center_tb_frame, fort_tb_layout.center, button_name)
end
local function right_button_ui_el(button_name)
    return button_ui_el('fort.toolbar_buttons.right', fort_right_tb_frame, fort_tb_layout.right, button_name)
end

-- button_ui_el, specialized for the secondary toolbars.
--
---@param toolbar_name DFLayout.Fort.SecondaryToolbar.Names
---@param button_name string
---@return DFLayout.DynamicUIElement
local function secondary_button_ui_el(toolbar_name, button_name)
    local frame_fn = fort_secondary_tb_frames[toolbar_name]
        or dfhack.error('secondary toolbar name not in fort_secondary_tb_frames: ' .. tostring(toolbar_name))
    local layout = fort_stb_layout[toolbar_name]
        or dfhack.error('secondary toolbar name not in fort_el_layout.secondary_toolbars: ' .. tostring(toolbar_name))
    return button_ui_el(
        ('fort.secondary_toolbar_buttons.%s.%s'):format(toolbar_name, button_name),
        frame_fn, layout, button_name)
end

elements = {
    fort = {
        toolbars = {
            ---@type DFLayout.DynamicUIElement
            left = ui_el('fort.toolbars.left', fort_left_tb_frame),
            ---@type DFLayout.DynamicUIElement
            center = ui_el('fort.toolbars.center', fort_center_tb_frame),
            ---@type DFLayout.DynamicUIElement
            right = ui_el('fort.toolbars.left', fort_right_tb_frame),
        },
        toolbar_buttons = {
            left = {
                ---@type DFLayout.DynamicUIElement
                MAIN_OPEN_CREATURES = left_button_ui_el('MAIN_OPEN_CREATURES'),
                ---@type DFLayout.DynamicUIElement
                MAIN_OPEN_TASKS = left_button_ui_el('MAIN_OPEN_TASKS'),
                ---@type DFLayout.DynamicUIElement
                MAIN_OPEN_PLACES = left_button_ui_el('MAIN_OPEN_PLACES'),
                ---@type DFLayout.DynamicUIElement
                MAIN_OPEN_LABOR = left_button_ui_el('MAIN_OPEN_LABOR'),
                ---@type DFLayout.DynamicUIElement
                MAIN_OPEN_WORK_ORDERS = left_button_ui_el('MAIN_OPEN_WORK_ORDERS'),
                ---@type DFLayout.DynamicUIElement
                MAIN_OPEN_NOBLES = left_button_ui_el('MAIN_OPEN_NOBLES'),
                ---@type DFLayout.DynamicUIElement
                MAIN_OPEN_OBJECTS = left_button_ui_el('MAIN_OPEN_OBJECTS'),
                ---@type DFLayout.DynamicUIElement
                MAIN_OPEN_JUSTICE = left_button_ui_el('MAIN_OPEN_JUSTICE'),
            },
            center = {
                ---@type DFLayout.DynamicUIElement
                DIG = center_button_ui_el('DIG'),
                ---@type DFLayout.DynamicUIElement
                CHOP = center_button_ui_el('CHOP'),
                ---@type DFLayout.DynamicUIElement
                GATHER = center_button_ui_el('GATHER'),
                ---@type DFLayout.DynamicUIElement
                SMOOTH = center_button_ui_el('SMOOTH'),
                ---@type DFLayout.DynamicUIElement
                ERASE = center_button_ui_el('ERASE'),
                ---@type DFLayout.DynamicUIElement
                MAIN_BUILDING_MODE = center_button_ui_el('MAIN_BUILDING_MODE'),
                ---@type DFLayout.DynamicUIElement
                MAIN_STOCKPILE_MODE = center_button_ui_el('MAIN_STOCKPILE_MODE'),
                ---@type DFLayout.DynamicUIElement
                MAIN_ZONE_MODE = center_button_ui_el('MAIN_ZONE_MODE'),
                ---@type DFLayout.DynamicUIElement
                MAIN_BURROW_MODE = center_button_ui_el('MAIN_BURROW_MODE'),
                ---@type DFLayout.DynamicUIElement
                MAIN_HAULING_MODE = center_button_ui_el('MAIN_HAULING_MODE'),
                ---@type DFLayout.DynamicUIElement
                TRAFFIC = center_button_ui_el('TRAFFIC'),
                ---@type DFLayout.DynamicUIElement
                ITEM_BUILDING = center_button_ui_el('ITEM_BUILDING'),
            },
            center_close = {
                ---@type DFLayout.DynamicUIElement
                DIG_LOWER_MODE = center_close_button_ui_el('DIG'),
                ---@type DFLayout.DynamicUIElement
                CHOP_LOWER_MODE = center_close_button_ui_el('CHOP'),
                ---@type DFLayout.DynamicUIElement
                GATHER_LOWER_MODE = center_close_button_ui_el('GATHER'),
                ---@type DFLayout.DynamicUIElement
                SMOOTH_LOWER_MODE = center_close_button_ui_el('SMOOTH'),
                ---@type DFLayout.DynamicUIElement
                ERASE_LOWER_MODE = center_close_button_ui_el('ERASE'),
                ---@type DFLayout.DynamicUIElement
                MAIN_BUILDING_MODE_LOWER_MODE = center_close_button_ui_el('MAIN_BUILDING_MODE'),
                ---@type DFLayout.DynamicUIElement
                MAIN_STOCKPILE_MODE_LOWER_MODE = center_close_button_ui_el('MAIN_STOCKPILE_MODE'),
                ---@type DFLayout.DynamicUIElement
                MAIN_ZONE_MODE_LOWER_MODE = center_close_button_ui_el('MAIN_ZONE_MODE'),
                ---@type DFLayout.DynamicUIElement
                MAIN_BURROW_MODE_LOWER_MODE = center_close_button_ui_el('MAIN_BURROW_MODE'),
                ---@type DFLayout.DynamicUIElement
                MAIN_HAULING_MODE_LOWER_MODE = center_close_button_ui_el('MAIN_HAULING_MODE'),
                ---@type DFLayout.DynamicUIElement
                TRAFFIC_LOWER_MODE = center_close_button_ui_el('TRAFFIC'),
                ---@type DFLayout.DynamicUIElement
                ITEM_BUILDING_LOWER_MODE = center_close_button_ui_el('ITEM_BUILDING'),
            },
            right = {
                ---@type DFLayout.DynamicUIElement
                MAIN_OPEN_SQUADS = right_button_ui_el('MAIN_OPEN_SQUADS'),
                ---@type DFLayout.DynamicUIElement
                MAIN_OPEN_WORLD = right_button_ui_el('MAIN_OPEN_WORLD'),
            },
        },
        secondary_toolbars = {
            ---@type DFLayout.DynamicUIElement
            DIG = ui_el('fort.secondary_toolbars.DIG', fort_secondary_tb_frames.DIG),
            ---@type DFLayout.DynamicUIElement
            CHOP = ui_el('fort.secondary_toolbars.CHOP', fort_secondary_tb_frames.CHOP),
            ---@type DFLayout.DynamicUIElement
            GATHER = ui_el('fort.secondary_toolbars.GATHER', fort_secondary_tb_frames.GATHER),
            ---@type DFLayout.DynamicUIElement
            SMOOTH = ui_el('fort.secondary_toolbars.SMOOTH', fort_secondary_tb_frames.SMOOTH),
            ---@type DFLayout.DynamicUIElement
            ERASE = ui_el('fort.secondary_toolbars.ERASE', fort_secondary_tb_frames.ERASE),
            ---@type DFLayout.DynamicUIElement
            MAIN_STOCKPILE_MODE = ui_el('fort.secondary_toolbars.MAIN_STOCKPILE_MODE', fort_secondary_tb_frames.MAIN_STOCKPILE_MODE),
            ---@type DFLayout.DynamicUIElement
            STOCKPILE_NEW = ui_el('fort.secondary_toolbars.STOCKPILE_NEW', fort_secondary_tb_frames.STOCKPILE_NEW),
            ---@type DFLayout.DynamicUIElement
            ['Add new burrow'] = ui_el('fort.secondary_toolbars.Add new burrow', fort_secondary_tb_frames['Add new burrow']),
            ---@type DFLayout.DynamicUIElement
            TRAFFIC = ui_el('fort.secondary_toolbars.TRAFFIC', fort_secondary_tb_frames.TRAFFIC),
            ---@type DFLayout.DynamicUIElement
            ITEM_BUILDING = ui_el('fort.secondary_toolbars.ITEM_BUILDING', fort_secondary_tb_frames.ITEM_BUILDING),
        },
        secondary_toolbar_buttons = {
            DIG = {
                ---@type DFLayout.DynamicUIElement
                DIG_DIG = secondary_button_ui_el('DIG', 'DIG_DIG'),
                ---@type DFLayout.DynamicUIElement
                DIG_STAIRS = secondary_button_ui_el('DIG', 'DIG_STAIRS'),
                ---@type DFLayout.DynamicUIElement
                DIG_RAMP = secondary_button_ui_el('DIG', 'DIG_RAMP'),
                ---@type DFLayout.DynamicUIElement
                DIG_CHANNEL = secondary_button_ui_el('DIG', 'DIG_CHANNEL'),
                ---@type DFLayout.DynamicUIElement
                DIG_REMOVE_STAIRS_RAMPS = secondary_button_ui_el('DIG', 'DIG_REMOVE_STAIRS_RAMPS'),
                ---@type DFLayout.DynamicUIElement
                DIG_PAINT_RECTANGLE = secondary_button_ui_el('DIG', 'DIG_PAINT_RECTANGLE'),
                ---@type DFLayout.DynamicUIElement
                DIG_FREE_PAINT = secondary_button_ui_el('DIG', 'DIG_FREE_PAINT'),
                ---@type DFLayout.DynamicUIElement
                DIG_OPEN_RIGHT = secondary_button_ui_el('DIG', 'DIG_OPEN_RIGHT'),
                ---@type DFLayout.DynamicUIElement
                DIG_CLOSE_LEFT = secondary_button_ui_el('DIG', 'DIG_OPEN_RIGHT'),
                ---@type DFLayout.DynamicUIElement
                DIG_MODE_ALL = secondary_button_ui_el('DIG', 'DIG_MODE_ALL'),
                ---@type DFLayout.DynamicUIElement
                DIG_MODE_AUTO = secondary_button_ui_el('DIG', 'DIG_MODE_AUTO'),
                ---@type DFLayout.DynamicUIElement
                DIG_MODE_ONLY_ORE_GEM = secondary_button_ui_el('DIG', 'DIG_MODE_ONLY_ORE_GEM'),
                ---@type DFLayout.DynamicUIElement
                DIG_MODE_ONLY_GEM = secondary_button_ui_el('DIG', 'DIG_MODE_ONLY_GEM'),
                ---@type DFLayout.DynamicUIElement
                DIG_PRIORITY_1 = secondary_button_ui_el('DIG', 'DIG_PRIORITY_1'),
                ---@type DFLayout.DynamicUIElement
                DIG_PRIORITY_2 = secondary_button_ui_el('DIG', 'DIG_PRIORITY_2'),
                ---@type DFLayout.DynamicUIElement
                DIG_PRIORITY_3 = secondary_button_ui_el('DIG', 'DIG_PRIORITY_3'),
                ---@type DFLayout.DynamicUIElement
                DIG_PRIORITY_4 = secondary_button_ui_el('DIG', 'DIG_PRIORITY_4'),
                ---@type DFLayout.DynamicUIElement
                DIG_PRIORITY_5 = secondary_button_ui_el('DIG', 'DIG_PRIORITY_5'),
                ---@type DFLayout.DynamicUIElement
                DIG_PRIORITY_6 = secondary_button_ui_el('DIG', 'DIG_PRIORITY_6'),
                ---@type DFLayout.DynamicUIElement
                DIG_PRIORITY_7 = secondary_button_ui_el('DIG', 'DIG_PRIORITY_7'),
                ---@type DFLayout.DynamicUIElement
                DIG_TO_BLUEPRINT = secondary_button_ui_el('DIG', 'DIG_TO_BLUEPRINT'),
                ---@type DFLayout.DynamicUIElement
                DIG_TO_STANDARD = secondary_button_ui_el('DIG', 'DIG_TO_BLUEPRINT'),
                ---@type DFLayout.DynamicUIElement
                DIG_GO_FROM_BLUEPRINT = secondary_button_ui_el('DIG', 'DIG_GO_FROM_BLUEPRINT'),
                ---@type DFLayout.DynamicUIElement
                DIG_GO_TO_BLUEPRINT = secondary_button_ui_el('DIG', 'DIG_GO_TO_BLUEPRINT'),
            },
            CHOP = {
                ---@type DFLayout.DynamicUIElement
                CHOP_REGULAR = secondary_button_ui_el('CHOP', 'CHOP_REGULAR'),
                ---@type DFLayout.DynamicUIElement
                CHOP_PAINT_RECTANGLE = secondary_button_ui_el('CHOP', 'CHOP_PAINT_RECTANGLE'),
                ---@type DFLayout.DynamicUIElement
                CHOP_FREE_PAINT = secondary_button_ui_el('CHOP', 'CHOP_FREE_PAINT'),
                ---@type DFLayout.DynamicUIElement
                CHOP_OPEN_RIGHT = secondary_button_ui_el('CHOP', 'CHOP_OPEN_RIGHT'),
                ---@type DFLayout.DynamicUIElement
                CHOP_CLOSE_LEFT = secondary_button_ui_el('CHOP', 'CHOP_OPEN_RIGHT'),
                ---@type DFLayout.DynamicUIElement
                CHOP_PRIORITY_1 = secondary_button_ui_el('CHOP', 'CHOP_PRIORITY_1'),
                ---@type DFLayout.DynamicUIElement
                CHOP_PRIORITY_2 = secondary_button_ui_el('CHOP', 'CHOP_PRIORITY_2'),
                ---@type DFLayout.DynamicUIElement
                CHOP_PRIORITY_3 = secondary_button_ui_el('CHOP', 'CHOP_PRIORITY_3'),
                ---@type DFLayout.DynamicUIElement
                CHOP_PRIORITY_4 = secondary_button_ui_el('CHOP', 'CHOP_PRIORITY_4'),
                ---@type DFLayout.DynamicUIElement
                CHOP_PRIORITY_5 = secondary_button_ui_el('CHOP', 'CHOP_PRIORITY_5'),
                ---@type DFLayout.DynamicUIElement
                CHOP_PRIORITY_6 = secondary_button_ui_el('CHOP', 'CHOP_PRIORITY_6'),
                ---@type DFLayout.DynamicUIElement
                CHOP_PRIORITY_7 = secondary_button_ui_el('CHOP', 'CHOP_PRIORITY_7'),
                ---@type DFLayout.DynamicUIElement
                CHOP_TO_BLUEPRINT = secondary_button_ui_el('CHOP', 'CHOP_TO_BLUEPRINT'),
                ---@type DFLayout.DynamicUIElement
                CHOP_TO_STANDARD = secondary_button_ui_el('CHOP', 'CHOP_TO_BLUEPRINT'),
                ---@type DFLayout.DynamicUIElement
                CHOP_GO_FROM_BLUEPRINT = secondary_button_ui_el('CHOP', 'CHOP_GO_FROM_BLUEPRINT'),
                ---@type DFLayout.DynamicUIElement
                CHOP_GO_TO_BLUEPRINT = secondary_button_ui_el('CHOP', 'CHOP_GO_TO_BLUEPRINT'),
            },
            GATHER = {
                ---@type DFLayout.DynamicUIElement
                GATHER_REGULAR = secondary_button_ui_el('GATHER', 'GATHER_REGULAR'),
                ---@type DFLayout.DynamicUIElement
                GATHER_PAINT_RECTANGLE = secondary_button_ui_el('GATHER', 'GATHER_PAINT_RECTANGLE'),
                ---@type DFLayout.DynamicUIElement
                GATHER_FREE_PAINT = secondary_button_ui_el('GATHER', 'GATHER_FREE_PAINT'),
                ---@type DFLayout.DynamicUIElement
                GATHER_OPEN_RIGHT = secondary_button_ui_el('GATHER', 'GATHER_OPEN_RIGHT'),
                ---@type DFLayout.DynamicUIElement
                GATHER_CLOSE_LEFT = secondary_button_ui_el('GATHER', 'GATHER_OPEN_RIGHT'),
                ---@type DFLayout.DynamicUIElement
                GATHER_PRIORITY_1 = secondary_button_ui_el('GATHER', 'GATHER_PRIORITY_1'),
                ---@type DFLayout.DynamicUIElement
                GATHER_PRIORITY_2 = secondary_button_ui_el('GATHER', 'GATHER_PRIORITY_2'),
                ---@type DFLayout.DynamicUIElement
                GATHER_PRIORITY_3 = secondary_button_ui_el('GATHER', 'GATHER_PRIORITY_3'),
                ---@type DFLayout.DynamicUIElement
                GATHER_PRIORITY_4 = secondary_button_ui_el('GATHER', 'GATHER_PRIORITY_4'),
                ---@type DFLayout.DynamicUIElement
                GATHER_PRIORITY_5 = secondary_button_ui_el('GATHER', 'GATHER_PRIORITY_5'),
                ---@type DFLayout.DynamicUIElement
                GATHER_PRIORITY_6 = secondary_button_ui_el('GATHER', 'GATHER_PRIORITY_6'),
                ---@type DFLayout.DynamicUIElement
                GATHER_PRIORITY_7 = secondary_button_ui_el('GATHER', 'GATHER_PRIORITY_7'),
                ---@type DFLayout.DynamicUIElement
                GATHER_TO_BLUEPRINT = secondary_button_ui_el('GATHER', 'GATHER_TO_BLUEPRINT'),
                ---@type DFLayout.DynamicUIElement
                GATHER_TO_STANDARD = secondary_button_ui_el('GATHER', 'GATHER_TO_BLUEPRINT'),
                ---@type DFLayout.DynamicUIElement
                GATHER_GO_FROM_BLUEPRINT = secondary_button_ui_el('GATHER', 'GATHER_GO_FROM_BLUEPRINT'),
                ---@type DFLayout.DynamicUIElement
                GATHER_GO_TO_BLUEPRINT = secondary_button_ui_el('GATHER', 'GATHER_GO_TO_BLUEPRINT'),
            },
            SMOOTH = {
                ---@type DFLayout.DynamicUIElement
                SMOOTH_SMOOTH = secondary_button_ui_el('SMOOTH', 'SMOOTH_SMOOTH'),
                ---@type DFLayout.DynamicUIElement
                SMOOTH_ENGRAVE = secondary_button_ui_el('SMOOTH', 'SMOOTH_ENGRAVE'),
                ---@type DFLayout.DynamicUIElement
                SMOOTH_TRACK = secondary_button_ui_el('SMOOTH', 'SMOOTH_TRACK'),
                ---@type DFLayout.DynamicUIElement
                SMOOTH_FORTIFY = secondary_button_ui_el('SMOOTH', 'SMOOTH_FORTIFY'),
                ---@type DFLayout.DynamicUIElement
                SMOOTH_PAINT_RECTANGLE = secondary_button_ui_el('SMOOTH', 'SMOOTH_PAINT_RECTANGLE'),
                ---@type DFLayout.DynamicUIElement
                SMOOTH_FREE_PAINT = secondary_button_ui_el('SMOOTH', 'SMOOTH_FREE_PAINT'),
                ---@type DFLayout.DynamicUIElement
                SMOOTH_OPEN_RIGHT = secondary_button_ui_el('SMOOTH', 'SMOOTH_OPEN_RIGHT'),
                ---@type DFLayout.DynamicUIElement
                SMOOTH_CLOSE_LEFT = secondary_button_ui_el('SMOOTH', 'SMOOTH_OPEN_RIGHT'),
                ---@type DFLayout.DynamicUIElement
                SMOOTH_PRIORITY_1 = secondary_button_ui_el('SMOOTH', 'SMOOTH_PRIORITY_1'),
                ---@type DFLayout.DynamicUIElement
                SMOOTH_PRIORITY_2 = secondary_button_ui_el('SMOOTH', 'SMOOTH_PRIORITY_2'),
                ---@type DFLayout.DynamicUIElement
                SMOOTH_PRIORITY_3 = secondary_button_ui_el('SMOOTH', 'SMOOTH_PRIORITY_3'),
                ---@type DFLayout.DynamicUIElement
                SMOOTH_PRIORITY_4 = secondary_button_ui_el('SMOOTH', 'SMOOTH_PRIORITY_4'),
                ---@type DFLayout.DynamicUIElement
                SMOOTH_PRIORITY_5 = secondary_button_ui_el('SMOOTH', 'SMOOTH_PRIORITY_5'),
                ---@type DFLayout.DynamicUIElement
                SMOOTH_PRIORITY_6 = secondary_button_ui_el('SMOOTH', 'SMOOTH_PRIORITY_6'),
                ---@type DFLayout.DynamicUIElement
                SMOOTH_PRIORITY_7 = secondary_button_ui_el('SMOOTH', 'SMOOTH_PRIORITY_7'),
                ---@type DFLayout.DynamicUIElement
                SMOOTH_TO_BLUEPRINT = secondary_button_ui_el('SMOOTH', 'SMOOTH_TO_BLUEPRINT'),
                ---@type DFLayout.DynamicUIElement
                SMOOTH_TO_STANDARD = secondary_button_ui_el('SMOOTH', 'SMOOTH_TO_BLUEPRINT'),
                ---@type DFLayout.DynamicUIElement
                SMOOTH_GO_FROM_BLUEPRINT = secondary_button_ui_el('SMOOTH', 'SMOOTH_GO_FROM_BLUEPRINT'),
                ---@type DFLayout.DynamicUIElement
                SMOOTH_GO_TO_BLUEPRINT = secondary_button_ui_el('SMOOTH', 'SMOOTH_GO_TO_BLUEPRINT'),
            },
            ERASE = {
                ---@type DFLayout.DynamicUIElement
                ITEM_BUILDING_PAINT_RECTANGLE = secondary_button_ui_el('ERASE', 'ITEM_BUILDING_PAINT_RECTANGLE'),
                ---@type DFLayout.DynamicUIElement
                ITEM_BUILDING_FREE_PAINT = secondary_button_ui_el('ERASE', 'ITEM_BUILDING_FREE_PAINT'),
            },
            MAIN_STOCKPILE_MODE = {
                ---@type DFLayout.DynamicUIElement
                STOCKPILE_NEW = secondary_button_ui_el('MAIN_STOCKPILE_MODE', 'STOCKPILE_NEW'),
            },
            STOCKPILE_NEW = {
                ---@type DFLayout.DynamicUIElement
                STOCKPILE_PAINT_RECTANGLE = secondary_button_ui_el('STOCKPILE_NEW', 'STOCKPILE_PAINT_RECTANGLE'),
                ---@type DFLayout.DynamicUIElement
                STOCKPILE_PAINT_FREE = secondary_button_ui_el('STOCKPILE_NEW', 'STOCKPILE_PAINT_FREE'),
                ---@type DFLayout.DynamicUIElement
                STOCKPILE_ERASE = secondary_button_ui_el('STOCKPILE_NEW', 'STOCKPILE_ERASE'),
                ---@type DFLayout.DynamicUIElement
                STOCKPILE_PAINT_REMOVE = secondary_button_ui_el('STOCKPILE_NEW', 'STOCKPILE_PAINT_REMOVE'),
            },
            ['Add new burrow'] = {
                ---@type DFLayout.DynamicUIElement
                BURROW_PAINT_RECTANGLE = secondary_button_ui_el('Add new burrow', 'BURROW_PAINT_RECTANGLE'),
                ---@type DFLayout.DynamicUIElement
                BURROW_PAINT_FREE = secondary_button_ui_el('Add new burrow', 'BURROW_PAINT_FREE'),
                ---@type DFLayout.DynamicUIElement
                BURROW_ERASE = secondary_button_ui_el('Add new burrow', 'BURROW_ERASE'),
                ---@type DFLayout.DynamicUIElement
                BURROW_PAINT_REMOVE = secondary_button_ui_el('Add new burrow', 'BURROW_PAINT_REMOVE'),
            },
            TRAFFIC = {
                ---@type DFLayout.DynamicUIElement
                TRAFFIC_HIGH = secondary_button_ui_el('TRAFFIC', 'TRAFFIC_HIGH'),
                ---@type DFLayout.DynamicUIElement
                TRAFFIC_NORMAL = secondary_button_ui_el('TRAFFIC', 'TRAFFIC_NORMAL'),
                ---@type DFLayout.DynamicUIElement
                TRAFFIC_LOW = secondary_button_ui_el('TRAFFIC', 'TRAFFIC_LOW'),
                ---@type DFLayout.DynamicUIElement
                TRAFFIC_RESTRICTED = secondary_button_ui_el('TRAFFIC', 'TRAFFIC_RESTRICTED'),
                ---@type DFLayout.DynamicUIElement
                TRAFFIC_PAINT_RECTANGLE = secondary_button_ui_el('TRAFFIC', 'TRAFFIC_PAINT_RECTANGLE'),
                ---@type DFLayout.DynamicUIElement
                TRAFFIC_FREE_PAINT = secondary_button_ui_el('TRAFFIC', 'TRAFFIC_FREE_PAINT'),
                ---@type DFLayout.DynamicUIElement
                TRAFFIC_OPEN_RIGHT = secondary_button_ui_el('TRAFFIC', 'TRAFFIC_OPEN_RIGHT'),
                ---@type DFLayout.DynamicUIElement
                TRAFFIC_CLOSE_LEFT = secondary_button_ui_el('TRAFFIC', 'TRAFFIC_OPEN_RIGHT'),
                ---@type DFLayout.DynamicUIElement
                ['TRAFFIC_SLIDERS.which'] = secondary_button_ui_el('TRAFFIC', 'TRAFFIC_SLIDERS.which'),
                ---@type DFLayout.DynamicUIElement
                ['TRAFFIC_SLIDERS.slider'] = secondary_button_ui_el('TRAFFIC', 'TRAFFIC_SLIDERS.slider'),
                ---@type DFLayout.DynamicUIElement
                ['TRAFFIC_SLIDERS.value'] = secondary_button_ui_el('TRAFFIC', 'TRAFFIC_SLIDERS.value'),
            },
            ITEM_BUILDING = {
                ---@type DFLayout.DynamicUIElement
                ITEM_BUILDING_CLAIM = secondary_button_ui_el('ITEM_BUILDING', 'ITEM_BUILDING_CLAIM'),
                ---@type DFLayout.DynamicUIElement
                ITEM_BUILDING_FORBID = secondary_button_ui_el('ITEM_BUILDING', 'ITEM_BUILDING_FORBID'),
                ---@type DFLayout.DynamicUIElement
                ITEM_BUILDING_DUMP = secondary_button_ui_el('ITEM_BUILDING', 'ITEM_BUILDING_DUMP'),
                ---@type DFLayout.DynamicUIElement
                ITEM_BUILDING_UNDUMP = secondary_button_ui_el('ITEM_BUILDING', 'ITEM_BUILDING_UNDUMP'),
                ---@type DFLayout.DynamicUIElement
                ITEM_BUILDING_MELT = secondary_button_ui_el('ITEM_BUILDING', 'ITEM_BUILDING_MELT'),
                ---@type DFLayout.DynamicUIElement
                ITEM_BUILDING_UNMELT = secondary_button_ui_el('ITEM_BUILDING', 'ITEM_BUILDING_UNMELT'),
                ---@type DFLayout.DynamicUIElement
                ITEM_BUILDING_UNHIDE = secondary_button_ui_el('ITEM_BUILDING', 'ITEM_BUILDING_UNHIDE'),
                ---@type DFLayout.DynamicUIElement
                ITEM_BUILDING_HIDE = secondary_button_ui_el('ITEM_BUILDING', 'ITEM_BUILDING_HIDE'),
                ---@type DFLayout.DynamicUIElement
                ITEM_BUILDING_PAINT_RECTANGLE = secondary_button_ui_el('ITEM_BUILDING', 'ITEM_BUILDING_PAINT_RECTANGLE'),
                ---@type DFLayout.DynamicUIElement
                ITEM_BUILDING_FREE_PAINT = secondary_button_ui_el('ITEM_BUILDING', 'ITEM_BUILDING_FREE_PAINT'),
            },
        },
    },
}

---@param minimum_inset DFLayout.Inset
---@param scrollbar_feature_name string
---@return DFLayout.FrameFn
local function get_info_frame_fn(minimum_inset, scrollbar_feature_name)
    ---@type DFLayout.FrameFn
    return function(interface_size, feature_tests)
        local l = minimum_inset.l
        local r = minimum_inset.r
        local t = minimum_inset.t
        local b = minimum_inset.b

        -- main info window tabs need to wrap in narrow interface widths
        if interface_size.width < 155 then
            t = t + 2
        end

        -- info item rows are 3 UI rows each; if there are extra UI rows, they
        -- go into the bottom inset (immediately below the last item row)
        local extra_ui_rows = (interface_size.height - (t + b)) % 3
        b = b + extra_ui_rows

        local h = interface_size.height - (t + b)

        -- if it is needed, the scroll bar (2 UI cols) is in the right inset area
        local displayable_count = h // 3
        local scrollbar = feature_tests[scrollbar_feature_name](displayable_count)
        if scrollbar then
            r = r + 2
        end

        return {
            l = l,
            w = interface_size.width - (l + r),
            r = r,

            t = t,
            h = h,
            b = b,
        }, {
            scrollbar = scrollbar,
        }
    end
end

-- A UI element that encompasses the area used to display orders in the Order DF
-- info window tab. The area includes unused rows.
--
---@type DFLayout.DynamicUIElement
local orders_ui_element = {
    name = 'Orders list rows',
    collapsable_inset = {
        r = 2, -- possible lack of scrollbar on right
        t = 2, -- possible un-wrapped info tab bar
        -- bottom inset has variation of 2 (mod 3 from item rows), but is at its
        -- minimum in MINIMUM_INTERFACE_SIZE, so it can not shrink any
    },
    frame_fn = get_info_frame_fn({
        l = 6,  -- 4 UI cols notification area, 1 UI col border, 1 UI col empty
        r = 35, -- 28 UI cols squad area, 1 UI col border, 6 UI cols new order button area, 0 or 2 UI cols scroll bar
        t = 8,  -- 4 UI rows top-bar, 1 UI row border, 2 or 4 UI rows info tabs, 1 UI row empty
        b = 9,  -- 3 UI rows bottom toolbar area, 1 UI row border, 1 UI row empty, 3 UI rows text blurb, 1 UI row empty, 0-2 UI rows expansion gap
    }, 'orders_needs_scrollbar'),
    state_fn = function()
        return #df.global.world.manager_orders.all
    end,
}

-- A UI element that encompasses the area used to display zones in the
-- Places/Zones DF info window tab. The area includes unused rows.
--
---@type DFLayout.DynamicUIElement
local zones_ui_element = {
    name = 'Places/Zones list rows',
    collapsable_inset = {
        r = 2, -- possible lack of scrollbar on right
        t = 2, -- possible un-wrapped info tab bar
        -- bottom has variation of 2 (mod 3 from item rows), and is at its
        -- maximum in MINIMUM_INTERFACE_SIZE, so it might shrink up to 2
        b = 2, -- possible item row gap-filler (mod 3)
    },
    frame_fn = get_info_frame_fn({
        l = 6,  -- 4 UI cols notification area, 1 UI col border, 1 UI col empty
        r = 30, -- 28 UI cols squad area, 1 UI col border, 1 UI col empty
        t = 10, -- 4 UI rows top-bar, 1 UI row border, 2 or 4 UI rows info tabs, 2 UI rows places tabs, 1 UI row empty
        b = 5,  -- 3 UI rows bottom toolbar area, 1 UI row border, 1 UI row empty, 0-2 UI rows expansion gap
    }, 'zones_needs_scrollbar'),
    state_fn = function()
        return #df.global.game.main_interface.info.buildings.list[df.buildings_mode_type.ZONES]
    end,
}

experimental_elements = {
    ---@type DFLayout.DynamicUIElement
    orders = orders_ui_element,
    ---@type DFLayout.DynamicUIElement
    zones = zones_ui_element,
}

default_feature_tests = {
    orders_needs_scrollbar = function(displayable_count)
        return #df.global.world.manager_orders.all > displayable_count
    end,
    zones_needs_scrollbar = function(displayable_count)
        return
            #df.global.game.main_interface.info.buildings.list[df.buildings_mode_type.ZONES]
            > displayable_count
    end,
}

--- UI Element Position and Relative Placement ---

-- Returns a "fully placed frame" for the `ui_element` in an interface area of
-- `interface_size`.
--
-- Throws an error if the computed frame does not span the `interface_size`, or
-- if it has negative inset values.
--
---@param ui_element DFLayout.DynamicUIElement
---@param interface_size DFLayout.Interface.Size
---@param feature_tests? DFLayout.FrameFn.FeatureTests for internal/test use
---@return DFLayout.Frame
function getUIElementFrame(ui_element, interface_size, feature_tests)
    local frame = ui_element.frame_fn(interface_size,
        feature_tests         -- getRelativePlacement
        or default_feature_tests)

    if frame.l < 0 or frame.w <= 0 or frame.r < 0
        or frame.l + frame.w + frame.r ~= interface_size.width
    then
        dfhack.error(('%s: horizontal placement is invalid: l=%d w=%d r=%d W=%d')
            :format(ui_element.name, frame.l, frame.w, frame.r, interface_size.width))
    end
    if frame.t < 0 or frame.h <= 0 or frame.b < 0
        or frame.t + frame.h + frame.b ~= interface_size.height
    then
        dfhack.error(('%s: vertical placement is invalid: t=%d h=%d b=%d H=%d')
            :format(ui_element.name, frame.t, frame.h, frame.b, interface_size.height))
    end
    return frame
end

-- Place a specified span (width or height) with the specified alignment with
-- respect to the given reference position and span.
--
---@param available_span integer
---@param ref_offset_before integer
---@param ref_offset_after integer
---@param placed_span integer
---@param placement DFLayout.Placement.GenericAlignment
---@param offset? integer
---@return integer before
---@return integer span
---@return integer after
local function place_span(available_span, ref_offset_before, ref_offset_after, placed_span, placement, offset)
    if placed_span >= available_span then
        return 0, available_span, 0
    end
    local before
    if placement == 'align start' then
        before = ref_offset_before
    elseif placement == 'align end' then
        before = available_span - ref_offset_after - placed_span
    elseif placement == 'place before' then
        before = ref_offset_before - placed_span
    elseif placement == 'place after' then
        before = available_span - ref_offset_after
    else
        dfhack.error('invalid generic placement: ' .. tostring(placement))
    end
    before = math.max(0, before + (offset or 0))
    local after = available_span - (before + placed_span)
    if after < 0 then
        before = before + after
        after = 0
    end
    return before, placed_span, after
end

local generic_placement_from_horizontal = {
    ['on left'] = 'place before',
    ['align left edges'] = 'align start',
    ['align right edges'] = 'align end',
    ['on right'] = 'place after',
}
local generic_placement_from_vertical = {
    ['above'] = 'place before',
    ['align top edges'] = 'align start',
    ['align bottom edges'] = 'align end',
    ['below'] = 'place after',
}

-- Place the specified area with respect to the specified reference with the
-- specified alignment and offset.
--
---@param spec DFLayout.Placement.Spec
---@param interface_size DFLayout.Interface.Size
---@param feature_tests? DFLayout.FrameFn.FeatureTests for internal/test use
---@return DFLayout.Frame
function getRelativePlacement(spec, interface_size, feature_tests)
    local ref_inset = getUIElementFrame(spec.ui_element, interface_size,
        feature_tests         -- get_overlay_placement_info
        or spec.feature_tests -- tests
        or default_feature_tests)

    local generic_h_placement = generic_placement_from_horizontal[spec.h_placement]
        or dfhack.error(('%s: invalid h_placement: %s'):format(spec.name, spec.h_placement))
    local l, w, r = place_span(interface_size.width,
        ref_inset.l, ref_inset.r,
        spec.size.w, generic_h_placement, spec.offset and spec.offset.x)

    local generic_v_placement = generic_placement_from_vertical[spec.v_placement]
        or dfhack.error(('%s: invalid v_placement: %s'):format(spec.name, spec.v_placement))
    local t, h, b = place_span(interface_size.height,
        ref_inset.t, ref_inset.b,
        spec.size.h, generic_v_placement, spec.offset and spec.offset.y)

    return {
        l = l,
        w = w,
        r = r,

        t = t,
        h = h,
        b = b,
    }
end

--- Automatic UI-relative Overlay Positioning ---

-- Calculate (or validate the requested) `default_pos` and required paddings
-- based on nominal position and forced paddings.
--
---@param spec_name string
---@param xy 'x' | 'y' the default_pos field name (used in error messages)
---@param pos? integer
---@param nominal_positive integer
---@param nominal_negative integer
---@param positive_pad integer extra padding to force on positive side
---@param negative_pad integer extra padding to force on negative side
---@param default_to_positive boolean controls which nominal value is used if `pos` is nil or 0
---@return integer pos `pos`, or one of its defaults (when 0 or falsy)
---@return integer padding_on_positive_side 0 or padding required to move from positive `value` to `nominal_positive`
---@return integer padding_on_negative_side 0 or padding required to move from negative `value` to `nominal_negative`
local function pos_and_paddings(spec_name, xy, pos, nominal_positive, nominal_negative, positive_pad, negative_pad, default_to_positive)
    nominal_positive = nominal_positive - positive_pad
    nominal_negative = nominal_negative + negative_pad
    if pos == nil or pos == 0 then
        pos = default_to_positive and nominal_positive or nominal_negative
    end
    if 0 < pos then
        if nominal_positive < pos then
            dfhack.error(('%s: specified placement requires 1 <= default_pos.%s <= %d')
                :format(spec_name, xy, nominal_positive))
        end
        return pos, positive_pad + nominal_positive - pos, negative_pad
    end
    if pos < nominal_negative then
        dfhack.error(('%s: specified placement requires %d <= default_pos.%s <= -1')
            :format(spec_name, nominal_negative, xy))
    end
    return pos, positive_pad, negative_pad + pos - nominal_negative
end

local state_changed_cache = setmetatable({}, { __mode = 'k' })

---@param widget widgets.Widget
---@param ui_element DFLayout.DynamicUIElement
---@return boolean
local function state_changed(widget, ui_element)
    if widget == nil or ui_element == nil or ui_element.state_fn == nil then return false end
    local previous = state_changed_cache[widget]
    local current = ui_element.state_fn()
    state_changed_cache[widget] = current
    if type(previous) ~= 'table' or type(current) ~= 'table' then
        return current ~= previous
    end
    for k, v in pairs(current) do
        if v ~= previous[k] then return true end
        previous[k] = nil
    end
    if next(previous) then return true end
    return false
end

-- The positions of some UI elements vary due to influences other than the
-- interface size. This function returns a function (the "checker") that checks
-- whether a UI element's position-influencing state (other than interface size)
-- has changed since the last time the "checker" was called.
--
-- The returned function should be strongly held somewhere that won't be
-- collected until the caller is no longer interested in the position of the UI
-- element (e.g., a field of a DFHack widget that relies on the UI element's
-- position).
--
-- When the checker function returns true, the position of the UI element might
-- have changed. It should be recomputed (e.g., `getUIElementFrame`) and used to
-- update whatever depends on the position of the UI element.
--
---@param ui_element DFLayout.DynamicUIElement
---@return fun(): boolean
function getUIElementStateChecker(ui_element)
    if ui_element == nil or ui_element.state_fn == nil then
        return function() return false end
    end
    local cache_key = {} -- unique value, strongly held by the closure, so be sure to strongly hold the returned closure
    return curry(state_changed, cache_key, ui_element)
end

---@type DFLayout.FrameFn.FeatureTests
all_features = setmetatable({}, {
    __index = function(table, field)
        return function() return true end
    end,
})

---@alias DFLayout.Placement.InsetFilter fun(inset: DFLayout.Inset): DFLayout.Inset

---@param overlay_placement_spec DFLayout.OverlayPlacement.Spec
---@param inset_filter? DFLayout.Placement.InsetFilter
---@return DFLayout.OverlayPlacementInfo overlay_placement_info
local function get_overlay_placement_info(overlay_placement_spec, inset_filter)
    overlay_placement_spec = utils.clone(overlay_placement_spec, true) --[[@as DFLayout.OverlayPlacement.Spec]]
    overlay_placement_spec.name = overlay_placement_spec.name or '[unnamed overlay placement]'

    -- get the "default" placement (in a MINIMUM_INTERFACE_SIZE area)
    local default_placement = getRelativePlacement(overlay_placement_spec, MINIMUM_INTERFACE_SIZE, all_features)

    local default_from_left = true -- default to left-relative
    local default_from_top = false -- default to bottom-relative

    -- decode overlay_placement_spec.default_pos into pos values and padding values
    local override_default_pos = overlay_placement_spec.default_pos or {}
    if override_default_pos.from_right ~= nil then
        if override_default_pos.x then
            dfhack.printerr(('warning: %s: default_pos.from_right will be ignored since .x is also present')
                :format(overlay_placement_spec.name))
        end
        default_from_left = not override_default_pos.from_right
    end
    if override_default_pos.from_top ~= nil then
        if override_default_pos.y then
            dfhack.printerr(('warning: %s: default_pos.from_top will be ignored since .y is also present')
                :format(overlay_placement_spec.name))
        end
        default_from_top = not not override_default_pos.from_top
    end
    local padding = overlay_placement_spec.ui_element.collapsable_inset or {}
    local x_pos, l_pad, r_pad = pos_and_paddings(
        overlay_placement_spec.name, 'x',
        override_default_pos.x,
        (default_placement.l + 1),  -- one-based, left-relative
        -(default_placement.r + 1), -- one-based, right-relative
        padding.l or 0,
        padding.r or 0,
        default_from_left
    )
    local y_pos, t_pad, b_pad = pos_and_paddings(
        overlay_placement_spec.name, 'y',
        override_default_pos.y,
        (default_placement.t + 1),  -- one-based, top-relative
        -(default_placement.b + 1), -- one-based, bottom-relative
        padding.t or 0,
        padding.b or 0,
        default_from_top
    )

    return {
        default_pos = {
            x = x_pos,
            y = y_pos,
        },
        frame = {
            w = default_placement.w,
            h = default_placement.h,
        },
        ---@param self_overlay_widget widgets.Widget overlay.OverlayWidget
        ---@param parent_rect gui.ViewRect
        preUpdateLayout_fn = function(self_overlay_widget, parent_rect)
            local placement = getRelativePlacement(overlay_placement_spec, parent_rect)
            local inset = {
                l = placement.l - default_placement.l + l_pad,
                r = placement.r - default_placement.r + r_pad,
                t = placement.t - default_placement.t + t_pad,
                b = placement.b - default_placement.b + b_pad,
            }
            inset = inset_filter and inset_filter(inset) or inset

            self_overlay_widget.frame_inset = inset
            self_overlay_widget.frame.w = inset.l + placement.w + inset.r
            self_overlay_widget.frame.h = inset.t + placement.h + inset.b
        end,
        ---@param self_overlay_widget widgets.Widget overlay.OverlayWidget
        ---@param painter gui.Painter
        onRenderBody_fn = function(self_overlay_widget, painter)
            if state_changed(self_overlay_widget, overlay_placement_spec.ui_element) then
                self_overlay_widget.frame.w = nil -- the overlay system will notice the change and call updateLayout
            end
        end,
    }
end

-- Return a table with values that can be used to automatically place an
-- overlay widget relative to a reference position.
--
---@param overlay_placement_spec DFLayout.OverlayPlacement.Spec
---@return DFLayout.OverlayPlacementInfo overlay_placement_info
function getOverlayPlacementInfo(overlay_placement_spec)
    return get_overlay_placement_info(overlay_placement_spec)
end

---@type DFLayout.Placement.InsetFilter
local function only_left_inset(inset)
    return {
        l = inset.l,
        r = 0,
        t = 0,
        b = 0,
    }
end
-- Similar to `getOverlayPlacementInfo`, but only arranges for "padding" on the
-- left. This is compatible with several existing, hand-rolled overlay
-- positioning calculations.
--
---@param overlay_placement_spec DFLayout.OverlayPlacement.Spec
---@return DFLayout.OverlayPlacementInfo overlay_placement_info
function getLeftOnlyOverlayPlacementInfo(overlay_placement_spec)
    return get_overlay_placement_info(overlay_placement_spec, only_left_inset)
end

return _ENV
