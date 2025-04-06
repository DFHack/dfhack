local _ENV = mkmodule('gui.dflayout')

-- Provide data-driven locations for the DF toolbars at the bottom of the
-- screen. Not quite as nice as getting the data from DF directly, but better
-- than hand-rolling calculations for each "interesting" button.

TOOLBAR_HEIGHT = 3
SECONDARY_TOOLBAR_HEIGHT = 3
MINIMUM_INTERFACE_SIZE = require('gui').mkdims_wh(0, 0, 114, 46)

-- Only width and height are used here. We could define a new "@class", but
-- LuaLS doesn't seem to accept gui.dimensions values as compatible with that
-- new class...
---@alias DFLayout.Rectangle.Size gui.dimension

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

---@alias DFLayout.Toolbar.NamedWidth table<string,integer> -- single entry, value is width
---@alias DFLayout.Toolbar.NamedOffsets table<string,integer> -- multiple entries, values are offsets
---@alias DFLayout.Toolbar.Button { offset: integer, width: integer }
---@alias DFLayout.Toolbar.NamedButtons table<string,DFLayout.Toolbar.Button> -- multiple entries

---@class DFLayout.Widget.frame: widgets.Widget.frame
---@field l integer Gap between the left edge of the frame and the parent.
---@field t integer Gap between the top edge of the frame and the parent.
---@field r integer Gap between the right edge of the frame and the parent.
---@field b integer Gap between the bottom edge of the frame and the parent.
---@field w integer Width
---@field h integer Height

---@class DFLayout.Toolbar.Base
---@field buttons DFLayout.Toolbar.NamedButtons
---@field width integer

---@class DFLayout.Toolbar: DFLayout.Toolbar.Base
---@field frame fun(interface_size: DFLayout.Rectangle.Size): DFLayout.Widget.frame

---@param widths DFLayout.Toolbar.NamedWidth[] single-name entries only!
---@return DFLayout.Toolbar.Base
local function button_widths_to_toolbar(widths)
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

---@param buttons string[]
---@return DFLayout.Toolbar.NamedWidth[]
local function buttons_to_widths(buttons)
    local widths = {}
    for _, button_name in ipairs(buttons) do
        table.insert(widths, { [button_name] = 4 })
    end
    return widths
end

---@param buttons string[]
---@return DFLayout.Toolbar.Base
local function buttons_to_toolbar(buttons)
    return button_widths_to_toolbar(buttons_to_widths(buttons))
end

-- Fortress mode toolbar definitions
fort = {}

---@alias DFLayout.PrimaryToolbarNames 'left' | 'center' | 'right'

---@type table<DFLayout.PrimaryToolbarNames,DFLayout.Toolbar>
fort.toolbars = {}

---@class DFLayout.Fort.Toolbar.Left: DFLayout.Toolbar
fort.toolbars.left = buttons_to_toolbar{
    'citizens', 'tasks', 'places', 'labor',
    'orders', 'nobles', 'objects', 'justice',
}

---@param interface_size DFLayout.Rectangle.Size
---@return DFLayout.Widget.frame
function fort.toolbars.left.frame(interface_size)
    return {
        l = 0,
        w = fort.toolbars.left.width,
        r = interface_size.width - fort.toolbars.left.width,

        t = interface_size.height - TOOLBAR_HEIGHT,
        h = TOOLBAR_HEIGHT,
        b = 0,
    }
end

local fort_left_center_toolbar_gap_minimum = 7

---@class DFLayout.Fort.Toolbar.Center: DFLayout.Toolbar
fort.toolbars.center = button_widths_to_toolbar{
    { _left_border = 1 },
    { dig = 4 }, { chop = 4 }, { gather = 4 }, { smooth = 4 }, { erase = 4 },
    { _divider = 1 },
    { build = 4 }, { stockpile = 4 }, { zone = 4 },
    { _divider = 1 },
    { burrow = 4 }, { cart = 4 }, { traffic = 4 },
    { _divider = 1 },
    { mass_designation = 4 },
    { _right_border = 1 },
}

---@param interface_size DFLayout.Rectangle.Size
---@return DFLayout.Widget.frame
function fort.toolbars.center.frame(interface_size)
    -- center toolbar is "centered" in interface area, but never closer to the
    -- left toolbar than fort_left_center_toolbar_gap_minimum

    local interface_offset_centered = math.ceil((interface_size.width - fort.toolbars.center.width + 1) / 2)
    local interface_offset_min = fort.toolbars.left.width + fort_left_center_toolbar_gap_minimum
    local interface_offset = math.max(interface_offset_min, interface_offset_centered)

    return {
        l = interface_offset,
        w = fort.toolbars.center.width,
        r = interface_size.width - interface_offset - fort.toolbars.center.width,

        t = interface_size.height - TOOLBAR_HEIGHT,
        h = TOOLBAR_HEIGHT,
        b = 0,
    }
end

---@alias DFLayout.Fort.SecondaryToolbar.ToolNames  'dig' | 'chop' | 'gather' | 'smooth' | 'erase' | 'build' | 'stockpile' |                     'zone' | 'burrow' |                   'cart' | 'traffic' | 'mass_designation'
---@alias DFLayout.Fort.SecondaryToolbar.Names      'dig' | 'chop' | 'gather' | 'smooth' | 'erase' |           'stockpile' | 'stockpile_paint' |                      'burrow_paint' |          'traffic' | 'mass_designation'

