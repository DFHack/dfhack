--@ module = true

local gui = require('gui')
local overlay = require('plugins.overlay')
local widgets = require('gui.widgets')

TradeAgreementOverlay = defclass(TradeAgreementOverlay, overlay.OverlayWidget)
TradeAgreementOverlay.ATTRS{
    default_pos={x=45, y=-6},
    default_enabled=true,
    viewscreens='dwarfmode/Diplomacy/Requests',
    frame={w=25, h=3},
    frame_style=gui.MEDIUM_FRAME,
    frame_background=gui.CLEAR_PEN,
}

local diplomacy = df.global.game.main_interface.diplomacy
local function diplomacy_toggle_cat()
    local priority_idx = diplomacy.taking_requests_tablist[diplomacy.taking_requests_selected_tab]
    local priority = diplomacy.environment.meeting.sell_requests.priority[priority_idx]
    if #priority == 0 then return end
    local target_val = priority[0] == 0 and 4 or 0
    for i in ipairs(priority) do
        priority[i] = target_val
    end
end

function TradeAgreementOverlay:init()
    self:addviews{
        widgets.HotkeyLabel{
            frame={t=0, l=0},
            label='Select all/none',
            key='CUSTOM_CTRL_A',
            on_activate=diplomacy_toggle_cat,
        },
    }
end
