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
 if repeating[name] ~= -1 then
  dfhack.timeout_active(repeating[name],nil)
 end
 repeating[name] = nil
 return true
end

function scheduleEvery(name,time,timeUnits,func)
 cancel(name)
 local function helper()
  func()
  if repeating[name] then
   repeating[name] = dfhack.timeout(time,timeUnits,helper)
  end
 end
 repeating[name] = -1
 helper()
end

function scheduleUnlessAlreadyScheduled(name,time,timeUnits,func)
 if repeating[name] then
  return
 end
 scheduleEvery(name,time,timeUnits,func)
end

return _ENV
