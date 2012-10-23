local _ENV = mkmodule('plugins.workflow')

--[[

 Native functions:

 * isEnabled()
 * setEnabled(enable)
 * listConstraints([job]) -> {...}
 * setConstraint(token, by_count, goal[, gap]) -> {...}

--]]

return _ENV
