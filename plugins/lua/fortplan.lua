local _ENV = mkmodule('plugins.fortplan')

require('dfhack.buildings')

function construct_building_from_params(building_type, x, y, z)
    local pos = xyz2pos(x, y, z)
    local bld, err =
            dfhack.buildings.constructBuilding{type=building_type, pos=pos}
    if err then error(err) end
    return bld
end

return _ENV
