local _ENV = mkmodule('gui.dflayout')

local utils = require('utils')

-- Provide data-driven locations for the DF toolbars at the bottom of the
-- screen. Not quite as nice as getting the data from DF directly, but better
-- than hand-rolling calculations for each "interesting" button.

TOOLBAR_HEIGHT = 3
SECONDARY_TOOLBAR_HEIGHT = 3

-- Basic rectangular size class. Should be structurally compatible with
-- gui.dimension (get_interface_rect) and gui.ViewRect (updateLayout's
-- parent_rect), but the LuaLSP disagrees.
---@class DFLayout.Rectangle.Size.class
---@field width integer
---@field height integer

-- An alias that gathers a few types that we know are compatible with our
-- width/height size requirements.
---@alias DFLayout.Rectangle.Size
--- | DFLayout.Rectangle.Size.class basic width/height size
--- | gui.dimension                 e.g., gui.get_interface_rect()
--- | gui.ViewRect                  e.g., parent_rect supplied to updateLayout subsidiary methods

---@type DFLayout.Rectangle.Size
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

---@class DFLayout.FullInsets
---@field l integer Gap between the left edge of the frame and the parent.
---@field t integer Gap between the top edge of the frame and the parent.
---@field r integer Gap between the right edge of the frame and the parent.
---@field b integer Gap between the bottom edge of the frame and the parent.

-- Like widgets.Widget.frame, but no optional fields.
---@class DFLayout.FullyPlacedFrame: DFLayout.FullInsets
---@field w integer Width
---@field h integer Height

-- Function that generates a "full placement" for a given interface size.
---@alias DFLayout.FrameFn fun(interface_size: DFLayout.Rectangle.Size): DFLayout.FullyPlacedFrame

---@alias DFLayout.Toolbar.Button { offset: integer, width: integer }
---@alias DFLayout.Toolbar.Buttons table<string,DFLayout.Toolbar.Button> -- multiple entries
---@alias DFLayout.Toolbar.Widths table<string,integer> -- single entry, value is width

---@class DFLayout.Toolbar.Layout
---@field buttons DFLayout.Toolbar.Buttons
---@field width integer

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

--- DF UI element definitions ---

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

---@class DFLayout.DynamicUIElement
---@field frame_fn DFLayout.FrameFn
---@field minimum_insets DFLayout.FullInsets

-- Create a DFLayout.DynamicUIElement from a DFLayout.FrameFn.
--
-- Note: The `frame_fn` must generate inset values that are non-decreasing as
-- the input interface size grows (i.e., the minimum insets are found when
-- placing the frame in a minimum-size interface area). This is true for all the
-- DF toolbars and their sub-components, but not for all UI elements in general.
---@param frame_fn DFLayout.FrameFn
---@return DFLayout.DynamicUIElement
local function nd_inset_ui_el(frame_fn)
    local min_frame = frame_fn(MINIMUM_INTERFACE_SIZE)
    return {
        frame_fn = frame_fn,
        minimum_insets = {
            l = min_frame.l,
            r = min_frame.r,
            t = min_frame.t,
            b = min_frame.b,
        }
    }
end

