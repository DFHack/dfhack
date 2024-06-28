local _ENV = mkmodule('script-manager')

local utils = require('utils')

---------------------
-- enabled API

-- for each script that can be loaded as a module, calls cb(script_name, env)
function foreach_module_script(cb, preprocess_script_file_fn)
    for _,script_path in ipairs(dfhack.internal.getScriptPaths()) do
        local files = dfhack.filesystem.listdir_recursive(
                                            script_path, nil, false)
        if not files then goto skip_path end
        for _,f in ipairs(files) do
            if f.isdir or not f.path:endswith('.lua') or
                    f.path:startswith('.git') or
                    f.path:startswith('test/') or
                    f.path:startswith('internal/') then
                goto continue
            end
            if preprocess_script_file_fn then
                preprocess_script_file_fn(script_path, f.path)
            end
            local script_name = f.path:sub(1, #f.path - 4) -- remove '.lua'
            local ok, script_env = pcall(reqscript, script_name)
            if ok then
                cb(script_name, script_env)
            end
            ::continue::
        end
        ::skip_path::
    end
end

local enabled_map = nil

local function process_script(env_name, env)
    local global_name = 'isEnabled'
    local fn = env[global_name]
    if not fn then return end
    if type(fn) ~= 'function' then
        dfhack.printerr(
                ('error registering %s() from "%s": global' ..
                ' value is not a function'):format(global_name, env_name))
        return
    end
    enabled_map[env_name] = fn
end

function reload(refresh_active_mod_scripts)
    enabled_map = utils.OrderedTable()
    local force_refresh_fn = refresh_active_mod_scripts and function(script_path, script_name)
        if script_path:find('scripts_modactive') then
            local full_path = script_path..'/'..script_name
            internal_script = dfhack.internal.scripts[full_path]
            if internal_script then
                dfhack.internal.scripts[full_path] = nil
            end
        end
    end or nil
    foreach_module_script(process_script, force_refresh_fn)
end

local function ensure_loaded()
    if not enabled_map then
        reload()
    end
end

function list()
    ensure_loaded()
    for name,fn in pairs(enabled_map) do
        print(('%21s  %-3s'):format(name..':', fn() and 'on' or 'off'))
    end
end

---------------------
-- mod paths

-- this perhaps could/should be queried from the Steam API
-- are there any installation configurations where this will be wrong, though?
local WORKSHOP_MODS_PATH = '../../workshop/content/975370/'
local MODS_PATH = 'mods/'
local INSTALLED_MODS_PATH = 'data/installed_mods/'

-- last instance of the same version of the same mod wins, so read them in this
-- order (in increasing order of liklihood that players may have made custom
-- changes to the files)
local MOD_PATH_ROOTS = {WORKSHOP_MODS_PATH, MODS_PATH, INSTALLED_MODS_PATH}

local function get_mod_id_and_version(path)
    local idfile = path .. '/info.txt'
    local ok, lines = pcall(io.lines, idfile)
    if not ok then return end
    local id, version
    for line in lines do
        if not id then
            _,_,id = line:find('^%[ID:([^%]]+)%]')
        end
        if not version then
            -- note this doesn't include the closing brace since some people put
            -- non-number characters in here, and DF only reads the leading digits
            -- as the numeric version
            _,_,version = line:find('^%[NUMERIC_VERSION:(%d+)')
        end
        -- note that we do *not* want to break out of this loop early since
        -- lines has to hit EOF to close the file
    end
    return id, version
end

local function add_mod_paths(mod_paths, id, base_path, subdir)
    local sep = base_path:endswith('/') and '' or '/'
    local path = ('%s%s%s'):format(base_path, sep, subdir)
    if dfhack.filesystem.isdir(path) then
        table.insert(mod_paths, {id=id, path=path})
    end
end

function get_mod_paths(installed_subdir, active_subdir)
    -- ordered map of mod id -> {handled=bool, versions=map of version -> path}
    local mods = utils.OrderedTable()
    local mod_paths = {}

    -- if a world is loaded, process active mods first, and lock to active version
    if dfhack.isWorldLoaded() then
        for _,path in ipairs(df.global.world.object_loader.object_load_order_src_dir) do
            path = tostring(path.value)
            -- skip vanilla "mods"
            if not path:startswith(INSTALLED_MODS_PATH) then goto continue end
            local id = get_mod_id_and_version(path)
            if not id then goto continue end
            mods[id] = {handled=true}
            if active_subdir then
                add_mod_paths(mod_paths, id, path, active_subdir)
            end
            add_mod_paths(mod_paths, id, path, installed_subdir)
            ::continue::
        end
    end

    -- assemble version -> path maps for all (non-handled) mod source dirs
    for _,mod_path_root in ipairs(MOD_PATH_ROOTS) do
        local files = dfhack.filesystem.listdir_recursive(mod_path_root, 0)
        if not files then goto skip_path_root end
        for _,f in ipairs(files) do
            if not f.isdir then goto continue end
            local id, version = get_mod_id_and_version(f.path)
            if not id or not version then goto continue end
            local mod = ensure_key(mods, id)
            if mod.handled then goto continue end
            ensure_key(mod, 'versions')[version] = f.path
            ::continue::
        end
        ::skip_path_root::
    end

    -- add paths from most recent version of all not-yet-handled mods
    for id,v in pairs(mods) do
        if v.handled then goto continue end
        local max_version, path
        for version,mod_path in pairs(v.versions) do
            if not max_version or max_version < version then
                path = mod_path
                max_version = version
            end
        end
        add_mod_paths(mod_paths, id, path, installed_subdir)
        ::continue::
    end

    return mod_paths
end

function get_mod_script_paths()
    local paths = {}
    for _,v in ipairs(get_mod_paths('scripts_modinstalled', 'scripts_modactive')) do
        table.insert(paths, v.path)
    end
    return paths
end

function getModSourcePath(mod_id)
    for _,v in ipairs(get_mod_paths('.')) do
        if v.id == mod_id then
            -- trim off the final '.'
            return v.path:sub(1, #v.path-1)
        end
    end
end

function getModStatePath(mod_id)
    local path = ('dfhack-config/mods/%s/'):format(mod_id)
    if not dfhack.filesystem.mkdir_recursive(path) then
        error(('failed to create mod state directory: "%s"'):format(path))
    end
    return path
end

---------------------
-- perf API

local function format_time(ms)
    return ('%8d ms (%dm %ds)'):format(ms, ms // 60000, (ms % 60000) // 1000)
end

local function format_relative_time(width, name, ms, rel1_ms, desc1, rel2_ms, desc2)
    local fmt = '%' .. tostring(width) .. 's %8d ms (%6.2f%% of %s'
    local str = fmt:format(name, ms, (ms * 100) / rel1_ms, desc1)
    if rel2_ms then
        str = str .. (', %6.2f%% of %s'):format((ms * 100) / rel2_ms, desc2)
    end
    return str .. ')'
end

local function print_sorted_timers(in_timers, width, rel1_ms, desc1, rel2_ms, desc2)
    local sum = 0
    local sorted = {}
    for name,timer in pairs(in_timers) do
        table.insert(sorted, {name=name, ms=timer})
        sum = sum + timer
    end
    table.sort(sorted, function(a, b) return a.ms > b.ms end)
    for _, elem in ipairs(sorted) do
        if elem.ms > 0 then
            print(format_relative_time(width, elem.name, elem.ms, rel1_ms, desc1, rel2_ms, desc2))
        end
    end
    local framework_time = math.max(0, rel1_ms - sum)
    print()
    print(format_relative_time(width, 'framework', framework_time, rel1_ms, desc1, rel2_ms, desc2))
    print(format_relative_time(width, 'all subtimers', sum, rel1_ms, desc1, rel2_ms, desc2))
end

function print_timers()
    local summary, em_per_event, em_per_plugin_per_event, update_per_plugin, state_change_per_plugin,
        update_lua_per_repeat, overlay_per_widget, zscreen_per_focus = dfhack.internal.getPerfCounters()

    local elapsed = summary.elapsed_ms
    local total_update_time = summary.total_update_ms
    local total_overlay_time = summary.total_overlay_ms
    local total_zscreen_time = summary.total_zscreen_ms

    print('Summary')
    print('-------')
    print()
    print(('Measuring %s'):format(summary.unpaused_only == 1 and 'unpaused time only' or 'paused and unpaused time'))
    print()
    print(('%7s %s'):format('elapsed', format_time(elapsed)))

    if elapsed <= 0 then return end

    local sum = summary.total_keybinding_ms + total_update_time + total_overlay_time + total_zscreen_time
    print(format_relative_time(7, 'dfhack', sum, elapsed, 'elapsed'))

    if sum > 0 then
        print()
        print()
        print('DFHack details')
        print('--------------')
        print()
        print(format_relative_time(10, 'keybinding', summary.total_keybinding_ms, sum, 'dfhack', elapsed, 'elapsed'))
        print(format_relative_time(10, 'update', total_update_time, sum, 'dfhack', elapsed, 'elapsed'))
        print(format_relative_time(10, 'overlay', total_overlay_time, sum, 'dfhack', elapsed, 'elapsed'))
        print(format_relative_time(10, 'zscreen', total_zscreen_time, sum, 'dfhack', elapsed, 'elapsed'))
    end

    if total_update_time > 0 then
        print()
        print()
        print('Update details')
        print('--------------')
        print()
        print(format_relative_time(15, 'event manager', summary.update_event_manager_ms, total_update_time, 'update', elapsed, 'elapsed'))
        print(format_relative_time(15, 'plugin onUpdate', summary.update_plugin_ms, total_update_time, 'update', elapsed, 'elapsed'))
        print(format_relative_time(15, 'lua timers', summary.update_lua_ms, total_update_time, 'update', elapsed, 'elapsed'))
    end

    if summary.update_event_manager_ms > 0 then
        print()
        print()
        print('Event manager per event type')
        print('----------------------------')
        print()
        print_sorted_timers(em_per_event, 25, summary.update_event_manager_ms, 'event manager', elapsed, 'elapsed')

        for k,v in pairs(em_per_plugin_per_event) do
            if em_per_event[k] <= 0 then goto continue end
            print()
            print()
            local title = ('Event manager %s event per plugin'):format(k)
            print(title)
            print(('-'):rep(#title))
            print()
            print_sorted_timers(v, 25, em_per_event[k], 'event type', elapsed, 'elapsed')
            ::continue::
        end
    end

    if summary.update_plugin_ms > 0 then
        print()
        print()
        print('Update per plugin')
        print('-----------------')
        print()
        print_sorted_timers(update_per_plugin, 25, summary.update_plugin_ms, 'update', elapsed, 'elapsed')
        print()
        print()
        print('State change per plugin')
        print('-----------------------')
        print()
        print_sorted_timers(state_change_per_plugin, 25, summary.update_plugin_ms, 'update', elapsed, 'elapsed')
    end

    if summary.update_lua_ms > 0 then
        print()
        print()
        print('Lua timer details')
        print('-----------------')
        print()
        print_sorted_timers(update_lua_per_repeat, 45, summary.update_lua_ms, 'lua timers', elapsed, 'elapsed')
    end

    if total_overlay_time > 0 then
        print()
        print()
        print('Overlay details')
        print('---------------')
        print()
        print_sorted_timers(overlay_per_widget, 45, total_overlay_time, 'overlay', elapsed, 'elapsed')
    end

    if total_zscreen_time > 0 then
        print()
        print()
        print('ZScreen details')
        print('---------------')
        print()
        print_sorted_timers(zscreen_per_focus, 45, total_zscreen_time, 'zscreen', elapsed, 'elapsed')
    end
end

return _ENV
