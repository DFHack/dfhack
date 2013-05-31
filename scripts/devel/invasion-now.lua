-- invasion-now civ_id delay : schedules an invasion in the near future

args = {...}
num = tonumber(args[1])

if ( num ~= nil ) then
    time = tonumber(args[2]) or 1
    time2 = tonumber(args[3]) or time
    for i = time,time2 do
        df.global.timed_events:insert('#',{
            new = df.timed_event,
            type = df.timed_event_type.CivAttack,
            season = df.global.cur_season,
            season_ticks = df.global.cur_season_tick+i,
            entity = df.historical_entity.find(num)
        })
    end
end
