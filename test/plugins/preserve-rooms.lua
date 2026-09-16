config.mode = 'fortress'
config.target = 'preserve-rooms'

local pr = require('plugins.preserve-rooms')

-- feature enabled state persists across sessions; restore it
local saved_features

config.wrapper = function(test_fn)
    local features = pr.preserve_rooms_getState()
    saved_features = {}
    for k, v in pairs(features) do
        saved_features[k] = v
    end
    return dfhack.with_finalize(function()
        for k, v in pairs(saved_features) do
            pr.preserve_rooms_setFeature(v, k)
        end
    end, test_fn)
end

function test.status_lists_features()
    local output = dfhack.run_command_silent('preserve-rooms', 'status')
    expect.str_find('track%-missions', output)
    expect.str_find('track%-roles', output)
end

function test.enable_disable_feature()
    pr.preserve_rooms_setFeature(false, 'track-missions')
    dfhack.run_command_silent('preserve-rooms', 'enable', 'track-missions')
    local features = pr.preserve_rooms_getState()
    expect.true_(features['track-missions'])
    dfhack.run_command_silent('preserve-rooms', 'disable', 'track-missions')
    features = pr.preserve_rooms_getState()
    expect.false_(features['track-missions'])
end

function test.enable_unknown_feature_errors()
    local output, status = dfhack.run_command_silent('preserve-rooms',
                                                   'enable', 'bogus-feature')
    expect.true_(output:find('unknown feature', 1, true) ~= nil)
end

function test.reset_feature()
    dfhack.run_command_silent('preserve-rooms', 'reset', 'track-roles')
    local output, status = dfhack.run_command_silent('preserve-rooms',
                                                   'reset', 'bogus-feature')
    expect.true_(output:find('unknown feature', 1, true) ~= nil)
end
