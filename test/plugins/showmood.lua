config.mode = 'fortress'
config.target = 'showmood'

function test.reports_no_active_mood()
    -- the test fort has no strange mood in progress
    local output, status = dfhack.run_command_silent('showmood')
    expect.eq(CR_OK, status)
    expect.str_find('No strange moods currently active', output)
end

function test.arg_is_wrong_usage()
    local _, status = dfhack.run_command_silent('showmood', 'bogus')
    expect.eq(CR_WRONG_USAGE, status)
end
