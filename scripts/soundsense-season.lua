-- On map load writes the current season to gamelog.txt

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
