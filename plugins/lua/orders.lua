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

local function get_import_choices()
    return dfhack.run_command_silent('orders', 'list'):split('\n')
end

local function do_import()
    local dlg
    local function get_dlg() return dlg end
    dlg = dialogs.ListBox{
        frame_title='Import/Delete Manager Orders',
        with_filter=true,
        choices=get_import_choices(),
        on_select=function(_, choice)
            dfhack.run_command('orders', 'import', choice.text)
        end,
        dismiss_on_select2=false,
        on_select2=function(_, choice)
            if choice.text:startswith('library/') then return end
            local fname = 'dfhack-config/orders/'..choice.text..'.json'
            if not dfhack.filesystem.isfile(fname) then return end
            dialogs.showYesNoPrompt('Delete orders file?',
                'Are you sure you want to delete "' .. fname .. '"?', nil,
                function()
                    print('deleting ' .. fname)
                    os.remove(fname)
                    local list = get_dlg().subviews.list
                    local filter = list:getFilter()
                    list:setChoices(get_import_choices(), list:getSelected())
                    list:setFilter(filter)
                end)
        end,
        select2_hint='Delete file',
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

local function do_recheck()
    dfhack.run_command('orders', 'recheck')
end

OrdersOverlay = defclass(OrdersOverlay, overlay.OverlayWidget)
OrdersOverlay.ATTRS{
    desc='Adds import, export, and other functions to the manager orders screen.',
    default_pos={x=53,y=-6},
    default_enabled=true,
    viewscreens='dwarfmode/Info/WORK_ORDERS/Default',
    frame={w=43, h=4},
}

function OrdersOverlay:init()
    self.minimized = false

    local main_panel = widgets.Panel{
        frame={t=0, l=0, r=0, h=4},
        frame_style=gui.MEDIUM_FRAME,
        frame_background=gui.CLEAR_PEN,
        visible=function() return not self.minimized end,
        subviews={
            widgets.HotkeyLabel{
                frame={t=0, l=0},
                label='import',
                key='CUSTOM_CTRL_I',
                auto_width=true,
                on_activate=do_import,
            },
            widgets.HotkeyLabel{
                frame={t=1, l=0},
                label='export',
                key='CUSTOM_CTRL_E',
                auto_width=true,
                on_activate=do_export,
            },
            widgets.HotkeyLabel{
                frame={t=0, l=15},
                label='recheck conditions',
                key='CUSTOM_CTRL_K',
                auto_width=true,
                on_activate=do_recheck,
            },
            widgets.HotkeyLabel{
                frame={t=1, l=15},
                label='sort',
                key='CUSTOM_CTRL_O',
                auto_width=true,
                on_activate=do_sort,
            },
            widgets.HotkeyLabel{
                frame={t=1, l=28},
                label='clear',
                key='CUSTOM_CTRL_C',
                auto_width=true,
                on_activate=do_clear,
            },
        },
    }

    local minimized_panel = widgets.Panel{
        frame={t=0, r=4, w=3, h=1},
        subviews={
            widgets.Label{
                frame={t=0, l=0, w=1, h=1},
                text='[',
                text_pen=COLOR_RED,
                visible=function() return self.minimized end,
            },
            widgets.Label{
                frame={t=0, l=1, w=1, h=1},
                text={{text=function() return self.minimized and string.char(31) or string.char(30) end}},
                text_pen=dfhack.pen.parse{fg=COLOR_BLACK, bg=COLOR_GREY},
                text_hpen=dfhack.pen.parse{fg=COLOR_BLACK, bg=COLOR_WHITE},
                on_click=function() self.minimized = not self.minimized end,
            },
            widgets.Label{
                frame={t=0, r=0, w=1, h=1},
                text=']',
                text_pen=COLOR_RED,
                visible=function() return self.minimized end,
            },
        },
    }

    self:addviews{
        main_panel,
        minimized_panel,
        widgets.HelpButton{
            frame={t=0, r=1},
            command='orders',
            visible=function() return not self.minimized end,
        },
    }
end

function OrdersOverlay:onInput(keys)
    if df.global.game.main_interface.job_details.open then return end
    if keys.CUSTOM_ALT_M then
        self.minimized = not self.minimized
        return true
    end
    if OrdersOverlay.super.onInput(self, keys) then
        return true
    end
end

function OrdersOverlay:render(dc)
    if df.global.game.main_interface.job_details.open then return end
    OrdersOverlay.super.render(self, dc)
end

-- Resets the selected work order to the `Checking` state

local function set_current_inactive()
    local scrConditions = df.global.game.main_interface.info.work_orders.conditions
    if scrConditions.open then
        dfhack.run_command('orders', 'recheck', 'this')
    else
        qerror("Order conditions is not open")
    end
end

local function can_recheck()
    local scrConditions = df.global.game.main_interface.info.work_orders.conditions
    local order = scrConditions.wq
    return order.status.active and #order.item_conditions > 0
end

-- -------------------
-- RecheckOverlay
--

local focusString = 'dwarfmode/Info/WORK_ORDERS/Conditions'

RecheckOverlay = defclass(RecheckOverlay, overlay.OverlayWidget)
RecheckOverlay.ATTRS{
    desc='Adds a button to the work order details page to tell the manager to recheck conditions.',
    default_pos={x=6,y=8},
    default_enabled=true,
    viewscreens=focusString,
    -- width is the sum of lengths of `[` + `Ctrl+A` + `: ` + button.label + `]`
    frame={w=1 + 6 + 2 + 19 + 1, h=3},
}

local function areTabsInTwoRows()
    -- get the tile above the order status icon
    local pen = dfhack.screen.readTile(7, 7, false)
    -- in graphics mode, `0` when one row, something else when two (`67` aka 'C' from "Creatures")
    -- in ASCII mode, `32` aka ' ' when one row, something else when two (`196` aka '-' from tab frame's top)
    return (pen.ch ~= 0 and pen.ch ~= 32)
end

function RecheckOverlay:updateTextButtonFrame()
    local twoRows = areTabsInTwoRows()
    if (self._twoRows == twoRows) then return false end

    self._twoRows = twoRows
    local frame = twoRows
            and {b=0, l=0, r=0, h=1}
            or  {t=0, l=0, r=0, h=1}
    self.subviews.button.frame = frame

    return true
end

function RecheckOverlay:init()
    self:addviews{
        widgets.TextButton{
            view_id = 'button',
            -- frame={t=0, l=0, r=0, h=1}, -- is set in `updateTextButtonFrame()`
            label='re-check conditions',
            key='CUSTOM_CTRL_A',
            on_activate=set_current_inactive,
            enabled=can_recheck,
        },
    }

    self:updateTextButtonFrame()
end

function RecheckOverlay:onRenderBody(dc)
    if (self.frame_rect.y1 == 7) then
        -- only apply this logic if the overlay is on the same row as
        -- originally thought: just above the order status icon

        if self:updateTextButtonFrame() then
            self:updateLayout()
        end
    end

    RecheckOverlay.super.onRenderBody(self, dc)
end

-- -------------------

OVERLAY_WIDGETS = {
    recheck=RecheckOverlay,
    overlay=OrdersOverlay,
}

return _ENV
