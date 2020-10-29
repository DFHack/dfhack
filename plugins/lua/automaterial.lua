local _ENV = mkmodule('plugins.automaterial')

local buildingplan = require('plugins.buildingplan')

-- construct the building and register it with buildingplan for item selection
function build_with_buildingplan_box_select(subtype, x, y, z)
    local pos = xyz2pos(x, y, z)
    local bld, err = dfhack.buildings.constructBuilding{
            type=df.building_type.Construction, subtype=subtype, pos=pos}
    -- it's not a user error if we can't place a building here; just indicate
    -- that no building was placed by returning false.
    if err then return false end
    buildingplan.addPlannedBuilding(bld)
    return true
end

function build_with_buildingplan_ui()
    for _,bld in ipairs(buildingplan.construct_buildings_from_ui_state()) do
        buildingplan.addPlannedBuilding(bld)
    end
end

return _ENV
