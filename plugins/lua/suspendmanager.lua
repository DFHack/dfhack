local _ENV = mkmodule('plugins.suspendmanager')

local utils = require('utils')
local gui = require('gui')
local overlay = require('plugins.overlay')
local widgets = require('gui.widgets')

--- Loop over all the construction jobs
---@param fn function A function taking a job as argument
function foreach_construction_job(fn)
    for _, job in utils.listpairs(df.global.world.jobs.list) do
        if job.job_type == df.job_type.ConstructBuilding then
            fn(job)
        end
    end
end

function isKeptSuspended(job)
    return suspendmanager_isKeptSuspended(job)
end

-- TODO: check that this is appropriate
function isBuildingPlanJob(job)
    --- @type building
    local building = dfhack.job.getHolder(job)
    return building and building.mat_type == -1
end

function runOnce(prevent_blocking, quiet)
    suspendmanager_runOnce(prevent_blocking)
    local stats = suspendmanager_getStatus()
    if (not quiet) then
        print(stats)
    end
end

-- Overlay Widgets
StatusOverlay = defclass(StatusOverlay, overlay.OverlayWidget)
StatusOverlay.ATTRS{
    desc='Adds information to suspended building panels about why it is suspended.',
    default_pos={x=-39,y=16},
    default_enabled=true,
    viewscreens='dwarfmode/ViewSheets/BUILDING',
    frame={w=59, h=3},
    frame_style=gui.MEDIUM_FRAME,
    frame_background=gui.CLEAR_PEN,
    default_enabled=true,
}

function StatusOverlay:init()
    self:addviews{
        widgets.Label{
            frame={t=0, l=0},
            text={
                {text=self:callback('get_status_string')}
            }
        },
    }
end

function StatusOverlay:get_status_string()
    local job = dfhack.gui.getSelectedJob(true)
    if job and job.flags.suspend then
        return "Suspended because: " .. suspendmanager_suspensionDescription(job) .. "."
    end
    return "Not suspended."
end

function StatusOverlay:render(dc)
    local job = dfhack.gui.getSelectedJob(true)
    if not job or job.job_type ~= df.job_type.ConstructBuilding or not isEnabled() or isBuildingPlanJob(job) then
        return
    end
    StatusOverlay.super.render(self, dc)
end

ToggleOverlay = defclass(ToggleOverlay, overlay.OverlayWidget)
ToggleOverlay.ATTRS{
    desc='Adds a link to suspended building panels for enabling suspendmanager.',
    default_pos={x=-57,y=23},
    default_enabled=true,
    viewscreens='dwarfmode/ViewSheets/BUILDING',
    frame={w=40, h=1},
    frame_background=gui.CLEAR_PEN,
    default_enabled=true,
}

function ToggleOverlay:init()
    self:addviews{
        widgets.ToggleHotkeyLabel{
            view_id="enable_toggle",
            frame={t=0, l=0, w=34},
            label="Suspendmanager is",
            key="CUSTOM_CTRL_M",
            options={{value=true, label="Enabled"},
                     {value=false, label="Disabled"}},
            initial_option = isEnabled(),
            on_change=function(val) dfhack.run_command{val and "enable" or "disable", "suspendmanager"} end
        },
    }
end

function ToggleOverlay:shouldRender()
    local job = dfhack.gui.getSelectedJob(true)
    return job and job.job_type == df.job_type.ConstructBuilding and not isBuildingPlanJob(job)
end

function ToggleOverlay:render(dc)
    if not self:shouldRender() then
        return
    end
    -- Update the option: the "initial_option" value is not up to date since the widget
    -- is not reinitialized for overlays
    self.subviews.enable_toggle:setOption(isEnabled(), false)
    ToggleOverlay.super.render(self, dc)
end

function ToggleOverlay:onInput(keys)
    if not self:shouldRender() then
        return
    end
    ToggleOverlay.super.onInput(self, keys)
end

OVERLAY_WIDGETS = {
    status=StatusOverlay,
    toggle=ToggleOverlay,
}


return _ENV
