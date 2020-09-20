local _ENV = mkmodule('plugins.buildingplan')

--[[

 Native functions:

 * bool isEnabled()
 * bool isPlannableBuilding(df::building_type type)
 * void addPlannedBuilding(df::building *bld)
 * void doCycle()

--]]

return _ENV
