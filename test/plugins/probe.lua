config.mode = 'fortress'
config.target = 'probe'

local saved_cursor

config.wrapper = function(test_fn)
    saved_cursor = xyz2pos(df.global.cursor.x, df.global.cursor.y,
                           df.global.cursor.z)
    return dfhack.with_finalize(function()
        df.global.cursor.x = saved_cursor.x
        df.global.cursor.y = saved_cursor.y
        df.global.cursor.z = saved_cursor.z
    end, test_fn)
end

local function set_cursor(pos)
    df.global.cursor.x = pos.x
    df.global.cursor.y = pos.y
    df.global.cursor.z = pos.z
end

local function any_map_pos()
    local block = df.global.world.map.map_blocks[0]
    return xyz2pos(block.map_pos.x + 8, block.map_pos.y + 8,
                   block.map_pos.z)
end

function test.probe_requires_cursor()
    set_cursor(xyz2pos(-30000, -30000, -30000))
    local output = dfhack.run_command_silent('probe')
    expect.str_find('No cursor', output)
end

function test.probe_reports_tiletype()
    set_cursor(any_map_pos())
    local output = dfhack.run_command_silent('probe')
    expect.str_find('tiletype', output)
    expect.str_find('dig', output)
end

function test.cprobe_requires_unit()
    set_cursor(xyz2pos(-30000, -30000, -30000))
    local output = dfhack.run_command_silent('cprobe')
    -- fails without a selected creature
    expect.ne('', output)
end
