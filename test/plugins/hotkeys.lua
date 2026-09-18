config.mode = 'fortress'
config.target = 'hotkeys'

local hotkeys = require('plugins.hotkeys')

local BINDING_SPEC = 'Ctrl-Shift-Alt-F11@dwarfmode'
local BINDING_CMD = 'echo dfhack-test-binding'

config.wrapper = function(test_fn)
    dfhack.run_command_silent{'keybinding', 'clear', BINDING_SPEC}
    return dfhack.with_finalize(function()
        dfhack.run_command_silent{'keybinding', 'clear', BINDING_SPEC}
        hotkeys.cleanupHotkeys()
    end, test_fn)
end

-- key symbol strings get normalized by the keybinding manager (e.g.
-- modifier order), so match added bindings by their command instead
local function find_binding(cmdline)
    local keys, bindings = hotkeys.getHotkeys()
    for _, sym in ipairs(keys) do
        if bindings[sym] == cmdline then return sym end
    end
end

local function with_mortal_mode(test_fn)
    local saved = dfhack.getMortalMode()
    dfhack.setMortalMode(true)
    return dfhack.with_finalize(function()
        dfhack.setMortalMode(saved)
    end, test_fn)
end

function test.should_hide_armok()
    -- createitem is tagged 'armok' in helpdb
    with_mortal_mode(function()
        expect.true_(hotkeys.should_hide_armok('createitem'))
        expect.false_(hotkeys.should_hide_armok('ls'))
        expect.false_(hotkeys.should_hide_armok('bogus-command'))
    end)
end

function test.should_hide_armok_strips_prefix()
    with_mortal_mode(function()
        expect.true_(hotkeys.should_hide_armok(':createitem'))
        expect.true_(hotkeys.should_hide_armok('   createitem --flags'))
        expect.false_(hotkeys.should_hide_armok(':'))
    end)
end

function test.should_hide_armok_not_mortal()
    -- armok commands stay visible when mortal mode is off
    with_mortal_mode(function()
        dfhack.setMortalMode(false)
        expect.false_(hotkeys.should_hide_armok('createitem'))
    end)
end

function test.getHotkeys_returns_state()
    local keys, bindings = hotkeys.getHotkeys()
    expect.ne(nil, keys)
    expect.ne(nil, bindings)
end

function test.getHotkeys_finds_added_binding()
    dfhack.run_command_silent{'keybinding', 'add', BINDING_SPEC, BINDING_CMD}
    expect.ne(nil, find_binding(BINDING_CMD))
end

function test.getHotkeys_filters_menu_bindings()
    -- stub keybindings that invoke the hotkeys menu itself must not be listed
    dfhack.run_command_silent{'keybinding', 'add', BINDING_SPEC,
                              'overlay trigger hotkeys.foo'}
    expect.nil_(find_binding('overlay trigger hotkeys.foo'))
    dfhack.run_command_silent{'keybinding', 'add', BINDING_SPEC, 'hotkeys'}
    expect.nil_(find_binding('hotkeys'))
end
