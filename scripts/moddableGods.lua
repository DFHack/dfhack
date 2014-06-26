-- moddableGods.lua
-- Sets player-defined gods to correct civilizations.
-- author: Putnam
-- edited by expwnent

--[[Here's an example of how to make a god:

[CREATURE:SHEOGORATH]
 [DOES_NOT_EXIST]
 [MALE]
 [NAME:jovial man:Daedra:madness] "Sheogorath, madness god." "Often depicted as a jovial man"
 [CASTE_NAME:Sheogorath:Sheogorath:Sheogorath]
 [DESCRIPTION:The Daedric Prince of madness.]
 [CREATURE_CLASS:DFHACK_GOD]
 [SPHERE:MUSIC]
 [SPHERE:ART]
 [SPHERE:CHAOS]
]]

local function getCreatureClasses(creatureRaw)
 local creatureClasses = {}
 for _,caste in ipairs(creatureRaw.caste) do
  for k,class in ipairs(caste.creature_class) do
   table.insert(creatureClasses,class.value)
  end
 end
 return creatureClasses
end

local function deityIsOfSpecialCreature(creatureRaw)
 for k,class in ipairs(getCreatureClasses(creatureRaw)) do
  if class=="DFHACK_GOD" then return true end
 end
 return false
end

local function scriptAlreadyRunOnThisWorld()
 local figures = df.global.world.history.figures
 for i=#figures-1,0,-1 do --goes through the table backwards because the particular hist figs involved are probably going to be the last
  local figure = figures[i]
  if not df.isnull(figure.flags) and figure.flags.deity and deityIsOfSpecialCreature(df.global.world.raws.creatures.all[figure.race]) then
   return true
  end
 end
 return false
end

local function findAGod()
 for k,fig in ipairs(df.global.world.history.figures) do
  if fig.flags.deity then
   return fig
  end
 end
 return nil
end

local function putListOfAllSpecialCreatureGodsTogether()
 local specialCreatures = {}
 for k,creatureRaw in ipairs(df.global.world.raws.creatures.all) do
  if deityIsOfSpecialCreature(creatureRaw) then
   table.insert(specialCreatures,{creatureRaw,k})
  end
 end
 return specialCreatures
end

local function stringStarts(String,Start)
 return string.sub(String,1,string.len(Start))==Start
end

local function getRacesOfGod(god)
 local civList={}
 for k,class in ipairs(getCreatureClasses(god)) do
  if stringStarts(class,"WORSHIP_ENTITY_") then
   table.insert(civList,string.sub(class,15))
  end
 end
 return civList
end

local function entityIsInGodsDomain(entity,entityRacesTable)
 for k,v in ipairs(entityRacesTable) do
  if v==entity.entity_raw.code then
   return true
  end
 end
 return false
end

local function setUpTemplate(godFig,templateGod)
 godFig.appeared_year=-1
 godFig.born_year=-1
 godFig.born_seconds=-1
 godFig.curse_year=-1
 godFig.curse_seconds=-1
 godFig.old_year=-1
 godFig.old_seconds=-1
 godFig.died_year=-1
 godFig.died_seconds=-1
 godFig.name.has_name=true
 godFig.breed_id=-1
 godFig.flags:assign(templateGod.flags)
 godFig.id = df.global.hist_figure_next_id
 godFig.info = df.historical_figure_info:new()
 godFig.info.spheres={new=true}
 godFig.info.secret=df.historical_figure_info.T_secret:new()
end

local function setUpGod(god,godID,templateGod)
 local godFig = df.historical_figure:new()
 setUpTemplate(godFig,templateGod)
 godFig.sex=god.caste[0].gender
 godFig.race=godID
 godFig.name.first_name=god.caste[0].caste_name[2] --the adjectival form of the caste_name is used for the god's name, E.G, [CASTE_NAME:god:god:armok]
 for k,v in ipairs(god.sphere) do --assigning spheres
  godFig.info.spheres:insert('#',v)
 end
 df.global.world.history.figures:insert('#',godFig)
 df.global.hist_figure_next_id=df.global.hist_figure_next_id+1
 return godFig
end

--[[this function isn't really working right now so it's dummied out
function setGodAsOfficialDeityOfItsParticularEntity(god,godFig)
 local entityRaces=getRacesOfGod(god)
 for k,entity in ipairs(df.global.world.entities.all) do
  if entityIsInGodsDomain(entity,entityRaces) then
   entity.unknown1b.worship:insert('#',godFig.id)
  end
 end
end
]]
local function moddableGods()
 if scriptAlreadyRunOnThisWorld() then
  print("Already run on world...")
  return false
 end
 local gods = putListOfAllSpecialCreatureGodsTogether()
 local templateGod=findAGod()
 for k,v in ipairs(gods) do --creature raws first
  local god = v[1]
  local godID = v[2]
  local godFig = setUpGod(god,godID,templateGod)
  --setGodAsOfficialDeityOfItsParticularEntity(god,godFig)
 end
end

dfhack.onStateChange.letThereBeModdableGods = function(state)
 if state == SC_WORLD_LOADED and df.global.gamemode~=3 then --make sure that the gods show up only after the rest of the histfigs do
  moddableGods()
 end
end
