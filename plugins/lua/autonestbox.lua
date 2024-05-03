local _ENV = mkmodule('plugins.autonestbox')

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

function parse_commandline(opts, args)
    local positionals = process_args(opts, args)

    if opts.help then return end

    for _,arg in ipairs(positionals) do
        if arg == 'now' then
            opts.now = true
        end
    end
end

return _ENV
