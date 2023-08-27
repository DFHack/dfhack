-- assigns minecarts to hauling routes
--@ module = true

local argparse = require('argparse')

function get_free_vehicles()
    local free_vehicles = {}
    for _,vehicle in ipairs(df.global.world.vehicles.active) do
        if vehicle and vehicle.route_id == -1 then
            table.insert(free_vehicles, vehicle)
        end
    end
    return free_vehicles
end

local function has_minecart(route)
    return #route.vehicle_ids > 0
end

local function has_stops(route)
    return #route.stops > 0
end

local function get_name(route)
    return route.name and #route.name > 0 and route.name or ('Route '..route.id)
end

local function get_id_and_name(route)
    return ('%d (%s)'):format(route.id, get_name(route))
end

local function assign_minecart_to_route(route, quiet, minecart)
    if has_minecart(route) then
        return true
    end
    if not has_stops(route) then
        if not quiet then
            dfhack.printerr(
                ('Route %s has no stops defined. Cannot assign minecart.')
                :format(get_id_and_name(route)))
        end
        return false
    end
    if not minecart then
        minecart = get_free_vehicles()[1]
        if not minecart then
            if not quiet then
                dfhack.printerr('No minecarts available! Please build some.')
            end
            return false
        end
    end
    route.vehicle_ids:insert('#', minecart.id)
    route.vehicle_stops:insert('#', 0)
    minecart.route_id = route.id
    if not quiet then
        print(('Assigned a minecart to route %s.')
              :format(get_id_and_name(route)))
    end
    return true
end

-- assign first free minecart to the most recently-created route
-- returns whether route now has a minecart assigned
function assign_minecart_to_last_route(quiet)
    local routes = df.global.plotinfo.hauling.routes
    local route_idx = #routes - 1
    if route_idx < 0 then
        return false
    end
    local route = routes[route_idx]
    return assign_minecart_to_route(route, quiet)
end

local function get_route_by_id(route_id)
    for _,route in ipairs(df.global.plotinfo.hauling.routes) do
        if route.id == route_id then
            return route
        end
    end
end

local function list()
    local routes = df.global.plotinfo.hauling.routes
    if 0 == #routes then
        print('No hauling routes defined.')
    else
        print(('Found %d route%s:\n')
              :format(#routes, #routes == 1 and '' or 's'))
        print('route id  minecart?  has stops?  route name')
        print('--------  ---------  ----------  ----------')
        for _,route in ipairs(routes) do
            print(('%-8d  %-9s  %-9s  %s')
                  :format(route.id,
                          has_minecart(route) and 'yes' or 'NO',
                          has_stops(route) and 'yes' or 'NO',
                          get_name(route)))
        end
    end
    local minecarts = get_free_vehicles()
    print(('\nYou have %d unassigned minecart%s.')
          :format(#minecarts, #minecarts == 1 and '' or 's'))
end

local function all(quiet)
    local minecarts, idx = get_free_vehicles(), 1
    local routes = df.global.plotinfo.hauling.routes
    for _,route in ipairs(routes) do
        if has_minecart(route) then
            goto continue
        end
        if not assign_minecart_to_route(route, quiet, minecarts[idx]) then
            return
        end
        idx = idx + 1
        ::continue::
    end
end

local function do_help(_)
    print(dfhack.script_help())
end

local command_switch = {
    list=list,
    all=all,
}

local function main(args)
    local help, quiet = false, false
    local command = argparse.processArgsGetopt(args, {
            {'h', 'help', handler=function() help = true end},
            {'q', 'quiet', handler=function() quiet = true end}})[1]

    if help then
        command = nil
    end

    local requested_route_id = tonumber(command)
    if requested_route_id then
        local route = get_route_by_id(requested_route_id)
        if not route then
            dfhack.printerr('route id not found: '..requested_route_id)
        elseif has_minecart(route) then
            if not quiet then
                print(('Route %s already has a minecart assigned.')
                    :format(get_id_and_name(route)))
            end
        else
            assign_minecart_to_route(route, quiet)
        end
        return
    end

    (command_switch[command] or do_help)(quiet)
end

if not dfhack_flags.module then
    main({...})
end
