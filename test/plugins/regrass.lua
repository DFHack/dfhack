config.mode = 'fortress'
config.target = 'regrass'

local regrass = require('plugins.regrass')

local function parse(args)
    local opts, pos1, pos2 = {}, {}, {}
    return regrass.parse_commandline(opts, pos1, pos2, args), opts, pos1, pos2
end

function test.plant_requires_force()
    expect.error_match('without %-%-force',
        function() parse({'--plant', 'GRASS'}) end)
end

function test.plant_with_force_resolves_grass_by_name()
    local grasses = df.global.world.raws.plants.grasses
    if #grasses == 0 then return end
    local grass = grasses[0]
    local _, opts = parse({'--force', '--plant', grass.id})
    expect.eq(grass.index, opts.forced_plant)
end

function test.plant_with_force_resolves_grass_by_index()
    local _, opts = parse({'--force', '--plant', '0'})
    expect.eq(0, opts.forced_plant)
end

function test.unknown_plant_errors()
    expect.error_match('Plant raw not found',
        function() parse({'--force', '--plant', 'NO_SUCH_PLANT_XYZ'}) end)
end

function test.list_flag_forces_plant_listing()
    local _, opts = parse({'--list'})
    expect.eq(-2, opts.forced_plant)
end

function test.force_without_plant_picks_random_grass()
    if #df.global.world.raws.plants.grasses == 0 then return end
    local _, opts = parse({'--force'})
    expect.true_(type(opts.forced_plant) == 'number')
    expect.true_(opts.forced_plant >= 0)
end

function test.option_flags_set()
    local _, opts = parse({'--max', '--new', '--ashes', '--buildings',
                           '--mud', '--block', '--zlevel'})
    expect.true_(opts.max_grass)
    expect.true_(opts.new_grass)
    expect.true_(opts.ashes)
    expect.true_(opts.buildings)
    expect.true_(opts.mud)
    expect.true_(opts.block)
    expect.true_(opts.zlevel)
end

function test.too_many_positionals_error()
    expect.error_match('Too many positionals',
        function() parse({'1,1,1', '2,2,2', '3,3,3'}) end)
end

function test.positionals_assign_coords()
    local _, _, pos1, pos2 = parse({'10,11,95', '20,21,96'})
    expect.eq(10, pos1.x)
    expect.eq(11, pos1.y)
    expect.eq(95, pos1.z)
    expect.eq(20, pos2.x)
    expect.eq(21, pos2.y)
    expect.eq(96, pos2.z)
end
