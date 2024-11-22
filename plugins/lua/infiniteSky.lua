local _ENV = mkmodule('plugins.infiniteSky')

local argparse = require('argparse')

local function process_args(opts, args)
    if args[1] == 'help' then
        opts.help = true
        return
    end

    if args[1] ~= nil then
        opts.n = argparse.positiveInt(args[1])
        return
    end

    return argparse.processArgsGetopt(args, {
            {'h', 'help', handler=function() opts.help = true end},
        })
end

function parse_commandline(opts, args)
    local positionals = process_args(opts, args)

    if opts.help then return end
end

return _ENV
