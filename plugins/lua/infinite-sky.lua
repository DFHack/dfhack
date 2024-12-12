local _ENV = mkmodule('plugins.infinite-sky')

local argparse = require('argparse')

function parse_commandline(opts, args)
    local positionals = argparse.processArgsGetopt(args, {
            {'h', 'help', handler=function() opts.help = true end},
        })
    if opts.help or positionals[1] == 'help' then
        opts.help = true
        return
    end
    if positionals[1] then
        opts.n = argparse.positiveInt(positionals[1])
    end
end

return _ENV
