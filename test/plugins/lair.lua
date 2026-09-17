config.mode = 'fortress'
config.target = 'lair'

local function any_lair_tile()
    for _, block in ipairs(df.global.world.map.map_blocks) do
        for x = 0, 15 do
            for y = 0, 15 do
                if block.occupancy[x][y].monster_lair then
                    return true
                end
            end
        end
    end
    return false
end

function test.set_and_reset()
    local output, status = dfhack.run_command_silent('lair')
    expect.eq(CR_OK, status)
    expect.str_find('Map marked as lair%.', output)
    expect.true_(any_lair_tile())

    output, status = dfhack.run_command_silent('lair', 'reset')
    expect.eq(CR_OK, status)
    expect.str_find('Map no longer marked as lair%.', output)
    expect.false_(any_lair_tile())
end
