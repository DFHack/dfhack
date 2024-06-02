-- DFHack developer test harness
--@ module = true

local expect = require('test_util.expect')
local gui = require('gui')
local helpdb = require('helpdb')
local json = require('json')
local mock = require('test_util.mock')
local script = require('gui.script')
local utils = require('utils')

local help_text =
[====[

test
====

Tags: dev

Command: "test"

    Run DFHack regression tests.

Discover DFHack functionality that has broken due to recent changes in DF or DFHack.

Usage
-----

    test [<options>] [<done_command>]

If a done_command is specified, it will be run after the tests complete.

Options
-------

-d, --test_dir  specifies which directory to look in for tests. defaults to
                the "hack/scripts/test" folder in your DF installation.
-m, --modes     only run tests in the given comma separated list of modes.
                see the next section for a list of valid modes. if not
                specified, the tests are not filtered by modes.
-r, --resume    skip tests that have already been run. remove the
                test_status.json file to reset the record.
-s, --save_dir  the save folder to load for "fortress" mode tests. this
                save is only loaded if a fort is not already loaded when
                a "fortress" mode test is run. if not specified, defaults to
                'region1'.
-t, --tests     only run tests that match one of the comma separated list of
                patterns. if not specified, no tests are filtered and all tessts
                are run.

Modes
-----

none      the test can be run on any screen
title     the test must be run on the DF title screen. note that if the game
          has a map loaded, "title" mode tests cannot be run
fortress  the test must be run while a map is loaded. if the game is
          currently on the title screen, the save specified by the save_dir
          parameter will be loaded.

Examples
--------

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
local DF_STATE_FILE = 'test_df_state.json'
local TestStatus = {
    PENDING = 'pending',
    PASSED = 'passed',
    FAILED = 'failed',
}

local function delay(frames)
    frames = frames or 1
    script.sleep(frames, 'frames')
end

-- Will call the predicate_fn every frame until it returns true. If it fails to
-- return true before timeout_frames have elapsed, throws an error. If
-- timeout_frames is not specified, defaults to 100.
local function delay_until(predicate_fn, timeout_frames)
    timeout_frames = tonumber(timeout_frames) or 100
    repeat
        delay()
        if predicate_fn() then return end
        timeout_frames = timeout_frames - 1
    until timeout_frames < 0
    error('timed out while waiting for predicate to return true')
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

-- clean_run_script and clean_reqscript force a clean load of scripts directly
-- or indirectly included from the test file. we use our own scripts table
-- instead of the one in dfhack.internal so we don't affect the state scripts
-- that are used outside the test harness.
local test_scripts = {}
local test_envvars = {}

-- clean_run_script is accessed via the dfhack table, not directly from the env.
-- therefore we use this function in wrap_test() below and not in test_envvars.
local function clean_run_script(name, ...)
    return dfhack.run_script_with_env(
        test_envvars,
        name,
        {scripts=test_scripts},
        ...)
end

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

local function is_title_screen()
    return dfhack.gui.matchFocusString('title/Default')
end

local function wait_for(ms, desc, predicate)
    local start_ms = dfhack.getTickCount()
    local prev_ms = start_ms
    while not predicate() do
        delay(10)
        local now_ms = dfhack.getTickCount()
        if now_ms - start_ms > ms then
            qerror(('%s took too long (timed out at %s)'):format(
                desc, dfhack.gui.getCurFocus(true)[1]))
        end
        if now_ms - prev_ms > 1000 then
            print(('Waiting for %s...'):format(desc))
            prev_ms = now_ms
        end
    end
end

-- This only handles pre-fortress-load screens. It will time out if the player
-- has already loaded a fortress or is in any screen that can't get to the title
-- screen by sending ESC keys.
local function ensure_title_screen()
    if df.viewscreen_dwarfmodest:is_instance(dfhack.gui.getDFViewscreen(true)) then
        qerror('Cannot reach title screen from loaded fort')
    end
    for i = 1, 100 do
        local scr = dfhack.gui.getCurViewscreen()
        if is_title_screen() then
            print('Found title screen')
            return
        end
        scr:feed_key(df.interface_key.LEAVESCREEN)
        delay(10)
        if i % 10 == 0 then print('Looking for title screen...') end
    end
    qerror(string.format('Could not find title screen (timed out at %s)',
                         dfhack.gui.getCurFocus(true)[1]))
end

local function is_fortress()
    return dfhack.gui.matchFocusString('dwarfmode/Default')
end

-- error out if we're not running in a CI environment
-- the tests may corrupt saves, and we don't want to unexpectedly ruin a real player save
-- this heuristic is not perfect, but it should be able to detect most cases
local function ensure_ci_save(scr)
    if #scr.savegame_header ~= 1
        or #scr.savegame_header_world ~= 1
        or not string.find(scr.savegame_header[0].fort_name, 'Dream')
    then
        qerror('Unexpected test save in slot 0; please manually load a fort for ' ..
            'running fortress mode tests. note that tests may alter or corrupt the ' ..
            'fort! Do not save after running tests.')
    end
end

local function click_top_title_button(scr)
    local sw, sh = dfhack.screen.getWindowSize()
    df.global.gps.mouse_x = sw // 2
    df.global.gps.precise_mouse_x = df.global.gps.mouse_x * df.global.gps.tile_pixel_x
    if sh < 60 then
        df.global.gps.mouse_y = 25
    else
        df.global.gps.mouse_y = (sh // 2) + 3
    end
    df.global.gps.precise_mouse_y = df.global.gps.mouse_y * df.global.gps.tile_pixel_y
    gui.simulateInput(scr, '_MOUSE_L')
end

local function load_first_save(scr)
    if #scr.savegame_header == 0 then
        qerror('no savegames available to load')
    end

    click_top_title_button(scr)
    wait_for(1000, 'world list', function()
        return scr.mode == 2
    end)
    click_top_title_button(scr)
    wait_for(1000, 'savegame list', function()
        return scr.mode == 3
    end)
    click_top_title_button(scr)
    wait_for(1000, 'loadgame progress bar', function()
        return dfhack.gui.matchFocusString('loadgame')
    end)
end

-- Requires that a fortress game is already loaded or is ready to be loaded via
-- the "Continue active game" option in the title screen. Otherwise the function
-- will time out and/or exit with error.
local function ensure_fortress(config)
    for screen_timeout = 1,10 do
        if is_fortress() then
            print('Fortress map is loaded')
            -- pause the game (if it's not already paused)
            dfhack.gui.resetDwarfmodeView(true)
            return
        end
        local scr = dfhack.gui.getCurViewscreen()
        if dfhack.gui.matchFocusString('title/Default', scr) then
            print('Attempting to load the test fortress')
            -- TODO: reinstate loading of a specified save dir; for now
            -- just load the first possible save, which will at least let us
            -- run fortress tests in CI
            -- qerror()'s on falure
            -- dfhack.run_script('load-save', config.save_dir)
            ensure_ci_save(scr)
            load_first_save(scr)
        elseif not dfhack.gui.matchFocusString('loadgame', scr) then
            -- if we're not actively loading a game, hope we're in
            -- a screen where hitting ESC will get us to the game map
            -- or the title screen
            scr:feed_key(df.interface_key.LEAVESCREEN)
        end
        -- wait for current screen to change
        local prev_focus_string = dfhack.gui.getCurFocus()[1]
        wait_for(60000, 'screen change', function()
            return dfhack.gui.getCurFocus()[1] ~= prev_focus_string
        end)
    end
    qerror(string.format('Could not load fortress (timed out at %s)',
                         table.concat(dfhack.gui.getCurFocus(), ' ')))
end

local MODES = {
    none = {order=1, detect=function() return true end},
    title = {order=2, detect=is_title_screen, navigate=ensure_title_screen},
    fortress = {order=3, detect=is_fortress, navigate=ensure_fortress},
}

local function load_test_config(config_file)
    local config = {}
    if dfhack.filesystem.isfile(config_file) then
        config = json.decode_file(config_file)
    end

    if not config.test_dir then
        config.test_dir = dfhack.getHackPath() .. 'scripts/test'
    end

    if not config.save_dir then
        config.save_dir = 'region1'
    end

    return config
end

local function TestTable()
    local inner = utils.OrderedTable()
    local meta = copyall(getmetatable(inner))

    function meta:__newindex(k, v)
        if inner[k] then
            error('Attempt to overwrite existing test: ' .. k)
        elseif type(v) ~= 'function' then
            error('Attempt to define test as non-function: ' .. k .. ' = ' .. tostring(v))
        else
            inner[k] = v
        end
    end

    local self = {}
    setmetatable(self, meta)
    return self
end

-- we have to save and use the original dfhack.printerr here so our test harness
-- output doesn't trigger its own dfhack.printerr usage detection (see
-- detect_printerr below)
local orig_printerr = dfhack.printerr
local function wrap_expect(func, private, path)
    return function(...)
        private.checks = private.checks + 1
        local ret = {func(...)}
        local ok = table.remove(ret, 1)
        if ok then
            private.checks_ok = private.checks_ok + 1
            return
        end
        local msg = ''
        for _, part in pairs(ret) do
            if part then
                msg = msg .. ': ' .. tostring(part)
            end
        end
        msg = msg:sub(3) -- strip leading ': '
        orig_printerr('Check failed! ' .. (msg or '(no message)'))
        -- Generate a stack trace with all function calls in the same file as the caller to expect.*()
        -- (this produces better stack traces when using helpers in tests)
        local frame = 2
        local caller_src
        while true do
            info = debug.getinfo(frame)
            if not info then break end
            if not caller_src then
                caller_src = info.short_src
            end
            -- Skip any frames corresponding to C calls, or Lua functions defined in another file
            -- these could include pcall(), with_finalize(), etc.
            if info.what == 'Lua' and (info.short_src == caller_src or info.short_src == path) then
                orig_printerr(('  at %s:%d'):format(info.short_src, info.currentline))
            end
            frame = frame + 1
        end
        print('')
    end
end

local function build_test_env(path)
    local env = {
        test = TestTable(),
        -- config values can be overridden in the test file to define
        -- requirements for the tests in that file
        config = {
            -- override with the required game mode
            mode = 'none',
            -- override with a test wrapper function with common setup and
            -- teardown for every test, if needed. for example:
            --   local function test_wrapper(test_fn)
            --       mock.patch(dfhack.filesystem,
            --                  'listdir_recursive',
            --                  mock.func({}),
            --                  test_fn)
            --   end
            --   config.wrapper = test_wrapper
            wrapper = nil,
        },
        expect = {},
        mock = mock,
        delay = delay,
        delay_until = delay_until,
        require = clean_require,
        reqscript = clean_reqscript,
    }
    local private = {
        checks = 0,
        checks_ok = 0,
    }
    for name, func in pairs(expect) do
        env.expect[name] = wrap_expect(func, private, path)
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
    local short_filename = file:sub((file:find('test') or -4) + 5, -1)
    print('Loading file: ' .. short_filename)
    local env, env_private = build_test_env(file)
    local code, err = loadfile(file, 't', env)
    if not code then
        dfhack.printerr('Failed to load file: ' .. tostring(err))
        return false
    end
    dfhack.internal.IN_TEST = true
    local ok, err = dfhack.pcall(code)
    dfhack.internal.IN_TEST = false
    if not ok then
        dfhack.printerr('Error when running file: ' .. tostring(err))
        return false
    end
    if not MODES[env.config.mode] then
        dfhack.printerr('Invalid config.mode: ' .. tostring(env.config.mode))
        return false
    end
    if not env.config.target then
        dfhack.printerr('Skipping tests for unspecified target in ' .. file)
        return true  -- TODO: change to false once existing tests have targets specified
    end
    local targets = type(env.config.target) == 'table' and env.config.target or {env.config.target}
    for _,target in ipairs(targets) do
        if target == 'core' then goto continue end
        if type(target) ~= 'string' or helpdb.has_tag(target, 'unavailable') then
            dfhack.printerr('Skipping tests for unavailable target: ' .. target)
            return true
        end
        ::continue::
    end
    for name, test_func in pairs(env.test) do
        if env.config.wrapper then
            local fn = test_func
            test_func = function() env.config.wrapper(fn) end
        end
        local test_data = {
            full_name = short_filename .. ':' .. name,
            func = test_func,
            private = env_private,
            config = env.config,
        }
        test_data.name = test_data.full_name:gsub('test/', ''):gsub('.lua', '')
        table.insert(tests, test_data)
    end
    return true
end

local function sort_tests(tests)
    -- to make sort stable
    local test_index = utils.invert(tests)
    table.sort(tests, function(a, b)
        if a.config.mode ~= b.config.mode then
            return MODES[a.config.mode].order < MODES[b.config.mode].order
        end
        return test_index[a] < test_index[b]
    end)
end

local function wrap_test(func)
    local saved_printerr = dfhack.printerr
    local printerr_called = false
    local printerr_wrapper = function(msg)
        if msg == nil then return end
        saved_printerr(msg)
        printerr_called = true
    end

    return mock.patch(
        {
            {dfhack, 'printerr', printerr_wrapper},
            {dfhack, 'run_script', clean_run_script},
            {dfhack, 'reqscript', clean_reqscript},
        },
        function()
            local ok, err = dfhack.pcall(func)
            if printerr_called then
                return false,
                       "dfhack.printerr was called outside of" ..
                            " expect.printerr_match(). please wrap your test" ..
                            " with expect.printerr_match()."
            end
            return ok, err
        end
    )
end

local function get_elapsed_str(elapsed)
    return elapsed < 1 and "<1" or tostring(elapsed)
end

local function run_test(test, status, counts)
    test.private.checks = 0
    test.private.checks_ok = 0
    counts.tests = counts.tests + 1
    dfhack.internal.IN_TEST = true
    local start_ms = dfhack.getTickCount()
    local ok, err = wrap_test(test.func)
    local elapsed_ms = dfhack.getTickCount() - start_ms
    dfhack.internal.IN_TEST = false
    local passed = false
    if not ok then
        dfhack.printerr('error: ' .. tostring(err) .. '\n')
        dfhack.printerr('test errored: ' .. test.name)
    elseif test.private.checks ~= test.private.checks_ok then
        dfhack.printerr('test failed: ' .. test.name)
    else
        local elapsed_str = get_elapsed_str(elapsed_ms)
        print(('test passed in %s ms: %s'):format(elapsed_str, test.name))
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

local function run_tests(tests, status, counts, config)
    wait_for(60000, 'game load', function()
        local scr = dfhack.gui.getDFViewscreen()
        return not df.viewscreen_initial_prepst:is_instance(scr)
    end)
    print(('Running %d tests'):format(#tests))
    local start_ms = dfhack.getTickCount()
    local num_skipped = 0
    for _, test in pairs(tests) do
        if MODES[test.config.mode].failed then
            num_skipped = num_skipped + 1
            goto skip
        end
        if not MODES[test.config.mode].detect() then
            print(('Switching to %s mode for test: %s'):format(test.config.mode, test.name))
            local ok, err = pcall(MODES[test.config.mode].navigate, config)
            if not ok then
                MODES[test.config.mode].failed = true
                dfhack.printerr(tostring(err))
                num_skipped = num_skipped + 1
                goto skip
            end
        end
        -- pre-emptively mark the test as failed in case we induce a crash
        status[test.full_name] = TestStatus.FAILED
        save_test_status(status)
        if run_test(test, status, counts) then
            status[test.full_name] = TestStatus.PASSED
            save_test_status(status)
        end
        ::skip::
    end
    local elapsed_ms = dfhack.getTickCount() - start_ms

    local function print_summary_line(ok, message)
        local print_fn = print
        if not ok then
            status['*'] = TestStatus.FAILED
            print_fn = dfhack.printerr
        end
        print_fn('  ' .. message)
    end

    status['*'] = status['*'] or TestStatus.PASSED
    print(('\nTests completed in %s ms:'):format(get_elapsed_str(elapsed_ms)))
    print_summary_line(counts.tests_ok == counts.tests,
        ('%d/%d tests passed'):format(counts.tests_ok, counts.tests))
    print_summary_line(counts.checks_ok == counts.checks,
        ('%d/%d checks passed'):format(counts.checks_ok, counts.checks))
    print_summary_line(counts.file_errors == 0,
        ('%d test files failed to load'):format(counts.file_errors))
    print_summary_line(num_skipped == 0,
        ('%d tests in unreachable modes'):format(num_skipped))

    save_test_status(status)
end

local function dump_df_state()
    local state = {
        enabler = {
            fps = df.global.enabler.fps,
            gfps = df.global.enabler.gfps,
            fullscreen_state = df.global.enabler.fullscreen_state.whole,
        },
        gps = {
            dimx = df.global.gps.dimx,
            dimy = df.global.gps.dimy,
            display_frames = df.global.gps.display_frames,
        },
    }
    json.encode_file(state, DF_STATE_FILE)
    print('DF global state: ' .. json.encode(state, {pretty = false}))
end

local function main(args)
    local help, resume, test_dir, mode_filter, save_dir, test_filter =
            false, false, nil, {}, nil, {}
    local other_args = utils.processArgsGetopt(args, {
            {'h', 'help', handler=function() help = true end},
            {'d', 'test_dir', hasArg=true,
             handler=function(arg) test_dir = arg end},
            {'m', 'modes', hasArg=true,
             handler=function(arg) mode_filter = utils.split_string(arg, ',') end},
            {'r', 'resume', handler=function() resume = true end},
            {'s', 'save_dir', hasArg=true,
             handler=function(arg) save_dir = arg end},
            {'t', 'tests', hasArg=true,
             handler=function(arg) test_filter = utils.split_string(arg, ',') end},
        })

    if help then print(help_text) return end

    local done_command = table.concat(other_args, ' ')
    local config = load_test_config(CONFIG_FILE)

    -- override config with any params specified on the commandline
    if test_dir then config.test_dir = test_dir end
    if resume then config.resume = true end
    if save_dir then config.save_dir = save_dir end
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

    dump_df_state()
    local test_files = get_test_files(config.test_dir)
    local tests = get_tests(test_files, counts)
    local status = filter_tests(tests, config)

    script.start(function()
        dfhack.call_with_finalizer(1, true,
                              finish_tests, config.done_command,
                              run_tests, tests, status, counts, config)
    end)
end

if not dfhack_flags.module then
    main({...})
end
