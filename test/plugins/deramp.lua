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

local function tiletype_at(x, y, z)
    return dfhack.maps.getTileType(x, y, z)
end

-- getTileBlock returns live refs into the map data, so field writes
-- through it mutate the map directly
local function set_tiletype(x, y, z, tt)
    dfhack.maps.getTileBlock(x, y, z).tiletype[x % 16][y % 16] = tt
end

function test.removes_designated_ramp()
    local block, x, y = find_ramp_pos()
    expect.ne(nil, block, 'test needs a revealed ramp tile')
    local ax, ay, az = block.map_pos.x + x, block.map_pos.y + y,
                       block.map_pos.z

    local des = block.designation[x][y]
    local orig_dig = des.dig
    -- ramps are two tiles tall; the usable-ramp cap is RampTop at z+1
    local above_before = tiletype_at(ax, ay, az + 1)
    return dfhack.with_finalize(function()
        block.designation[x][y].dig = orig_dig
    end, function()
        -- mark the ramp for removal, as if the player had designated it
        des.dig = df.tile_dig_designation.Default
        local _, status = dfhack.run_command_silent('deramp')
        expect.eq(CR_OK, status)
        local shape = df.tiletype.attrs[block.tiletype[x][y]].shape
        expect.ne(df.tiletype_shape.RAMP, shape)
        -- the tile above must be cleared iff it was the ramp's cap
        local above_after = tiletype_at(ax, ay, az + 1)
        if above_before == df.tiletype.RampTop then
            expect.eq(df.tiletype.OpenSpace, above_after)
        else
            expect.eq(above_before, above_after)
        end
    end)
end

function test.leaves_non_ramptop_above()
    -- fabricate the second case: a ramp whose z+1 tile is not RampTop
    local block, x, y = find_ramp_pos()
    expect.ne(nil, block, 'test needs a revealed ramp tile')
    local ax, ay, az = block.map_pos.x + x, block.map_pos.y + y,
                       block.map_pos.z

    local des = block.designation[x][y]
    local orig_dig = des.dig
    local orig_above = tiletype_at(ax, ay, az + 1)
    return dfhack.with_finalize(function()
        block.designation[x][y].dig = orig_dig
        set_tiletype(ax, ay, az + 1, orig_above)
    end, function()
        set_tiletype(ax, ay, az + 1,
                                df.tiletype.StoneFloorSmooth)
        des.dig = df.tile_dig_designation.Default
        local _, status = dfhack.run_command_silent('deramp')
        expect.eq(CR_OK, status)
        expect.eq(df.tiletype.StoneFloorSmooth,
                  tiletype_at(ax, ay, az + 1))
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
