config.mode = 'fortress'
config.target = 'timestream'

local timestream = require('plugins.timestream')

local saved_fps = timestream.timestream_getFps()

config.wrapper = function(test_fn)
    return dfhack.with_finalize(function()
        timestream.timestream_setFps(saved_fps)
    end, test_fn)
end

function test.status_and_unknown_command()
    expect.true_(timestream.parse_commandline({}))
    expect.true_(timestream.parse_commandline({'status'}))
    expect.false_(timestream.parse_commandline({'help'}))
    expect.false_(timestream.parse_commandline({'bogus'}))
end

function test.set_fps_roundtrip()
    expect.true_(timestream.parse_commandline({'set', 'fps', '60'}))
    expect.eq(60, timestream.timestream_getFps())
end

function test.set_fps_clamps_to_minimum()
    -- the C++ layer clamps the target to at least 10 fps
    expect.true_(timestream.parse_commandline({'set', 'fps', '5'}))
    expect.eq(10, timestream.timestream_getFps())
end

function test.set_fps_rejects_bad_args()
    expect.error_match('must specify setting and value',
        function() timestream.parse_commandline({'set'}) end)
    expect.error_match('must specify setting and value',
        function() timestream.parse_commandline({'set', 'fps'}) end)
    expect.error_match('must specify setting and value',
        function() timestream.parse_commandline({'set', 'bogus', '60'}) end)
    expect.error_match('must specify setting and value',
        function() timestream.parse_commandline({'set', 'fps', 'fast'}) end)
end

function test.reset_restores_default()
    timestream.timestream_setFps(43)
    expect.true_(timestream.parse_commandline({'reset'}))
    expect.ne(43, timestream.timestream_getFps())
end
