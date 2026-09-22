config.mode = 'fortress'
config.target = 'liquids'

local dwarfmode = require('gui.dwarfmode')
local liquids = require('plugins.liquids')

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

-- wait for any screen above the map to go away (e.g. a lingering load
-- screen, or the prerelease warning shown by dev builds on map load)
-- so that commands guarded by the cursor_hotkey UI check can run
local function ensure_dwarfmode_view()
    delay_until(function()
        local top = dfhack.gui.getCurViewscreen()
        if df.viewscreen_dwarfmodest:is_instance(top) then return true end
        if type(top.dismiss) == 'function' then
            top:dismiss()
        end
        return false
    end)
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
        ensure_dwarfmode_view()
        set_cursor(x, y, z)

        local _, status = dfhack.run_command_silent('liquids-here')
        expect.eq(CR_OK, status)
        local des = des_at(x, y, z)
        expect.eq(7, des.flow_size)
        -- liquid_type is a 1-bit field exposed to Lua as a boolean
        expect.eq(true, des.liquid_type)
    end)
end

local OPEN_SHAPES = {
    [df.tiletype_shape.EMPTY] = true,
    [df.tiletype_shape.RAMP_TOP] = true,
}

-- find an open tile directly above another open, sky-lit tile
local function find_stacked_open_tiles()
    for _, block in ipairs(df.global.world.map.map_blocks) do
        local below = block.map_pos.z >= 1 and
            dfhack.maps.getTileBlock(block.map_pos.x, block.map_pos.y,
                                     block.map_pos.z - 1) or nil
        if below then
            for x = 0, 15 do
                for y = 0, 15 do
                    local des_below = below.designation[x][y]
                    if des_below.light and des_below.outside
                            and OPEN_SHAPES[df.tiletype.attrs[
                                block.tiletype[x][y]].shape]
                            and OPEN_SHAPES[df.tiletype.attrs[
                                below.tiletype[x][y]].shape] then
                        return block.map_pos.x + x, block.map_pos.y + y,
                               block.map_pos.z
                    end
                end
            end
        end
    end
end

-- find an open, sky-lit tile in the topmost allocated block of a map
-- column whose column to the east also reaches that z (needed by the
-- range test)
local function find_sky_top_pos()
    for _, block in ipairs(df.global.world.map.map_blocks) do
        local bx, by, bz = block.map_pos.x, block.map_pos.y, block.map_pos.z
        if not dfhack.maps.getTileBlock(bx, by, bz + 1)
                and dfhack.maps.getTileBlock(bx + 16, by, bz) then
            for x = 0, 15 do
                for y = 0, 15 do
                    local tt = block.tiletype[x][y]
                    local des = block.designation[x][y]
                    if df.tiletype.attrs[tt].shape == df.tiletype_shape.EMPTY
                            and des.light and des.outside then
                        return bx + x, by + y, bz
                    end
                end
            end
        end
    end
end

-- detach a block from block_index so its position reads as unallocated;
-- ensureTileBlock can then materialize it from the block below in the
-- same column. returns a function that restores the original block
local function unallocate_block(x, y, z)
    local column = df.global.world.map.block_index
        [math.floor(x / 16)][math.floor(y / 16)]
    local block = column[z]
    column[z] = nil
    expect.ne(nil, block, 'test needs an allocated block to detach')
    expect.eq(nil, dfhack.maps.getTileBlock(x, y, z),
              'test setup could not detach the block')
    return function() column[z] = block end
end

-- snapshot the light/outside/subterranean flags of the open column below
-- pos; paint may propagate coverage down through all of it
local function snapshot_column_flags(x, y, z)
    local column = {}
    for dz = z, 0, -1 do
        local b = dfhack.maps.getTileBlock(x, y, dz)
        if not b or not OPEN_SHAPES[
                df.tiletype.attrs[b.tiletype[x % 16][y % 16]].shape] then
            break
        end
        local d = b.designation[x % 16][y % 16]
        column[dz] = {block=b, light=d.light, outside=d.outside,
                      subterranean=d.subterranean}
    end
    return column
end

local function restore_column_flags(x, y, column)
    for dz, orig in pairs(column) do
        local d = orig.block.designation[x % 16][y % 16]
        d.light = orig.light
        d.outside = orig.outside
        d.subterranean = orig.subterranean
    end
end

function test.paint_obsidian_covers_tiles_below()
    local x, y, z = find_stacked_open_tiles()
    expect.ne(nil, x, 'test needs vertically adjacent open, lit tiles')

    local block = dfhack.maps.getTileBlock(x, y, z)
    local orig_tt = block.tiletype[x % 16][y % 16]
    local orig_des_whole = block.designation[x % 16][y % 16].whole
    local column = snapshot_column_flags(x, y, z - 1)

    return dfhack.with_finalize(function()
        block.tiletype[x % 16][y % 16] = orig_tt
        block.designation[x % 16][y % 16].whole = orig_des_whole
        restore_column_flags(x, y, column)
    end, function()
        liquids.paint(xyz2pos(x, y, z), 'point', 'obsidian')

        expect.eq(df.tiletype.LavaWall, dfhack.maps.getTileType(x, y, z))
        local des = des_at(x, y, z - 1)
        expect.false_(des.light)
        expect.false_(des.outside)
    end)
end

