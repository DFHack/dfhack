-- Resets the selected work order to the `Checking` state

--@ module = true

local widgets = require('gui.widgets')
local overlay = require('plugins.overlay')

local function set_current_inactive()
    local scrConditions = df.global.game.main_interface.info.work_orders.conditions
    if scrConditions.open then
        local order = scrConditions.wq
        order.status.active = false
    else
        qerror("Order conditions is not open")
    end
end

local function is_current_active()
    local scrConditions = df.global.game.main_interface.info.work_orders.conditions
    local order = scrConditions.wq
    return order.status.active
end

-- -------------------
-- RecheckOverlay
--

local focusString = 'dwarfmode/Info/WORK_ORDERS/Conditions'

RecheckOverlay = defclass(RecheckOverlay, overlay.OverlayWidget)
RecheckOverlay.ATTRS{
    default_pos={x=6,y=8},
    default_enabled=true,
    viewscreens=focusString,
    -- width is the sum of lengths of `[` + `Ctrl+A` + `: ` + button.label + `]`
    frame={w=1 + 6 + 2 + 16 + 1, h=3},
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
            label='request re-check',
            key='CUSTOM_CTRL_A',
            on_activate=set_current_inactive,
            enabled=is_current_active,
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
}

if dfhack_flags.module then
    return
end

-- Check if on the correct screen and perform the action if so
if not dfhack.gui.matchFocusString(focusString) then
    qerror('workorder-recheck must be run from the manager order conditions view')
end

set_current_inactive()
