local _ENV = mkmodule('plugins.autonestbox')

local argparse = require('argparse')

local function is_int(val)
    return val and val == math.floor(val)
end

local function is_positive_int(val)
    return is_int(val) and val > 0
end

local function process_args(opts, args)
    if args[1] == 'help' then
        opts.help = true
        return
    end

    return argparse.processArgsGetopt(args, {
            {'h', 'help', handler=function() opts.help = true end},
        })
end

function parse_commandline(opts, ...)
    local positionals = process_args(opts, {...})

    if opts.help then return end

    local in_ticks = false
    for _,arg in ipairs(positionals) do
        if in_ticks then
            arg = tonumber(arg)
            if not is_positive_int(arg) then
                qerror('number of ticks must be a positive integer: ' .. arg)
            else
                opts.ticks = arg
            end
            in_ticks = false
        elseif arg == 'ticks' then
            in_ticks = true
        elseif arg == 'now' then
            opts.now = true
        end
    end

    if in_ticks then
        qerror('missing number of ticks')
    end
end

return _ENV
