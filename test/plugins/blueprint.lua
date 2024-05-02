config.target = 'blueprint'

local b = require('plugins.blueprint')

-- also covers code shared between parse_gui_commandline and parse_commandline
function test.parse_gui_commandline()
    local opts = {}
    b.parse_gui_commandline(opts, {})
    expect.table_eq({auto_phase=true, format='minimal', split_strategy='none',
                     name='blueprint'},
                    opts)

    opts = {}
    b.parse_gui_commandline(opts, {'help'})
    expect.table_eq({help=true}, opts)

    opts = {}
    b.parse_gui_commandline(opts, {'--help'})
    expect.table_eq({help=true, format='minimal', split_strategy='none'}, opts)

    opts = {}
    b.parse_gui_commandline(opts, {'-h'})
    expect.table_eq({help=true, format='minimal', split_strategy='none'}, opts)

    opts = {}
    b.parse_gui_commandline(opts, {'--nometa'})
    expect.table_eq({auto_phase=true, format='minimal', split_strategy='none',
                     name='blueprint', nometa=true},
                    opts)

    opts = {}
    mock.patch(dfhack.maps, 'isValidTilePos', mock.func(true),
               function()
                   b.parse_gui_commandline(opts, {'--cursor=1,2,3'})
               end)
    expect.table_eq({auto_phase=true, format='minimal', split_strategy='none',
                     name='blueprint', start={x=1,y=2,z=3}},
                    opts)

    opts = {}
    b.parse_gui_commandline(opts, {'-e'})
    expect.table_eq({auto_phase=true, format='minimal', split_strategy='none',
                     name='blueprint', engrave=true,},
                    opts)

    opts = {}
    b.parse_gui_commandline(opts, {'--smooth'})
    expect.table_eq({auto_phase=true, format='minimal', split_strategy='none',
                     name='blueprint', smooth=true,},
                    opts)

    opts = {}
    b.parse_gui_commandline(opts, {'--engrave'})
    expect.table_eq({auto_phase=true, format='minimal', split_strategy='none',
                     name='blueprint', engrave=true,},
                    opts)

    opts = {}
    b.parse_gui_commandline(opts, {'-fminimal'})
    expect.table_eq({auto_phase=true, format='minimal', split_strategy='none',
                     name='blueprint'},
                    opts)

    opts = {}
    b.parse_gui_commandline(opts, {'--format', 'pretty'})
    expect.table_eq({auto_phase=true, format='pretty', split_strategy='none',
                     name='blueprint'},
                    opts)

    expect.error_match('unknown format',
                       function() b.parse_gui_commandline({}, {'-ffoo'}) end)

    opts = {}
    b.parse_gui_commandline(opts, {'-tnone'})
    expect.table_eq({auto_phase=true, format='minimal', split_strategy='none',
                     name='blueprint'},
                    opts)

    opts = {}
    b.parse_gui_commandline(opts, {'--playback-start', '2,3,imacomment'})
    expect.table_eq({auto_phase=true, format='minimal', split_strategy='none',
                     name='blueprint', playback_start={x=2, y=3},
                     playback_start_comment='imacomment'},
                    opts)

    opts = {}
    b.parse_gui_commandline(opts, {'-s2,3'})
    expect.table_eq({auto_phase=true, format='minimal', split_strategy='none',
                     name='blueprint', playback_start={x=2, y=3}},
                    opts)

    expect.error_match('must be positive integers',
                       function() b.parse_gui_commandline({}, {'-s10'}) end)
    expect.error_match('must be positive integers',
                       function() b.parse_gui_commandline({}, {'-s1,-1'}) end)
    expect.error_match('must be positive integers',
                       function() b.parse_gui_commandline({}, {'-s-1,1'}) end)
    expect.error_match('must be positive integers',
                       function() b.parse_gui_commandline({}, {'-s1,0'}) end)
    expect.error_match('must be positive integers',
                       function() b.parse_gui_commandline({}, {'-s0,1'}) end)

    opts = {}
    b.parse_gui_commandline(opts, {'--splitby', 'phase'})
    expect.table_eq({auto_phase=true, format='minimal', split_strategy='phase',
                     name='blueprint', nometa=true},
                    opts)

    expect.error_match('unknown split_strategy',
                       function() b.parse_gui_commandline({}, {'-tfoo'}) end)

    opts = {}
    b.parse_gui_commandline(opts, {'imaname'})
    expect.table_eq({auto_phase=true, format='minimal', split_strategy='none',
                     name='imaname'},
                    opts)

    expect.error_match('invalid basename',
                       function() b.parse_gui_commandline({}, {''}) end)

    opts = {}
    b.parse_gui_commandline(opts, {'imaname', 'dig', 'place'})
    expect.table_eq({auto_phase=false, format='minimal', split_strategy='none',
                     name='imaname', dig=true, place=true},
                    opts)

    expect.error_match('unknown phase',
                       function() b.parse_gui_commandline(
                                        {}, {'imaname', 'garbagephase'}) end)
end

