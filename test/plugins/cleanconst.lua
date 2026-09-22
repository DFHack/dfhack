config.mode = 'fortress'
config.target = 'cleanconst'

-- cleanconst scans all items for bogus construction flags. On a healthy map
-- it cleans nothing but still prints its summary; the value here is
-- exercising the full item scan for crashes.
function test.clean_run()
    local output, status = dfhack.run_command_silent('cleanconst')
    expect.eq(CR_OK, status)
    expect.str_find('Done%. %d+ construction items cleaned up%.', output)
end
