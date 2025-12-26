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
    dfhack.print(('tailor is %s'):format(isEnabled() and "enabled" or "disabled"))
    print((' %s confiscating tattered clothing'):format(tailor_getConfiscate() and "and" or "but not"))
    print(('tailor %s automating dye'):format(tailor_getAutomateDye() and "is" or "is not"))
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

function setConfiscate(opt)
    local fl = argparse.boolean(opt[1], "set confiscate")
    tailor_setConfiscate(fl)
end

function setAutomateDye(opt)
    local fl = argparse.boolean(opt[1], "set dye")
    tailor_setAutomateDye(fl)
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
    elseif command == 'confiscate' then
        setConfiscate(positionals)
    elseif command == 'dye' then
        setAutomateDye(positionals)
    else
        return false
    end

    return true
end

return _ENV
