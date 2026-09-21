config.mode = 'fortress'
config.target = 'liquids'

local dwarfmode = require('gui.dwarfmode')

local function find_floor_pos()
    for _, block in ipairs(df.global.world.map.map_blocks) do
        for x = 0, 15 do
            for y = 0, 15 do
                local tt = block.tiletype[x][y]
                local des = block.designation[x][y]
                local occ = block.occupancy[x][y]
                if df.tiletype.attrs[tt].shape == df.tiletype_shape.FLOOR
                        and not des.hidden and des.flow_size == 0
                        and occ.building == 0 then
                    return block.map_pos.x + x, block.map_pos.y + y,
                           block.map_pos.z
                end
            end
        end
    end
end

local function set_cursor(x, y, z)
    dwarfmode.setCursorPos(xyz2pos(x, y, z))
end

local function clear_cursor()
    dwarfmode.clearCursorPos()
end

local function des_at(x, y, z)
    return select(1, dfhack.maps.getTileFlags(x, y, z))
end

function test.liquids_here_spawns_magma_at_cursor()
    local x, y, z = find_floor_pos()
    expect.ne(nil, x, 'test needs a revealed dry floor tile')

    local was_paused = dfhack.world.ReadPauseState()
    return dfhack.with_finalize(function()
        local des = des_at(x, y, z)
        des.flow_size = 0
        dfhack.world.SetPauseState(was_paused)
        clear_cursor()
    end, function()
        -- pause so the magma cannot spread before we clean it up
        dfhack.world.SetPauseState(true)
        set_cursor(x, y, z)

        local _, status = dfhack.run_command_silent('liquids-here')
        expect.eq(CR_OK, status)
        local des = des_at(x, y, z)
        expect.eq(7, des.flow_size)
        -- liquid_type is a 1-bit field exposed to Lua as a boolean
        expect.eq(true, des.liquid_type)
    end)
end

function test.liquids_needs_console()
    -- the interactive liquid spawner requires a console; the dispatcher
    -- rejects non-console invocations before the plugin runs
    local _, status = dfhack.run_command_silent('liquids')
    expect.eq(CR_NEEDS_CONSOLE, status)
end

function test.liquids_here_help_is_wrong_usage()
    local _, status = dfhack.run_command_silent('liquids-here', '?')
    expect.eq(CR_WRONG_USAGE, status)
end

function test.liquids_here_no_cursor_is_failure()
    return dfhack.with_finalize(clear_cursor, function()
        clear_cursor()
        -- the cursor_hotkey command guard rejects before the plugin runs
        local output, status = dfhack.run_command_silent('liquids-here')
        expect.eq(CR_WRONG_USAGE, status)
        expect.str_find('unsuitable UI state', output)
    end)
end
