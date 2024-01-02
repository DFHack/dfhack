local _ENV = mkmodule('plugins.autobutcher')

local argparse = require('argparse')

local function is_int(val)
    return val and val == math.floor(val)
end

local function is_positive_int(val)
    return is_int(val) and val > 0
end

local function check_nonnegative_int(str)
    local val = tonumber(str)
    if is_positive_int(val) or val == 0 then return val end
    qerror('expecting a non-negative integer, but got: '..tostring(str))
end

local function process_args(opts, args)
    if args[1] == 'help' then
        opts.help = true
        return {}
    end

    return argparse.processArgsGetopt(args, {
            {'h', 'help', handler=function() opts.help = true end},
        })
end

local function process_races(opts, races, start_idx)
    if #races < start_idx then
        qerror('missing list of races (or "all" or "new" keywords)')
    end
    for i=start_idx,#races do
        local race = races[i]
        if race == 'all' then
            opts.races_all = true
        elseif race == 'new' then
            opts.races_new = true
        else
            local str = df.new('string')
            str.value = race
            opts.races:insert('#', str)
        end
    end
end

function parse_commandline(opts, ...)
    local positionals = process_args(opts, {...})

    local command = positionals[1]
    if command then opts.command = command end

    if opts.help or not command or command == 'now' or
            command == 'autowatch' or command == 'noautowatch' or
            command == 'list' or command == 'list_export' then
        return
    end

    if command == 'watch' or command == 'unwatch' or command == 'forget' then
        process_races(opts, positionals, 2)
    elseif command == 'target' then
        opts.fk = check_nonnegative_int(positionals[2])
        opts.mk = check_nonnegative_int(positionals[3])
        opts.fa = check_nonnegative_int(positionals[4])
        opts.ma = check_nonnegative_int(positionals[5])
        process_races(opts, positionals, 6)
    else
        qerror(('unrecognized command: "%s"'):format(command))
    end
end

return _ENV
