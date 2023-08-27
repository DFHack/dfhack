-- Force a migrant wave (only after hardcoded waves)
--[====[

migrants-now
============
Forces an immediate migrant wave. Only works after migrants have
arrived naturally. Roughly equivalent to `modtools/force` with::

    modtools/force -eventType migrants

]====]
df.global.timed_events:insert('#',{
    new = true,
    type = df.timed_event_type.Migrants,
    season = df.global.cur_season,
    season_ticks = df.global.cur_season_tick+1,
    entity = df.historical_entity.find(df.global.plotinfo.civ_id)
})
