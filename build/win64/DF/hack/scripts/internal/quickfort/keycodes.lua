-- keycode conversion logic for the quickfort script query module
--@ module = true

if not dfhack_flags.module then
    qerror('this script cannot be called directly')
end

local quickfort_common = reqscript('internal/quickfort/common')
local quickfort_reader = reqscript('internal/quickfort/reader')
local log = quickfort_common.log

local keycodes_file = 'data/init/interface.txt'

local interface_txt_mtime, keycodes = nil, nil

-- number keys (but not numpad number keys) can appear as either "SYM:0:%d" or
-- "KEY:%d". we arbitrarily choose to standardize on the "KEY" format so we know
-- how to find the mappings later.
local function canonicalize_keyspec(keyspec)
    local _, _, number = string.find(keyspec, '^%[SYM:0:(%d)%]')
    if not number then return keyspec end
    return ('[KEY:%d]'):format(number)
end

local function reload_keycodes(reader)
    -- add "Empty" pseudo-keycode that expands to a 0-length list
    keycodes = {['[SYM:0:Empty]']={}}
    local num_keycodes, cur_binding, line = 0, nil, reader:get_next_row()
    while line do
        local _, _, binding = string.find(line, '^%[BIND:([0-9_A-Z]+):.*')
        if binding then
            cur_binding = binding
        elseif cur_binding and #line > 0 then
            -- it's a keycode definition
            line = canonicalize_keyspec(line)
            if not keycodes[line] then keycodes[line] = {} end
            table.insert(keycodes[line], cur_binding)
            num_keycodes = num_keycodes + 1
        end
        line = reader:get_next_row()
    end
    return num_keycodes
end

function init_keycodes()
    local mtime = dfhack.filesystem.mtime(keycodes_file)
    if interface_txt_mtime == mtime then return end
    local num_keycodes =
            reload_keycodes(quickfort_reader.TextReader{filepath=keycodes_file})
    log('successfully read in %d keycodes from "%s"',
        num_keycodes, keycodes_file)
    interface_txt_mtime = mtime
end

-- code is an interface key name from the keycodes_file, like 'a' or 'Down',
-- with shift, ctrl, or alt modifiers recorded in the modifiers table.
-- returns a list of all the keycodes that the input could translate to
function get_keycodes(code, modifiers)
    if not code then return nil end
    local mod = 0
    if modifiers['shift'] then
        mod = 1
    end
    if modifiers['ctrl'] then
        mod = mod + 2
    end
    if modifiers['alt'] then
        mod = mod + 4
    end
    local key = nil
    if mod == 0 and #code == 1 then
        key = string.format('[KEY:%s]', code)
    else
        key = string.format('[SYM:%d:%s]', mod, code)
    end
    return keycodes[key]
end

if dfhack.internal.IN_TEST then
    unit_test_hooks = {
        canonicalize_keyspec=canonicalize_keyspec,
        reload_keycodes=reload_keycodes,
        get_keycodes=get_keycodes,
    }
end
