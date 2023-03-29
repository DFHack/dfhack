local _ENV = mkmodule('plugins.tailor')

local argparse = require('argparse')
local utils = require('utils')

local function process_args(opts, args)
    if args[1] == 'help' then
        opts.help = true
        return
    end

    return argparse.processArgsGetopt(args, {
            {'h', 'help', handler=function() opts.help = true end},
        })
end

function status()
    print(('tailor is %s'):format(isEnabled() and "enabled" or "disabled"))
    print('materials preference order:')
    for _,name in ipairs(tailor_getMaterialPreferences()) do
        print(('  %s'):format(name))
    end
end

function setMaterials(names)
    local idxs = utils.invert(names)
    tailor_setMaterialPreferences(
            idxs.silk or -1,
            idxs.cloth or -1,
            idxs.yarn or -1,
            idxs.leather or -1,
            idxs.adamantine or -1)
end

function setDebugMode(opt)
    local fl = (opt[1] == "true" or opt[1] == "on")
    tailor_setDebugFlag(fl)
end

function parse_commandline(...)
    local args, opts = {...}, {}
    local positionals = process_args(opts, args)

    if opts.help then
        return false
    end

    local command = table.remove(positionals, 1)
    if not command or command == 'status' then
        status()
    elseif command == 'now' then
        tailor_doCycle()
    elseif command == 'materials' then
        setMaterials(positionals)
    elseif command == 'debugging' then
        setDebugMode(positionals)
    else
        return false
    end

    return true
end

return _ENV
