-- lua/plugins/repeatUtil.lua
-- author expwnent
-- tools for registering callbacks periodically
-- vaguely based on a script by Putnam

local _ENV = mkmodule("repeat-util")

repeating = repeating or {}

dfhack.onStateChange.repeatUtilStateChange = function(code)
 if code == SC_WORLD_UNLOADED then
  repeating = {}
 end
end

function cancel(name)
 if not repeating[name] then
  return false
 end
 dfhack.timeout_active(repeating[name],nil)
 repeating[name] = nil
 return true
end

function scheduleEvery(name,time,timeUnits,func)
 cancel(name)
 local function helper()
  func()
  repeating[name] = dfhack.timeout(time,timeUnits,helper)
 end
 helper()
end

function scheduleUnlessAlreadyScheduled(name,time,timeUnits,func)
 if repeating[name] then
  return
 end
 scheduleEvery(name,time,timeUnits,func)
end

return _ENV

