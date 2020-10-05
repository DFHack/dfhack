local _ENV = mkmodule('plugins.buildingplan')

--[[

 Native functions:

 * bool isPlannableBuilding(df::building_type type, int16_t subtype, int32_t custom)
 * void addPlannedBuilding(df::building *bld)
 * void doCycle()
 * void scheduleCycle()

--]]

return _ENV
