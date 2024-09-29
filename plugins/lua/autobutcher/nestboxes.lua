local _ENV = mkmodule('plugins.autobutcher.nestboxes')
---------------------------------------------------------------------------------------------------
local nestboxesCommon = require('plugins.autobutcher.common')
local nestboxesEvent = require('plugins.autobutcher.nestboxesEvent')
local eventful = require('plugins.eventful')
local utils = require('utils')
local printLocal = nestboxesCommon.printLocal
local printDetails = nestboxesCommon.printDetails
local handleError = nestboxesCommon.handleError
local dumpToString = nestboxesCommon.dumpToString
local GLOBAL_KEY = 'autobutcher.nestboxes'
local EVENT_FREQ = 7
local string_or_int_to_boolean = {
  ['true'] = true,
  ['false'] = false,
  ['1'] = true,
  ['0'] = false,
  ['Y'] = true,
  ['N'] = false,
  [1] = true,
  [0] = false
}
local function getBoolean(value)
  return string_or_int_to_boolean[value]
end
---------------------------------------------------------------------------------------------------
local default_table = {
  watched = true, -- monitor eggs for race
  target = 4, --basic target for protected(forbidden) eggs
  ama = true, --Add Missing Animals to basic target for prottected eggs, difference between current amount of animals and target set for autobutcher, speeds up population growth in initial phase whiile limitting population explosion near end if  egg target has low value
  stop = true -- stop protecting eggs once autobutcher target for live animals is reached
}
---------------------------------------------------------------------------------------------------
local function getDefaultState()
  return {
    enabled = false, -- enabled status for nestboxes
    verbose = false, -- verbose mode
    ignore_autobutcher = false, -- if set to true nestboxes will work on their own ignoring autobutcher enabled status or if it's watching race
    migration_from_cpp_to_lua_done = false, --flag to handle migration from cpp nestboxes
    split_stacks = true, -- should eggs stacks be split if only part is over limit, if set to false whole stack will be forbiden
    default = default_table, -- default settings applied to new races
    config_per_race = {} -- config per race
  }
end --getDefaultState
---------------------------------------------------------------------------------------------------
state = state or getDefaultState()
---------------------------------------------------------------------------------------------------
local function persistState()
  printDetails(('start persistState'))
  local state_to_persist = {}
  state_to_persist = utils.clone(state)
  state_to_persist.config_per_race = {}

  if state.config_per_race ~= nil then
    for k, v in pairs(state.config_per_race) do
      state_to_persist.config_per_race[tostring(k)] = v
    end
  end

  dfhack.persistent.saveSiteData(GLOBAL_KEY, state_to_persist)
  printDetails(('end persistState'))
end --persistState
---------------------------------------------------------------------------------------------------
local function readPersistentConfig(key, index)
  if dfhack.internal.readPersistentSiteConfigInt ~= nil then
    return dfhack.internal.readPersistentSiteConfigInt(key, index)
  end
  return nil
end -- readPersistentConfig

---------------------------------------------------------------------------------------------------
local function migrateEnabledStatusFromCppNestboxes()
  printLocal('About to attempt migration from nestboxes')
  local nestboxes_status = readPersistentConfig('nestboxes/config', '0')
  printLocal(
    ('Migrating status %s from cpp nestboxes'):format(getBoolean(nestboxes_status) and 'enabled' or 'disabled')
  )
  state.enabled = getBoolean(nestboxes_status) or false
  state.migration_from_cpp_to_lua_done = true
  dfhack.persistent.deleteSiteData('nestboxes/config')
  persistState()
  printLocal('Migration from cpp to lua done')
end --migrateEnabledStatusFromCppNestboxes
---------------------------------------------------------------------------------------------------
---------------------------------------------------------------------------------------------------
local function initEggwatchCommon()
  nestboxesCommon.verbose = state.verbose
  nestboxesCommon.prefix = GLOBAL_KEY
end --initEggwatchCommon
---------------------------------------------------------------------------------------------------
--- Load the saved state of the script
local function loadState()
  printDetails(('start loadState'))
  -- load persistent data
  local persisted_data = dfhack.persistent.getSiteData(GLOBAL_KEY, getDefaultState())
  local processed_persisted_data = {}
  if persisted_data ~= nil then
    processed_persisted_data = utils.clone(persisted_data)
    processed_persisted_data.config_per_race = {}
    if persisted_data.config_per_race ~= nil then
      for k, v in pairs(persisted_data.config_per_race) do
        local default = utils.clone(default_table)
        processed_persisted_data.config_per_race[tonumber(k)] = utils.assign(default, v)
      end
    end
  end
  state = getDefaultState()
  utils.assign(state, processed_persisted_data)
  if not state.migration_from_cpp_to_lua_done then
    migrateEnabledStatusFromCppNestboxes()
  end
  initEggwatchCommon()
  printDetails(('end loadState'))
