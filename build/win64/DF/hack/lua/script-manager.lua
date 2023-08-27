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
        print('indexing mod path: ' .. path)
        table.insert(mod_paths, {id=id, path=path})
    end
end

function get_mod_paths(installed_subdir, active_subdir)
    -- ordered map of mod id -> {handled=bool, versions=map of version -> path}
    local mods = utils.OrderedTable()
    local mod_paths = {}

    -- if a world is loaded, process active mods first, and lock to active version
    if active_subdir and dfhack.isWorldLoaded() then
        for _,path in ipairs(df.global.world.object_loader.object_load_order_src_dir) do
            path = tostring(path.value)
            -- skip vanilla "mods"
            if not path:startswith(INSTALLED_MODS_PATH) then goto continue end
            local id = get_mod_id_and_version(path)
            if not id then goto continue end
            mods[id] = {handled=true}
            add_mod_paths(mod_paths, id, path, active_subdir)
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

return _ENV
