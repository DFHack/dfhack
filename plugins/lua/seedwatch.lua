local _ENV = mkmodule('plugins.seedwatch')

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

local function print_status()
    print(('seedwatch is %s'):format(isEnabled() and "enabled" or "disabled"))
    print()
    print('usable seed counts and current targets:')
    local watch_map, seed_counts = seedwatch_getData()
    local sum = 0
    local plants = df.global.world.raws.plants.all
    for k,v in pairs(seed_counts) do
        print(('  %4d/%s %s'):format(v, watch_map[k] or '-', plants[k].id))
        sum = sum + v
    end
    print()
    print(('total usable seeds: %d'):format(sum))
end

local function set_target(name, num)
    if not name or #name == 0 then
        qerror('must specify "all" or plant name')
    end

    num = tonumber(num)
    num = num and math.floor(num) or nil
    if not num or num < 0 then
        qerror('target must be a non-negative integer')
    end

    seedwatch_setTarget(name, num)
end

function parse_commandline(...)
    local args, opts = {...}, {}
    local positionals = process_args(opts, args)

    if opts.help then
        return false
    end

    local command = positionals[1]
    if not command or command == 'status' then
        print_status()
    elseif command == 'clear' then
        set_target('all', 0)
    elseif positionals[2] then
        set_target(command, positionals[2])
    else
        return false
    end

    return true
end

return _ENV
