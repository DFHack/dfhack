-- DFHack-native implementation of the classic Quickfort utility
--@module = true

local argparse = require('argparse')

local quickfort_api = reqscript('internal/quickfort/api')
local quickfort_command = reqscript('internal/quickfort/command')
local quickfort_common = reqscript('internal/quickfort/common')
local quickfort_list = reqscript('internal/quickfort/list')
local quickfort_set = reqscript('internal/quickfort/set')

local GLOBAL_KEY = 'quickfort'

function refresh_scripts()
    -- reqscript all internal files here, even if they're not directly used by this
    -- top-level file. this ensures modified transitive dependencies are properly
    -- reloaded when this script is run.
    reqscript('internal/quickfort/aliases')
    reqscript('internal/quickfort/api')
    reqscript('internal/quickfort/build')
    reqscript('internal/quickfort/building')
    reqscript('internal/quickfort/command')
    reqscript('internal/quickfort/common')
    reqscript('internal/quickfort/dig')
    reqscript('internal/quickfort/keycodes')
    reqscript('internal/quickfort/list')
    reqscript('internal/quickfort/map')
    reqscript('internal/quickfort/meta')
    reqscript('internal/quickfort/notes')
    reqscript('internal/quickfort/orders')
    reqscript('internal/quickfort/parse')
    reqscript('internal/quickfort/place')
    reqscript('internal/quickfort/preview')
    reqscript('internal/quickfort/reader')
    reqscript('internal/quickfort/set')
    reqscript('internal/quickfort/transform')
    reqscript('internal/quickfort/zone')
end
refresh_scripts()

-- public API
function apply_blueprint(params)
    local data, cursor = quickfort_api.normalize_data(params.data, params.pos)
    local ctx = quickfort_api.init_api_ctx(params, cursor)

    quickfort_common.verbose = not not params.verbose
    dfhack.with_finalize(
        function() quickfort_common.verbose = false end,
        function()
            for zlevel,grid in pairs(data) do
                quickfort_command.do_command_raw(params.mode, zlevel, grid, ctx)
            end
        end)
    return quickfort_api.clean_stats(ctx.stats)
end

-- interactive script
if dfhack_flags.module then
    return
end

local function do_help()
    print(dfhack.script_help())
end

local function do_gui(params)
    dfhack.run_script('gui/quickfort', table.unpack(params))
end

local function do_reset()
    quickfort_list.blueprint_dirs = nil
    quickfort_set.do_reset()
end

dfhack.onStateChange[GLOBAL_KEY] = function(sc)
    if sc == SC_WORLD_LOADED or sc == SC_WORLD_UNLOADED then
        do_reset()
    end
end

local action_switch = {
    set=quickfort_set.do_set,
    reset=do_reset,
    list=quickfort_list.do_list,
    gui=do_gui,
    run=quickfort_command.do_command,
    orders=quickfort_command.do_command,
    undo=quickfort_command.do_command
}
setmetatable(action_switch, {__index=function() return do_help end})

local args = {...}
local action = table.remove(args, 1) or 'help'
args.commands = argparse.stringList(action)

local action_fn = action_switch[args.commands[1]]

if (action == 'run' or action == 'orders' or action == 'undo') and
        not dfhack.isMapLoaded() then
    qerror('quickfort needs a fortress map to be loaded.')
end

action_fn(args)
