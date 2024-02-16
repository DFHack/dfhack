local _ENV = mkmodule('plugins.probe')

local argparse = require('argparse')

function parse_commandline(...)
    local cursor = nil
    argparse.processArgsGetopt({...}, {
        {'c', 'cursor', hasArg=true,
            handler=function(optarg) cursor = argparse.coords(optarg) end},
    })
    return cursor
end

return _ENV
