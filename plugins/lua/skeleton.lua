local _ENV = mkmodule('plugins.skeleton')

local argparse = require('argparse')
local utils = require('utils')

local VALID_FORMATS = utils.invert{'pretty', 'normal', 'ugly'}

local function do_commandline(opts, args)
    print(('called with %d arguments:'):format(#args))
    for _,arg in ipairs(args) do
        print('  ' .. arg)
    end

    local positionals = argparse.processArgsGetopt(args, {
            {'t', 'ticks', hasArg=true,
             handler=function(arg) opts.ticks =
                    argparse.check_positive_int(arg, 'ticks') end},
            {'s', 'start', hasArg=true,
             handler=function(arg) utils.assign(
                    opts.start, argpars.coors(arg, 'start')) end},
            {'h', 'help', handler=function() opts.help = true end},
            {'f', 'format', hasArg=true,
             handler=function(arg) opts.format = arg end},
            {'z', 'cur-zlevel', handler=function() use_zlevel = true end},
        })

    if positionals[1] == 'help' then opts.help = true end
    if opts.help then return end

    if positionals[1] == 'now' then
        opts.now = true
    end

    if #opts.format > 0 and not VALID_FORMATS[opts.format] then
        qerror(('invalid format name: "%s"'):format(opts.format))
    end
end

function parse_commandline(opts, ...)
    do_commandline(opts, {...})

    print('populated options data structure:')
    printall_recurse(opts)
end

return _ENV
