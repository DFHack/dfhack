local _ENV = mkmodule('plugins.buildingplan')

--[[

 Native functions:

 * bool isPlannableBuilding(df::building_type type, int16_t subtype, int32_t custom)
 * void addPlannedBuilding(df::building *bld)
 * void doCycle()
 * void scheduleCycle()

--]]

local guidm = require('gui.dwarfmode')
require('dfhack.buildings')

-- needs the core suspended
function construct_building_from_ui_state()
    local uibs = df.global.ui_build_selector
    local world = df.global.world
    local direction = world.selected_direction
    local _, width, height = dfhack.buildings.getCorrectSize(
        world.building_width, world.building_height, uibs.building_type,
        uibs.building_subtype, uibs.custom_type, direction)
    -- the cursor is at the center of the building; we need the upper-left
    -- corner of the building
    local pos = guidm.getCursorPos()
    pos.x = pos.x - math.floor(width/2)
    pos.y = pos.y - math.floor(height/2)
    local bld, err = dfhack.buildings.constructBuilding{
        type=uibs.building_type, subtype=uibs.building_subtype,
        custom=uibs.custom_type, pos=pos, width=width, height=height,
        direction=direction}
    if err then error(err) end
    -- TODO: assign fields for the types that need them. we can't pass them all
    -- in to the call to constructBuilding since the unneeded fields will cause
    -- errors
    --local fields = {
    --    friction=uibs.friction,
    --    use_dump=uibs.use_dump,
    --    dump_x_shift=uibs.dump_x_shift,
    --    dump_y_shift=uibs.dump_y_shift,
    --    speed=uibs.speed
    --}
    -- TODO: use quickfort's post_construction_fns? maybe move those functions
    -- into the library so they get applied automatically
    return bld
end

return _ENV
