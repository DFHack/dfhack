-- Weaken and eventually destroy undead over time.
--@enable = true
--@module = true

local argparse = require('argparse')
local json = require('json')
local persist = require('persist-table')

local GLOBAL_KEY = 'starvingdead'

starvingDeadInstance = starvingDeadInstance or nil

function isEnabled()
  return starvingDeadInstance ~= nil
end

local function persist_state()
  persist.GlobalTable[GLOBAL_KEY] = json.encode({
    enabled = starvingDeadInstance ~= nil,
    decay_rate = starvingDeadInstance and starvingDeadInstance.decay_rate or 1,
    death_threshold = starvingDeadInstance and starvingDeadInstance.death_threshold or 6
  })
end

dfhack.onStateChange[GLOBAL_KEY] = function(sc)
  if sc == SC_MAP_UNLOADED then
      enabled = false
      return
  end

  if sc ~= SC_MAP_LOADED or df.global.gamemode ~= df.game_mode.DWARF then
      return
  end

  local persisted_data = json.decode(persist.GlobalTable[GLOBAL_KEY] or '{}')

  if persisted_data.enabled then
    starvingDeadInstance = StarvingDead{
      decay_rate = persisted_data.decay_rate,
      death_threshold = persisted_data.death_threshold
    }
  end
end

StarvingDead = defclass(StarvingDead)
StarvingDead.ATTRS{
  decay_rate = 1,
  death_threshold = 6,
}

function StarvingDead:init()
  self.timeout_id = nil
  -- Percentage goal each attribute should reach before death.
  local attribute_goal = 10
  self.attribute_decay = (attribute_goal ^ (1 / ((self.death_threshold * 28 / self.decay_rate)))) / 100

  self:checkDecay()
  print(([[StarvingDead started, checking every %s days and killing off at %s months]]):format(self.decay_rate, self.death_threshold))
end

function StarvingDead:checkDecay()
  for _, unit in pairs(df.global.world.units.active) do
    if (unit.enemy.undead and not unit.flags1.inactive) then
      -- time_on_site is measured in ticks, a month is 33600 ticks.
      -- @see https://dwarffortresswiki.org/index.php/Time
      for _, attribute in pairs(unit.body.physical_attrs) do
        attribute.value = math.floor(attribute.value - (attribute.value * self.attribute_decay))
      end

      if unit.curse.time_on_site > (self.death_threshold * 33600) then
        unit.animal.vanish_countdown = 1
      end
    end
  end

  self.timeout_id = dfhack.timeout(self.decay_rate, 'days', self:callback('checkDecay'))
end

if dfhack_flags.module then
  return
end

local options, args = {
  decay_rate = nil,
  death_threshold = nil
}, {...}

local positionals = argparse.processArgsGetopt(args, {
  {'h', 'help', handler = function() options.help = true end},
  {'r', 'decay-rate', hasArg = true, handler=function(arg) options.decay_rate = argparse.positiveInt(arg, 'decay-rate') end },
  {'t', 'death-threshold', hasArg = true, handler=function(arg) options.death_threshold = argparse.positiveInt(arg, 'death-threshold') end },
})

if dfhack_flags.enable then
  if dfhack_flags.enable_state then
    if starvingDeadInstance then
      return
    end

    starvingDeadInstance = StarvingDead{}
    persist_state()
  else
    if not starvingDeadInstance then
      return
    end

    dfhack.timeout_active(starvingDeadInstance.timeout_id, nil)
    starvingDeadInstance = nil
  end
else
  if not dfhack.isMapLoaded() then
    qerror('This script requires a fortress map to be loaded')
  end

  if positionals[1] == "help" or options.help then
    print(dfhack.script_help())
    return
  end

  if positionals[1] == nil then
    if starvingDeadInstance then
      starvingDeadInstance.decay_rate = options.decay_rate or starvingDeadInstance.decay_rate
      starvingDeadInstance.death_threshold = options.death_threshold or starvingDeadInstance.death_threshold

      print(([[StarvingDead is running, checking every %s days and killing off at %s months]]):format(
        starvingDeadInstance.decay_rate, starvingDeadInstance.death_threshold
      ))
    else
      print("StarvingDead is not running!")
    end
  end
end
