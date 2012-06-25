-- Makes the game immediately save the state.

if not dfhack.isMapLoaded() then
    qerror("World and map aren't loaded.")
end

local ui_main = df.global.ui.main
local flags4 = df.global.d_init.flags4

local function restore_autobackup()
    if ui_main.autosave_request and dfhack.isMapLoaded() then
        dfhack.timeout(10, 'frames', restore_autobackup)
    else
        flags4.AUTOBACKUP = true
    end
end

-- Request auto-save
ui_main.autosave_request = true

-- And since it will overwrite the backup, disable it temporarily
if flags4.AUTOBACKUP then
    flags4.AUTOBACKUP = false
    restore_autobackup()
end

print 'The game should save the state now.'
