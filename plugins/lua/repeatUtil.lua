-- lua/plugins/repeatUtil.lua
-- author expwnent
-- vaguely based on a script by Putnam

local _ENV = mkmodule("repeatUtil")

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

return _ENV

