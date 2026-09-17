config.mode = 'fortress'
config.target = 'spectate'

require('plugins.spectate')

function test.status_shows_settings()
    local output = dfhack.run_command_silent('spectate')
    expect.true_(output:find('auto%-unpause') ~= nil
                 or output:find('follow%-seconds') ~= nil)
end

function test.set_boolean()
    dfhack.run_command_silent('spectate', 'set', 'auto-unpause', 'true')
    local output = dfhack.run_command_silent('spectate')
    expect.str_find('auto%-unpause.-true', output)
    dfhack.run_command_silent('spectate', 'set', 'auto-unpause', 'false')
    output = dfhack.run_command_silent('spectate')
    expect.str_find('auto%-unpause.-false', output)
end

function test.set_number()
    dfhack.run_command_silent('spectate', 'set', 'follow-seconds', '42')
    local output = dfhack.run_command_silent('spectate')
    expect.str_find('follow%-seconds.-42', output)
    dfhack.run_command_silent('spectate', 'set', 'follow-seconds', '10')
end

function test.set_toggle_value()
    dfhack.run_command_silent('spectate', 'set', 'auto-unpause', 'false')
    dfhack.run_command_silent('spectate', 'set', 'auto-unpause', 'toggle')
    local output = dfhack.run_command_silent('spectate')
    expect.str_find('auto%-unpause.-true', output)
end

function test.set_unknown_option()
    local output, status = dfhack.run_command_silent('spectate', 'set',
                                                   'bogus-key', '1')
    expect.true_(output:find('unknown option', 1, true) ~= nil)
end

function test.set_missing_key()
    local output, status = dfhack.run_command_silent('spectate', 'set')
    expect.true_(output:find('missing key', 1, true) ~= nil)
end

function test.unknown_command_shows_usage()
    local output, status = dfhack.run_command_silent('spectate', 'boguscmd')
    expect.true_(output:find('status', 1, true) ~= nil
                 or output:find('Usage', 1, true) ~= nil
                 or output:find('usage', 1, true) ~= nil)
end
