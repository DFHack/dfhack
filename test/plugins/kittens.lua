config.mode = 'fortress'
config.target = 'kittens'

-- kittens is a developer plugin and is not built unless BUILD_DEV_PLUGINS is
-- on, so these tests are skipped on builds where throwtest is unavailable
local function has_throwtest()
    for _, plugin in ipairs(dfhack.internal.listPlugins()) do
        if plugin == 'kittens' then
            for _, command in ipairs(dfhack.internal.listCommands(plugin)) do
                if command == 'throwtest' then
                    return true
                end
            end
        end
    end
    return false
end

function test.exception_returns_failure()
    if not has_throwtest() then return end
    local output, status = dfhack.run_command_silent('throwtest')
    expect.eq(CR_FAILURE, status)
    expect.str_find('test exception', output)
end

function test.nonstd_exception_returns_failure()
    if not has_throwtest() then return end
    local output, status = dfhack.run_command_silent('throwtest nonstd')
    expect.eq(CR_FAILURE, status)
    expect.str_find("Exception in kittens command", output)
end
