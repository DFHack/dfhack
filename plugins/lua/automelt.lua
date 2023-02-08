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
        if not config then
            dfhack.printerr('invalid stockpile: '..tostring(bspec))
        else
            config[var_name] = val
            automelt_setStockpileConfig(config.id, config.monitor, config.melt)
        end
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
    elseif command == 'monitor' then
        do_set_stockpile_config('monitor', true, args[2])
    elseif command == 'nomonitor' or command == 'unmonitor' then
        do_set_stockpile_config('monitor', false, args[2])
    else
        return false
    end

    return true
end

-- used by gui/automelt
function setStockpileConfig(config)
    automelt_setStockpileConfig(config.id, config.monitored)
end

function getItemCountsAndStockpileConfigs()
    local fmt = 'Stockpile #%-5s'
    local data = {automelt_getItemCountsAndStockpileConfigs()}
    local ret = {}
    ret.summary = table.remove(data, 1)
    ret.item_counts = table.remove(data, 1)
    ret.marked_item_counts = table.remove(data, 1)
    ret.premarked_item_counts = table.remove(data, 1)
    local unparsed_stockpile_configs = table.remove(data, 1)
    ret.stockpile_configs = {}

    for idx,c in pairs(unparsed_stockpile_configs) do
        if not c.id or c.id == -1 then
            c.name = "ERROR"
            c.monitored = false
        else
            c.name = df.building.find(c.id).name
            if not c.name or c.name == '' then
                c.name = (fmt):format(tostring(df.building.find(c.id).stockpile_number))
            end
            c.monitored = c.monitored ~= 0
        end
        table.insert(ret.stockpile_configs, c)

    end
    return ret
end

return _ENV