---@param interface_size DFLayout.Rectangle.Size
---@param tool_name DFLayout.Fort.SecondaryToolbar.ToolNames
---@param secondary_toolbar DFLayout.Toolbar.Base
---@return DFLayout.Widget.frame
local function center_secondary_frame(interface_size, tool_name, secondary_toolbar)
    local toolbar_offset = fort.toolbars.center.frame(interface_size).l
    local toolbar_button = fort.toolbars.center.buttons[tool_name] or dfhack.error('invalid tool name: ' .. tool_name)

    -- Ideally, the secondary toolbar is positioned directly above the (main) toolbar button
    local ideal_offset = toolbar_offset + toolbar_button.offset

    -- In "narrow" interfaces conditions, a wide secondary toolbar (pretty much
    -- any tool that has "advanced" options) that was ideally positioned above
    -- its tool's button would extend past the right edge of the interface area.
    -- Such wide secondary toolbars are instead right justified with a bit of
    -- padding.

    -- padding necessary to line up width-constrained secondaries
    local secondary_padding = 5
    local width_constrained_offset = math.max(0, interface_size.width - (secondary_toolbar.width + secondary_padding))

    -- Use whichever position is left-most.
    local l = math.min(ideal_offset, width_constrained_offset)
    return {
        l = l,
        w = secondary_toolbar.width,
        r = interface_size.width - l - secondary_toolbar.width,

        t = interface_size.height - TOOLBAR_HEIGHT - SECONDARY_TOOLBAR_HEIGHT,
        h = SECONDARY_TOOLBAR_HEIGHT,
        b = TOOLBAR_HEIGHT,
    }
end

---@type table<DFLayout.Fort.SecondaryToolbar.Names,DFLayout.Toolbar>
fort.secondary_toolbars = {}

---@param tool_name DFLayout.Fort.SecondaryToolbar.ToolNames
---@param secondary_name DFLayout.Fort.SecondaryToolbar.Names
---@param toolbar DFLayout.Toolbar.Base
---@return DFLayout.Toolbar
local function define_center_secondary(tool_name, secondary_name, toolbar)
    local ntb = toolbar --[[@as DFLayout.Toolbar]]
    ---@param interface_size DFLayout.Rectangle.Size
    ---@return DFLayout.Widget.frame
    function ntb.frame(interface_size)
        return center_secondary_frame(interface_size, tool_name, ntb)
    end
    fort.secondary_toolbars[secondary_name] = ntb
    return ntb
end

define_center_secondary('dig', 'dig', buttons_to_toolbar{
    'dig', 'stairs', 'ramp', 'channel', 'remove_construction', '_gap',
    'rectangle', 'draw', '_gap',
    'advanced_toggle', '_gap',
    'all', 'auto', 'ore_gem', 'gem', '_gap',
    'p1', 'p2', 'p3', 'p4', 'p5', 'p6', 'p7', '_gap',
    'blueprint', 'blueprint_to_standard', 'standard_to_blueprint',
})
define_center_secondary('chop', 'chop', buttons_to_toolbar{
    'chop', '_gap',
    'rectangle', 'draw', '_gap',
    'advanced_toggle', '_gap',
    'p1', 'p2', 'p3', 'p4', 'p5', 'p6', 'p7', '_gap',
    'blueprint', 'blueprint_to_standard', 'standard_to_blueprint',
})
define_center_secondary('gather', 'gather', buttons_to_toolbar{
    'gather', '_gap',
    'rectangle', 'draw', '_gap',
    'advanced_toggle', '_gap',
    'p1', 'p2', 'p3', 'p4', 'p5', 'p6', 'p7', '_gap',
    'blueprint', 'blueprint_to_standard', 'standard_to_blueprint',
})
define_center_secondary('smooth', 'smooth', buttons_to_toolbar{
    'smooth', 'engrave', 'carve_track', 'carve_fortification', '_gap',
    'rectangle', 'draw', '_gap',
    'advanced_toggle', '_gap',
    'p1', 'p2', 'p3', 'p4', 'p5', 'p6', 'p7', '_gap',
    'blueprint', 'blueprint_to_standard', 'standard_to_blueprint',
})
define_center_secondary( 'erase', 'erase', buttons_to_toolbar{
    'rectangle',
    'draw',
})
-- build -- completely different and quite variable
define_center_secondary('stockpile', 'stockpile', buttons_to_toolbar{ 'add_stockpile' })
define_center_secondary('stockpile', 'stockpile_paint', buttons_to_toolbar{
    'rectangle', 'draw', 'erase_toggle', 'remove',
})
-- zone -- no secondary toolbar
-- burrow -- no direct secondary toolbar
define_center_secondary('burrow', 'burrow_paint', buttons_to_toolbar{
    'rectangle', 'draw', 'erase_toggle', 'remove',
})
-- cart -- no secondary toolbar
define_center_secondary('traffic', 'traffic', button_widths_to_toolbar(
    concat_sequences{ buttons_to_widths{
        'high', 'normal', 'low', 'restricted', '_gap',
        'rectangle', 'draw', '_gap',
        'advanced_toggle', '_gap',
    }, {
        { weight_which = 4 },
        { weight_slider = 26 },
        { weight_input = 6 },
    } }
))
define_center_secondary('mass_designation', 'mass_designation', buttons_to_toolbar{
    'claim', 'forbid', 'dump', 'no_dump', 'melt', 'no_melt', 'hidden', 'visible', '_gap',
    'rectangle', 'draw',
})

---@class DFLayout.Fort.Toolbar.Right: DFLayout.Toolbar
fort.toolbars.right = buttons_to_toolbar{
    'squads', 'world',
}

---@param interface_size DFLayout.Rectangle.Size
---@return DFLayout.Widget.frame
function fort.toolbars.right.frame(interface_size)
    return {
        l = interface_size.width - fort.toolbars.right.width,
        w = fort.toolbars.right.width,
        r = 0,

        t = interface_size.height - TOOLBAR_HEIGHT,
        h = TOOLBAR_HEIGHT,
        b = 0,
    }
end

return _ENV
