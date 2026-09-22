config.mode = 'fortress'
config.target = 'plant'

local plant = require('plugins.plant')

-- parse_commandline takes C++-allocated outputs: an options table, two
-- coords, and a vector<int32_t> filter. stl vectors can't be created from
-- Lua, so borrow an int32 vector field from a temporary burrow.
local function parse(args)
    local opts = {}
    local pos1, pos2 = xyz2pos(0, 0, -30000), xyz2pos(0, 0, -30000)
    local b = df.burrow:new()
    local ok, err = pcall(plant.parse_commandline, opts, pos1, pos2, b.block_x, args)
    -- copy out before deleting the burrow; the vector dangles with it
    local filter = {}
    for i = 0, #b.block_x - 1 do filter[i + 1] = b.block_x[i] end
    local result = {opts=opts, pos1=pos1, pos2=pos2, filter=filter}
    b:delete()
    if not ok then error(err, 0) end
    return result
end

local function find_valid_plant()
    for _,p in ipairs(df.global.world.raws.plants.bushes) do
        if p.id and #p.id > 0 then return p end
    end
end

function test.modes_set_flags()
    expect.eq(-2, parse({'list'}).opts.plant_idx)
    expect.true_(parse({'grow'}).opts.grow)
    expect.true_(parse({'remove'}).opts.del)
end

function test.create_requires_plant_id()
    expect.error_match('Must specify plant_id',
        function() parse({'create'}) end)
end

function test.create_resolves_plant_by_name_and_index()
    local p = find_valid_plant()
    if not p then return end
    local by_name = parse({'create', p.id})
    expect.true_(by_name.opts.create)
    expect.eq(p.index, by_name.opts.plant_idx)
    expect.eq(p.index, parse({'create', tostring(p.index)}).opts.plant_idx)
end

function test.create_unknown_plant_errors()
    expect.error_match('Plant raw not found',
        function() parse({'create', 'DFHACK_NO_SUCH_PLANT'}) end)
end

function test.invalid_and_missing_modes_error()
    expect.error_match('Specify mode', function() parse({}) end)
    expect.error_match('Invalid mode', function() parse({'bogus'}) end)
end

function test.too_many_positionals_error()
    expect.error_match('Too many positionals',
        function() parse({'grow', '1,1,100', '2,2,100', 'extra'}) end)
end

function test.age_option_parsing()
    expect.eq(40320*3-1, parse({'grow', '--age', 'tree'}).opts.age)
    expect.eq(40320*5-1, parse({'grow', '--age', '5'}).opts.age)
    expect.eq(40320*1250-1, parse({'grow', '--age', '9999'}).opts.age)
    expect.error_match('Invalid age',
        function() parse({'grow', '--age', 'bogus'}) end)
end

function test.filter_and_exclude_populate_vector()
    local p = find_valid_plant()
    if not p then return end
    local res = parse({'remove', '--filter', p.id})
    expect.eq(1, #res.filter)
    expect.eq(p.index, res.filter[1])
    res = parse({'remove', '--exclude', p.id})
    expect.true_(res.opts.filter_ex)
end

function test.double_filter_errors()
    expect.error_match('Filter already defined',
        function()
            local p = find_valid_plant()
            if not p then error('skip') end
            parse({'remove', '--filter', p.id, '--exclude', p.id})
        end)
end

function test.option_flags()
    local opts = parse({'remove', '-s', '-p', '-t', '-d', '-z', '-n', '-c'}).opts
    expect.true_(opts.shrubs)
    expect.true_(opts.saplings)
    expect.true_(opts.trees)
    expect.true_(opts.dead)
    expect.true_(opts.zlevel)
    expect.true_(opts.dry_run)
    expect.true_(opts.force)
end

function test.coords_assigned_to_positions()
    local res = parse({'grow', '3,4,100', '7,8,101'})
    expect.eq(3, res.pos1.x)
    expect.eq(4, res.pos1.y)
    expect.eq(100, res.pos1.z)
    expect.eq(7, res.pos2.x)
    expect.eq(8, res.pos2.y)
    expect.eq(101, res.pos2.z)
end

function test.create_uses_third_positional_as_pos1()
    local p = find_valid_plant()
    if not p then return end
    local res = parse({'create', p.id, '9,10,102'})
    expect.eq(9, res.pos1.x)
    expect.eq(102, res.pos1.z)
end
