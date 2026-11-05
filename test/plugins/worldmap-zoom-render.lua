config.mode = 'title'
config.target = 'worldmap-zoom-render'

-- The native renderer is currently built only on Windows. Do not require an
-- unavailable native module while the test runner is discovering Linux tests.
if dfhack.getOSType() ~= 'windows' then return end

local renderer = require('plugins.worldmap-zoom-render')

function test.rejects_invalid_camera_parameters()
    for _, zoom in ipairs{0, 3, 8, math.huge, -math.huge, 0/0} do
        expect.error_match('supported context', function()
            renderer.setCamera(false, 0, zoom, 0, 0)
        end)
    end
    for _, context in ipairs{-1, 3, 2^32 + 1} do
        expect.error_match('supported context', function()
            renderer.setCamera(false, context, 2, 0, 0)
        end)
    end
    for _, coord in ipairs{math.huge, -math.huge, 0/0} do
        expect.error_match('finite coordinates', function()
            renderer.setCamera(false, 0, 2, coord, 0)
        end)
        expect.error_match('finite coordinates', function()
            renderer.setCamera(false, 0, 2, 0, coord)
        end)
    end
end

function test.disabled_camera_does_not_install_hooks()
    local before = renderer.status()
    for _, zoom in ipairs{1, 2, 4} do
        renderer.setCamera(false, 0, zoom, 0, 0)
    end
    local after = renderer.status()
    expect.eq(before.ready, after.ready)
    expect.eq(before.canvas_creations, after.canvas_creations)
    expect.eq(before.transformed_calls, after.transformed_calls)
end

function test.camera_requires_supported_screen_and_preparation()
    expect.error_match('not ready', function()
        renderer.setCamera(true, 1, 2, 0, 0)
    end)
    expect.error_match('not ready', function()
        renderer.setCamera(true, 2, 4, 0, 0)
    end)
end

function test.external_client_aliases()
    expect.eq(renderer.setCamera, renderer.set_camera)
    expect.eq(renderer.snapshotUi, renderer.snapshot_ui)
    expect.eq(renderer.uiHit, renderer.ui_hit)
end

function test.stop_without_prepare_requires_cleanup_before_unload()
    -- Loading a plugin is not the only way it can own a queued callback:
    -- stop() itself queues cleanup, even when no renderer hooks were prepared.
    local name = 'worldmap-zoom-render'
    local _, result = dfhack.run_command_silent('unload', name)
    expect.eq(0, result)
    _, result = dfhack.run_command_silent('load', name)
    expect.eq(0, result)
    renderer.stop()
    _, result = dfhack.run_command_silent('unload', name)
    if result ~= 0 then
        -- A suspended test cannot wait synchronously for the render thread.
        delay_until(function() return renderer.status().stopped end, 120)
        expect.true_(renderer.status().stopped)
        _, result = dfhack.run_command_silent('unload', name)
    end
    expect.eq(0, result)
    expect.error_match('has been unloaded', function() renderer.status() end)
    -- Allow core to drain queue wrappers after the DLL is gone. They must not
    -- retain the unprepared stop callback or its plugin-owned function manager.
    delay(2)
    _, result = dfhack.run_command_silent('load', name)
    expect.eq(0, result)
    expect.false_(renderer.status().ready)
end
