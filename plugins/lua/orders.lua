local _ENV = mkmodule('plugins.orders')

local dialogs = require('gui.dialogs')
local gui = require('gui')
local overlay = require('plugins.overlay')
local widgets = require('gui.widgets')

--
-- OrdersOverlay
--

local function is_orders_panel_visible()
    local info = df.global.game.main_interface.info
    return info.open and info.current_mode == df.info_interface_mode_type.WORK_ORDERS
end

local function do_sort()
    dfhack.run_command('orders', 'sort')
end

local function do_clear()
    dialogs.showYesNoPrompt('Clear manager orders?',
        'Are you sure you want to clear the manager orders?', nil,
        function() dfhack.run_command('orders', 'clear') end)
end

local function do_import()
    local output = dfhack.run_command_silent('orders', 'list')
    dialogs.ListBox{
        frame_title='Import Manager Orders',
        with_filter=true,
        choices=output:split('\n'),
        on_select=function(idx, choice)
            dfhack.run_command('orders', 'import', choice.text)
        end,
    }:show()
end

local function do_export()
    dialogs.InputBox{
        frame_title='Export Manager Orders',
        on_input=function(text)
            dfhack.run_command('orders', 'export', text)
        end
    }:show()
end

OrdersOverlay = defclass(OrdersOverlay, overlay.OverlayWidget)
OrdersOverlay.ATTRS{
    default_pos={x=53,y=-6},
    default_enabled=true,
    viewscreens='dwarfmode',
    frame={w=30, h=4},
    frame_style=gui.MEDIUM_FRAME,
    frame_background=gui.CLEAR_PEN,
}

function OrdersOverlay:init()
    self:addviews{
        widgets.HotkeyLabel{
            frame={t=0, l=0},
            label='import',
            key='CUSTOM_CTRL_I',
            on_activate=do_import,
        },
        widgets.HotkeyLabel{
            frame={t=1, l=0},
            label='export',
            key='CUSTOM_CTRL_E',
            on_activate=do_export,
        },
        widgets.HotkeyLabel{
            frame={t=0, l=15},
            label='sort',
            key='CUSTOM_CTRL_O',
            on_activate=do_sort,
        },
        widgets.HotkeyLabel{
            frame={t=1, l=15},
            label='clear',
            key='CUSTOM_CTRL_C',
            on_activate=do_clear,
        },
    }
end

function OrdersOverlay:render(dc)
    if not is_orders_panel_visible() then return false end
    OrdersOverlay.super.render(self, dc)
end

function OrdersOverlay:onInput(keys)
    if not is_orders_panel_visible() then return false end
    OrdersOverlay.super.onInput(self, keys)
end

OVERLAY_WIDGETS = {
    overlay=OrdersOverlay,
}

return _ENV
