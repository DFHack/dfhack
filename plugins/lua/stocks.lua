local _ENV = mkmodule('plugins.stocks')

local gui = require('gui')
local overlay = require('plugins.overlay')
local widgets = require('gui.widgets')

local stocks = df.global.game.main_interface.stocks

-- these are only safe to call while the stocks page is open; guard against
-- stray activations as the page closes
local function collapse_all()
    if not stocks.open then return end
    local num_sections = #stocks.current_type_a_expanded
    for idx=0,num_sections-1 do
        stocks.current_type_a_expanded[idx] = false
    end
    stocks.i_height = num_sections * 3
    -- the collapsed list is much shorter; reset the scroll position so the
    -- view isn't left pointing past the end of the list
    stocks.scroll_position_item = 0
end

local function expand_all()
    if not stocks.open then return end
    local num_sections = #stocks.current_type_a_expanded
    for idx=0,num_sections-1 do
        stocks.current_type_a_expanded[idx] = true
    end
    local num_items = #stocks.current_type_i_list
    stocks.i_height = (num_items + num_sections) * 3
end

local function all_collapsed()
    if not stocks.open then return true end
    for idx=0,#stocks.current_type_a_expanded-1 do
        if stocks.current_type_a_expanded[idx] then return false end
    end
    return true
end

local function toggle_all()
    if not stocks.open then return end
    if all_collapsed() then expand_all() else collapse_all() end
end

local function remove_empty()
    if not stocks.open then return end
    local empties = {}
    for itype,v in ipairs(stocks.storeamount) do
        if v == 0 and stocks.badamount[itype] == 0 then
            empties[itype] = true
        end
    end
    for idx=#stocks.type_list-1,0,-1 do
        if empties[stocks.type_list[idx]] then stocks.type_list:erase(idx) end
    end
    for idx=#stocks.filtered_type_list-1,0,-1 do
        if empties[stocks.filtered_type_list[idx]] then stocks.filtered_type_list:erase(idx) end
    end
    -- removing types shortens the type list; keep the scroll position in bounds
    if stocks.scroll_position_type >= #stocks.filtered_type_list then
        stocks.scroll_position_type = math.max(0, #stocks.filtered_type_list - 1)
    end
end

-- -------------------
-- StocksOverlay
--

StocksOverlay = defclass(StocksOverlay, overlay.OverlayWidget)
StocksOverlay.ATTRS{
    desc='Adds productivity actions to the stocks page.',
    default_pos={x=-3,y=-20},
    default_enabled=true,
    viewscreens='dwarfmode/Stocks',
    frame={w=27, h=7},
    frame_style=gui.MEDIUM_FRAME,
    frame_background=gui.CLEAR_PEN,
}

function StocksOverlay:init()
    self:addviews{
        widgets.HotkeyLabel{
            frame={t=0, l=0},
            label=function()
                return all_collapsed() and 'expand all' or 'collapse all'
            end,
            key='CUSTOM_CTRL_X',
            on_activate=toggle_all,
        },
        widgets.HotkeyLabel{
            frame={t=1, l=0},
            label='expand all',
            key='CUSTOM_CTRL_Z',
            on_activate=expand_all,
        },
        widgets.HotkeyLabel{
            frame={t=2, l=0},
            label='remove empties',
            key='CUSTOM_CTRL_E',
            on_activate=remove_empty,
        },
        widgets.Label{
            frame={b=0, l=0},
            text = 'Shift+Scroll',
            text_pen=COLOR_LIGHTGREEN,
        },
        widgets.Label{
            frame={b=0, l=12},
            text = ': fast scroll',
        },
    }
end

OVERLAY_WIDGETS = {
    overlay=StocksOverlay,
}

return _ENV
