local _ENV = mkmodule('plugins.suspendmanager')

local utils = require('utils')
local gui = require('gui')
local guidm = require('gui.dwarfmode')
local overlay = require('plugins.overlay')
local widgets = require('gui.widgets')

local argparse = require('argparse')

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

function isBuildingPlanJob(job)
    return suspendmanager_isBuildingPlanJob(job)
end

function runOnce(prevent_blocking, quiet)
    suspendmanager_runOnce(prevent_blocking)
    if (not quiet) then
        print(suspendmanager_getStatus())
    end
end

function unsuspend_command(...)
    local quiet, skipblocking = false, false
    argparse.processArgsGetopt({ ... }, {
        { 'q', 'quiet',        handler = function() quiet = true end },
        { 's', 'skipblocking', handler = function() skipblocking = true end },
    })

    runOnce(not skipblocking, quiet)
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

-- suspend overlay (formerly in unsuspend.lua)

local textures = dfhack.textures.loadTileset('hack/data/art/unsuspend.png', 32, 32, true)

local ok, buildingplan = pcall(require, 'plugins.buildingplan')
if not ok then
    buildingplan = nil
end

SuspendOverlay = defclass(SuspendOverlay, overlay.OverlayWidget)
SuspendOverlay.ATTRS{
    desc='Annotates suspended buildings with a visible marker.',
    viewscreens='dwarfmode',
    default_enabled=true,
    overlay_only=true,
    overlay_onupdate_max_freq_seconds=30,
}

function SuspendOverlay:init()
    self:reset()
    -- there can only be one of these widgets allocated at a time, so this is
    -- safe
    dfhack.onStateChange.unsuspend = function(code)
        if code ~= SC_MAP_LOADED then return end
        self:reset()
    end
end

function SuspendOverlay:reset()
    -- value of df.global.building_next_id on last scan
    self.prev_building_next_id = 0

    -- increments on every refresh so we can clear old data from the map
    self.data_version = 0

    -- map of building id -> {suspended=bool, suspend_count=int, version=int}
    -- suspended is the job suspension state as of the last refresh
    -- if suspend_count > 1 then this is a repeat offender (red 'X')
    -- only buildings whose construction is in progress should be in this map.
    self.in_progress_buildings = {}

    -- viewport for cached screen_buildings
    self.viewport = {}

    -- map of building ids to current visible screen position
    self.screen_buildings = {}
end

function SuspendOverlay:overlay_onupdate()
    local added = false
    self.data_version = self.data_version + 1
    foreach_construction_job(function(job)
        self:update_building(dfhack.job.getHolder(job).id, job)
        added = true
    end)
    self.prev_building_next_id = df.global.building_next_id
    if added then
        -- invalidate screen_buildings cache
        self.viewport = {}
    end
    -- clear out old data
    for bld_id,data in pairs(self.in_progress_buildings) do
        if data.version ~= self.data_version then
            self.in_progress_buildings[bld_id] = nil
        end
    end
end

function SuspendOverlay:update_building(bld_id, job)
    local suspended = job.flags.suspend
    local data = self.in_progress_buildings[bld_id]
    if not data then
        self.in_progress_buildings[bld_id] = {suspended=suspended,
                suspend_count=suspended and 1 or 0, version=self.data_version}
    else
        if suspended and suspended ~= data.suspended then
            data.suspend_count = data.suspend_count + 1
        end
        data.suspended = suspended
        data.version = self.data_version
    end
end

function SuspendOverlay:process_new_buildings()
    local added = false
    for bld_id=self.prev_building_next_id,df.global.building_next_id-1 do
        local bld = df.building.find(bld_id)
        if not bld or #bld.jobs ~= 1 then
            goto continue
        end
        local job = bld.jobs[0]
        if job.job_type ~= df.job_type.ConstructBuilding then
            goto continue
        end
        self:update_building(bld_id, job)
        added = true
        ::continue::
    end
    self.prev_building_next_id = df.global.building_next_id
    if added then
        -- invalidate screen_buildings cache
        self.viewport = {}
    end
end

-- returns true if viewport has changed
function SuspendOverlay:update_viewport(viewport)
    local pviewport = self.viewport
    if viewport.z == pviewport.z
            and viewport.x1 == pviewport.x1 and viewport.x2 == pviewport.x2
            and viewport.y1 == pviewport.y1 and viewport.y2 == pviewport.y2 then
        return false
    end
    self.viewport = viewport
    return true
end

function SuspendOverlay:refresh_screen_buildings()
    local viewport = guidm.Viewport.get()
    if not self:update_viewport(viewport) then return end
    local screen_buildings, z = {}, viewport.z
    for bld_id,data in pairs(self.in_progress_buildings) do
        local bld = df.building.find(bld_id)
        if bld then
            local pos = {x=bld.centerx, y=bld.centery, z=bld.z}
            if viewport:isVisible(pos) then
                screen_buildings[bld_id] = viewport:tileToScreen(pos)
            end
        end
    end
    self.screen_buildings = screen_buildings
end

local tp = function(offset)
    return dfhack.textures.getTexposByHandle(textures[offset])
end

function SuspendOverlay:render_marker(dc, bld, screen_pos)
    if not bld or #bld.jobs ~= 1 then return end
    local data = self.in_progress_buildings[bld.id]
    if not data then return end
    local job = bld.jobs[0]
    if job.job_type ~= df.job_type.ConstructBuilding
            or not job.flags.suspend then
        return
    end
    local color, ch, texpos = COLOR_YELLOW, 'x', tp(2)
    if buildingplan and buildingplan.isPlannedBuilding(bld) then
        color, ch, texpos = COLOR_GREEN, 'P', tp(4)
    elseif isKeptSuspended(job) then
        color, ch, texpos = COLOR_WHITE, 'x', tp(3)
    elseif data.suspend_count > 1 then
        color, ch, texpos = COLOR_RED, 'X', tp(1)
    end
    dc:seek(screen_pos.x, screen_pos.y):tile(ch, texpos, color)
end

function SuspendOverlay:onRenderFrame(dc)
    if not df.global.pause_state and not dfhack.screen.inGraphicsMode() then
        return
    end

    self:process_new_buildings()
    self:refresh_screen_buildings()

    dc:map(true)
    for bld_id,screen_pos in pairs(self.screen_buildings) do
        self:render_marker(dc, df.building.find(bld_id), screen_pos)
    end
    dc:map(false)
end

-- register widgets
OVERLAY_WIDGETS = {
    status=StatusOverlay,
    toggle=ToggleOverlay,
    overlay=SuspendOverlay
}


return _ENV
