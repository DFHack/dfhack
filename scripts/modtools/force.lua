-- force.lua
-- author Putnam
-- edited by expwnent
-- Forces an event.

local function findCiv(arg)
 local entities = df.global.world.entities.all
 if tonumber(arg) then return arg end
 if arg then 
  for eid,entity in ipairs(entities) do
   if entity.entity_raw.code == arg then return entity end
  end
 end
 return nil
end

local args = {...}

if not args or not args[1] then
 qerror('Needs an argument. Valid arguments are caravan, migrants, diplomat, megabeast, curiousbeast, mischievousbeast, flier, siege and nightcreature. Second argument is civ, either raw entity ID or "player" for player\'s civ.')
end

local eventType = string.lower(args[1])

forceEntity = args[2]=="player" and df.historical_entity.find(df.global.ui.civ_id) or findCiv(args[2])

if (eventType == "caravan" or eventType == "diplomat" or eventType == "siege") and not forceEntity then
 qerror('caravan, diplomat and siege require a civilization ID to be included.')
end

local function eventTypeIsNotValid()
 local eventTypes = {
  "caravan",
  "migrants",
  "diplomat",
  "megabeast",
  "curiousbeast",
  "mischevousbeast",
  "mischeviousbeast",
  "flier",
  "siege",
  "nightcreature"
 }
 for _,v in ipairs(eventTypes) do
  if args[1] == v then return false end
 end
 return true
end    

--code may be kind of bad below :V Putnam ain't experienced in lua... --Putnam's comment, not mine ~expwnent
if eventTypeIsNotValid() then
 qerror('Invalid argument. Valid arguments are caravan, migrants, diplomat, megabeast, curiousbeast, mischievousbeast, flier, siege and nightcreature.')
end

allEventTypes={}
allEventTypes["megabeast"]=function()
 df.global.timed_events:insert('#', { new = df.timed_event, type = df.timed_event_type.Megabeast, season = df.global.cur_season, season_ticks = df.global.cur_season_tick } )
end
allEventTypes["migrants"]=function()
 df.global.timed_events:insert('#', { new = df.timed_event, type = df.timed_event_type.Migrants, season = df.global.cur_season, season_ticks = df.global.cur_season_tick, entity = df.global.world.entities.all[df.global.ui.civ_id] } )
end
allEventTypes["caravan"]=function()
 df.global.timed_events:insert('#', { new = df.timed_event, type = df.timed_event_type.Caravan, season = df.global.cur_season, season_ticks = df.global.cur_season_tick, entity = forceEntity } )
end
allEventTypes["diplomat"]=function()
 df.global.timed_events:insert('#', { new = df.timed_event, type = df.timed_event_type.Diplomat, season = df.global.cur_season, season_ticks = df.global.cur_season_tick, entity = forceEntity } )
end
allEventTypes["curious"]=function()
 df.global.timed_events:insert('#', { new = df.timed_event, type = df.timed_event_type.WildlifeCurious, season = df.global.cur_season, season_ticks = df.global.cur_season_tick } )
end
allEventTypes["mischevousbeast"]=function()
 df.global.timed_events:insert('#', { new = df.timed_event, type = df.timed_event_type.WildlifeMichievous, season = df.global.cur_season, season_ticks = df.global.cur_season_tick } )
end
allEventTypes["flier"]=function()
 df.global.timed_events:insert('#', { new = df.timed_event, type = df.timed_event_type.WildlifeFlier, season = df.global.cur_season, season_ticks = df.global.cur_season_tick } )
end
allEventTypes["siege"]=function()
 df.global.timed_events:insert('#', { new = df.timed_event, type = df.timed_event_type.CivAttack, season = df.global.cur_season, season_ticks = df.global.cur_season_tick, entity = forceEntity } )
end
allEventTypes["nightcreature"]=function()
 df.global.timed_events:insert('#', { new = df.timed_event, type = df.timed_event_type.NightCreature, season = df.global.cur_season, season_ticks = df.global.cur_season_tick } )
end

allEventTypes[eventType]()
