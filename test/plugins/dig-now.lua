config.mode = 'fortress'
config.target = 'dig-now'

local dignow = require('plugins.dig-now')

local function make_opts()
    return {
        boulder_percents={},
        dump_pos={},
        start={},
        ['end']={},
    }
end

function test.clean_sets_zero_percentages()
    local opts = make_opts()
    dignow.parse_commandline(opts, '--clean')
    expect.eq(0, opts.boulder_percents.layer)
    expect.eq(0, opts.boulder_percents.vein)
    expect.eq(0, opts.boulder_percents.small_cluster)
    expect.eq(0, opts.boulder_percents.deep)
end

function test.everywhere_sets_full_percentages()
    local opts = make_opts()
    dignow.parse_commandline(opts, '--everywhere')
    expect.eq(100, opts.boulder_percents.layer)
    expect.eq(100, opts.boulder_percents.deep)
end

function test.percentages_custom_values()
    local opts = make_opts()
    dignow.parse_commandline(opts, '--percentages', '0,33,100,100')
    expect.eq(0, opts.boulder_percents.layer)
    expect.eq(33, opts.boulder_percents.vein)
    expect.eq(100, opts.boulder_percents.small_cluster)
    expect.eq(100, opts.boulder_percents.deep)
end

function test.percentages_rejects_bad_values()
    local opts = make_opts()
    expect.error_match('invalid percentages',
        function() dignow.parse_commandline(opts, '-p', '0,33,101,100') end)
    expect.error_match('invalid percentages',
        function() dignow.parse_commandline(opts, '-p', '0,33,-5,100') end)
    expect.error(function()
        dignow.parse_commandline(opts, '-p', '0,33,100') end)
end

function test.coords_normalize_start_end()
    local opts = make_opts()
    dignow.parse_commandline(opts, '9,8,95', '3,4,94')
    expect.eq(3, opts.start.x)
    expect.eq(9, opts['end'].x)
    expect.eq(4, opts.start.y)
    expect.eq(8, opts['end'].y)
    expect.eq(94, opts.start.z)
    expect.eq(95, opts['end'].z)
end

function test.single_coord_sets_same_end()
    local opts = make_opts()
    dignow.parse_commandline(opts, '5,6,94')
    expect.eq(5, opts.start.x)
    expect.eq(5, opts['end'].x)
    expect.eq(6, opts['end'].y)
    expect.eq(94, opts['end'].z)
end

function test.cur_zlevel_covers_whole_map()
    local opts = make_opts()
    dignow.parse_commandline(opts, '--cur-zlevel')
    local map = df.global.world.map
    expect.eq(0, opts.start.x)
    expect.eq(map.x_count - 1, opts['end'].x)
    expect.eq(map.y_count - 1, opts['end'].y)
    expect.eq(df.global.window_z, opts.start.z)
    expect.eq(df.global.window_z, opts['end'].z)
end

function test.help_flag_leaves_opts_untouched()
    local opts = make_opts()
    dignow.parse_commandline(opts, '--help')
    expect.true_(opts.help)
    expect.nil_(opts.start.x)
    dignow.parse_commandline(opts, 'help')
    expect.true_(opts.help)
end

function test.dump_pos_parses_coords()
    local opts = make_opts()
    dignow.parse_commandline(opts, '--dump', '1,2,90')
    expect.eq(1, opts.dump_pos.x)
    expect.eq(2, opts.dump_pos.y)
    expect.eq(90, opts.dump_pos.z)
end
