config.mode = 'fortress'
config.target = 'kittens'

function test.exception_returns_failure()
    local output, status = dfhack.run_command_silent('throwtest')
    expect.eq(CR_FAILURE, status)
    expect.str_find('test exception', output)
end

function test.nonstd_exception_returns_failure()
    local output, status = dfhack.run_command_silent('throwtest nonstd')
    expect.eq(CR_FAILURE, status)
    expect.str_find("Exception in kittens command", output)
end
