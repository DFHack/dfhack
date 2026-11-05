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
