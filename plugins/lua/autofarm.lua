local _ENV = mkmodule('plugins.autofarm')

local gui = require('gui')
local overlay = require('plugins.overlay')
local widgets = require('gui.widgets')

local function get_farm()
    local bld = dfhack.gui.getSelectedBuilding(true)
    if bld and bld:getType() == df.building_type.FarmPlot then
        return bld
    end
end

local function farm_is_managed(farm)
    return farm and autofarm_isFarmExcluded and
        not autofarm_isFarmExcluded(farm.id)
end

--------------------------------
-- FarmControlsOverlay
--
-- toggle strip shown on the farm plot info sheet: enables/disables autofarm
-- globally, marks the inspected farm as managed or manually controlled, and
-- provides a link to the autofarm configuration GUI

FarmControlsOverlay = defclass(FarmControlsOverlay, overlay.OverlayWidget)
FarmControlsOverlay.ATTRS{
    desc='Adds autofarm controls to the farm plot info sheet.',
    default_pos={x=-43, y=13},
    version=3,
    default_enabled=true,
    viewscreens='dwarfmode/ViewSheets/BUILDING/FarmPlot',
    frame={w=50, h=2},
    frame_background=gui.CLEAR_PEN,
}

function FarmControlsOverlay:init()
    self:addviews{
        widgets.ToggleHotkeyLabel{
            view_id='autofarm_enabled',
            frame={t=0, l=0},
            label='autofarm is',
            key='CUSTOM_CTRL_A',
            options={{value=true, label='enabled', pen=COLOR_GREEN},
                     {value=false, label='disabled', pen=COLOR_RED}},
            on_change=function(val) setEnabled(val) end,
        },
        widgets.ToggleHotkeyLabel{
            view_id='farm_managed',
            frame={t=1, l=0},
            label='this farm is',
            key='CUSTOM_CTRL_M',
            enabled=function() return isEnabled() end,
            options={{value=true, label='managed by autofarm', pen=COLOR_GREEN},
                     {value=false, label='managed manually', pen=COLOR_YELLOW}},
            on_change=function(val)
                local farm = get_farm()
                if farm then autofarm_setFarmExcluded(farm.id, not val) end
            end,
        },
        widgets.TextButton{
            frame={t=0, r=0, w=18, h=1},
            label='configure',
            key='CUSTOM_CTRL_C',
            on_activate=function() dfhack.run_script('gui/autofarm') end,
        },
    }
end

function FarmControlsOverlay:render(dc)
    self.subviews.autofarm_enabled:setOption(isEnabled())
    self.subviews.farm_managed:setOption(farm_is_managed(get_farm()))
    FarmControlsOverlay.super.render(self, dc)
end

--------------------------------
-- FarmCoverOverlay
--
-- opaque panel shown over the crop list of farms managed by autofarm. warns
-- the player that manual crop selection is overridden by autofarm and
-- provides links to the autofarm configuration GUI and to per-farm control

local function get_crop_name(plant_id)
    if not plant_id or plant_id < 0 then return 'fallow' end
    local raw = df.plant_raw.find(plant_id)
    return raw and raw.name or 'unknown'
end

FarmCoverOverlay = defclass(FarmCoverOverlay, overlay.OverlayWidget)
FarmCoverOverlay.ATTRS{
    desc='Covers the crop list of farm plots managed by autofarm.',
    default_pos={x=-40, y=16},
    version=4,
    default_enabled=true,
    viewscreens='dwarfmode/ViewSheets/BUILDING/FarmPlot',
    frame={w=57, h=22},
    frame_style=gui.FRAME_MEDIUM,
    frame_background=gui.CLEAR_PEN,
    visible=function()
        local farm = get_farm()
        return isEnabled() and farm_is_managed(farm) and
            farm:getBuildStage() == farm:getMaxBuildStage()
    end,
}

function FarmCoverOverlay:init()
    self:addviews{
        widgets.Label{
            frame={t=1, l=2},
            text='This farm is managed by autofarm',
            text_pen=COLOR_LIGHTGREEN,
        },
        widgets.Label{
            view_id='now_planting',
            frame={t=2, l=2},
            text='Now planting: ?',
        },
        widgets.WrappedLabel{
            frame={t=3, l=2, r=2},
            text_to_wrap='Crops are chosen automatically based on your'..
                ' configured stock thresholds. Manual crop selection'..
                ' will be overridden.',
        },
        widgets.WrappedLabel{
            view_id='fallow_note',
            frame={t=6, l=2, r=2},
            text_pen=COLOR_YELLOW,
        },
        widgets.TextButton{
            frame={b=4, l=2, r=2, h=1},
            label='configure autofarm',
            key='CUSTOM_CTRL_C',
            on_activate=function() dfhack.run_script('gui/autofarm') end,
        },
        widgets.TextButton{
            frame={b=3, l=2, r=2, h=1},
            label='manage this farm manually',
            key='CUSTOM_CTRL_M',
            on_activate=function()
                local farm = get_farm()
                if farm then autofarm_setFarmExcluded(farm.id, true) end
            end,
        },
    }
end

function FarmCoverOverlay:render(dc)
    local farm = get_farm()
    if farm then
        self.subviews.now_planting:setText{
            'Now planting: ',
            {text=get_crop_name(farm.plant_id[df.global.cur_season]),
             pen=COLOR_LIGHTCYAN},
        }
        local all_fallow = true
        for i=0,3 do
            if farm.plant_id[i] >= 0 then
                all_fallow = false
                break
            end
        end
        self.subviews.fallow_note:setText(all_fallow and
            'All seasons are fallow: no plantable seeds available, or all'..
            ' crops are above their thresholds.' or '')
    end
    FarmCoverOverlay.super.render(self, dc)
end

-- swallow mouse input over the covered area so that the underlying crop
-- list cannot be manipulated while autofarm is in control
function FarmCoverOverlay:onInput(keys)
    if FarmCoverOverlay.super.onInput(self, keys) then return true end
    if (keys._MOUSE_L or keys._MOUSE_R) and self:getMousePos() then
        return true
    end
    return false
end

OVERLAY_WIDGETS = {
    farm_controls=FarmControlsOverlay,
    farm_cover=FarmCoverOverlay,
}

return _ENV
