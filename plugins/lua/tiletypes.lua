local _ENV = mkmodule('plugins.tiletypes')

local utils = require('utils')

local function parse_cursor(opts, arg)
    local _, _, x, y, z = arg:find('^%s*(%d+)%s*,%s*(%d+)%s*,%s*(%d+)%s*$')
    if not x then
        qerror(('invalid argument for --cursor option: "%s"; expected format' ..
                ' is "<x>,<y>,<z>", for example: "30,60,150"'):format(arg))
    end
    opts.cursor.x = tonumber(x)
    opts.cursor.y = tonumber(y)
    opts.cursor.z = tonumber(z)
end

function parse_commandline(opts, ...)
    local positionals = utils.processArgsGetopt({...}, {
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
