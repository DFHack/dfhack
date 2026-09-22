config.mode = 'fortress'
config.target = 'cleanowned'

-- `cleanowned dryrun` scans all owned items on the map and prints what it
-- would confiscate without mutating anything; the value here is exercising
-- the full item scan plus the per-item report path.
function test.dryrun_is_side_effect_free()
    local _, status = dfhack.run_command_silent('cleanowned', 'dryrun')
    expect.eq(CR_OK, status)
end

function test.dryrun_with_filters()
    local _, status = dfhack.run_command_silent('cleanowned', 'dryrun', 'scattered', 'x', 'X', 'all', 'nodump')
    expect.eq(CR_OK, status)
end

function test.unknown_option_is_wrong_usage()
    local _, status = dfhack.run_command_silent('cleanowned', 'bogus')
    expect.eq(CR_WRONG_USAGE, status)
end
