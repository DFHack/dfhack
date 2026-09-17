config.mode = 'fortress'
config.target = 'nestboxes'

-- burrow config is persisted to the site; restore whatever was set
local saved_burrow_name

config.wrapper = function(test_fn)
    local output = dfhack.run_command_silent('nestboxes')
    saved_burrow_name = output:match('inside burrow: ([^\n]+)')
    return dfhack.with_finalize(function()
        if saved_burrow_name then
            dfhack.run_command_silent('nestboxes', 'burrow',
                                      saved_burrow_name)
        else
            dfhack.run_command_silent('nestboxes', 'all')
        end
    end, test_fn)
end

function test.status_reports_protection_scope()
    local output = dfhack.run_command_silent('nestboxes')
    expect.true_(output:find('nestboxes is', 1, true) ~= nil)
    expect.true_(output:find('protecting', 1, true) ~= nil
                 or output:find('disabled', 1, true) ~= nil)
end

function test.burrow_unknown_name_clears_scope()
    dfhack.run_command_silent('nestboxes', 'burrow',
                              'dfhack-test-nonexistent-burrow')
    local output = dfhack.run_command_silent('nestboxes')
    expect.true_(output:find('inside burrow', 1, true) == nil)
end

function test.all_clears_burrow_scope()
    dfhack.run_command_silent('nestboxes', 'all')
    local output = dfhack.run_command_silent('nestboxes')
    expect.true_(output:find('inside burrow', 1, true) == nil)
end

function test.unknown_command_shows_usage()
    local output, status = dfhack.run_command_silent('nestboxes', 'bogus')
    expect.true_(output:find('nestboxes', 1, true) ~= nil or status ~= 0)
end
