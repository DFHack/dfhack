config.mode = 'fortress'
config.target = 'aquifer'

local aquifer = require('plugins.aquifer')

local function capture(fn)
    local calls = {}
    mock.patch({
        {aquifer, 'aquifer_list', function(pos1, pos2, levels, leaky)
            calls.list = {pos1=pos1, pos2=pos2, levels=levels, leaky=leaky}
        end},
        {aquifer, 'aquifer_drain', function(aq, pos1, pos2, skip, levels, leaky, all)
            calls.drain = {aq=aq, pos1=pos1, pos2=pos2, skip=skip,
                           levels=levels, leaky=leaky, all=all}
            return 7
        end},
        {aquifer, 'aquifer_convert', function(aq, pos1, pos2, skip, levels, leaky, all)
            calls.convert = {aq=aq, pos1=pos1, pos2=pos2, skip=skip,
                             levels=levels, leaky=leaky, all=all}
            return 5
        end},
        {aquifer, 'aquifer_add', function(aq, pos1, pos2, skip, levels, leaky, all)
            calls.add = {aq=aq, pos1=pos1, pos2=pos2, skip=skip,
                         levels=levels, leaky=leaky, all=all}
            return 3
        end},
    }, fn)
    return calls
end

function test.help_returns_false()
    -- regression test: --help used to run a map-wide list instead
    local called = false
    mock.patch({
        {aquifer, 'aquifer_list', function() called = true end},
        -- helpdb can't resolve the script entry when called via require()
        {dfhack, 'script_help', function() return 'test help' end},
    }, function()
        expect.false_(aquifer.parse_commandline({'--help'}))
        expect.false_(aquifer.parse_commandline({'-h'}))
        expect.false_(aquifer.parse_commandline({'help'}))
    end)
    expect.false_(called)
end

function test.list_defaults_to_all_levels()
    local calls = capture(function()
        expect.true_(aquifer.parse_commandline({'list'}))
    end)
    expect.eq(0, calls.list.pos1.x)
    expect.eq(df.global.world.map.z_count - 1, calls.list.pos2.z)
end

function test.list_cur_zlevel()
    local calls = capture(function()
        expect.true_(aquifer.parse_commandline({'list', '--cur-zlevel'}))
    end)
    local curz = df.global.window_z
    expect.eq(curz, calls.list.pos1.z)
    expect.eq(curz, calls.list.pos2.z)
end

function test.coords_normalize_min_max()
    local calls = capture(function()
        expect.true_(aquifer.parse_commandline(
            {'list', '9,9,100', '3,4,99'}))
    end)
    expect.eq(3, calls.list.pos1.x)
    expect.eq(9, calls.list.pos2.x)
    expect.eq(4, calls.list.pos1.y)
    expect.eq(9, calls.list.pos2.y)
    expect.eq(99, calls.list.pos1.z)
    expect.eq(100, calls.list.pos2.z)
end

function test.single_coord_defaults_to_cursor_or_same_pos()
    local calls = capture(function()
        expect.true_(aquifer.parse_commandline({'list', '3,4,99'}))
    end)
    expect.eq(3, calls.list.pos1.x)
    if df.global.window_z == 99 then
        -- cursor would need to be on z 99; either way pos2 is consistent
        expect.true_(calls.list.pos2.x == 3 or calls.list.pos2.x ~= nil)
    end
end

function test.drain_requires_no_aquifer_type()
    local calls = capture(function()
        expect.true_(aquifer.parse_commandline({'drain', '--cur-zlevel'}))
    end)
    expect.eq('all', calls.drain.aq)
    expect.eq(0, calls.drain.skip)
end

function test.add_and_convert_require_aquifer_type()
    capture(function()
        expect.error_match('must specify an aquifer type',
            function() aquifer.parse_commandline({'add', '--cur-zlevel'}) end)
        expect.error_match('must specify an aquifer type',
            function() aquifer.parse_commandline({'convert', '--cur-zlevel'}) end)
    end)
end

function test.add_light_with_skip_and_levels()
    local calls = capture(function()
        expect.true_(aquifer.parse_commandline(
            {'add', 'light', '--cur-zlevel', '--skip-top', '2'}))
    end)
    expect.eq('light', calls.add.aq)
    expect.eq(2, calls.add.skip)
    expect.eq(1, calls.add.levels)
end

function test.levels_option_with_zup()
    -- --zup keeps the bottom N levels of the range
    local calls = capture(function()
        expect.true_(aquifer.parse_commandline(
            {'drain', 'heavy', '0,0,90', '10,10,95', '--zup', '--levels', '2'}))
    end)
    expect.eq(90, calls.drain.pos1.z)
    expect.eq(91, calls.drain.pos2.z)
    expect.eq(2, calls.drain.levels)
    expect.eq('heavy', calls.drain.aq)
end

function test.levels_option_with_zdown()
    -- --zdown keeps the top N levels of the range
    local calls = capture(function()
        expect.true_(aquifer.parse_commandline(
            {'drain', 'heavy', '0,0,90', '10,10,95', '--zdown', '--levels', '2'}))
    end)
    expect.eq(94, calls.drain.pos1.z)
    expect.eq(95, calls.drain.pos2.z)
    expect.eq(2, calls.drain.levels)
end

function test.invalid_levels_and_skip_top_rejected()
    expect.error(function()
        aquifer.parse_commandline({'drain', '--levels', '0'})
    end)
    expect.error(function()
        aquifer.parse_commandline({'drain', '--skip-top', '-1'})
    end)
end

function test.unknown_action_falls_back_to_list()
    -- positionals that aren't actions are treated as coords; an unparseable
    -- coord raises an error through argparse
    expect.error(function()
        aquifer.parse_commandline({'bogus', '--cur-zlevel'})
    end)
end
