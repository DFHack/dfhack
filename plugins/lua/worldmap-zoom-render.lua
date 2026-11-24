-- Native world-map composition service. See docs/plugins/worldmap-zoom-render.rst.
local _ENV = mkmodule('plugins.worldmap-zoom-render')

-- Compatibility with the external World Map Zoom client API. New clients
-- should use the camelCase names documented in DFHack.
set_camera = setCamera
snapshot_ui = snapshotUi
ui_hit = uiHit
test_window = testWindow
frame_profile = frameProfile
test_focus = testFocus

return _ENV