function test.paint_obsidian_floor_covers_tiles_below()
    local x, y, z = find_stacked_open_tiles()
    expect.ne(nil, x, 'test needs vertically adjacent open, lit tiles')

    local block = dfhack.maps.getTileBlock(x, y, z)
    local orig_tt = block.tiletype[x % 16][y % 16]
    local orig_des_whole = block.designation[x % 16][y % 16].whole
    local column = snapshot_column_flags(x, y, z - 1)

    return dfhack.with_finalize(function()
        block.tiletype[x % 16][y % 16] = orig_tt
        block.designation[x % 16][y % 16].whole = orig_des_whole
        restore_column_flags(x, y, column)
    end, function()
        liquids.paint(xyz2pos(x, y, z), 'point', 'obsidian_floor')

        local tt = dfhack.maps.getTileType(x, y, z)
        expect.eq(df.tiletype_shape.FLOOR, df.tiletype.attrs[tt].shape)
        expect.eq(df.tiletype_material.LAVA_STONE,
                  df.tiletype.attrs[tt].material)
        local des = des_at(x, y, z - 1)
        expect.false_(des.light)
        expect.false_(des.outside)
    end)
end

function test.paint_river_source_covers_tiles_below()
    local x, y, z = find_stacked_open_tiles()
    expect.ne(nil, x, 'test needs vertically adjacent open, lit tiles')

    local block = dfhack.maps.getTileBlock(x, y, z)
    local orig_tt = block.tiletype[x % 16][y % 16]
    local orig_des_whole = block.designation[x % 16][y % 16].whole
    local column = snapshot_column_flags(x, y, z - 1)

    local was_paused = dfhack.world.ReadPauseState()
    return dfhack.with_finalize(function()
        block.tiletype[x % 16][y % 16] = orig_tt
        block.designation[x % 16][y % 16].whole = orig_des_whole
        restore_column_flags(x, y, column)
        dfhack.world.SetPauseState(was_paused)
    end, function()
        -- pause so the spawned water cannot spread before we clean it up
        dfhack.world.SetPauseState(true)
        liquids.paint(xyz2pos(x, y, z), 'point', 'riversource')

        expect.eq(df.tiletype.RiverSource, dfhack.maps.getTileType(x, y, z))
        local des = des_at(x, y, z - 1)
        expect.false_(des.light)
        expect.false_(des.outside)
    end)
end

function test.paint_water_allocates_sky_block()
    local x, y, z = find_sky_top_pos()
    expect.ne(nil, x, 'test needs a top-of-column sky block')

    -- detach the block so its position is unallocated; the paint should
    -- recreate it via ensureTileBlock
    local restore_block = unallocate_block(x, y, z)

    local was_paused = dfhack.world.ReadPauseState()
    return dfhack.with_finalize(function()
        -- swap the original block back in; the block allocated by the
        -- paint remains in map_blocks but is no longer referenced
        restore_block()
        dfhack.world.SetPauseState(was_paused)
    end, function()
        -- pause so the spawned water cannot spread before we clean it up
        dfhack.world.SetPauseState(true)
        liquids.paint(xyz2pos(x, y, z), 'point', 'water', 7)

        local block = dfhack.maps.getTileBlock(x, y, z)
        expect.ne(nil, block, 'paint did not allocate the block')
        local des = des_at(x, y, z)
        expect.eq(7, des.flow_size)
        -- liquid_type is a 1-bit field exposed to Lua as a boolean
        expect.eq(false, des.liquid_type)
    end)
end

function test.paint_obsidian_allocates_sky_block()
    local x, y, z = find_sky_top_pos()
    expect.ne(nil, x, 'test needs a top-of-column sky block')

    local restore_block = unallocate_block(x, y, z)

    -- the painted wall also covers open tiles in the block below
    local column = snapshot_column_flags(x, y, z - 1)
    return dfhack.with_finalize(function()
        restore_block()
        restore_column_flags(x, y, column)
    end, function()
        liquids.paint(xyz2pos(x, y, z), 'point', 'obsidian')

        local block = dfhack.maps.getTileBlock(x, y, z)
        expect.ne(nil, block, 'paint did not allocate the block')
        expect.eq(df.tiletype.LavaWall, dfhack.maps.getTileType(x, y, z))
    end)
end

function test.paint_water_range_into_unallocated_sky()
    local x, y, z = find_sky_top_pos()
    expect.ne(nil, x, 'test needs adjacent top-of-column sky blocks')

    -- the range brush paints forward from the cursor, so a 4x1x1 range
    -- starting near the block edge paints two tiles in each block
    local bx = x - (x % 16)
    local x0, y0 = bx + 14, y
    local restore_west = unallocate_block(bx, y0, z)
    local restore_east = unallocate_block(bx + 16, y0, z)

    local was_paused = dfhack.world.ReadPauseState()
    return dfhack.with_finalize(function()
        restore_east()
        restore_west()
        dfhack.world.SetPauseState(was_paused)
    end, function()
        -- pause so the spawned water cannot spread before we clean it up
        dfhack.world.SetPauseState(true)
        liquids.paint(xyz2pos(x0, y0, z), 'range', 'water', 5,
                      xyz2pos(4, 1, 1))

        for dx = 0, 3 do
            local des = des_at(x0 + dx, y0, z)
            expect.eq(5, des.flow_size,
                      ('tile (%d,%d,%d)'):format(x0 + dx, y0, z))
        end
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
