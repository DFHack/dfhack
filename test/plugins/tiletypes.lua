config.mode = 'fortress'
config.target = 'tiletypes'

local tiletypes = require('plugins.tiletypes')

local function set_cursor(pos)
    df.global.cursor.x = pos.x
    df.global.cursor.y = pos.y
    df.global.cursor.z = pos.z
end

local function reset_cursor()
    df.global.cursor.x = -30000
    df.global.cursor.y = -30000
    df.global.cursor.z = -30000
end

-- options is a list of "<option> <value>" paint spec strings; each is emitted
-- as its own paint command since a paint command consumes a single
-- option/value pair
local function paint_at(pos, options)
    set_cursor(pos)
    local parts = {'tiletypes-command point', 'paint any'}
    for _, opt in ipairs(options) do
        table.insert(parts, 'paint ' .. opt)
    end
    table.insert(parts, 'run')
    return dfhack.run_command_silent(table.concat(parts, ' ; '))
end

local function tiletype_spec(tt)
    local attrs = df.tiletype.attrs[tt]
    return {
        'shape ' .. (df.tiletype_shape[attrs.shape] or 'ANY'),
        'material ' .. (df.tiletype_material[attrs.material] or 'ANY'),
        'special ' .. (df.tiletype_special[attrs.special] or 'ANY'),
        'variant ' .. (df.tiletype_variant[attrs.variant] or 'ANY'),
    }
end

local WALL_SPEC = {'shape WALL', 'material SOIL', 'special NORMAL'}

local function find_plant(want_tree)
    for _, plant in ipairs(df.global.world.plants.all) do
        if want_tree == (plant.tree_info ~= nil) and
                dfhack.maps.getTileType(plant.pos) then
            return plant
        end
    end
end

local function check_plant_survival(pos, expect_removed)
    local plant = dfhack.maps.getPlantAtTile(pos)
    if expect_removed then
        expect.nil_(plant)
    else
        expect.ne(nil, plant)
    end
end

function test.painting_away_tile_removes_plant()
    local plant = find_plant(false)
    if not plant then qerror('no shrub or sapling found on the map') end
    local pos = xyz2pos(plant.pos.x, plant.pos.y, plant.pos.z)
    local orig_tt = dfhack.maps.getTileType(pos)
    dfhack.with_finalize(function()
        paint_at(pos, tiletype_spec(orig_tt))
        reset_cursor()
    end, function()
        check_plant_survival(pos, false)
        local _, status = paint_at(pos, WALL_SPEC)
        expect.eq(CR_OK, status)
        check_plant_survival(pos, true)
    end)
end

function test.painting_compatible_tile_keeps_plant()
    local plant = find_plant(false)
    if not plant then qerror('no shrub or sapling found on the map') end
    local pos = xyz2pos(plant.pos.x, plant.pos.y, plant.pos.z)
    dfhack.with_finalize(function()
        reset_cursor()
    end, function()
        local orig_tt = dfhack.maps.getTileType(pos)
        local _, status = paint_at(pos, tiletype_spec(orig_tt))
        expect.eq(CR_OK, status)
        check_plant_survival(pos, false)
    end)
end

function test.painting_trunk_removes_tree()
    local plant = find_plant(true)
    if not plant then qerror('no grown tree found on the map') end
    local pos = xyz2pos(plant.pos.x, plant.pos.y, plant.pos.z)
    local orig_tt = dfhack.maps.getTileType(pos)
    dfhack.with_finalize(function()
        paint_at(pos, tiletype_spec(orig_tt))
        reset_cursor()
    end, function()
        check_plant_survival(pos, false)
        local _, status = paint_at(pos, WALL_SPEC)
        expect.eq(CR_OK, status)
        check_plant_survival(pos, true)
    end)
end

function test.tiletypes_needs_console()
    local _, status = dfhack.run_command_silent('tiletypes')
    expect.eq(CR_NEEDS_CONSOLE, status)
end

function test.command_help_runs()
    local _, status = dfhack.run_command_silent('tiletypes-command help')
    expect.eq(CR_OK, status)
end

function test.here_point_bad_option_is_failure()
    local _, status = dfhack.run_command_silent('tiletypes-here-point --bogus')
    expect.eq(CR_FAILURE, status)
end

local function parse(args)
    local opts = {cursor = xyz2pos(0, 0, 0)}
    tiletypes.parse_commandline(opts, args)
    return opts
end

function test.help_flag()
    expect.true_(parse({'--help'}).help)
    expect.true_(parse({'-h'}).help)
    expect.true_(parse({'help'}).help)
    expect.true_(parse({'?'}).help)
end

function test.quiet_flag()
    expect.true_(parse({'-q'}).quiet)
    expect.true_(parse({'--quiet'}).quiet)
end

function test.cursor_option()
    local opts = parse({'-c', '5,7,99'})
    expect.eq(5, opts.cursor.x)
    expect.eq(7, opts.cursor.y)
    expect.eq(99, opts.cursor.z)
end

function test.cursor_option_long_form()
    local opts = parse({'--cursor', '1,2,3'})
    expect.eq(1, opts.cursor.x)
    expect.eq(2, opts.cursor.y)
    expect.eq(3, opts.cursor.z)
end

function test.bad_cursor_errors()
    expect.error(function()
        tiletypes.parse_commandline({cursor = xyz2pos(0, 0, 0)},
            {'-c', 'not-a-coord'})
    end)
end

function test.positionals_do_not_set_flags()
    local opts = parse({'paint', 'stone', 'microcline'})
    expect.nil_(opts.help)
    expect.nil_(opts.quiet)
end
