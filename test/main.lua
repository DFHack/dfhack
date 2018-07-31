local script = require 'gui.script'
local utils = require 'utils'

local args = {...}
local done_command = args[1]

expect = {}
function expect.true_(value, comment)
    return not not value, comment, 'expected true'
end
function expect.false_(value, comment)
    return not value, comment, 'expected false'
end
function expect.eq(a, b, comment)
    return a == b, comment, ('%s ~= %s'):format(a, b)
end
function expect.ne(a, b, comment)
    return a ~= b, comment, ('%s == %s'):format(a, b)
end
function expect.error(func, ...)
    local ok, ret = pcall(func, ...)
    if ok then
        return false, 'no error raised by function call'
    else
        return true
    end
end

function delay(frames)
    frames = frames or 1
    script.sleep(frames, 'frames')
end

function build_test_env()
    local env = {
        test = utils.OrderedTable(),
        expect = {},
        delay = delay,
    }
    local private = {
        checks = 0,
        checks_ok = 0,
    }
    for name, func in pairs(expect) do
        env.expect[name] = function(...)
            private.checks = private.checks + 1
            local ret = {func(...)}
            local ok = table.remove(ret, 1)
            local msg = ''
            for _, part in pairs(ret) do
                if part then
                    msg = msg .. ': ' .. tostring(part)
                end
            end
            msg = msg:sub(3) -- strip leading ': '
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

    print('Looking for title screen...')
    for i = 0, 100 do
        local scr = dfhack.gui.getCurViewscreen()
        if df.viewscreen_titlest:is_instance(scr) then
            print('Found title screen')
            break
        else
            scr:feed_key(df.interface_key.LEAVESCREEN)
            delay(10)
        end
    end
    if not df.viewscreen_titlest:is_instance(dfhack.gui.getCurViewscreen()) then
        qerror('Could not find title screen')
    end

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

script.start(main)
