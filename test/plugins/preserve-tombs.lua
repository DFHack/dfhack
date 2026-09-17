config.mode = 'fortress'
config.target = 'preserve-tombs'

-- enabled state persists; capture it and restore afterwards
local was_enabled

config.wrapper = function(test_fn)
    local output = dfhack.run_command_silent('preserve-tombs')
    was_enabled = output:find('currently enabled', 1, true) ~= nil
    return dfhack.with_finalize(function()
        dfhack.run_command_silent(was_enabled and 'enable' or 'disable',
                                  'preserve-tombs')
    end, test_fn)
end

function test.status_reports_state()
    dfhack.run_command_silent('disable', 'preserve-tombs')
    local output = dfhack.run_command_silent('preserve-tombs')
    expect.str_find('preserve%-tombs is currently disabled', output)
end

function test.now_requires_enable()
    dfhack.run_command_silent('disable', 'preserve-tombs')
    local output = dfhack.run_command_silent('preserve-tombs', 'now')
    expect.str_find('Cannot update', output)
end

function test.enable_then_now()
    dfhack.run_command_silent('enable', 'preserve-tombs')
    local output = dfhack.run_command_silent('preserve-tombs')
    expect.str_find('preserve%-tombs is currently enabled', output)
    output = dfhack.run_command_silent('preserve-tombs', 'now')
    expect.str_find('Updated tomb assignments', output)
end
