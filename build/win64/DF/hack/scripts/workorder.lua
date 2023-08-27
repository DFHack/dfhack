-- workorder allows queuing manager jobs; it's smart about shear and milk creature jobs.

-- This script is inspired by stockflow.
-- It wouldn't've been possible w/o the df-ai by jjyg (https://github.com/jjyg/df-ai)
-- which is a great place to look up stuff like "How the hell do I find out if
-- a creature can be sheared?!!"

local function print_help()
    print(dfhack.script_help())
end

local utils = require 'utils'
local json = require 'json'
local path_json = "dfhack-config/workorder/"

local df = df
local world = df.global.world
local printerr = dfhack.printerr

local verbose = false
local debug_verbose = false

local function toggle_verbose()
    verbose = not verbose
    debug_verbose = debug_verbose and verbose

    print("workorder: verbose mode " .. (verbose and "on" or "off"))
end

local function toggle_debug_verbose()
    debug_verbose = not debug_verbose
    verbose = debug_verbose

    print("workorder: very verbose mode " .. (debug_verbose and "on" or "off"))
end

local used_types = {
    df.job_type,
    df.item_type,
    df.manager_order.T_frequency,
    df.manager_order_condition_item.T_compare_type,
    df.manager_order_condition_order.T_condition,
    df.tool_uses,
    df.job_art_specification.T_type
}
local function print_types(_, filter)
    for _, t in ipairs(used_types) do
        local type_name = tostring(t)
        if not filter
        or type_name:find(filter)
        then
            print(type_name)
            printall_ipairs(t)
        end
    end
end

-- [[ from stockflow.lua:

-- Compare the job specification of two orders.
local function orders_match(a, b)
    local fields = {
        "job_type",
        "item_subtype",
        "reaction_name",
        "mat_type",
        "mat_index",
    }

    for _, fieldname in ipairs(fields) do
        if a[fieldname] ~= b[fieldname] then
            return false
        end
    end

    local subtables = {
        "item_category",
        "material_category",
    }

    for _, fieldname in ipairs(subtables) do
        local aa = a[fieldname]
        local bb = b[fieldname]
        for key, value in ipairs(aa) do
            if bb[key] ~= value then
                return false
            end
        end
    end

    return true
end

-- Get the remaining quantity for open matching orders in the queue.
local function cur_order_quantity(order)
    local amount, cur_order, cur_idx = 0, nil, nil
    for idx, managed in ipairs(world.manager_orders) do
        if orders_match(order, managed) then
            -- if infinity, don't plan anything
            if 0 == managed.amount_total then
                return 0, managed, idx
            end
            amount = amount + managed.amount_left
            cur_order = cur_order or managed
            cur_idx = cur_idx or idx
        end
    end
    return amount, cur_order, cur_idx
end

-- make sure we have 'WEAPON' not 24.
local function ensure_df_string(df_list, key)
    local tmptype = type(key)
    if tmptype=='number' then
        return df_list[key]
    elseif tmptype=='string' then
        local n = tonumber(key)
        return n and df_list[n]
            or df_list[key] and key
    end
end

-- make sure we have 24 not 'WEAPON'
local function ensure_df_id(df_list, id)
    local tmptype = type(id)
    if tmptype=='number' then
        return df_list[id] and id
    elseif tmptype=='string' then
        local n = tonumber(id)
        return n and df_list[n] and n
            or df_list[id]
    end
end

local function set_flags_from_list(source, ...)
    local flags = utils.invert(source)

    for _, target in ipairs({...}) do
        for k in pairs(target) do
            if flags[k] then
                target[k] = true
                flags[k] = nil
            end
        end
    end

    local bad = {}
    for k in pairs(flags) do
        bad[#bad + 1] = k
    end

    return #bad == 0, (#bad == 0 or table.concat(bad, ", "))
end

-- returns subtype for given item_type; see orders.cpp
local function get_itemdef(item_type, subtype)
    item_type = ensure_df_string(df.item_type, item_type)
    if not item_type then
        printerr ("Unknown item_type: " .. tostring(item_type))
        return
    end

    local itemdef_st_name = 'itemdef_'.. string.lower ( item_type ) .. 'st'
    local def = df[ itemdef_st_name ]
    if not def then
        printerr ("Type df." .. itemdef_st_name .. " doesn't exist!" )
        return
    end

    local tmptype = type(subtype)
    if tmptype == 'number' then
        return def.find(subtype)
    elseif tmptype == 'string' then
        for _, v in ipairs(def.get_vector()) do
            if v.id == subtype then
                return v
            end
        end
    end
end

-- creates a df.manager_order from it's definition.
-- this is translated orders.cpp to Lua,
local function create_orders(orders)
    -- is dfhack.with_suspend necessary?

    -- we need id mapping to restore saved order_conditions
    local id_mapping = {}
    for _, it in ipairs(orders) do
        id_mapping[it["id"]] = world.manager_order_next_id
        world.manager_order_next_id = world.manager_order_next_id + 1
    end

    for _, it in ipairs (orders) do
        local order = df.manager_order:new()
        dfhack.with_onerror(function() order:delete() -- cleanup in case of errors
                                       if debug_verbose then
                                           printerr("Error in order:")
                                           printall_recurse(it)
                                       end
                            end,
        function()
        order.id = id_mapping[ it["id"] ]

        order.job_type = ensure_df_id(df.job_type, it["job"])
                    or qerror("Invalid job type for manager order: " .. it["job"])

        if it["reaction"] then
            order.reaction_name = it["reaction"]
        end

        if it["item_type"] then
            local tmp = ensure_df_id(df.item_type, it["item_type"])
            if not tmp or tmp == ensure_df_id(df.item_type, 'NONE') then
                qerror("Invalid item type for manager order: " .. it["item_type"])
            end
            order.item_type = tmp
        end

        if it["item_subtype"] then
            local tmp_item_type = order.item_type
            tmp_item_type = tmp_item_type == ensure_df_id(df.item_type, 'NONE')
                        and df.job_type.attrs[order.job_type].item
                        or tmp_item_type
            local def = get_itemdef(tmp_item_type, it["item_subtype"]);
            order.item_subtype = def and def.subtype
                        or qerror( "Invalid item subtype for manager order: "
                               .. df.item_type[order.item_type] .. ":" .. it["item_subtype"] )
        end

        if it["meal_ingredients"] then
            order.mat_type = tonumber(it["meal_ingredients"])
                        or qerror ( "Invalid meal ingredients for managed order: " .. it["meal_ingredients"]
                                .. " (is not a number). ")
            order.mat_index = -1
        elseif it["material"] then
            local mat = dfhack.matinfo.find(it["material"])
            if not mat then
                qerror( "Invalid material for manager order: " .. it["material"] )
            end
            order.mat_type = mat.type
            order.mat_index = mat.index
        end

        if it["item_category"] then
            local ok, bad = set_flags_from_list(it["item_category"], order.item_category)
            if not ok then
                qerror ("Invalid item_category value for manager order: " .. bad)
            end
        end

        if it["hist_figure"] then
            if not df.historical_figure.find(tonumber(it["hist_figure"])) then
                qerror("Missing historical figure for manager order: " .. it["hist_figure"])
            end

            order.hist_figure_id = tonumber(it["hist_figure"])
        end

        if it["material_category"] then
            local ok, bad = set_flags_from_list(it["material_category"], order.material_category)
            if not ok then
                qerror("Invalid material_category value for manager order: " .. bad)
            end
        end

        if it["art"] then
            order.art_spec.type = ensure_df_id(df.job_art_specification.T_type, it["art"]["type"])
                        or qerror ("Invalid art type value for manager order: " .. it["art"]["type"])
            order.art_spec.id = tonumber( it["art"]["id"] )
            if it["art"]["subid"] then
                order.art_spec.subid = tonumber( it["art"]["subid"] )
            end
        end

        --order.amount_left = tonumber(it["amount_left"]) -- handle later
        --order.amount_total = tonumber(it["amount_total"]) -- handle later
        --order.status.validated = it["is_validated"] -- ignoring
        --order.status.active = it["is_active"] -- ignoring

        order.frequency = ensure_df_id(df.manager_order.T_frequency, it["frequency"])
                        or qerror("Invalid frequency value for manager order: " .. it["frequency"])

        -- finished_year, finished_year_tick

        if it["workshop_id"] then
            local ws = df.building.find(tonumber(it["workshop_id"]))
            if not ws then
                qerror( "Missing workshop for manager order: " .. it["workshop_id"] )
            end
            order.workshop_id = tonumber(it["workshop_id"])
        end

        if it["max_workshops"] then
            order.max_workshops = tonumber(it["max_workshops"])
        end

        if it["item_conditions"] then
            for _, it2 in ipairs(it["item_conditions"]) do
                condition = df.manager_order_condition_item:new()
                dfhack.with_onerror(function() condition:delete() end, -- cleanup in case of errors
                function()
                condition.compare_type = ensure_df_id(df.manager_order_condition_item.T_compare_type, it2["condition"])
                                    or qerror ("Invalid item condition for manager order: " .. it2["condition"] )
                condition.compare_val = tonumber(it2["value"])

                if it2["flags"] then
                    local ok, bad = set_flags_from_list(it2["flags"],
                                            condition.flags1,
                                            condition.flags2,
                                            condition.flags3) -- flags4, flags5
                    if not ok then
                        qerror("Invalid item condition flags for manager order: " .. bad)
                    end
                end

                if it2["item_type"] then
                    local tmp = ensure_df_id(df.item_type, it2["item_type"])
                    if not tmp or tmp == ensure_df_id(df.item_type, 'NONE') then
                        qerror("Invalid item condition item type for manager order: " .. it2["item_type"])
                    end
                    condition.item_type = tmp
                end

                if it2["item_subtype"] then
                    local def = get_itemdef(condition.item_type, it2["item_subtype"]);
                    condition.item_subtype = def and def.subtype
                                    or qerror ( "Invalid item condition item subtype for manager order: "
                                            .. df.item_type[condition.item_type]
                                            .. ":" .. it2["item_subtype"] )
                end

                if it2["material"] then
                    local mat = dfhack.matinfo.find(it2["material"])
                    if not mat then
                        qerror( "Invalid item condition material for manager order: " .. it2["material"] )
                    end
                    condition.mat_type = mat.type
                    condition.mat_index = mat.index
                end

                if it2["bearing"] then
                    local bearing = it2["bearing"]
                    local idx
                    for i, raw in ipairs(world.raws.inorganics) do
                        if raw.id == bearing then
                            idx = i
                            break
                        end
                    end
                    condition.inorganic_bearing = idx
                                            or qerror( "Invalid item condition inorganic bearing type for manager order: " .. it2["bearing"] )
                end

                if it2["reaction_class"] then
                    condition.reaction_class = it2["reaction_class"]
                end

                if it2["reaction_product"] then
                    condition.has_material_reaction_product = it2["reaction_product"]
                end

                if it2["tool"] then
                    local tmp = ensure_df_id(df.tool_uses, it2["tool"])
                    if not tmp or tmp == ensure_df_id(df.tool_uses, 'NONE') then
                        qerror("Invalid item condition tool use for manager order: " .. it2["tool"])
                    end
                    condition.has_tool_use = tmp
                end

                condition.min_dimension = -1
                -- condition.contains = ?
                condition.reaction_id = -1

                order.item_conditions:insert('#', condition)
                end)
            end
        end

        if it["order_conditions"] then
            for _, it2 in ipairs(it["order_conditions"]) do
                local condition = df.manager_order_condition_order:new()
                dfhack.with_onerror(function() condition:delete() end, -- cleanup in case of errors
                function()
                local id = tonumber(it2["order"])
                condition.order_id = id ~= it["id"] and id_mapping[id]
                                    or qerror("Missing order condition target for manager order: " .. it2["order"])

                condition.condition = ensure_df_id(df.manager_order_condition_order.T_condition, it2["condition"])
                                    or qerror ( "Invalid order condition type for manager order: " .. it2["condition"] )

                -- condition.unk_1

                order.order_conditions:insert('#', condition)
                end)
            end
        end
        --order.items = vector<job_item*>

        local amount = it.amount_total
        if it.__reduce_amount then
            -- modify existing order if possible
            local cur_amount, cur_order, cur_order_idx = cur_order_quantity(order)
            if cur_order then
                if 0 == cur_amount then
                    amount = -1
                elseif 0 ~= amount then
                    local diff = amount - cur_order.amount_left
                    amount = -1
                    if verbose then print('adjusting existing order by', diff) end
                    cur_order.amount_left = cur_order.amount_left + diff
                    cur_order.amount_total = cur_order.amount_total + diff
                    if cur_order.amount_left <= 0 then
                        if verbose then print('negative amount; removing existing order') end
                        world.manager_orders:erase(cur_order_idx)
                        cur_order:delete()
                    end
                end
            end
        end

        if amount < 0 then
            if verbose then
                print(string.format("Order %s (%s) not queued.",
                    it.id, df.job_type[order.job_type]))
            end
            order:delete()
        else
            order.amount_left = amount
            order.amount_total = amount

            print("Queuing " .. df.job_type[order.job_type]
                .. (amount==0 and " infinitely" or " x"..amount))
            world.manager_orders:insert('#', order)
        end
        end)
    end
end

-- set missing values, process special `amount_total` value
local function preprocess_orders(orders)
    -- if called with single order make an array
    if orders.job then
        orders = {orders}
    end

    local ret = {}
    for i, order in ipairs(orders) do
        -- every order needs an id.
        if not order.id then order.id = -i end

        -- allow smart choices
        if not order.amount_total then
            order.__reduce_amount = (order.__reduce_amount == nil) and true
                                or order.__reduce_amount
            local fn = _ENV[ "calcAmountFor_" .. tostring(ensure_df_string(df.job_type, order.job)) ]
            if fn and type(fn)=="function" then
                order.amount_total = fn(order)
            end
        elseif type(order.amount_total)=="string" then
            local fn = _ENV[ order.amount_total ]
                    or load(order.amount_total) -- allow custom "inline" functions
            if fn and type(fn)=="function" then
                order.amount_total = fn(order, orders)
            end
        end

        -- allow omitting amount
        local amount = tonumber(order.amount_total)
        if not amount then
            amount = order.__reduce_amount and 1
                    or 0
            if order.amount_total == nil then
                if verbose then
                    print ("Missing amount_total, set to " .. (amount==0 and "infinity" or amount))
                end
            else
                printerr ("Invalid amount_total: " .. tostring(order.amount_total)
                    .. " changed to " .. (amount==0 and "infinity" or amount))
            end
        end
        order.amount_total = amount

        --- this needs to be postponed to when the order is created,
        --- because values here use human-readable names, while orders_match
        --- uses DF internal ids. This can be improved in future versions.
        -- allow choosing to reduce existing amount
        --if order.amount_total > 0 and order.__reduce_amount then
        --    order.amount_total = order_quantity(order, order.amount_total)
        --    if 0 == order.amount_total then order.amount_total = -1 end
        --end

        if debug_verbose then
            print(string.format("order.id<json>: %s; job: %s; .amount_total: %s; .__reduce_amount: %s",
            order.id, df.job_type[ order.job ], order.amount_total, order.__reduce_amount))
        end
        if order.amount_total >= 0 or order.__reduce_amount then
            ret[#ret + 1] = order
        end
    end

    return ret
end

local order_defaults = {
    frequency = 'OneTime'
}
local _order_mt = {__index = order_defaults}
local function fillin_defaults(orders)
    for _, order in ipairs(orders) do
        setmetatable(order, _order_mt)
    end
end

default_action = function (...)
    if debug_verbose then
        print("Parameters:")
        for k,v in pairs({...}) do
            print(k,v)
        end
    end

    local v, n = ...
    local jobtype, orders
    if v == "-f" or v == "--file" then
        orders = json.decode_file(path_json .. n .. ".json")
    else
        jobtype = df.job_type[tonumber(v)] and tonumber(v)
               or df.job_type[ v ]
        orders = not jobtype and json.decode( table.concat({...}, " ") )
    end

    if not (jobtype or orders) then
        printerr ("Unknown jobtype: " .. tostring(v))
        return
    end

    if jobtype then
        local order = {}
        order.job = jobtype
        order.amount_total = tonumber(n)
        order.__reduce_amount = not order.amount_total
        if debug_verbose then
            print(string.format("order.job: %s; .amount_total: %s; .__reduce_amount: %s",
            df.job_type[ order.job ], order.amount_total, order.__reduce_amount))
        end

        orders = {order}
    end

    orders = preprocess_orders(orders)
    if verbose then print ("Got " .. #orders .. " orders, processing...") end
    fillin_defaults(orders)
    create_orders(orders)
end

local uu = dfhack.units
local function isValidAnimal(u)
    -- this should also check for the absence of misc trait 55 (as of 50.09), but we don't
    -- currently have an enum definition for that value yet
    return uu.isOwnCiv(u)
        and uu.isAlive(u)
        and uu.isAdult(u)
        and uu.isActive(u)
        and uu.isFortControlled(u)
        and uu.isTame(u)
        and not uu.isMarkedForSlaughter(u)
        and not uu.getMiscTrait(u, df.misc_trait_type.Migrant, false)
end

calcAmountFor_MilkCreature = function ()
    local cnt = 0
    if debug_verbose then print "Milkable units:" end
    for i, u in pairs(world.units.active) do
        if isValidAnimal(u) and uu.isMilkable(u) and not uu.isPet(u) then
            local mt_milk = uu.getMiscTrait(u, df.misc_trait_type.MilkCounter, false)
            if not mt_milk then cnt = cnt + 1 end

            if debug_verbose then
                local mt_milk_val = mt_milk and mt_milk.value or "not milked recently"
                print(u.id, uu.getRaceName(u), mt_milk_val)
            end
        end
    end
    if debug_verbose then print ("Milking jobs needed: " .. cnt) end
    return (cnt==0 and -1 or cnt)
end

-- true/false or nil if no shearable_tissue_layer with length > 0.
local function canShearCreature(u)
    local stls = world.raws.creatures
            .all[u.race]
            .caste[u.caste]
            .shearable_tissue_layer

    local any
    for _, stl in ipairs(stls) do
        if stl.length > 0 then

            for _, bpi in ipairs(stl.bp_modifiers_idx) do
                any = {u.appearance.bp_modifiers[bpi], stl.length}
                if u.appearance.bp_modifiers[bpi] >= stl.length then
                    return true, any
                end
            end

        end
    end

    if any then return false, any end
    -- otherwise: nil
end

calcAmountFor_ShearCreature = function ()
    local cnt = 0
    if debug_verbose then print "Shearable units:" end
    for i, u in pairs(world.units.active) do
        if isValidAnimal(u) then
            local can, info = canShearCreature(u)
            if can then cnt = cnt + 1 end

            if debug_verbose and (can ~= nil) then
                print(i, uu.getRaceName(u), can, tostring(info[1]) .. '/' .. tostring(info[2]))
            end
        end
    end
    if debug_verbose then print ("Shearing jobs needed: " .. cnt) end

    return (cnt==0 and -1 or cnt)
end

actions = {
    -- help
    ["-?"] = print_help,
    ["?"] = print_help,
    ["--help"] = print_help,
    ["help"] = print_help,
    -- useful info
    ["--listtypes"] = print_types,
    ["listtypes"] = print_types,
    ["l"] = print_types,
    ["-l"] = print_types,
    -- controlling state
    ["--verbose"] = toggle_verbose,
    ["-v"] = toggle_verbose,
    ["--very-verbose"] = toggle_debug_verbose,
    ["-vv"] = toggle_debug_verbose,
    ["--reset"] = function() initialized = false end,
}

-- Lua is beautiful.
(actions[ (...) or "?" ] or default_action)(...)
