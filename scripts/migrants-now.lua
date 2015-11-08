-- Force a migrant wave (only works after hardcoded waves)
--[[=begin

migrants-now
============
Forces an immediate migrant wave.  Only works after migrants have
arrived naturally.  Equivalent to `modtools/force` ``-eventType migrants``

=end]]
df.global.timed_events:insert('#',{
    new = true,
    type = df.timed_event_type.Migrants,
    season = df.global.cur_season,
    season_ticks = df.global.cur_season_tick+1,
    entity = df.historical_entity.find(df.global.ui.civ_id)
})
