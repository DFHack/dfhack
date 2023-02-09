local _ENV = mkmodule('plugins.misery')

local argparse = require('argparse')

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
    print(('misery is %s'):format(isEnabled() and "enabled" or "disabled"))
    print(('misery factor is: %d'):format(misery_getFactor()))
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
    elseif command == 'factor' then
        misery_setFactor(positionals[1])
    elseif command == 'clear' then
        misery_clear()
    else
        return false
    end

    return true
end

return _ENV
