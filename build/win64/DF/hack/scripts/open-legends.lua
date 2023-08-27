-- open legends screen when in fortress mode
--@ module = true

local dialogs = require('gui.dialogs')
local gui = require('gui')
local utils = require('utils')

Restorer = defclass(Restorer, gui.Screen)
Restorer.ATTRS{
    focus_path='open-legends'
}

function Restorer:init()
    print('initializing restorer')
    self.region_details_backup = {} --as:df.world_region_details[]
    local v = df.global.world.world_data.region_details
    while (#v > 0) do
        table.insert(self.region_details_backup, 1, v[0])
        v:erase(0)
    end
end

function Restorer:onIdle()
    self:dismiss()
end

function Restorer:onDismiss()
    print('dismissing restorer')
    local v = df.global.world.world_data.region_details
    while (#v > 0) do v:erase(0) end
    for _,item in pairs(self.region_details_backup) do
        v:insert(0, item)
    end
end

function show_screen()
    local ok, err = pcall(function()
        Restorer{}:show()
        dfhack.screen.show(df.viewscreen_legendsst:new())
    end)
    if not ok then
        qerror('Failed to set up legends screen: ' .. tostring(err))
    end
end

function main(force)
    if not dfhack.isWorldLoaded() then
        qerror('no world loaded')
    end

    local view = df.global.gview.view
    while view do
        if df.viewscreen_legendsst:is_instance(view) then
            qerror('legends screen already displayed')
        end
        view = view.child
    end

    if not dfhack.world.isFortressMode(df.global.gametype) and not dfhack.world.isAdventureMode(df.global.gametype) and not force then
        qerror('mode not tested: ' .. df.game_type[df.global.gametype] .. ' (use "force" to force)')
    end

    if force then
        show_screen()
    else
        dialogs.showYesNoPrompt('Save corruption possible',
            'This script can CORRUPT YOUR SAVE. If you care about this world,\n' ..
            'DO NOT SAVE AFTER RUNNING THIS SCRIPT - run "die" to quit DF\n' ..
            'without saving.\n\n' ..
            'To use this script safely,\n' ..
            '1. Press "esc" to exit this prompt\n' ..
            '2. Pause DF\n' ..
            '3. Run "quicksave" to save this world\n' ..
            '4. Run this script again and press ENTER to enter legends mode\n' ..
            '5. IMMEDIATELY AFTER EXITING LEGENDS, run "die" to quit DF\n\n' ..
            'Press "esc" below to go back, or "y" to enter legends mode.\n' ..
            'By pressing "y", you acknowledge that your save could be\n' ..
            'permanently corrupted if you do not follow the above steps.',
            COLOR_LIGHTRED,
            show_screen
        )
    end
end

if dfhack_flags.module then
    return
end

local iargs = utils.invert{...}
main(iargs.force)
