-- invasion-now civName start end : schedules an invasion in the near future, or several

args = {...}
civName = args[1]

if ( civName ~= nil ) then
    --find the civ with that name
    evilEntity = nil;
    for _,candidate in ipairs(df.global.world.entities.all) do
        if candidate.entity_raw.code == civName then
            evilEntity = candidate
            break
        end
    end
    if ( evilEntity == nil ) then
        do return end
    end
    time = tonumber(args[2]) or 1
    time2 = tonumber(args[3]) or time
    for i = time,time2 do
        df.global.timed_events:insert('#',{
            new = df.timed_event,
            type = df.timed_event_type.CivAttack,
            season = df.global.cur_season,
            season_ticks = df.global.cur_season_tick+i,
            entity = evilEntity
        })
    end
end