end --loadState
---------------------------------------------------------------------------------------------------
local function updateEventListener()
  printDetails(('start updateEventListener'))
  if state.enabled then
    eventful.enableEvent(eventful.eventType.ITEM_CREATED, EVENT_FREQ)
    eventful.onItemCreated[GLOBAL_KEY] = checkItemCreated
    printLocal(('Subscribing in eventful for %s with frequency %s'):format('ITEM_CREATED', EVENT_FREQ))
  else
    eventful.onItemCreated[GLOBAL_KEY] = nil
    printLocal(('Unregistering from eventful for %s'):format('ITEM_CREATED'))
  end
  printDetails(('end updateEventListener'))
end --updateEventListener
---------------------------------------------------------------------------------------------------
local function doEnable()
  printDetails(('start doEnable'))
  state.enabled = true
  updateEventListener()
  printDetails(('end doEnable'))
end --doEnable
---------------------------------------------------------------------------------------------------
local function doDisable()
  printDetails(('start doDisable'))
  state.enabled = false
  updateEventListener()
  printDetails(('end doDisable'))
end --doDisable
---------------------------------------------------------------------------------------------------
local function getConfigForRace(race)
  printDetails(('start getConfigForRace'))
  printDetails(('getting config for race %s '):format(race))
  if state.config_per_race[race] == nil and state.default.watched then
    state.config_per_race[race] = state.default
    persistState()
  end
  printDetails(('end getConfigForRace'))
  return state.config_per_race[race]
end --getConfigForRace
---------------------------------------------------------------------------------------------------
local function validateCreatureOrRace(value)
  printDetails(('start validate_creature_id'))
  if value == 'DEFAULT' or value == 'ALL' then
    return value
  end
  for id, c in ipairs(df.global.world.raws.creatures.all) do
    if c.creature_id == value or id == value then
      for _, c in ipairs(c.caste) do
        if c.flags.LAYS_EGGS then
          return id
        end
      end
      handleError(('%s is not egglayer'):format(value))
    end
  end
  handleError(('could not find %s'):format(value))
end --validateCreatureOrRace
---------------------------------------------------------------------------------------------------
local function setTarget(target_race, target_count, add_missing_animals, stop, watch_race)
  printDetails(('start setTarget'))

  if type(target_race) == 'string' then
    target_race = string.upper(target_race)
  end

  if target_race == nil or target_race == '' then
    handleError('must specify DEFAULT, ALL, valid creature_id or race id')
  end

  local new_config = {
    watched = getBoolean(watch_race),
    target = tonumber(target_count),
    ama = getBoolean(add_missing_animals),
    stop = getBoolean(stop)
  }

  local race = validateCreatureOrRace(target_race)
  if race == 'DEFAULT' then
    utils.assign(state.default, new_config)
  elseif race == 'ALL' then
    for _, v in pairs(state.config_per_race) do
      utils.assign(v, new_config)
      utils.assign(state.default, new_config)
    end
  elseif race >= 0 then
    utils.assign(state.config_per_race[race], new_config)
  else
    handleError('must specify DEFAULT, ALL, valid creature_id or race id')
  end
  printDetails(('end setTarget'))
end --setTarget
---------------------------------------------------------------------------------------------------
local function setSplitStacks(value)
  state.split_stacks = getBoolean(value)
end
---------------------------------------------------------------------------------------------------
local function clearConfig()
  state = getDefaultState()
  updateEventListener()
end
---------------------------------------------------------------------------------------------------
local function setIgnore(value)
  state.ignore_autobutcher = getBoolean(value)
end
---------------------------------------------------------------------------------------------------
local function setVerbose(value)
  state.verbose = getBoolean(value)
  nestboxesCommon.verbose = state.verbose
end
---------------------------------------------------------------------------------------------------
local function format_target_count_row(category, row)
  return (('%s: watched: %s; target: %s; ama: %s; stop: %s'):format(
      category,
      row.watched and 'enabled' or 'disabled',
      tostring(row.target),
      row.ama and 'enabled' or 'disabled',
      row.stop and 'enabled' or 'disabled'
    ))
