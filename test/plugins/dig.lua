config.mode = 'fortress'
config.target = 'dig'

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

local function designation_at(pos)
    local block = dfhack.maps.getTileBlock(pos)
    return block.designation[pos.x % 16][pos.y % 16].dig
end

-- find a solid, undesignated block-center wall tile to dig in.
-- a diameter-5 circle around the block center stays within the block
local function find_diggable_pos()
    for _, block in ipairs(df.global.world.map.map_blocks) do
        local attrs = df.tiletype.attrs[block.tiletype[8][8]]
        local mat = attrs.material
        local basic = df.tiletype_shape.attrs[attrs.shape].basic_shape
        if (mat == df.tiletype_material.SOIL
                or mat == df.tiletype_material.STONE)
                and basic == df.tiletype_shape_basic.Wall
                and block.designation[8][8].dig
                    == df.tile_dig_designation.No then
            return xyz2pos(block.map_pos.x + 8,
                           block.map_pos.y + 8,
                           block.map_pos.z)
        end
    end
    error('no diggable tile found')
end

function test.digcircle_help()
    local output, status = dfhack.run_command_silent('digcircle', 'help')
    expect.true_(output:find('designation of filled and hollow circles', 1, true)
                 ~= nil)
end

function test.digcircle_requires_cursor()
    set_cursor(xyz2pos(-30000, -30000, -30000))
    local output, status = dfhack.run_command_silent('digcircle', 'filled', '5')
    expect.true_(output:find("Can't get the cursor coords", 1, true) ~= nil)
end

function test.digcircle_designates_and_unsets()
    -- digcircle remembers set/unset, diameter, and fill across calls
    -- ("do it again" feature), so always pass them explicitly
    local pos = find_diggable_pos()
    set_cursor(pos)
    dfhack.run_command_silent('digcircle', 'set', 'filled', '5', 'dig')
    expect.eq(df.tile_dig_designation.Default, designation_at(pos))
    dfhack.run_command_silent('digcircle', 'unset', 'filled', '5', 'dig')
    expect.eq(df.tile_dig_designation.No, designation_at(pos))
end

-- channels apply to floors/stairs, not walls (see dig.cpp dig() checks)
local function find_floor_pos()
    for _, block in ipairs(df.global.world.map.map_blocks) do
        local tt = block.tiletype[8][8]
        local attrs = df.tiletype.attrs[tt]
        if attrs.shape == df.tiletype_shape.FLOOR
                and attrs.material ~= df.tiletype_material.CONSTRUCTION
                and block.designation[8][8].dig
                    == df.tile_dig_designation.No
                and not block.designation[8][8].hidden then
            return xyz2pos(block.map_pos.x + 8,
                           block.map_pos.y + 8,
                           block.map_pos.z)
        end
    end
    error('no channelable floor tile found')
end

function test.digcircle_channel_designation()
    -- channels designate on floors/stairs; this floor tile is a valid target
    local pos = find_floor_pos()
    set_cursor(pos)
    dfhack.run_command_silent('digcircle', 'set', 'filled', '3', 'chan')
    expect.eq(df.tile_dig_designation.Channel, designation_at(pos))
    dfhack.run_command_silent('digcircle', 'unset', 'filled', '3', 'chan')
    expect.eq(df.tile_dig_designation.No, designation_at(pos))
end

function test.digexp_help()
    local output, status = dfhack.run_command_silent('digexp', 'help')
    expect.true_(output:find('pattern', 1, true) ~= nil)
end
