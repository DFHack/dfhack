local _ENV = mkmodule('plugins.buildingplan.mechanisms')

local itemselection = require('plugins.buildingplan.itemselection')
local overlay = require('plugins.overlay')
local widgets = require('gui.widgets')

local view_sheets = df.global.game.main_interface.view_sheets

--------------------------------
-- MechanismOverlay
--

MechanismOverlay = defclass(MechanismOverlay, overlay.OverlayWidget)
MechanismOverlay.ATTRS{
    default_pos={x=5,y=5},
    default_enabled=true,
    viewscreens='dwarfmode/LinkingLever',
    frame={w=57, h=13},
}

function MechanismOverlay:init()
    self:addviews{
        widgets.BannerPanel{
            frame={t=5, l=1, r=1, h=1},
            subviews={
                widgets.Label{
                    frame={t=0, l=1},
                    text='Mechanism safety:'
                },
                widgets.CycleHotkeyLabel{
                    view_id='safety_lever',
                    frame={t=0, l=20, w=15},
                    key='CUSTOM_G',
                    label='Lever:',
                    options={
                        {label='Any', value=0},
                        {label='Magma', value=2, pen=COLOR_RED},
                        {label='Fire', value=1, pen=COLOR_LIGHTRED},
                    },
                    initial_option=0,
                    on_change=self:callback('choose_mechanism', 'lever', true),
                },
                widgets.CycleHotkeyLabel{
                    view_id='safety_target',
                    frame={t=0, l=38, w=16},
                    key='CUSTOM_SHIFT_G',
                    label='Target:',
                    options={
                        {label='Any', value=0},
                        {label='Magma', value=2, pen=COLOR_RED},
                        {label='Fire', value=1, pen=COLOR_LIGHTRED},
                    },
                    initial_option=0,
                    on_change=self:callback('choose_mechanism', 'target', true),
                },
            }
        },
        widgets.HotkeyLabel{
            frame={t=7, l=8, w=49, h=2},
            key='CUSTOM_M',
            label=function()
                return itemselection.get_item_description(view_sheets.linking_lever_mech_lever_id,
                    nil,
                    self.subviews.safety_lever:getOptionLabel())
            end,
            auto_height=false,
            enabled=function() return view_sheets.linking_lever_mech_lever_id ~= -1 end,
            on_activate=self:callback('choose_mechanism', 'lever', false),
        },
        widgets.HotkeyLabel{
            frame={t=10, l=8, w=49, h=2},
            key='CUSTOM_SHIFT_M',
            label=function()
                return itemselection.get_item_description(view_sheets.linking_lever_mech_target_id,
                    nil,
                    self.subviews.safety_target:getOptionLabel())
            end,
            auto_height=false,
            enabled=function() return view_sheets.linking_lever_mech_target_id ~= -1 end,
            on_activate=self:callback('choose_mechanism', 'target', false),
        },
    }
end

local item_selection_dlg
local function reset_dlg()
    if item_selection_dlg then
        if item_selection_dlg:isActive() then
            item_selection_dlg:dismiss()
        end
        item_selection_dlg = nil
    end
end

local function get_available_items(safety, other_mechanism)
    local item_ids = require('plugins.buildingplan').getAvailableItemsByHeat(
        df.building_type.Trap, df.trap_type.Lever, -1, 0, safety)
    for idx,item_id in ipairs(item_ids) do
        if item_id == other_mechanism then
            table.remove(item_ids, idx)
            break
        end
    end
    return item_ids
end

function MechanismOverlay:save_id(which, item_id)
    local saved_id = ('saved_%s_id'):format(which)
    local ui_id = ('linking_lever_mech_%s_id'):format(which)
    view_sheets[ui_id] = item_id
    self[saved_id] = item_id
end

function MechanismOverlay:choose_mechanism(which, autoselect)
    local widget_id = 'safety_' .. which
    local safety = self.subviews[widget_id]:getOptionValue()
    local ui_other_id = ('linking_lever_mech_%s_id'):format(which == 'lever' and 'target' or 'lever')
    local available_item_ids = get_available_items(safety, view_sheets[ui_other_id])

    if autoselect then
        self:save_id(which, available_item_ids[1] or -1)
        return
    end

    -- to integrate with ItemSelection's last used sorting
    df.global.buildreq.building_type = df.building_type.Trap

    local desc = self.subviews[widget_id]:getOptionLabel()
    if desc ~= 'Any' then
        desc = desc .. ' safe'
    end
    desc = desc .. ' mechanism'

    item_selection_dlg = item_selection_dlg or itemselection.ItemSelectionScreen{
        get_available_items_fn=function() return available_item_ids end,
        desc=desc,
        quantity=1,
        autoselect=false,
        on_cancel=reset_dlg,
        on_submit=function(chosen_ids)
            self:save_id(which, chosen_ids[1] or available_item_ids[1] -1)
            reset_dlg()
        end,
    }:show()
end

function MechanismOverlay:onInput(keys)
    if MechanismOverlay.super.onInput(self, keys) then
        return true
    end
    if keys._MOUSE_L then
        if self:getMousePos() then
            -- don't let clicks bleed through the panel
            return true
        end
        -- don't allow the lever to be linked if mechanisms are not set
        return view_sheets.linking_lever_mech_lever_id == -1 or
            view_sheets.linking_lever_mech_target_id == -1
    end
end

function MechanismOverlay:onRenderFrame(dc, rect)
    MechanismOverlay.super.onRenderFrame(self, dc, rect)
    if self.saved_lever_id ~= view_sheets.linking_lever_mech_lever_id then
        self:choose_mechanism('lever', true)
    end
    if self.saved_target_id ~= view_sheets.linking_lever_mech_target_id then
        self:choose_mechanism('target', true)
    end
end

return _ENV
