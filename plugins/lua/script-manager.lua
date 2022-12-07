local _ENV = mkmodule('plugins.script-manager')

local utils = require('utils')

-- for each script that can be loaded as a module, calls cb(script_name, env)
function foreach_module_script(cb)
    for _,script_path in ipairs(dfhack.internal.getScriptPaths()) do
        local files = dfhack.filesystem.listdir_recursive(
                                            script_path, nil, false)
        if not files then goto skip_path end
        for _,f in ipairs(files) do
            if not f.isdir and
                    f.path:endswith('.lua') and
                    not f.path:startswith('test/') and
                    not f.path:startswith('internal/') then
                local script_name = f.path:sub(1, #f.path - 4) -- remove '.lua'
                local ok, script_env = pcall(reqscript, script_name)
                if ok then
                    cb(script_name, script_env)
                end
            end
        end
        ::skip_path::
    end
end

local enabled_map = {}

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

function reload()
    enabled_map = utils.OrderedTable()
    foreach_module_script(process_script)
end

function list()
    for name,fn in pairs(enabled_map) do
        print(('%20s\t%-3s'):format(name..':', fn() and 'on' or 'off'))
    end
end

return _ENV
