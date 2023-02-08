local _ENV = mkmodule('plugins.autochop')

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

function setTargets(max, min)
    max = tonumber(max)
    max = max and math.floor(max) or nil
    if not max or max < 0 then
        qerror('target maximum must be a non-negative integer')
    end

    min = tonumber(min)
    min = min and math.floor(min) or nil
    if min and (min < 0 or min > max) then
        qerror('target minimum must be between 0 and the maximum, inclusive')
    end
    if not min then
        min = math.floor(max * 0.8)
    end
    autochop_setTargets(max, min)
end

local function do_set_burrow_config(var_name, val, burrows)
    for _,bspec in ipairs(argparse.stringList(burrows)) do
        local config = autochop_getBurrowConfig(bspec)
        config[var_name] = val
        autochop_setBurrowConfig(config.id, config.chop, config.clearcut,
                config.protect_brewable, config.protect_edible,
                config.protect_cookable)
    end
end

local function do_set_burrow_protect_config(types, val, burrows)
    for _,tname in ipairs(argparse.stringList(types)) do
        do_set_burrow_config('protect_'..tname, val, burrows)
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
        autochop_printStatus()
    elseif command == 'designate' then
        autochop_designate()
    elseif command == 'undesignate' then
        autochop_undesignate()
    elseif command == 'target' then
        setTargets(args[2], args[3])
    elseif command == 'chop' then
        do_set_burrow_config('chop', true, args[2])
    elseif command == 'nochop' then
        do_set_burrow_config('chop', false, args[2])
    elseif command == 'clearcut' or command == 'clear' then
        do_set_burrow_config('clearcut', true, args[2])
    elseif command == 'noclearcut' or command == 'noclear' then
        do_set_burrow_config('clearcut', false, args[2])
    elseif command == 'protect' then
        do_set_burrow_protect_config(args[2], true, args[3])
    elseif command == 'unprotect' or command == 'noprotect' then
        do_set_burrow_protect_config(args[2], false, args[3])
    else
        return false
    end

    return true
end

-- used by gui/autochop
function setBurrowConfig(config)
    autochop_setBurrowConfig(config.id, config.chop, config.clearcut,
            config.protect_brewable, config.protect_edible,
            config.protect_cookable)
end

function getTreeCountsAndBurrowConfigs()
    local data = {autochop_getTreeCountsAndBurrowConfigs()}
    local ret = {}
    ret.summary = table.remove(data, 1)
    ret.tree_counts = table.remove(data, 1)
    ret.designated_tree_counts = table.remove(data, 1)
    local unparsed_burrow_configs = table.remove(data, 1)

    ret.burrow_configs = {}
    for idx,c in pairs(unparsed_burrow_configs) do
        c.name = df.burrow.find(c.id).name
        c.chop = c.chop ~= 0
        c.clearcut = c.clearcut ~= 0
        c.protect_brewable = c.protect_brewable ~= 0
        c.protect_edible = c.protect_edible ~= 0
        c.protect_cookable = c.protect_cookable ~= 0
        table.insert(ret.burrow_configs, c)
    end
    return ret
end

return _ENV
