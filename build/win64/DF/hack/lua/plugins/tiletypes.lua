local _ENV = mkmodule('plugins.tiletypes')

local argparse = require('argparse')
local utils = require('utils')

local function parse_cursor(opts, arg)
    utils.assign(opts.cursor, argparse.coords(arg))
end

function parse_commandline(opts, ...)
    local positionals = argparse.processArgsGetopt({...}, {
            {'c', 'cursor', hasArg=true,
             handler=function(arg) parse_cursor(opts, arg) end},
            {'h', 'help', handler=function() opts.help = true end},
            {'q', 'quiet', handler=function() opts.quiet = true end},
        })

    if positionals[1] == 'help' or positionals[1] == '?' then
        opts.help = true
    end
end

return _ENV
