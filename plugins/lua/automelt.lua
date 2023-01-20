local _ENV = mkmodule('plugins.automelt')

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


local function do_set_stockpile_config(var_name, val, stockpiles)
    for _,bspec in ipairs(argparse.stringList(stockpiles)) do
        local config = automelt_getStockpileConfig(bspec)
        config[var_name] = val
        automelt_setStockpileConfig(config.id, config.monitor, config.melt)
    end
end


function parse_commandline(...)
    local args, opts = {...}, {}
    local positionals = process_args(opts, args)

    if opts.help then
        return false
    end

    local command = positionals[1]
    if not command or command == 'status' then
        automelt_printStatus()
    elseif command == 'designate' then
        automelt_designate()
    elseif command == 'melt' then
        do_set_stockpile_config('melt', true, args[2])
    elseif command == 'nomelt' then
        do_set_stockpile_config('melt', false, args[2])
    elseif command == 'monitor' then
        do_set_stockpile_config('monitor', true, args[2])
    elseif command == 'nomonitor'then
        do_set_stockpile_config('monitor', false, args[2])
    else
        return false
    end

    return true
end

-- used by gui/autochop
function setStockpileConfig(config)
    automelt_setStockpileConfig(config.id, config.monitored, config.melt)
end

function getItemCountsAndStockpileConfigs()
    local data = {automelt_getItemCountsAndStockpileConfigs()}
    local ret = {}
    ret.summary = table.remove(data, 1)
    ret.item_counts = table.remove(data, 1)
    ret.marked_item_counts = table.remove(data, 1)
    ret.premarked_item_counts = table.remove(data, 1)
    ret.stockpile_configs = data
    for _,c in ipairs(ret.stockpile_configs) do
        c.name = df.building.find(c.id).name
        c.monitored = c.monitored ~= 0
        c.melt = c.melt ~= 0
    end
    return ret
end

return _ENV
