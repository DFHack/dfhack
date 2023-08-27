-- game map-related logic and functions for the quickfort modules
--@ module = true

if not dfhack_flags.module then
    qerror('this script cannot be called directly')
end

local guidm = require('gui.dwarfmode')

function move_cursor(pos)
    guidm.setCursorPos(pos)
    dfhack.gui.refreshSidebar()
end

MapBoundsChecker = defclass(MapBoundsChecker, nil)
MapBoundsChecker.ATTRS{
    -- if unset, bounds will be initialized with the bounds of the currently
    -- loaded game map. format is: {x=width, y=height, z=depth}
    dims=DEFAULT_NIL
}

function MapBoundsChecker:init()
    self.x_min, self.y_min, self.z_min = 0, 0, 0
    local dims = self.dims or {}
    if not dims or not dims.x or not dims.y or not dims.z then
        dims.x, dims.y, dims.z = df.global.world.map.x_count,
                df.global.world.map.y_count, df.global.world.map.z_count
    end
    self.x_max, self.y_max, self.z_max = dims.x - 1, dims.y - 1, dims.z - 1
end

-- returns true if the coordinates are on the map and are more than gap tiles
-- from the edges. gap defaults to 0.
function MapBoundsChecker:is_on_map(pos, gap)
    gap = gap or 0
    return self:is_on_map_x(pos.x, gap) and
           self:is_on_map_y(pos.y, gap) and
           self:is_on_map_z(pos.z)
end

-- returns true if the coordinates are on the map and are more than gap tiles
-- from the edges, considering only the x coord. gap defaults to 0.
function MapBoundsChecker:is_on_map_x(x, gap)
    gap = gap or 0
    return x >= self.x_min + gap and x <= self.x_max - gap
end

-- returns true if the coordinates are on the map and are more than gap tiles
-- from the edges, considering only the y coord. gap defaults to 0.
function MapBoundsChecker:is_on_map_y(y, gap)
    gap = gap or 0
    return y >= self.y_min + gap and y <= self.y_max - gap
end

-- returns true if the coordinates are on the map and are more than gap tiles
-- from the edges. gap defaults to 0.
function MapBoundsChecker:is_on_map_z(z)
    return z >= self.z_min and z <= self.z_max
end

-- returns true if the coordinates are on one of the edge tiles of the map
function MapBoundsChecker:is_on_map_edge(pos)
    return self:is_on_map(pos) and not self:is_on_map(pos, 1)
end
