-- Cause selected item types to quickly rot away
--@module = true

local argparse = require('argparse')
local utils = require('utils')

local function get_clothes_vectors()
    return {df.global.world.items.other.GLOVES,
            df.global.world.items.other.ARMOR,
            df.global.world.items.other.SHOES,
            df.global.world.items.other.PANTS,
            df.global.world.items.other.HELM}
end

local function get_corpse_vectors()
    return {df.global.world.items.other.ANY_CORPSE}
end

local function get_remains_vectors()
    return {df.global.world.items.other.REMAINS}
end

local function get_food_vectors()
    return {df.global.world.items.other.FISH,
            df.global.world.items.other.FISH_RAW,
            df.global.world.items.other.EGG,
            df.global.world.items.other.CHEESE,
            df.global.world.items.other.PLANT,
            df.global.world.items.other.PLANT_GROWTH,
            df.global.world.items.other.FOOD}
end

local function is_valid_clothing(item)
    return item.subtype.armorlevel == 0 and item.flags.on_ground
            and item.wear > 0
end

local function keep_usable(opts, item)
    return opts.keep_usable and (
                not item.corpse_flags.unbutchered and (
                    item.corpse_flags.bone or
                    item.corpse_flags.horn or
                    item.corpse_flags.leather or
                    item.corpse_flags.skull2 or
                    item.corpse_flags.tooth) or (
                item.corpse_flags.hair_wool or
                item.corpse_flags.pearl or
                item.corpse_flags.plant or
                item.corpse_flags.shell or
                item.corpse_flags.silk or
                item.corpse_flags.yarn) )
end

local function is_valid_corpse(opts, item)
    -- check if the corpse is a resident of the fortress and is not keep_usable
    local unit = df.unit.find(item.unit_id)
    if not unit then
        return not keep_usable(opts, item)
    end
    local hf = df.historical_figure.find(unit.hist_figure_id)
    if not hf then
        return not keep_usable(opts, item)
    end
    for _,link in ipairs(hf.entity_links) do
        if link.entity_id == df.global.plotinfo.group_id and df.histfig_entity_link_type[link:getType()] == 'MEMBER' then
            return false
        end
    end
    return not keep_usable(opts, item)
end

local function is_valid_remains(opts, item)
    return true
end

local function is_valid_food(opts, item)
    return true
end

local function increment_clothes_wear(item)
    item.wear_timer = math.ceil(item.wear_timer * (item.wear + 0.5))
    return item.wear > 2
end

local function increment_generic_wear(item, threshold)
    item.wear_timer = item.wear_timer + 1
    if item.wear_timer > threshold then
        item.wear_timer = 0
        item.wear = item.wear + 1
    end
    return item.wear > 3
end

local function increment_corpse_wear(item)
    return increment_generic_wear(item, 24)
end

local function increment_remains_wear(item)
    return increment_generic_wear(item, 6)
end

local function increment_food_wear(item)
    return increment_generic_wear(item, 24)
end

local function deteriorate(opts, get_item_vectors_fn, is_valid_fn, increment_wear_fn)
    local count = 0
    for _,v in ipairs(get_item_vectors_fn()) do
        for _,item in ipairs(v) do
            if is_valid_fn(opts, item) and increment_wear_fn(item)
                    and not item.flags.garbage_collect then
                item.flags.garbage_collect = true
                item.flags.hidden = true
                count = count + 1
            end
        end
    end
    return count
end

local function always_worn()
    return true
end

local function deteriorate_clothes(opts, now)
    return deteriorate(opts, get_clothes_vectors, is_valid_clothing,
                       now and always_worn or increment_clothes_wear)
end

local function deteriorate_corpses(opts, now)
    return deteriorate(opts, get_corpse_vectors, is_valid_corpse,
                       now and always_worn or increment_corpse_wear)
            + deteriorate(opts, get_remains_vectors, is_valid_remains,
                          now and always_worn or increment_remains_wear)
end

local function deteriorate_food(opts, now)
    return deteriorate(opts, get_food_vectors, is_valid_food,
                       now and always_worn or increment_food_wear)
end

local type_fns = {
    clothes=deteriorate_clothes,
    corpses=deteriorate_corpses,
    food=deteriorate_food,
}

