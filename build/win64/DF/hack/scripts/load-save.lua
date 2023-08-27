-- load a save non-interactively - intended to be run on startup

local gui = require 'gui'

local folder_name = ({...})[1] or qerror("No folder name given")

local loadgame_screen = dfhack.gui.getViewscreenByType(df.viewscreen_loadgamest, 0)
if not loadgame_screen then
    local title_screen = dfhack.gui.getViewscreenByType(df.viewscreen_titlest, 0)
    if not title_screen then
        qerror("Can't find title or load game screen")
    end
    local found = false
    for idx, item in ipairs(title_screen.menu_line_id) do
        if item == df.viewscreen_titlest.T_menu_line_id.Continue then
            found = true
            title_screen.sel_menu_line = idx
            break
        end
    end
    if not found then
        qerror("Can't find 'Continue Playing' option")
    end
    gui.simulateInput(title_screen, 'SELECT')
end

loadgame_screen = dfhack.gui.getViewscreenByType(df.viewscreen_loadgamest, 0) or
    qerror("Can't find load game screen")

local found = false
for idx, save in ipairs(loadgame_screen.saves) do
    if save.folder_name == folder_name then
        found = true
        loadgame_screen.sel_idx = idx
        break
    end
end
if not found then
    qerror("Can't find save: " .. folder_name)
end

--[[
If gui/load-screen is active, it will be inserted later this frame and prevent
the viewscreen_loadgamest from working. To work around this, SELECT is fed to
the screen one frame later.
]]
function triggerLoad(loadgame_screen)
    -- Get rid of child screens (e.g. gui/load-screen)
    if loadgame_screen.child then
        local child = loadgame_screen.child
        while child.child do
            child = child.child
        end
        while child ~= loadgame_screen do
            child = child.parent
            dfhack.screen.dismiss(child.child)
        end
    end

    gui.simulateInput(loadgame_screen, 'SELECT')
end

dfhack.timeout(1, 'frames', function() triggerLoad(loadgame_screen) end)
