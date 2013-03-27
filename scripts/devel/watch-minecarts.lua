-- Logs minecart coordinates and speeds to console.

last_stats = last_stats or {}

function compare_one(vehicle)
    local last = last_stats[vehicle.id]
    local item = df.item.find(vehicle.item_id)
    local ipos = item.pos
    local new = {
        ipos.x*100000 + vehicle.offset_x, vehicle.speed_x,
        ipos.y*100000 + vehicle.offset_y, vehicle.speed_y,
        ipos.z*100000 + vehicle.offset_z, vehicle.speed_z
    }

    if (last == nil) or item.flags.on_ground then
        local delta = { vehicle.id }
        local show = (last == nil)

        for i=1,6 do
            local rv = 0
            if last then
                rv = last[i]
            end
            delta[i*2] = new[i]/100000
            local dv = new[i] - rv
            delta[i*2+1] = dv/100000
            if dv ~= 0 then
                show = true
            end
        end

        if show then
            print(table.unpack(delta))
        end
    end

    last_stats[vehicle.id] = new
end

function compare_all()
    local seen = {}
    for _,v in ipairs(df.global.world.vehicles.all) do
        seen[v.id] = true
        compare_one(v)
    end
    for k,v in pairs(last_stats) do
        if not seen[k] then
            print(k,'DEAD')
        end
    end
end

function start_timer()
    if not dfhack.timeout_active(timeout_id) then
        timeout_id = dfhack.timeout(1, 'ticks', function()
            compare_all()
            start_timer()
        end);
        if not timeout_id then
            dfhack.printerr('Could not start timer in watch-minecarts')
        end
    end
end

compare_all()

local cmd = ...

if cmd == 'start' then
    start_timer()
elseif cmd == 'stop' then
    dfhack.timeout_active(timeout_id, nil)
end

