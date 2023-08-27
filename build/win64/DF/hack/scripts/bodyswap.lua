-- Shifts player control over to another unit in adventure mode.
-- author: Atomic Chicken
-- based on "assumecontrol.lua" by maxthyme, as well as the defunct advtools plugin "adv-bodyswap"
-- calls "modtools/create-unit" for nemesis and histfig creation

--@ module = true

local utils = require 'utils'
local validArgs = utils.invert({
  'unit',
  'help'
})
local args = utils.processArgs({...}, validArgs)

local usage = [====[

bodyswap
========
This script allows the player to take direct control of any unit present in
adventure mode whilst giving up control of their current player character.

To specify the target unit, simply select it in the user interface,
such as by opening the unit's status screen or viewing its description,
and enter "bodyswap" in the DFHack console.

Alternatively, the target unit can be specified by its unit id as shown below.

Arguments::

    -unit id
        replace "id" with the unit id of your target
        example:
            bodyswap -unit 42

]====]

if args.help then
  print(usage)
  return
end

function setNewAdvNemFlags(nem)
  nem.flags.ACTIVE_ADVENTURER = true
  nem.flags.RETIRED_ADVENTURER = false
  nem.flags.ADVENTURER = true
end
function setOldAdvNemFlags(nem)
  nem.flags.ACTIVE_ADVENTURER = false
end

function clearNemesisFromLinkedSites(nem)
-- omitting this step results in duplication of the unit entry in df.global.world.units.active when the site to which the historical figure is linked is reloaded with said figure present as a member of the player party
-- this can be observed as part of the normal recruitment process when the player adds a site-linked historical figure to their party
  if not nem.figure then
    return
  end
  for _,link in ipairs(nem.figure.site_links) do
    local site = df.world_site.find(link.site)
    for i = #site.unk_1.nemesis-1, 0, -1 do
      if site.unk_1.nemesis[i] == nem.id then
        site.unk_1.nemesis:erase(i)
      end
    end
  end
end

function createNemesis(unit)
  local nemesis = reqscript('modtools/create-unit').createNemesis(unit,unit.civ_id)
  nemesis.figure.flags.never_cull = true
  return nemesis
end

function isPet(nemesis)
  if nemesis.unit then
    if nemesis.unit.relationship_ids.Pet ~= -1 then
      return true
    end
  elseif nemesis.figure then -- in case the unit is offloaded
    for  _, link in ipairs(nemesis.figure.histfig_links) do
      if link._type == df.histfig_hf_link_pet_ownerst then
        return true
      end
    end
  end
  return false
end

function processNemesisParty(nemesis, targetUnitID, alreadyProcessed)
-- configures the target and any leaders/companions to behave as cohesive adventure mode party members
  local alreadyProcessed = alreadyProcessed or {}
  alreadyProcessed[tostring(nemesis.id)] = true

  local nemUnit = nemesis.unit
  if nemesis.unit_id == targetUnitID then -- the target you're bodyswapping into
    df.global.adventure.interactions.party_core_members:insert('#', nemesis.figure.id)
    nemUnit.relationship_ids.GroupLeader = -1
  elseif isPet(nemesis) then -- pets belonging to the target or to their companions
    df.global.adventure.interactions.party_pets:insert('#', nemesis.figure.id)
  else
    df.global.adventure.interactions.party_core_members:insert('#', nemesis.figure.id) -- placing all non-pet companions into the core party list to enable tactical mode swapping
    nemesis.flags.ADVENTURER = true
    if nemUnit then -- check in case the companion is offloaded
      nemUnit.relationship_ids.GroupLeader = targetUnitID
    end
  end
-- the hierarchy of nemesis-level leader/companion relationships appears to be left untouched when the player character is changed using the inbuilt "tactical mode" party system

  clearNemesisFromLinkedSites(nemesis)

  if nemesis.group_leader_id ~= -1 and not alreadyProcessed[tostring(nemesis.group_leader_id)] then
    local leader = df.nemesis_record.find(nemesis.group_leader_id)
    if leader then
      processNemesisParty(leader, targetUnitID, alreadyProcessed)
    end
  end
  for _, id in ipairs(nemesis.companions) do
    if not alreadyProcessed[tostring(id)] then
      local companion = df.nemesis_record.find(id)
      if companion then
        processNemesisParty(companion, targetUnitID, alreadyProcessed)
      end
    end
  end
end

function configureAdvParty(targetNemesis)
  local party = df.global.adventure.interactions
  party.party_core_members:resize(0)
  party.party_pets:resize(0)
  party.party_extra_members:resize(0)
  processNemesisParty(targetNemesis, targetNemesis.unit_id)
end

function swapAdvUnit(newUnit)

  if not newUnit then
    qerror('Target unit not specified!')
  end

  local oldNem = df.nemesis_record.find(df.global.adventure.player_id)
  local oldUnit = oldNem.unit
  if newUnit == oldUnit then
    return
  end

  local activeUnits = df.global.world.units.active
  local oldUnitIndex
  if activeUnits[0] == oldUnit then
    oldUnitIndex = 0
  else -- unlikely; this is just in case
    for i,u in ipairs(activeUnits) do
      if u == oldUnit then
        oldUnitIndex = i
        break
      end
    end
  end
  local newUnitIndex
  for i,u in ipairs(activeUnits) do
    if u == newUnit then
      newUnitIndex = i
      break
    end
  end

  if not newUnitIndex then
    qerror("Target unit index not found!")
  end

  local newNem = dfhack.units.getNemesis(newUnit) or createNemesis(newUnit)
  if not newNem then
    qerror("Failed to obtain target nemesis!")
  end

  setOldAdvNemFlags(oldNem)
  setNewAdvNemFlags(newNem)
  configureAdvParty(newNem)
  df.global.adventure.player_id = newNem.id
  activeUnits[newUnitIndex] = oldUnit
  activeUnits[oldUnitIndex] = newUnit
  oldUnit.idle_area:assign(oldUnit.pos)
end

if not dfhack_flags.module then
  if df.global.gamemode ~= df.game_mode.ADVENTURE then
    qerror("This script can only be used in adventure mode!")
  end

  local unit = args.unit and df.unit.find(tonumber(args.unit)) or dfhack.gui.getSelectedUnit()
  if not unit then
    print("Enter the following if you require assistance: bodyswap -help")
    if args.unit then
      qerror("Invalid unit id: "..args.unit)
    else
      qerror("Target unit not specified!")
    end
  end
  swapAdvUnit(unit)
end
