config.mode = 'fortress'
config.target = 'filltraffic'

local dwarfmode = require('gui.dwarfmode')

local function find_floor_pos()
    for _, block in ipairs(df.global.world.map.map_blocks) do
        for x = 0, 15 do
            for y = 0, 15 do
                local tt = block.tiletype[x][y]
                local shape = df.tiletype.attrs[tt].shape
                local des = block.designation[x][y]
                local occ = block.occupancy[x][y]
                if shape == df.tiletype_shape.FLOOR and not des.hidden
                        and occ.building == 0 then
                    return block.map_pos.x + x, block.map_pos.y + y,
                           block.map_pos.z
                end
            end
        end
    end
end

local function traffic_at(x, y, z)
    return dfhack.maps.getTileBlock(x, y, z).designation[x % 16][y % 16].traffic
end

local function set_cursor(x, y, z)
    dwarfmode.setCursorPos(xyz2pos(x, y, z))
end

local function clear_cursor()
    dwarfmode.clearCursorPos()
end

function test.fill_high_then_restore()
    local x, y, z = find_floor_pos()
    expect.ne(nil, x, 'test needs a revealed floor tile')

    return dfhack.with_finalize(function()
        set_cursor(x, y, z)
        -- always leave the tile normal
        dfhack.run_command_silent('filltraffic', 'N')
        clear_cursor()
    end, function()
        set_cursor(x, y, z)
        local _, status = dfhack.run_command_silent('filltraffic', 'H')
        expect.eq(CR_OK, status)
        expect.eq(df.tile_traffic.High, traffic_at(x, y, z))

        local _, status2 = dfhack.run_command_silent('filltraffic', 'N')
        expect.eq(CR_OK, status2)
        expect.eq(df.tile_traffic.Normal, traffic_at(x, y, z))
    end)
end

function test.same_type_is_failure()
    local x, y, z = find_floor_pos()
    expect.ne(nil, x)

    return dfhack.with_finalize(clear_cursor, function()
        set_cursor(x, y, z)
        -- the tile is already Normal, so filling Normal is a no-op failure
        local output, status = dfhack.run_command_silent('filltraffic', 'N')
        expect.eq(CR_FAILURE, status)
        expect.str_find('already set to the target', output)
    end)
end

function test.no_cursor_is_failure()
    clear_cursor()
    -- the cursor_hotkey command guard rejects before the plugin runs
    local output, status = dfhack.run_command_silent('filltraffic', 'H')
    expect.eq(CR_WRONG_USAGE, status)
    expect.str_find('unsuitable UI state', output)
end

function test.wall_is_failure()
    -- find a revealed wall tile
    local wx, wy, wz
    for _, block in ipairs(df.global.world.map.map_blocks) do
        for x = 0, 15 do
            for y = 0, 15 do
                local tt = block.tiletype[x][y]
                if df.tiletype.attrs[tt].shape == df.tiletype_shape.WALL
                        and not block.designation[x][y].hidden then
                    wx, wy, wz = block.map_pos.x + x,
                                 block.map_pos.y + y, block.map_pos.z
                end
            end
        end
    end
    expect.ne(nil, wx, 'test needs a revealed wall tile')

    return dfhack.with_finalize(clear_cursor, function()
        set_cursor(wx, wy, wz)
        local output, status = dfhack.run_command_silent('filltraffic', 'H')
        expect.eq(CR_FAILURE, status)
        expect.str_find('wall', output)
    end)
end

function test.bad_option_is_wrong_usage()
    local _, status = dfhack.run_command_silent('filltraffic', 'Q')
    expect.eq(CR_WRONG_USAGE, status)
end

function test.multi_char_option_is_wrong_usage()
    local _, status = dfhack.run_command_silent('filltraffic', 'HH')
    expect.eq(CR_WRONG_USAGE, status)
end

function test.restrictice_and_restrictliquids()
    -- restrict traffic on ice/liquid tiles; safe full-map passes
    local _, status = dfhack.run_command_silent('restrictice')
    expect.eq(CR_OK, status)
    local _, status2 = dfhack.run_command_silent('restrictliquids')
    expect.eq(CR_OK, status2)
end
