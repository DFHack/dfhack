-- DFHack developer test harness
--@ module = true

local expect = require 'test.expect'
local json = require 'json'
local script = require 'gui.script'
local utils = require 'utils'

local help_text =
[====[

test
====

Run DFHack tests.

Usage:

    test [<options>] [<done_command>]

If a done_command is specified, it will be run after the tests complete.

Options:

    -h, --help      display this help message and exit.
    -d, --test_dir  specifies which directory to look in for tests. defaults to
                    the "hack/scripts/test" folder in your DF installation.
    -m, --modes     only run tests in the given comma separated list of modes.
                    valid modes are 'none' (test can be run on any screen) and
                    'title' (test must be run on the DF title screen). if not
                    specified, no modes are filtered.
    -r, --resume    skip tests that have already been run. remove the
                    test_status.json file to reset the record.
    -t, --tests     only run tests that match one of the comma separated list of
                    patterns. if not specified, no tests are filtered.

Examples:

    test                 runs all tests
    test -r              runs all tests that haven't been run before
    test -m none         runs tests that don't need the game to be in a
                         specific mode
    test -t quickfort    runs quickfort tests
    test -d /path/to/dfhack-scripts/repo/test
                         runs tests in your dev scripts repo

Default values for the options may be set in a file named test_config.json in
your DF folder. Options with comma-separated values should be written as json
arrays. For example:

    {
        "test_dir": "/home/myk/src/dfhack-scripts/test",
        "modes": [ "none" ],
        "tests": [ "quickfort", "devel" ],
        "done_command": "devel/luacov -c"
    }

]====]

local CONFIG_FILE = 'test_config.json'
local STATUS_FILE = 'test_status.json'
local TestStatus = {
    PENDING = 'pending',
    PASSED = 'passed',
    FAILED = 'failed',
}

local VALID_MODES = utils.invert{'none', 'title', 'fortress'}

local function delay(frames)
    frames = frames or 1
    script.sleep(frames, 'frames')
end

local function clean_require(module)
    -- wrapper around require() - forces a clean load of every module to ensure
    -- that modules checking for dfhack.internal.IN_TEST at load time behave
    -- properly
    if package.loaded[module] then
        reload(module)
    end
    return require(module)
end

-- forces clean load of scripts directly or indirectly included from the test
-- file. we use our own scripts table instead of the one in dfhack.internal so
-- we don't affect the state scripts that are used outside the test harness.
local test_scripts = {is_test_scripts=true}
local test_envvars = {}
local function clean_reqscript(name)
    local path = dfhack.findScript(name)
    if test_scripts[path] then return test_scripts[path].env end
    local _, env = dfhack.run_script_with_env(
        test_envvars,
        name,
        {
            scripts=test_scripts,
            module=true,
            module_strict=true
        })
    return env
end
test_envvars.require = clean_require
test_envvars.reqscript = clean_reqscript

local function ensure_title_screen()
    if df.viewscreen_titlest:is_instance(dfhack.gui.getCurViewscreen()) then
        return
    end
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
        error('Could not find title screen')
    end
end

local MODE_NAVIGATE_FNS = {
    none = function() end,
    title = ensure_title_screen,
}

local function load_test_config(config_file)
    local config = {}
    if dfhack.filesystem.isfile(config_file) then
        config = json.decode_file(config_file)
    end

    if not config.test_dir then
        config.test_dir = dfhack.getHackPath() .. 'scripts/test'
    end

    return config
end