-- Derive the DynamicUIElement for the named button given a toolbar's frame_fn, and its button button layout.
---@param toolbar_frame_fn DFLayout.FrameFn
---@param toolbar_layout DFLayout.Toolbar.Layout
---@param button_name string
---@return DFLayout.DynamicUIElement
local function button_ui_el(toolbar_frame_fn, toolbar_layout, button_name)
    local button = toolbar_layout.buttons[button_name]
        or dfhack.error('button not present in given toolbar layout: ' .. tostring(button_name))
    return nd_inset_ui_el(function(interface_size)
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

-- button_ui_el, specialized for the secondary toolbars.
---@param toolbar_name DFLayout.Fort.SecondaryToolbar.Names
---@param button_name string
---@return DFLayout.DynamicUIElement
local function secondary_button_ui_el(toolbar_name, button_name)
    local frame_fn = fort_secondary_tb_frames[toolbar_name]
        or dfhack.error('secondary toolbar name not in fort_secondary_tb_frames: ' .. tostring(toolbar_name))
    local layout = fort_stb_layout[toolbar_name]
        or dfhack.error('secondary toolbar name not in fort_el_layout.secondary_toolbars: ' .. tostring(toolbar_name))
    return button_ui_el(frame_fn, layout, button_name)
end

elements = {
    fort = {
        toolbars = {
            ---@type DFLayout.DynamicUIElement
            left = nd_inset_ui_el(fort_left_tb_frame),
            ---@type DFLayout.DynamicUIElement
            center = nd_inset_ui_el(fort_center_tb_frame),
            ---@type DFLayout.DynamicUIElement
            right = nd_inset_ui_el(fort_right_tb_frame),
        },
        toolbar_buttons = {
            left = {
                ---@type DFLayout.DynamicUIElement
                MAIN_OPEN_CREATURES = button_ui_el(fort_left_tb_frame, fort_tb_layout.left, 'MAIN_OPEN_CREATURES'),
                ---@type DFLayout.DynamicUIElement
                MAIN_OPEN_TASKS = button_ui_el(fort_left_tb_frame, fort_tb_layout.left, 'MAIN_OPEN_TASKS'),
                ---@type DFLayout.DynamicUIElement
                MAIN_OPEN_PLACES = button_ui_el(fort_left_tb_frame, fort_tb_layout.left, 'MAIN_OPEN_PLACES'),
                ---@type DFLayout.DynamicUIElement
                MAIN_OPEN_LABOR = button_ui_el(fort_left_tb_frame, fort_tb_layout.left, 'MAIN_OPEN_LABOR'),
                ---@type DFLayout.DynamicUIElement
                MAIN_OPEN_WORK_ORDERS = button_ui_el(fort_left_tb_frame, fort_tb_layout.left, 'MAIN_OPEN_WORK_ORDERS'),
                ---@type DFLayout.DynamicUIElement
                MAIN_OPEN_NOBLES = button_ui_el(fort_left_tb_frame, fort_tb_layout.left, 'MAIN_OPEN_NOBLES'),
                ---@type DFLayout.DynamicUIElement
                MAIN_OPEN_OBJECTS = button_ui_el(fort_left_tb_frame, fort_tb_layout.left, 'MAIN_OPEN_OBJECTS'),
                ---@type DFLayout.DynamicUIElement
                MAIN_OPEN_JUSTICE = button_ui_el(fort_left_tb_frame, fort_tb_layout.left, 'MAIN_OPEN_JUSTICE'),
            },
            center = {
                ---@type DFLayout.DynamicUIElement
                DIG = button_ui_el(fort_center_tb_frame, fort_tb_layout.center, 'DIG'),
                ---@type DFLayout.DynamicUIElement
                CHOP = button_ui_el(fort_center_tb_frame, fort_tb_layout.center, 'CHOP'),
                ---@type DFLayout.DynamicUIElement
                GATHER = button_ui_el(fort_center_tb_frame, fort_tb_layout.center, 'GATHER'),
                ---@type DFLayout.DynamicUIElement
                SMOOTH = button_ui_el(fort_center_tb_frame, fort_tb_layout.center, 'SMOOTH'),
                ---@type DFLayout.DynamicUIElement
                ERASE = button_ui_el(fort_center_tb_frame, fort_tb_layout.center, 'ERASE'),
                ---@type DFLayout.DynamicUIElement
                MAIN_BUILDING_MODE = button_ui_el(fort_center_tb_frame, fort_tb_layout.center, 'MAIN_BUILDING_MODE'),
                ---@type DFLayout.DynamicUIElement
                MAIN_STOCKPILE_MODE = button_ui_el(fort_center_tb_frame, fort_tb_layout.center, 'MAIN_STOCKPILE_MODE'),
                ---@type DFLayout.DynamicUIElement
                MAIN_ZONE_MODE = button_ui_el(fort_center_tb_frame, fort_tb_layout.center, 'MAIN_ZONE_MODE'),
                ---@type DFLayout.DynamicUIElement
                MAIN_BURROW_MODE = button_ui_el(fort_center_tb_frame, fort_tb_layout.center, 'MAIN_BURROW_MODE'),
                ---@type DFLayout.DynamicUIElement
                MAIN_HAULING_MODE = button_ui_el(fort_center_tb_frame, fort_tb_layout.center, 'MAIN_HAULING_MODE'),
                ---@type DFLayout.DynamicUIElement
                TRAFFIC = button_ui_el(fort_center_tb_frame, fort_tb_layout.center, 'TRAFFIC'),
                ---@type DFLayout.DynamicUIElement
                ITEM_BUILDING = button_ui_el(fort_center_tb_frame, fort_tb_layout.center, 'ITEM_BUILDING'),
            },
            center_close = {
                ---@type DFLayout.DynamicUIElement
                DIG_LOWER_MODE = button_ui_el(fort_center_tb_frame, fort_tb_layout.center, 'DIG'),
                ---@type DFLayout.DynamicUIElement
                CHOP_LOWER_MODE = button_ui_el(fort_center_tb_frame, fort_tb_layout.center, 'CHOP'),
                ---@type DFLayout.DynamicUIElement
                GATHER_LOWER_MODE = button_ui_el(fort_center_tb_frame, fort_tb_layout.center, 'GATHER'),
                ---@type DFLayout.DynamicUIElement
                SMOOTH_LOWER_MODE = button_ui_el(fort_center_tb_frame, fort_tb_layout.center, 'SMOOTH'),
                ---@type DFLayout.DynamicUIElement
                ERASE_LOWER_MODE = button_ui_el(fort_center_tb_frame, fort_tb_layout.center, 'ERASE'),
                ---@type DFLayout.DynamicUIElement
                MAIN_BUILDING_MODE_LOWER_MODE = button_ui_el(fort_center_tb_frame, fort_tb_layout.center, 'MAIN_BUILDING_MODE'),
                ---@type DFLayout.DynamicUIElement
                MAIN_STOCKPILE_MODE_LOWER_MODE = button_ui_el(fort_center_tb_frame, fort_tb_layout.center, 'MAIN_STOCKPILE_MODE'),
                ---@type DFLayout.DynamicUIElement
                MAIN_ZONE_MODE_LOWER_MODE = button_ui_el(fort_center_tb_frame, fort_tb_layout.center, 'MAIN_ZONE_MODE'),
                ---@type DFLayout.DynamicUIElement
                MAIN_BURROW_MODE_LOWER_MODE = button_ui_el(fort_center_tb_frame, fort_tb_layout.center, 'MAIN_BURROW_MODE'),
                ---@type DFLayout.DynamicUIElement
                MAIN_HAULING_MODE_LOWER_MODE = button_ui_el(fort_center_tb_frame, fort_tb_layout.center, 'MAIN_HAULING_MODE'),
                ---@type DFLayout.DynamicUIElement
                TRAFFIC_LOWER_MODE = button_ui_el(fort_center_tb_frame, fort_tb_layout.center, 'TRAFFIC'),
                ---@type DFLayout.DynamicUIElement
                ITEM_BUILDING_LOWER_MODE = button_ui_el(fort_center_tb_frame, fort_tb_layout.center, 'ITEM_BUILDING'),
            },
            right = {
                ---@type DFLayout.DynamicUIElement
                MAIN_OPEN_SQUADS = button_ui_el(fort_right_tb_frame, fort_tb_layout.right, 'MAIN_OPEN_SQUADS'),
                ---@type DFLayout.DynamicUIElement
                MAIN_OPEN_WORLD = button_ui_el(fort_right_tb_frame, fort_tb_layout.right, 'MAIN_OPEN_WORLD'),
            },
        },
        secondary_toolbars = {
            ---@type DFLayout.DynamicUIElement
            DIG = nd_inset_ui_el(fort_secondary_tb_frames.DIG),
            ---@type DFLayout.DynamicUIElement
            CHOP = nd_inset_ui_el(fort_secondary_tb_frames.CHOP),
            ---@type DFLayout.DynamicUIElement
            GATHER = nd_inset_ui_el(fort_secondary_tb_frames.GATHER),
            ---@type DFLayout.DynamicUIElement
            SMOOTH = nd_inset_ui_el(fort_secondary_tb_frames.SMOOTH),
            ---@type DFLayout.DynamicUIElement
            ERASE = nd_inset_ui_el(fort_secondary_tb_frames.ERASE),
            ---@type DFLayout.DynamicUIElement
            MAIN_STOCKPILE_MODE = nd_inset_ui_el(fort_secondary_tb_frames.MAIN_STOCKPILE_MODE),
            ---@type DFLayout.DynamicUIElement
            STOCKPILE_NEW = nd_inset_ui_el(fort_secondary_tb_frames.STOCKPILE_NEW),
            ---@type DFLayout.DynamicUIElement
            ['Add new burrow'] = nd_inset_ui_el(fort_secondary_tb_frames['Add new burrow']),
            ---@type DFLayout.DynamicUIElement
            TRAFFIC = nd_inset_ui_el(fort_secondary_tb_frames.TRAFFIC),
            ---@type DFLayout.DynamicUIElement
            ITEM_BUILDING = nd_inset_ui_el(fort_secondary_tb_frames.ITEM_BUILDING),
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

--- Automatic UI-relative Overlay Positioning ---

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
---@alias DFLayout.Placement.DefaultPos { x?: integer, y?: integer }

---@class DFLayout.Placement.Size
---@field w integer
---@field h integer

---@class DFLayout.Placement.Spec
---@field size DFLayout.Placement.Size the static size of overlay
---@field ui_element DFLayout.DynamicUIElement a UI element value from the `elements` tree
---@field h_placement DFLayout.Placement.HorizontalAlignment how to align the overlay's horizontal position against the `ui_element`
---@field v_placement DFLayout.Placement.VerticalAlignment how to align the overlay's vertical position against the `ui_element`
---@field offset? DFLayout.Placement.Offset how far to move overlay after alignment with `ui_element`
---@field default_pos? DFLayout.Placement.DefaultPos supply "legacy" overlay default_pos for placement compatibility

---@alias DFLayout.Placement.GenericAlignment 'place before' | 'align start' | 'align end' | 'place after'

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

-- Place a specified span (width or height) with the specified alignment with
-- respect to the given reference position and span.
---@param available_span integer
---@param ref_offset_before integer
---@param ref_span integer
---@param placed_span integer
---@param placement DFLayout.Placement.GenericAlignment
---@param offset? integer
---@return integer before
---@return integer span
---@return integer after
local function place_span(available_span, ref_offset_before, ref_span, placed_span, placement, offset)
    if placed_span >= available_span then
        return 0, available_span, 0
    end
    local before
    if placement == 'align start' then
        before = ref_offset_before
    elseif placement == 'align end' then
        before = ref_offset_before + ref_span - placed_span
    elseif placement == 'place before' then
        before = ref_offset_before - placed_span
    elseif placement == 'place after' then
        before = ref_offset_before + ref_span
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


-- Runs `frame_fn(interface_size)` and checks the resulting frame for "sanity"
-- (non-negative paddings, positive sizes, paddings and sizes span
-- `interface_size`).
--
-- Returns the frame or throws an error.
---@param interface_size DFLayout.Rectangle.Size
---@param frame_fn DFLayout.FrameFn
---@return DFLayout.FullyPlacedFrame
local function checked_frame(interface_size, frame_fn)
    local frame = frame_fn(interface_size)
    if frame.l < 0 or frame.w <= 0 or frame.r < 0
        or frame.l + frame.w + frame.r ~= interface_size.width
    then
        dfhack.error(
            ('horizontal placement is invalid: l=%d w=%d r=%d W=%d')
            :format(frame.l, frame.w, frame.r, interface_size.width))
    end
    if frame.t < 0 or frame.h <= 0 or frame.b < 0
        or frame.t + frame.h + frame.b ~= interface_size.height
    then
        dfhack.error(
            ('vertical placement is invalid: t=%d h=%d rb%d H=%d')
            :format(frame.t, frame.h, frame.b, interface_size.height))
    end
    return frame
end

-- Place the specified area with respect to the specified reference with the
-- specified alignment and offset.
---@param interface_size DFLayout.Rectangle.Size
---@param spec DFLayout.Placement.Spec
---@return DFLayout.FullyPlacedFrame
local function place_overlay_frame(interface_size, spec)
    local ref_frame = checked_frame(interface_size, spec.ui_element.frame_fn)

    local generic_h_placement = generic_placement_from_horizontal[spec.h_placement]
        or dfhack.error('invalid h_placement: ' .. tostring(spec.h_placement))
    local l, w, r = place_span(interface_size.width,
        ref_frame.l, ref_frame.w,
        spec.size.w, generic_h_placement, spec.offset and spec.offset.x)

    local generic_v_placement = generic_placement_from_vertical[spec.v_placement]
        or dfhack.error('invalid v_placement: ' .. tostring(spec.v_placement))
    local t, h, b = place_span(interface_size.height,
        ref_frame.t, ref_frame.h,
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

-- Provide default `default_pos` values based on nominal values, and compute the
-- direction-specific delta to those nominal values.
---@param xy 'x' | 'y' the default_pos field name (used in error messages)
---@param pos? integer
---@param nominal_positive integer
---@param nominal_negative integer
---@param default_to_positive boolean controls which nominal value is used if `pos` is falsy
---@return integer pos `pos`, or one of its defaults (when 0 or falsy)
---@return integer padding_on_positive_side 0 or padding required to move from positive `value` to `nominal_positive`
---@return integer padding_on_negative_side 0 or padding required to move from negative `value` to `nominal_negative`
local function pos_and_paddings(xy, pos, nominal_positive, nominal_negative, default_to_positive)
    if not pos or pos == 0 then
        pos = default_to_positive and nominal_positive or nominal_negative
    end
    if 0 < pos then
        if nominal_positive < pos then
            dfhack.error('specified placement requires 1 <= default_pos.'..xy..' <= '..nominal_positive)
        end
        return pos, nominal_positive - pos, 0
    end
    if pos < nominal_negative then
        dfhack.error('specified placement requires -1 >= default_pos.'..xy..' >= '..nominal_negative)
    end
    return pos, 0, pos - nominal_negative
end

---@class DFLayout.OverlayPlacementInfo
---@field default_pos { x: integer, y: integer } use for the overlay's default_pos
---@field frame widgets.Widget.frame use for the overlay's initial frame
---@field preUpdateLayout_fn fun(self_overlay_widget: widgets.Widget, parent_rect: gui.ViewRect) use the overlay's preUpdateLayout method (the "self" param is overlay.OverlayWidget, but that isn't a declared type)

---@alias DFLayout.Placement.InsetsFilter fun(insets: DFLayout.FullInsets): DFLayout.FullInsets

---@param overlay_placement_spec DFLayout.Placement.Spec
---@param insets_filter? DFLayout.Placement.InsetsFilter
---@return DFLayout.OverlayPlacementInfo overlay_placement_info
local function get_overlay_placement_info(overlay_placement_spec, insets_filter)
    overlay_placement_spec = utils.clone(overlay_placement_spec, true) --[[@as DFLayout.Placement.Spec]]
    local minimum_placement = place_overlay_frame(MINIMUM_INTERFACE_SIZE, overlay_placement_spec)

    -- decode spec.default_pos into pos values and padding values
    local override_default_pos = overlay_placement_spec.default_pos
    local x_pos, l_pad, r_pad = pos_and_paddings('x',
        override_default_pos and override_default_pos.x,
        (minimum_placement.l + 1),  -- one-based, left-relative
        -(minimum_placement.r + 1), -- one-based, right-relative
        true                        -- default to left-relative
    )
    local y_pos, t_pad, b_pad = pos_and_paddings('y',
        override_default_pos and override_default_pos.y,
        (minimum_placement.t + 1),  -- one-based, top-relative
        -(minimum_placement.b + 1), -- one-based, bottom-relative
        false                       -- default to bottom-relative
    )

    return {
        default_pos = {
            x = x_pos,
            y = y_pos,
        },
        frame = {
            w = math.min(MINIMUM_INTERFACE_SIZE.width, overlay_placement_spec.size.w),
            h = math.min(MINIMUM_INTERFACE_SIZE.height, overlay_placement_spec.size.h),
        },
        ---@param self_overlay_widget widgets.Widget
        ---@param parent_rect gui.ViewRect
        preUpdateLayout_fn = function(self_overlay_widget, parent_rect)
            local el_frame = overlay_placement_spec.ui_element.frame_fn(parent_rect)
            local minimum_el_insets = overlay_placement_spec.ui_element.minimum_insets
            local insets = {
                l = math.max(0, el_frame.l - minimum_el_insets.l) + l_pad,
                r = math.max(0, el_frame.r - minimum_el_insets.r) + r_pad,
                t = math.max(0, el_frame.t - minimum_el_insets.t) + t_pad,
                b = math.max(0, el_frame.b - minimum_el_insets.b) + b_pad,
            }
            insets = insets_filter and insets_filter(insets) or insets
            local placement = place_overlay_frame(parent_rect, overlay_placement_spec)

            self_overlay_widget.frame_inset = insets
            self_overlay_widget.frame.w = insets.l + placement.w + insets.r
            self_overlay_widget.frame.h = insets.t + placement.h + insets.b
        end,
    }
end

-- Return a table with values that can be used to automatically place an
-- overlay widget relative to a reference position.
---@param overlay_placement_spec DFLayout.Placement.Spec
---@return DFLayout.OverlayPlacementInfo overlay_placement_info
function getOverlayPlacementInfo(overlay_placement_spec)
    return get_overlay_placement_info(overlay_placement_spec)
end

---@type DFLayout.Placement.InsetsFilter
local function only_left_inset(insets)
    return {
        l = insets.l,
        r = 0,
        t = 0,
        b = 0,
    }
end
-- Similar to `getOverlayPlacementInfo`, but only arranges for "padding" on the
-- left. This is compatible with several existing, hand-rolled overlay
-- positioning calculations.
---@param overlay_placement_spec DFLayout.Placement.Spec
---@return DFLayout.OverlayPlacementInfo overlay_placement_info
function getLeftOnlyOverlayPlacementInfo(overlay_placement_spec)
    return get_overlay_placement_info(overlay_placement_spec, only_left_inset)
end

return _ENV