end
---------------------------------------------------------------------------------------------------
local function printStatus()
  printLocal(('Status %s.'):format(state.enabled and 'enabled' or 'disabled'))
  printLocal(('Stack splitting: %s'):format(state.split_stacks and 'enabled' or 'disabled'))
  printLocal(
    ('%s autobutcher\'s enabled status'):format(state.ignore_autobutcher and 'Ignoring' or 'Tespecting')
  )
  printDetails(('verbose mode is %s'):format(state.verbose and 'enabled' or 'disabled'))
  printDetails(
    ('Migration from cpp to lua is %s'):format(state.migration_from_cpp_to_lua_done and 'done' or 'not done')
  )
  printLocal(format_target_count_row('Default', state.default))
  if state.config_per_race ~= nil then
    for k, v in pairs(state.config_per_race) do
      printLocal(format_target_count_row(df.global.world.raws.creatures.all[k].creature_id, v))
    end
  end
  --printDetails(dumpToString(state))
end
---------------------------------------------------------------------------------------------------
local function handleOnStateChange(sc)
  if sc == SC_MAP_UNLOADED then
    doDisable()
    return
  end
  if sc ~= SC_MAP_LOADED or df.global.gamemode ~= df.game_mode.DWARF then
    return
  end
  loadState()
  printStatus()
  updateEventListener()
end --handleOnStateChange
---------------------------------------------------------------------------------------------------
local function getInfoFromAutobutcher(race)
  local v_return = {
    enabled = false,
    watched = false,
    mac = 0
  }
  printDetails(('getInfoFromAutobutcher for race= %s'):format(race))
  local autobutcher = require('plugins.autobutcher')
  if autobutcher.autobutcher_getInfoForNestboxes ~= nil then
    local autobutcher_return = autobutcher.autobutcher_getInfoForNestboxes(race)
    if autobutcher_return ~= nil then
      utils.assign(v_return, autobutcher_return)
    else
      printDetails(('got nil from autobutcher_return'))
    end
  else
    printDetails(('got nil from autobutcher_getInfoForNestboxes'))
  end
  return v_return
end --getInfoFromAutobutcher
---------------------------------------------------------------------------------------------------
--<Global functions>
---------------------------------------------------------------------------------------------------
-- checkItemCreated function, called from eventfful on ITEM_CREATED event
function checkItemCreated(item_id)
  local item = df.item.find(item_id)
  if item == nil or df.item_type.EGG ~= item:getType() or not nestboxesEvent.validateEggs(item) then
    return
  end

  local autobutcher_info = getInfoFromAutobutcher(item.race)
  if not ((autobutcher_info.watched and autobutcher_info.enabled) or state.ignore_autobutcher) then
    printDetails(('Did not check eggs, race %s not watched by autobutcher, autobutcher settings respected'):format(item.race))
    return
  end

  local race_config = getConfigForRace(item.race)
  if race_config == nil then
    printDetails(('Did not check eggs, race %s without config and no new races are being watched'):format(item.race))
    return
  end

  if (race_config.stop and autobutcher_info.mac == 0 and not state.ignore_autobutcher) then
    printDetails(('Did not check eggs, missing animal count for race %s is 0 and stop once animal target reached is enabled'):format(item.race))
    return
  end

  if (race_config.watched) then
    nestboxesEvent.handleEggs(item, race_config.target, race_config.ama, state.split_stacks, autobutcher_info.mac)
  else
    printDetails(('Did not check eggs, race %s not watched by %s'):format(item.race, GLOBAL_KEY))
  end
end --checkItemCreated
---------------------------------------------------------------------------------------------------
dfhack.onStateChange[GLOBAL_KEY] = function(sc)
  handleOnStateChange(sc)
end
---------------------------------------------------------------------------------------------------
function handleCommand(positionals, opts)
  loadState()
  local command = positionals[2]

  if command ~= nil then
    command = string.upper(command)
  end

  if command == 'ENABLE' or command == 'E' then
    doEnable()
  elseif command == 'DISABLE' or command == 'D' then
    doDisable()
  elseif command == 'UPDATE' or command == 'U' then
    updateEventListener()
  elseif command == 'TARGET' or command == 'T' then
    setTarget(positionals[3], positionals[4], positionals[5], positionals[6], positionals[7])
  elseif command == 'IGNORE' or command == 'I' then
    setIgnore(positionals[3])
  elseif command == 'VERBOSE' or command == 'V' then
    setVerbose(positionals[3])
  elseif command == 'SPLIT_STACKS' or command == 'S' then
    setSplitStacks(positionals[3])
  elseif command == 'CLEAR' then
    clearConfig()
  elseif positionals[2] ~= nil then
    handleError(('Command "% s" is not recognized'):format(positionals[2]))
  end

  printStatus()
  persistState()
end --handleCommand
---------------------------------------------------------------------------------------------------
--<Global functions/>
return _ENV
