config.mode = 'fortress'
config.target = 'reveal'

-- reveal/unreveal mutate every map block's hidden flag, and the plugin
-- force-pauses the game while the map is revealed. Every test that reveals
-- must unreveal in a finalize so a failed check can't leave the game
-- revealed (and stuck paused) for the rest of the suite.
local function ensure_unrevealed()
    dfhack.run_command_silent('unreveal')
end

local function count_hidden()
    local count = 0
    for _, block in ipairs(df.global.world.map.map_blocks) do
        for x = 0, 15 do
            for y = 0, 15 do
                if block.designation[x][y].hidden then
                    count = count + 1
                end
            end
        end
    end
    return count
end

function test.reveal_and_unreveal()
    local before = count_hidden()
    expect.true_(before > 0, 'test needs at least one hidden tile on the map')

    return dfhack.with_finalize(ensure_unrevealed, function()
        local output, status = dfhack.run_command_silent('reveal')
        expect.eq(CR_OK, status)
        expect.str_find('Map revealed%.', output)
        -- blocks containing trigger events (encased horrors, treasure veins)
        -- stay hidden by design, so compare counts rather than spot tiles
        expect.lt(count_hidden(), before)

        output, status = dfhack.run_command_silent('unreveal')
        expect.eq(CR_OK, status)
        expect.eq(before, count_hidden())
    end)
end

function test.reveal_twice_fails()
    return dfhack.with_finalize(ensure_unrevealed, function()
        dfhack.run_command_silent('reveal')
        local output, status = dfhack.run_command_silent('reveal')
        expect.eq(CR_FAILURE, status)
        expect.str_find('already revealed', output)
    end)
end

function test.unreveal_nothing_fails()
    local output, status = dfhack.run_command_silent('unreveal')
    expect.eq(CR_FAILURE, status)
    expect.str_find('nothing to revert', output)
end

function test.demon_is_disabled()
    local output, status = dfhack.run_command_silent('reveal', 'demon')
    expect.eq(CR_FAILURE, status)
    expect.str_find('currently disabled', output)
end

function test.revtoggle_toggles()
    local before = count_hidden()
    expect.true_(before > 0, 'test needs at least one hidden tile on the map')

    return dfhack.with_finalize(ensure_unrevealed, function()
        local _, status = dfhack.run_command_silent('revtoggle')
        expect.eq(CR_OK, status)
        expect.lt(count_hidden(), before)

        _, status = dfhack.run_command_silent('revtoggle')
        expect.eq(CR_OK, status)
        expect.eq(before, count_hidden())
    end)
end
