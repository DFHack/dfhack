config.mode = 'fortress'
config.target = 'stockpiles'

local stockpiles = require('plugins.stockpiles')

local TEST_NAME = 'test_dfstock'

local function test_file_path()
    return ('%s/stockpiles/%s.dfstock'):format(dfhack.getConfigPath(),
                                             TEST_NAME)
end

local function rm_test_file()
    os.remove(test_file_path())
end

config.wrapper = function(test_fn)
    rm_test_file()
    return dfhack.with_finalize(rm_test_file, test_fn)
end

local function make_stockpile()
    -- a 2x2 stockpile on any free floor tile
    for _, block in ipairs(df.global.world.map.map_blocks) do
        local tt = df.tiletype.attrs[block.tiletype[8][8]]
        if tt.shape == df.tiletype_shape.FLOOR
                and block.occupancy[8][8].building
                    == df.tile_building_occ.None then
            local pos = xyz2pos(block.map_pos.x + 7, block.map_pos.y + 7,
                                block.map_pos.z)
            local bld, err = dfhack.buildings.constructBuilding{
                pos=pos, type=df.building_type.Stockpile,
                width=2, height=2, abstract=true}
            if bld then return bld end
        end
    end
    error('could not place a test stockpile')
end

function test.status_and_help()
    expect.true_(stockpiles.parse_commandline({'status'}))
    expect.false_(stockpiles.parse_commandline({'help'}))
    expect.false_(stockpiles.parse_commandline({'bogus'}))
end

function test.export_requires_name()
    expect.error_match('name missing or empty', function()
        stockpiles.parse_commandline({'export'})
    end)
end

function test.export_rejects_unsafe_name()
    expect.error_match('numbers, letters', function()
        stockpiles.parse_commandline({'export', 'bad/name'})
    end)
    expect.error_match('numbers, letters', function()
        stockpiles.parse_commandline({'export', 'bad name'})
    end)
end

function test.export_requires_stockpile()
    -- valid name, but no stockpile selected or specified
    expect.error_match('select a stockpile', function()
        stockpiles.parse_commandline({'export', TEST_NAME})
    end)
end

function test.stockpile_opt_unknown_name()
    expect.error_match('could not find stockpile', function()
        stockpiles.parse_commandline({'status', '-s', 'no_such_pile'})
    end)
end

function test.list_settings_files()
    -- the library dir ships stock settings files, so list must succeed
    expect.true_(stockpiles.parse_commandline({'list'}))
end

function test.export_import_roundtrip()
    local sp = make_stockpile()
    local ok, err = pcall(function()
        expect.true_(stockpiles.parse_commandline(
            {'export', TEST_NAME, '-s', tostring(sp.id)}))
        expect.true_(dfhack.filesystem.isfile(test_file_path()))
        expect.true_(stockpiles.parse_commandline(
            {'import', TEST_NAME, '-s', tostring(sp.id)}))
    end)
    dfhack.buildings.deconstruct(sp)
    if not ok then error(err, 0) end
end