-- maps the type string to {id=int, time=int, timeunit=string}
timeout_ids = timeout_ids or {
    clothes={},
    corpses={},
    food={},
}

local function _stop(item_type)
    local timeout_id = timeout_ids[item_type].id
    if timeout_id then
        dfhack.timeout_active(timeout_id, nil) -- cancel callback
        timeout_ids[item_type].id = nil
        return true
    end
end

local function make_timeout_cb(item_type, opts)
    local fn
    fn = function(first_time)
        local timeout_data = timeout_ids[item_type]
        timeout_data.time, timeout_data.mode = opts.time, opts.mode
        timeout_data.id = dfhack.timeout(opts.time, opts.mode, fn)
        if not timeout_ids[item_type].id then
            print('Map has been unloaded; stopping deteriorate')
            for k in pairs(type_fns) do
                _stop(k)
            end
            return
        end
        if not first_time then
            local count = type_fns[item_type](opts)
            if count > 0 then
                print(('Deteriorated %d %s'):format(count, item_type))
            end
        end
    end
    return fn
end

local function start(opts)
    for _,v in ipairs(opts.types) do
        _stop(v)
        if not opts.quiet then
            print(('Deterioration of %s commencing...'):format(v))
        end
        -- create a callback and call it to make it register itself
        make_timeout_cb(v, opts)(true)
    end
end

local function stop(opts)
    for _,v in ipairs(opts.types) do
        if _stop(v) and not opts.quiet then
            print('Stopped deteriorating ' .. v)
        end
    end
end

local function status()
    for k in pairs(type_fns) do
        local timeout_data = timeout_ids[k]
        local status_str = 'Stopped'
        if timeout_data.id then
            local time, mode = timeout_data.time, timeout_data.mode
            if time == 1 then
                mode = mode:sub(1, #mode - 1) -- make singular
            end
            status_str = ('Running (every %s %s)') :format(time, mode)
        end
        print(('%7s:\t%s'):format(k, status_str))
    end
end

local function now(opts)
    for _,v in ipairs(opts.types) do
        local count = type_fns[v](opts, true)
        if not opts.quiet then
            print(('Deteriorated %d %s'):format(count, v))
        end
    end
end

local function help()
    print(dfhack.script_help())
end

if dfhack_flags.module then
    return
end

if not dfhack.isMapLoaded() then
    qerror('deteriorate needs a fortress map to be loaded.')
end

local command_switch = {
    start=start,
    stop=stop,
    status=status,
    now=now,
}

local valid_timeunits = utils.invert{'days', 'months', 'years'}

local function parse_freq(arg)
    local elems = argparse.stringList(arg)
    local num = tonumber(elems[1])
    if not num or num <= 0 then
        qerror('number parameter for --freq option must be greater than 0')
    end
    if #elems == 1 then
        return num, 'days'
    end
    local timeunit = elems[2]:lower()
    if valid_timeunits[timeunit] then return num, timeunit end
    timeunit = timeunit .. 's' -- it's ok if the user specified a singular
    if valid_timeunits[timeunit] then return num, timeunit end
    qerror(('invalid time unit: "%s"'):format(elems[2]))
end

local function parse_types(arg)
    local types = argparse.stringList(arg)
    for _,v in ipairs(types) do
        if not type_fns[v] then
            qerror(('unrecognized type: "%s"'):format(v))
        end
    end
    return types
end

local opts = {
    time = 1,
    mode = 'days',
    quiet = false,
    types = {},
    keep_usable = false,
    help = false,
}

local nonoptions = argparse.processArgsGetopt({...}, {
        {'f', 'freq', 'frequency', hasArg=true,
         handler=function(optarg) opts.time,opts.mode = parse_freq(optarg) end},
        {'h', 'help', handler=function() opts.help = true end},
        {'q', 'quiet', handler=function() opts.quiet = true end},
        {'k', 'keep-usable', handler=function() opts.keep_usable = true end},
        {'t', 'types', hasArg=true,
         handler=function(optarg) opts.types = parse_types(optarg) end}})

local command = nonoptions[1]
if not command or not command_switch[command] then opts.help = true end

if not opts.help and command ~= 'status' and #opts.types == 0 then
    qerror('no item types specified! try adding a --types parameter.')
end

(command_switch[command] or help)(opts)
