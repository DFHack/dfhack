local dfhack = dfhack
local _ENV = dfhack.BASE_G
local buildings = dfhack.buildings

--[[
    Wraps all steps necessary to create a building with
    a construct job into one function.

    dfhack.buildings.constructBuilding{
      -- Position:
        pos = { x = ..., y = ..., z = ... },
      -- OR
        x = ..., y = ..., z = ...,

      -- Type:
        type = df.building_type.FOO, subtype = ..., custom = ...,

      -- Field initialization:
        fields = { ... },

      -- Size and orientation:
        width = ..., height = ..., direction = ...,

      -- Abort if not all tiles in the rectangle are available:
        full_rectangle = true,

      -- Materials:
        items = { item, item ... },
      -- OR
        filter = { { ... }, { ... }... }
    }

    Returns: the created building, or 'nil, error'
--]]

function buildings.constructBuilding(info)
    local btype = info.type
    local subtype = info.subtype or -1
    local custom = info.custom or -1

    if not (info.pos or info.x) then
        error('position is required')
    end
    if not (info.items or info.filters) then
        error('either items or filters are required')
    elseif info.filters then
        for _,v in ipairs(info.filters) do
            v.new = true
        end
    end
    if type(btype) ~= 'number' or not df.building_type[btype] then
        error('Invalid building type: '..tostring(btype))
    end

    local pos = info.pos or xyz2pos(info.x, info.y, info.z)

    local instance = buildings.allocInstance(pos, btype, subtype, custom)
    if not instance then
        error('Could not create building of type '..df.building_type[btype])
    end

    local to_delete = instance
    return dfhack.with_finalize(
        function()
            df.delete(to_delete)
        end,
        function()
            if info.fields then
                instance:assign(info.fields)
            end
            local ok,w,h,area,r_area = buildings.setSize(
                instance,info.width,info.height,info.direction
            )
            if not ok then
                return nil, "cannot place at this position"
            end
            if info.full_rectangle and area ~= r_area then
                return nil, "not all tiles can be used"
            end
            if info.items then
                ok = buildings.constructWithItems(instance, info.items)
            else
                ok = buildings.constructWithFilters(instance, info.filters)
            end
            if not ok then
                return nil, "could not construct the building"
            end
            -- Success
            to_delete = nil
            return instance
        end
    )
end

return buildings