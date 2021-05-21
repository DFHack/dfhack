local b = require('plugins.blueprint')

-- also covers code shared between parse_gui_commandline and parse_commandline
function test.parse_gui_commandline()
    local opts = {}
    b.parse_gui_commandline(opts, {})
    expect.table_eq({auto_phase=true, name='blueprint'}, opts)

    opts = {}
    b.parse_gui_commandline(opts, {'help'})
    expect.table_eq({help=true}, opts)

    opts = {}
    b.parse_gui_commandline(opts, {'--help'})
    expect.table_eq({help=true}, opts)

    opts = {}
    b.parse_gui_commandline(opts, {'-h'})
    expect.table_eq({help=true}, opts)

    opts = {}
    b.parse_gui_commandline(opts, {'--cursor=1,2,3'})
    expect.table_eq({auto_phase=true, name='blueprint', start={x=1,y=2,z=3}},
                    opts)

    opts = {}
    expect.error_match('invalid argument',
                       function() b.parse_gui_commandline(
                                        opts, {'--cursor=-1,2,3'}) end,
                       'negative coordinate')

    opts = {}
    expect.error_match('invalid argument',
                       function() b.parse_gui_commandline(
                                        opts, {'--cursor=1,b,3'}) end,
                       'non-numeric coordinate')

    opts = {}
    b.parse_gui_commandline(opts, {'imaname'})
    expect.table_eq({auto_phase=true, name='imaname'}, opts)

    opts = {}
    expect.error_match('invalid basename',
                       function() b.parse_gui_commandline(opts, {''}) end)

    opts = {}
    b.parse_gui_commandline(opts, {'imaname', 'dig', 'query'})
    expect.table_eq({auto_phase=false, name='imaname', dig=true, query=true},
                    opts)

    opts = {}
    expect.error_match('unknown phase',
                       function() b.parse_gui_commandline(
                                        opts, {'imaname', 'garbagephase'}) end)
end

function test.parse_commandline()
    local opts = {}
    b.parse_commandline(opts, '1', '2')
    expect.table_eq({auto_phase=true,name='blueprint',width=1,height=2,depth=1},
                    opts)

    opts = {}
    b.parse_commandline(opts, '1', '2', '3')
    expect.table_eq({auto_phase=true,name='blueprint',width=1,height=2,depth=3},
                    opts)

    opts = {}
    b.parse_commandline(opts, '1', '2', '-3')
    expect.table_eq({auto_phase=true,name='blueprint',width=1,height=2,depth=-3},
                    opts)

    opts = {}
    b.parse_commandline(opts, '1', '2', 'imaname')
    expect.table_eq({auto_phase=true,name='imaname',width=1,height=2,depth=1},
                    opts)

    opts = {}
    b.parse_commandline(opts, '1', '2', '10imaname')
    expect.table_eq({auto_phase=true,name='10imaname',width=1,height=2,depth=1},
                    opts, 'invalid depth is considered a basename')

    opts = {}
    b.parse_commandline(opts, '1', '2', '-10imaname')
    expect.table_eq({auto_phase=true,name='-10imaname',width=1,height=2,depth=1},
                    opts, 'invalid negative depth is considered a basename')

    opts = {}
    b.parse_commandline(opts, '1', '2', '3', 'imaname')
    expect.table_eq({auto_phase=true,name='imaname',width=1,height=2,depth=3},
                    opts)

    opts = {}
    expect.error_match('invalid width or height',
                       function() b.parse_commandline(opts) end,
                       'missing width')

    opts = {}
    expect.error_match('invalid width or height',
                       function() b.parse_commandline(opts, '10') end,
                       'missing height')

    opts = {}
    expect.error_match('invalid width or height',
                       function() b.parse_commandline(opts, '0') end,
                       'zero height')

    opts = {}
    expect.error_match('invalid width or height',
                       function() b.parse_commandline(opts, 'hi') end,
                       'invalid width')

    opts = {}
    expect.error_match('invalid width or height',
                       function() b.parse_commandline(opts, '10', 'hi') end,
                       'invalid height')

    opts = {}
    expect.error_match('invalid depth',
                       function() b.parse_commandline(opts, '1', '2', '0') end,
                       'zero depth')
end

function test.do_phase_positive_dims()
    local mock_run = mock.func()
    mock.patch(b, 'run', mock_run,
        function()
              local spos = {x=10, y=20, z=30}
              local epos = {x=11, y=21, z=31}
              b.query(spos, epos, 'imaname')
              expect.eq(1, mock_run.call_count)
              expect.table_eq({'2', '2', '2', 'imaname', 'query',
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
              b.query(spos, epos, 'imaname')
              expect.eq(1, mock_run.call_count)
              expect.table_eq({'2', '2', '-2', 'imaname', 'query',
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
              b.query(spos, epos, 'imaname')
              expect.eq(1, mock_run.call_count)
              expect.table_eq({'2', '2', '2', 'imaname', 'query',
                               '--cursor=10,20,30'},
                              mock_run.call_args[1])
        end)
end
