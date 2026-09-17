config.mode = 'fortress'
config.target = 'autodump'

local function find_floor_pos()
    for _, block in ipairs(df.global.world.map.map_blocks) do
        for x = 0, 15 do
            for y = 0, 15 do
                local tt = block.tiletype[x][y]
                local des = block.designation[x][y]
                local occ = block.occupancy[x][y]
                if df.tiletype.attrs[tt].shape == df.tiletype_shape.FLOOR
                        and not des.hidden and occ.building == 0 then
                    return block.map_pos.x + x, block.map_pos.y + y,
                           block.map_pos.z
                end
            end
        end
    end
end

local function set_cursor(x, y, z)
    df.global.cursor:assign{x=x, y=y, z=z}
end

local function clear_cursor()
    df.global.cursor:assign{x=-30000, y=-30000, z=-30000}
end

local function find_dumpable_item()
    for _, item in ipairs(df.global.world.items.other.IN_PLAY) do
        if not item.flags.dump and not item.flags.construction
                and not item.flags.in_building and not item.flags.artifact
                and not item.flags.in_job and not item.flags.forbid
                and not item.flags.owned and not item.flags.in_inventory then
            return item
        end
    end
end

function test.dump_moves_item_to_cursor()
    local x, y, z = find_floor_pos()
    local item = find_dumpable_item()
    expect.ne(nil, x, 'test needs a revealed floor tile')
    expect.ne(nil, item, 'test needs a dumpable item')

    return dfhack.with_finalize(function()
        item.flags.dump = false
        clear_cursor()
    end, function()
        set_cursor(x, y, z)
        item.flags.dump = true

        local _, status = dfhack.run_command_silent('autodump')
        expect.eq(CR_OK, status)
        -- item was teleported to the cursor and marked as dumped
        expect.eq(x, item.pos.x)
        expect.eq(y, item.pos.y)
        expect.eq(z, item.pos.z)
        expect.false_(item.flags.dump)
        expect.true_(item.flags.forbid)
    end)
end

function test.conflicting_filters_is_wrong_usage()
    local output, status = dfhack.run_command_silent('autodump', 'visible', 'hidden')
    expect.eq(CR_WRONG_USAGE, status)
    expect.str_find("both hidden and visible", output)
end

function test.bad_option_is_wrong_usage()
    local _, status = dfhack.run_command_silent('autodump', 'bogus')
    expect.eq(CR_WRONG_USAGE, status)
end

function test.destroy_and_undestroy()
    local item = find_dumpable_item()
    expect.ne(nil, item, 'test needs a dumpable item')

    local was_paused = df.global.pause_state
    return dfhack.with_finalize(function()
        df.global.pause_state = was_paused
        dfhack.run_command_silent('autodump', 'undestroy')
        item.flags.dump = false
        item.flags.garbage_collect = false
        item.flags.forbid = false
        item.flags.hidden = false
    end, function()
        item.flags.dump = true
        -- undestroy only restores marks made in the same frame, so the
        -- game must stay paused between destroy and undestroy
        df.global.pause_state = true

        local _, status = dfhack.run_command_silent('autodump', 'destroy')
        expect.eq(CR_OK, status)
        expect.true_(item.flags.garbage_collect)
        expect.true_(item.flags.forbid)
        expect.true_(item.flags.hidden)

        local output, status2 = dfhack.run_command_silent('autodump', 'undestroy')
        expect.eq(CR_OK, status2)
        expect.str_find('unmarked for destruction', output)
        expect.false_(item.flags.garbage_collect)
        expect.false_(item.flags.forbid)
        expect.false_(item.flags.hidden)
        -- undestroy restores the pre-destroy flags, including the dump
        -- flag we set ourselves
        expect.true_(item.flags.dump)
    end)
end
