-- On map load writes the current season to gamelog.txt

local seasons = {
    [-1] = 'Nothing', -- worldgen
    [0] = 'Spring',
    [1] = 'Summer',
    [2] = 'Autumn',
    [3] = 'Winter',
}

local args = {...}

local function write_gamelog(msg)
    local log = io.open('gamelog.txt', 'a')
    log:write(msg.."\n")
    log:close()
end

if args[1] == 'disable' then
    dfhack.onStateChange[_ENV] = nil
else
    dfhack.onStateChange[_ENV] = function(op)
        if op == SC_WORLD_LOADED then
            write_gamelog(seasons[df.global.cur_season]..' has arrived on the calendar.')
        end
    end
end
