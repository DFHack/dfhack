config.mode = 'fortress'
config.target = 'deramp'

local function find_ramp_pos()
    for _, block in ipairs(df.global.world.map.map_blocks) do
        for x = 0, 15 do
            for y = 0, 15 do
                local tt = block.tiletype[x][y]
                if df.tiletype.attrs[tt].shape == df.tiletype_shape.RAMP
                        and not block.designation[x][y].hidden then
                    return block, x, y
                end
            end
        end
    end
end

function test.removes_designated_ramp()
    local block, x, y = find_ramp_pos()
    expect.ne(nil, block, 'test needs a revealed ramp tile')

    local des = block.designation[x][y]
    local orig_dig = des.dig
    return dfhack.with_finalize(function()
        block.designation[x][y].dig = orig_dig
    end, function()
        -- mark the ramp for removal, as if the player had designated it
        des.dig = df.tile_dig_designation.Default
        local _, status = dfhack.run_command_silent('deramp')
        expect.eq(CR_OK, status)
        local shape = df.tiletype.attrs[block.tiletype[x][y]].shape
        expect.ne(df.tiletype_shape.RAMP, shape)
    end)
end

function test.no_designations_is_ok()
    local _, status = dfhack.run_command_silent('deramp')
    expect.eq(CR_OK, status)
end

function test.arg_is_wrong_usage()
    local _, status = dfhack.run_command_silent('deramp', 'bogus')
    expect.eq(CR_WRONG_USAGE, status)
end