local function build_test_env()
    local env = {
        test = utils.OrderedTable(),
        config = {
            mode = 'none',
        },
        expect = {},
        delay = delay,
        require = clean_require,
        reqscript = clean_reqscript,
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

local function get_test_files(test_dir)
    local files = {}
    print('Loading tests from ' .. test_dir)
    for _, entry in ipairs(dfhack.filesystem.listdir_recursive(test_dir)) do
        if not entry.isdir then
            table.insert(files, entry.path)
        end
    end
    table.sort(files)
    return files
end

local function load_test_status()
    if dfhack.filesystem.isfile(STATUS_FILE) then
        return json.decode_file(STATUS_FILE)
    end
end

local function save_test_status(status)
    json.encode_file(status, STATUS_FILE)
end

local function finish_tests(done_command)
    dfhack.internal.IN_TEST = false
    if done_command and #done_command > 0 then
        dfhack.run_command(done_command)
    end
end

local function load_tests(file, tests)
    local short_filename = file:sub((file:find('test') or -4)+5, -1)
    print('Loading file: ' .. short_filename)
    local env, env_private = build_test_env()
    local code, err = loadfile(file, 't', env)
    if not code then
        dfhack.printerr('Failed to load file: ' .. tostring(err))
        return false
    else
        dfhack.internal.IN_TEST = true
        local ok, err = pcall(code)
        dfhack.internal.IN_TEST = false
        if not ok then
            dfhack.printerr('Error when running file: ' .. tostring(err))
            return false
        else
            if not VALID_MODES[env.config.mode] then
                dfhack.printerr('Invalid config.mode: ' .. tostring(env.config.mode))
                return false
            end
            for name, test_func in pairs(env.test) do
                local test_data = {
                    full_name = short_filename .. ':' .. name,
                    func = test_func,
                    private = env_private,
                    config = env.config,
                }
                test_data.name = test_data.full_name:gsub('test/', ''):gsub('.lua', '')
                table.insert(tests, test_data)
            end
        end
    end
    return true
end

local function sort_tests(tests)
    -- to make sort stable
    local test_index = utils.invert(tests)
    table.sort(tests, function(a, b)
        if a.config.mode ~= b.config.mode then
            return VALID_MODES[a.config.mode] < VALID_MODES[b.config.mode]
        else
            return test_index[a] < test_index[b]
        end
    end)
end

local function run_test(test, status, counts)
    test.private.checks = 0
    test.private.checks_ok = 0
    counts.tests = counts.tests + 1
    dfhack.internal.IN_TEST = true
    local ok, err = pcall(test.func)
    dfhack.internal.IN_TEST = false
    local passed = false
    if not ok then
        dfhack.printerr('test errored: ' .. test.name .. ': ' .. tostring(err))
    elseif test.private.checks ~= test.private.checks_ok then
        dfhack.printerr('test failed: ' .. test.name)
    else
        print('test passed: ' .. test.name)
        passed = true
        counts.tests_ok = counts.tests_ok + 1
    end
    counts.checks = counts.checks + (tonumber(test.private.checks) or 0)
    counts.checks_ok = counts.checks_ok + (tonumber(test.private.checks_ok) or 0)
    return passed
end

local function get_tests(test_files, counts)
    local tests = {}
    for _, file in ipairs(test_files) do
        if not load_tests(file, tests) then
            counts.file_errors = counts.file_errors + 1
        end
    end
    return tests
end

local function filter_tests(tests, config)
    if config.tests or config.modes then
        for _,filter in ipairs({{'tests', 'name pattern'},{'modes', 'mode'}}) do
            if config[filter[1]] and #config[filter[1]] > 0 then
                print(string.format('Filtering tests by %s:', filter[2]))
                for _,v in ipairs(config[filter[1]]) do
                    print(string.format('  %s', v))
                end
            end
        end
        local orig_length = #tests
        for i = #tests, 1, -1 do
            local remove = false
            if config.modes then
                remove = true
                -- allow test if it matches any of the given modes
                for _, mode in pairs(config.modes) do
                    if tests[i].config.mode == mode then
                        remove = false
                        break
                    end
                end
            end
            if config.tests and not remove then
                remove = true
                -- allow test if it matches any of the given patterns
                for _, pattern in pairs(config.tests) do
                    if tests[i].name:match(pattern) then
                        remove = false
                        break
                    end
                end
            end
            if remove then table.remove(tests, i) end
        end
        print('Selected tests: ' .. #tests .. '/' .. orig_length)
    end

    local status = {}
    if config.resume then
        status = load_test_status() or status
        for i = #tests, 1, -1 do
            local test = tests[i]
            if not status[test.full_name] then
                status[test.full_name] = TestStatus.PENDING
            elseif status[test.full_name] ~= TestStatus.PENDING then
                print(('skipping test: %s: state = %s'):format(
                        test.name, status[test.full_name]))
                table.remove(tests, i)
            end
        end
    end

    sort_tests(tests)
    return status
end

local function run_tests(tests, status, counts)
    print(('Running %d tests'):format(#tests))
    for _, test in pairs(tests) do
        MODE_NAVIGATE_FNS[test.config.mode]()
        local passed = run_test(test, status, counts)
        status[test.full_name] = passed and TestStatus.PASSED or TestStatus.FAILED
        save_test_status(status)
    end

    local function print_summary_line(ok, message)
        local print_fn = print
        if not ok then
            status['*'] = TestStatus.FAILED
            print_fn = dfhack.printerr
        end
        print_fn(message)
    end

    status['*'] = status['*'] or TestStatus.PASSED
    print('\nTest summary:')
    print_summary_line(counts.tests_ok == counts.tests,
        ('%d/%d tests passed'):format(counts.tests_ok, counts.tests))
    print_summary_line(counts.checks_ok == counts.checks,
        ('%d/%d checks passed'):format(counts.checks_ok, counts.checks))
    print_summary_line(counts.file_errors == 0,
        ('%d test files failed to load'):format(counts.file_errors))

    save_test_status(status)
end

local function main(args)
    local help, resume, test_dir, mode_filter, test_filter =
            false, false, nil, {}, {}
    local other_args = utils.processArgsGetopt(args, {
            {'h', 'help', handler=function() help = true end},
            {'d', 'test_dir', hasArg=true,
            handler=function(arg) test_dir = arg end},
            {'m', 'modes', hasArg=true,
            handler=function(arg) mode_filter = arg:split(',') end},
            {'r', 'resume', handler=function() resume = true end},
            {'t', 'tests', hasArg=true,
            handler=function(arg) test_filter = arg:split(',') end},
        })

    if help then print(help_text) return end

    local done_command = table.concat(other_args, ' ')
    local config = load_test_config(CONFIG_FILE)

    -- override config with any params specified on the commandline
    if test_dir then config.test_dir = test_dir end
    if resume then config.resume = true end
    if #mode_filter > 0 then config.modes = mode_filter end
    if #test_filter > 0 then config.tests = test_filter end
    if #done_command > 0 then config.done_command = done_command end

    if not dfhack.filesystem.isdir(config.test_dir) then
        qerror(('Invalid test folder: "%s"'):format(config.test_dir))
    end

    local counts = {
        tests = 0,
        tests_ok = 0,
        checks = 0,
        checks_ok = 0,
        file_errors = 0,
    }

    local test_files = get_test_files(config.test_dir)
    local tests = get_tests(test_files, counts)
    local status = filter_tests(tests, config)

    script.start(function()
        dfhack.call_with_finalizer(1, true,
                              finish_tests, config.done_command,
                              run_tests, tests, status, counts)
    end)
end

if not dfhack_flags.module then
    main({...})
end
