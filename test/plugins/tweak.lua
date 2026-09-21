config.mode = 'fortress'
config.target = 'tweak'

local tweak = require('plugins.tweak')

-- tweak enabled state is runtime-only; ensure test hooks end disabled
local touched_tweaks

config.wrapper = function(test_fn)
    touched_tweaks = {}
    return dfhack.with_finalize(function()
        for name in pairs(touched_tweaks) do
            dfhack.run_command_silent('tweak', name, 'disable', 'quiet')
        end
    end, test_fn)
end

function test.status_lists_tweaks()
    local status = tweak.tweak_get_status()
    expect.ne(nil, status['eggs-fertile'])
    expect.ne(nil, status['fast-heat'])
end

function test.list_output()
    local output = dfhack.run_command_silent('tweak')
    expect.str_find('eggs%-fertile', output)
    expect.str_find('enabled', output)
end

function test.enable_disable_tweak()
    touched_tweaks['eggs-fertile'] = true
    local output = dfhack.run_command_silent('tweak', 'eggs-fertile')
    expect.str_find('Enabled tweak eggs%-fertile', output)
    expect.true_(tweak.tweak_get_status()['eggs-fertile'])
    output = dfhack.run_command_silent('tweak', 'eggs-fertile', 'disable')
    expect.str_find('Disabled tweak eggs%-fertile', output)
    expect.false_(tweak.tweak_get_status()['eggs-fertile'])
end

function test.unknown_tweak_errors()
    local output, status = dfhack.run_command_silent('tweak', 'bogus-tweak')
    expect.str_find('Unrecognized tweak', output)
end
