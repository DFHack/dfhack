local _ENV = mkmodule('plugins.stocks')

local gui = require('gui')
local overlay = require('plugins.overlay')
local widgets = require('gui.widgets')

local stocks = df.global.game.main_interface.stocks

local function collapse_all()
    local num_sections = #stocks.current_type_a_expanded
    for idx=0,num_sections-1 do
        stocks.current_type_a_expanded[idx] = false
    end
    stocks.i_height = num_sections * 3
end

local function remove_empty()
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
            label='collapse all',
            key='CUSTOM_CTRL_X',
            on_activate=collapse_all,
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
