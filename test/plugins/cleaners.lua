config.mode = 'fortress'
config.target = 'cleaners'

-- the plugin's command is `clean` (the plugin binary is named cleaners).
-- each subcommand prints a summary only when it actually removed something,
-- so on a clean map the output is empty but the status is still CR_OK.

local function expect_clean_ok(output, status, pattern)
    expect.eq(CR_OK, status)
    if #output > 0 then
        expect.str_find(pattern, output)
    end
end

function test.clean_map()
    local output, status = dfhack.run_command_silent('clean', 'map')
    expect_clean_ok(output, status, 'Cleaned %d+ of %d+ map blocks%.')
end

function test.clean_units()
    local output, status = dfhack.run_command_silent('clean', 'units')
    expect_clean_ok(output, status, 'Removed %d+ contaminants from %d+ creatures%.')
end

function test.clean_items()
    local output, status = dfhack.run_command_silent('clean', 'items')
    expect_clean_ok(output, status, 'Removed %d+ contaminants from %d+ items%.')
end

function test.clean_plants()
    local output, status = dfhack.run_command_silent('clean', 'plants')
    expect_clean_ok(output, status, 'Removed %d+ contaminants from %d+ plants%.')
end

function test.clean_all()
    local _, status = dfhack.run_command_silent('clean', 'all')
    expect.eq(CR_OK, status)
end

function test.no_args_is_wrong_usage()
    local _, status = dfhack.run_command_silent('clean')
    expect.eq(CR_WRONG_USAGE, status)
end

function test.unknown_option_is_wrong_usage()
    local _, status = dfhack.run_command_silent('clean', 'bogus')
    expect.eq(CR_WRONG_USAGE, status)
end