function test.parse_commandline()
    local opts = {}
    b.parse_commandline(opts, {'1', '2'})
    expect.table_eq({auto_phase=true, format='minimal', split_strategy='none',
                     name='blueprint', width=1, height=2, depth=1},
                    opts)

    opts = {}
    b.parse_commandline(opts, {'1', '2', '3'})
    expect.table_eq({auto_phase=true, format='minimal', split_strategy='none',
                     name='blueprint', width=1, height=2, depth=3},
                    opts)

    opts = {}
    b.parse_commandline(opts, {'1', '2', '-3'})
    expect.table_eq({auto_phase=true, format='minimal', split_strategy='none',
                     name='blueprint', width=1, height=2, depth=-3},
                    opts)

    opts = {}
    b.parse_commandline(opts, {'1', '2', 'imaname'})
    expect.table_eq({auto_phase=true, format='minimal', split_strategy='none',
                     name='imaname', width=1, height=2, depth=1},
                    opts)

    opts = {}
    b.parse_commandline(opts, {'1', '2', '10imaname'})
    expect.table_eq({auto_phase=true, format='minimal', split_strategy='none',
                     name='10imaname', width=1, height=2, depth=1},
                    opts, 'invalid depth is considered a basename')

    opts = {}
    b.parse_commandline(opts, {'1', '2', '-10imaname'})
    expect.table_eq({auto_phase=true, format='minimal', split_strategy='none',
                     name='-10imaname', width=1, height=2, depth=1},
                    opts, 'invalid negative depth is considered a basename')

    opts = {}
    b.parse_commandline(opts, {'1', '2', '3', 'imaname'})
    expect.table_eq({auto_phase=true, format='minimal', split_strategy='none',
                     name='imaname', width=1, height=2, depth=3},
                    opts)

    expect.error_match('invalid width or height',
                       function() b.parse_commandline({}, {}) end,
                       'missing width')
    expect.error_match('invalid width or height',
                       function() b.parse_commandline({}, {'10'}) end,
                       'missing height')
    expect.error_match('invalid width or height',
                       function() b.parse_commandline({}, {'0'}) end,
                       'zero height')
    expect.error_match('invalid width or height',
                       function() b.parse_commandline({}, {'hi'}) end,
                       'invalid width')
    expect.error_match('invalid width or height',
                       function() b.parse_commandline({}, {'10', 'hi'}) end,
                       'invalid height')
    expect.error_match('invalid depth',
                       function() b.parse_commandline({}, {'1', '2', '0'}) end,
                       'zero depth')

    expect.error_match('x offset outside width of blueprint',
                       function() b.parse_commandline(
                                {}, {'3', '2', '1', '-s4,1'}) end)
    expect.error_match('y offset outside height of blueprint',
                       function() b.parse_commandline(
                                {}, {'3', '2', '1', '-s1,3'}) end)
end

function test.do_phase_positive_dims()
    local mock_run = mock.func()
    mock.patch(b, 'run', mock_run,
        function()
              local spos = {x=10, y=20, z=30}
              local epos = {x=11, y=21, z=31}
              b.place(spos, epos, 'imaname')
              expect.eq(1, mock_run.call_count)
              expect.table_eq({'2', '2', '2', 'imaname', 'place',
                               '--cursor=10,20,30'},
                              mock_run.call_args[1])
        end)
end

function test.do_phase_negative_dims()
    local mock_run = mock.func()
    mock.patch(b, 'run', mock_run,
        function()
              local spos = {x=11, y=21, z=31}
              local epos = {x=10, y=20, z=30}
              b.place(spos, epos, 'imaname')
              expect.eq(1, mock_run.call_count)
              expect.table_eq({'2', '2', '-2', 'imaname', 'place',
                               '--cursor=10,20,31'},
                              mock_run.call_args[1])
        end)
end

function test.do_phase_ensure_cursor_is_at_upper_left()
    local mock_run = mock.func()
    mock.patch(b, 'run', mock_run,
        function()
              local spos = {x=11, y=20, z=30}
              local epos = {x=10, y=21, z=31}
              b.place(spos, epos, 'imaname')
              expect.eq(1, mock_run.call_count)
              expect.table_eq({'2', '2', '2', 'imaname', 'place',
                               '--cursor=10,20,30'},
                              mock_run.call_args[1])
        end)
end

function test.get_filename()
    local opts = {name='a', split_strategy='none'}
    expect.eq('dfhack-config/blueprints/a.csv', b.get_filename(opts, 'dig', 1))

    opts = {name='a/', split_strategy='none'}
    expect.eq('dfhack-config/blueprints/a/a.csv', b.get_filename(opts, 'dig', 1))

    opts = {name='a', split_strategy='phase'}
    expect.eq('dfhack-config/blueprints/a-1-dig.csv', b.get_filename(opts, 'dig', 1))

    opts = {name='a/', split_strategy='phase'}
    expect.eq('dfhack-config/blueprints/a/a-5-dig.csv', b.get_filename(opts, 'dig', 5))

    expect.error_match('could not parse basename', function()
            b.get_filename({name='', split_strategy='none'})
        end)
end
