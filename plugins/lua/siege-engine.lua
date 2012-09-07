local _ENV = mkmodule('plugins.siege-engine')

--[[

 Native functions:

 * getTargetArea(building) -> point1, point2
 * clearTargetArea(building)
 * setTargetArea(building, point1, point2) -> true/false

--]]

return _ENV