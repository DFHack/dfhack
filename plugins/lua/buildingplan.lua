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

-- does not need the core suspended
function get_num_filters(btype, subtype, custom)
    local filters = dfhack.buildings.getFiltersByType(
        {}, btype, subtype, custom)
    if filters then return #filters end
    return 0
end

local function to_title_case(str)
    str = str:gsub('(%a)([%w_]*)',
        function (first, rest) return first:upper()..rest:lower() end)
    str = str:gsub('_', ' ')
    return str
end

-- returns a reasonable label for the item based on the qualities of the filter
-- does not need the core suspended
-- reverse_idx is 0-based and is expected to be counted from the *last* filter
function get_item_label(btype, subtype, custom, reverse_idx)
    local filters = dfhack.buildings.getFiltersByType(
        {}, btype, subtype, custom)
    if not filters then return 'No item' end
    if reverse_idx < 0 or reverse_idx >= #filters then
        return 'Invalid index'
    end
    local filter = filters[#filters-reverse_idx]
    if filter.has_tool_use then
        return to_title_case(df.tool_uses[filter.has_tool_use])
    end
    if filter.item_type then
        return to_title_case(df.item_type[filter.item_type])
    end
    if filter.flags2 and filter.flags2.building_material then
        if filter.flags2.fire_safe then
            return "Fire-safe building material";
        end
        if filter.flags2.magma_safe then
            return "Magma-safe building material";
        end
        return "Generic building material";
    end
    if filter.vector_id then
        return to_title_case(df.job_item_vector_id[filter.vector_id])
    end
    return "Unknown";
end

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
    -- in to the call to constructBuilding since attempting to assign unrelated
    -- fields to building types that don't support them causes errors.
    for k,v in pairs(bld) do
        if k == 'friction' then bld.friction = uibs.friction end
        if k == 'use_dump' then bld.use_dump = uibs.use_dump end
        if k == 'dump_x_shift' then bld.dump_x_shift = uibs.dump_x_shift end
        if k == 'dump_y_shift' then bld.dump_y_shift = uibs.dump_y_shift end
        if k == 'speed' then bld.speed = uibs.speed end
    end
    -- TODO: use quickfort's post_construction_fns? maybe move those functions
    -- into the library so they get applied automatically
    return bld
end

return _ENV
