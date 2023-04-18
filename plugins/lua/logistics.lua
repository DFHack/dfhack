local _ENV = mkmodule('plugins.logistics')

local argparse = require('argparse')
local utils = require('utils')

local function make_stat(name, stockpile_number, stats, configs)
    return {
        enabled=configs[stockpile_number] and configs[stockpile_number][name] == 'true',
        designated=stats[name..'_designated'][stockpile_number] or 0,
        can_designate=stats[name..'_can_designate'][stockpile_number] or 0,
    }
end

function getStockpileData()
    local stats, configs = logistics_getStockpileData()
    local data = {}
    for _,bld in ipairs(df.global.world.buildings.other.STOCKPILE) do
        local stockpile_number, name = bld.stockpile_number, bld.name
        local sort_key = name
        if #name == 0 then
            name = ('Stockpile #%d'):format(bld.stockpile_number)
            sort_key = ('Stockpile #%09d'):format(bld.stockpile_number)
        end
        table.insert(data, {
            stockpile_number=stockpile_number,
            name=name,
            sort_key=sort_key,
            melt=make_stat('melt', stockpile_number, stats, configs),
            trade=make_stat('trade', stockpile_number, stats, configs),
            dump=make_stat('dump', stockpile_number, stats, configs),
        })
    end
    table.sort(data, function(a, b) return a.sort_key < b.sort_key end)
    return data
end

local function print_status()
    print(('logistics is %sactively monitoring stockpiles and marking items')
            :format(isEnabled() and '' or 'not '))

    if df.global.gamemode ~= df.game_mode.DWARF or not dfhack.isMapLoaded() then
        return
    end

    local data = getStockpileData()

    print()
    if not data[1] then
        print 'No stockpiles defined -- go make some!'
        return
    end

    local name_len = 12
    for _,sp in ipairs(data) do
        name_len = math.min(40, math.max(name_len, #sp.name))
    end

    print('Designated/designatable items in stockpiles:')
    print()
    local fmt = '%6s  %-' .. name_len .. 's  %11s  %11s  %11s';
    print(fmt:format('number', 'name', 'melt', 'trade', 'dump'))
    local function uline(len) return ('-'):rep(len) end
    print(fmt:format(uline(6), uline(name_len), uline(11), uline(11), uline(11)))
    local function get_enab(stats) return ('[%s]'):format(stats.enabled and 'x' or ' ') end
    local function get_dstat(stats) return ('%d/%d '):format(stats.designated, stats.designated + stats.can_designate) end
    for _,sp in ipairs(data) do
        print(fmt:format(sp.stockpile_number, sp.name, get_enab(sp.melt), get_enab(sp.trade), get_enab(sp.dump)))
        print(fmt:format('', '', get_dstat(sp.melt), get_dstat(sp.trade), get_dstat(sp.dump)))
    end
end

local function for_stockpiles(opts, fn)
    if not opts.sp then
        local selected_sp = dfhack.gui.getSelectedStockpile()
        if not selected_sp then qerror('please specify or select a stockpile') end
        fn(selected_sp.stockpile_number)
        return
    end
    for _,sp in ipairs(argparse.stringList(opts.sp)) do
        fn(sp)
    end
end

local function do_add_stockpile_config(features, opts)
    for_stockpiles(opts, function(sp)
        local configs = logistics_getStockpileConfigs(tonumber(sp) or sp)
        if not configs then
            dfhack.printerr('invalid stockpile: '..sp)
        else
            for _,config in ipairs(configs) do
                logistics_setStockpileConfig(config.stockpile_number,
                        features.melt or config.melt,
                        features.trade or config.trade,
                        features.dump or config.dump)
            end
        end
    end)
end

local function do_clear_stockpile_config(all, opts)
    if all then
        logistics_clearAllStockpileConfigs()
        return
    end
    for_stockpiles(opts, function(sp)
        logistics_clearStockpileConfig(tonumber(sp) or sp)
    end)
end

local function process_args(opts, args)
    if args[1] == 'help' then
        opts.help = true
        return
    end

    return argparse.processArgsGetopt(args, {
        {'h', 'help', handler=function() opts.help = true end},
        {'s', 'stockpile', hasArg=true, handler=function(arg) opts.sp = arg end},
    })
end

function parse_commandline(args)
    local opts = {}
    local positionals = process_args(opts, args)

    if opts.help or not positionals then
        return false
    end

    local command = table.remove(positionals, 1)
    if not command or command == 'status' then
        print_status()
    elseif command == 'now' then
        logistics_cycle()
    elseif command == 'add' then
        do_add_stockpile_config(utils.invert(positionals), opts)
    elseif command == 'clear' then
        do_clear_stockpile_config(utils.invert(positionals).all, opts)
    else
        return false
    end

    return true
end

return _ENV
