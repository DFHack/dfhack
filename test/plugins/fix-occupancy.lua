config.mode = 'fortress'
config.target = 'fix-occupancy'

local fo = require('plugins.fix-occupancy')

local function any_map_pos()
    local block = df.global.world.map.map_blocks[0]
    return xyz2pos(block.map_pos.x + 8, block.map_pos.y + 8,
                   block.map_pos.z)
end

function test.fix_map_dry_run_leaves_occupancy()
    local pos = any_map_pos()
    local block = dfhack.maps.getTileBlock(pos)
    local before = block.occupancy[pos.x % 16][pos.y % 16].whole
    fo.fix_map(true)
    expect.eq(before, block.occupancy[pos.x % 16][pos.y % 16].whole)
end

function test.fix_tile_dry_run()
    local pos = any_map_pos()
    fo.fix_tile(pos, true)
    -- smoke: ran without error
    expect.true_(true)
end

function test.fix_map_run()
    fo.fix_map(false)
    expect.true_(true)
end
