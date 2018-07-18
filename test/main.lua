local args = {...}
local done_command = args[1]

expect = {}
function expect.true_(value)
    return not not value
end
function expect.false_(value)
    return not value
end
function expect.eq(a, b)
    return a == b
end
function expect.ne(a, b)
    return a ~= b
end
function expect.error(func, ...)
    local ok, ret = pcall(func, ...)
    if ok then
        return false, 'no error raised by function call'
    else
        return true
    end
end

function build_test_env()
    local env = {
        test = {},
        expect = {},
    }
    local private = {
        checks = 0,
        checks_ok = 0,
    }
    for name, func in pairs(expect) do
        env.expect[name] = function(...)
            private.checks = private.checks + 1
            local ok, msg = func(...)
            if ok then
                private.checks_ok = private.checks_ok + 1
            else
                dfhack.printerr('Check failed! ' .. (msg or '(no message)'))
                local info = debug.getinfo(2)
                dfhack.printerr(('  at %s:%d'):format(info.short_src, info.currentline))
                print('')
            end
        end
    end
    setmetatable(env, {__index = _G})
    return env, private
end

function get_test_files()
    local files = {}
    for _, entry in ipairs(dfhack.filesystem.listdir_recursive(dfhack.getHackPath() .. 'test')) do
        if not entry.isdir and not entry.path:match('main.lua') then
            table.insert(files, entry.path)
        end
    end
    table.sort(files)
    return files
end

function set_test_stage(stage)
    local f = io.open('test_stage.txt', 'w')
    f:write(stage)
    f:close()
end

function finish_tests(ok)
    if ok then
        print('Tests finished')
    else
        dfhack.printerr('Tests failed!')
    end
    if done_command then
        dfhack.run_command(done_command)
    end
end

function main()
    local files = get_test_files()

    local counts = {
        tests = 0,
        tests_ok = 0,
        checks = 0,
        checks_ok = 0,
        file_errors = 0,
    }
    local passed = true

    print('Running tests')
    for _, file in ipairs(files) do
        print('Running file: ' .. file:sub(file:find('test'), -1))
        local env, env_private = build_test_env()
        local code, err = loadfile(file, 't', env)
        if not code then
            passed = false
            counts.file_errors = counts.file_errors + 1
            dfhack.printerr('Failed to load file: ' .. tostring(err))
        else
            local ok, err = pcall(code)
            if not ok then
                passed = false
                counts.file_errors = counts.file_errors + 1
                dfhack.printerr('Error when running file: ' .. tostring(err))
            else
                for name, test in pairs(env.test) do
                    env_private.checks = 0
                    env_private.checks_ok = 0
                    counts.tests = counts.tests + 1
                    local ok, err = pcall(test)
                    if not ok then
                        dfhack.printerr('test errored: ' .. name .. ': ' .. tostring(err))
                        passed = false
                    elseif env_private.checks ~= env_private.checks_ok then
                        dfhack.printerr('test failed: ' .. name)
                        passed = false
                    else
                        print('test passed: ' .. name)
                        counts.tests_ok = counts.tests_ok + 1
                    end
                    counts.checks = counts.checks + (tonumber(env_private.checks) or 0)
                    counts.checks_ok = counts.checks_ok + (tonumber(env_private.checks_ok) or 0)
                end
            end
        end
    end

    print('\nTest summary:')
    print(('%d/%d tests passed'):format(counts.tests_ok, counts.tests))
    print(('%d/%d checks passed'):format(counts.checks_ok, counts.checks))
    print(('%d test files failed to load'):format(counts.file_errors))

    set_test_stage(passed and 'done' or 'fail')
    finish_tests(passed)
end

local check_count = 0
function check_title()
    local scr = dfhack.gui.getCurViewscreen()
    if df.viewscreen_titlest:is_instance(scr) then
        print('Found title screen')
        main()
    else
        check_count = check_count + 1
        if check_count > 100 then
            qerror('Could not find title screen')
        end
        scr:feed_key(df.interface_key.LEAVESCREEN)
        dfhack.timeout(10, 'frames', check_title)
    end
end

print('Looking for title screen...')
check_title()
