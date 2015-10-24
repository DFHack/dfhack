-- On map load writes the current season to gamelog.txt
--[[=begin

soundsense-season
=================
It is a well known issue that Soundsense cannot detect the correct
current season when a savegame is loaded and has to play random
season music until a season switch occurs.

This script registers a hook that prints the appropriate string
to :file:`gamelog.txt` on every map load to fix this. For best results
call the script from :file:`dfhack.init`.

=end]]

local seasons = {
    [-1] = 'Nothing', -- worldgen
    [0] = 'Spring',
    [1] = 'Summer',
    [2] = 'Autumn',
    [3] = 'Winter',
}

local args = {...}

if args[1] == 'disable' then
    dfhack.onStateChange[_ENV] = nil
else
    dfhack.onStateChange[_ENV] = function(op)
        if op == SC_WORLD_LOADED then
            dfhack.gui.writeToGamelog(seasons[df.global.cur_season]..' has arrived on the calendar.')
        end
    end
end
