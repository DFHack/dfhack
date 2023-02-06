local _ENV = mkmodule('plugins.orders')

local dialogs = require('gui.dialogs')
local gui = require('gui')
local overlay = require('plugins.overlay')
local widgets = require('gui.widgets')

--
-- OrdersOverlay
--

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
    viewscreens='dwarfmode/Info/WORK_ORDERS',
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

OVERLAY_WIDGETS = {
    overlay=OrdersOverlay,
}

return _ENV
