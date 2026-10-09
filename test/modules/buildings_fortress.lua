config.target = 'core'
config.mode = 'fortress'

local function contains(vec, value)
    for _,entry in ipairs(vec) do
        if entry == value then return true end
    end
    return false
end

local function construct_window_in_zone(window_type)
    for _,zone in ipairs(df.global.world.buildings.other.ANY_ZONE) do
        if zone.room.extents then
            for y = zone.y1, zone.y2 do
                for x = zone.x1, zone.x2 do
                    local pos = xyz2pos(x, y, zone.z)
                    if not dfhack.buildings.findAtTile(pos) then
                        local bld = dfhack.buildings.constructBuilding{
                            pos=pos,
                            type=window_type,
                            filters=dfhack.buildings.getFiltersByType(
                                {}, window_type, -1, -1),
                        }
                        if bld then return bld, zone end
                    end
                end
            end
        end
    end
end

local function expect_window_linked(window_type)
    local bld, zone = construct_window_in_zone(window_type)
    expect.ne(nil, bld, 'could not find a zone tile suitable for a window')
    if not bld then return end
    dfhack.with_finalize(
        function() dfhack.buildings.deconstruct(bld) end,
        function()
            expect.true_(contains(bld.relations, zone))
            expect.true_(contains(zone.contained_buildings, bld))
        end)
end

function test.constructed_gem_window_is_linked_to_zone()
    expect_window_linked(df.building_type.WindowGem)
end

function test.constructed_glass_window_is_linked_to_zone()
    expect_window_linked(df.building_type.WindowGlass)
end

local function expect_zoned_window_linked(window_type)
    local bld, _ = construct_window_in_zone(window_type)
    expect.ne(nil, bld, 'could not find a tile suitable for a window')
    if not bld then return end
    dfhack.with_finalize(
        function() dfhack.buildings.deconstruct(bld) end,
        function()
            local pos = xyz2pos(bld.centerx, bld.centery, bld.z)
            local extents = df.reinterpret_cast(df.building_extents_type,
                                                df.new('uint8_t', 1))
            extents[0] = 1
            local zone, err = dfhack.buildings.constructBuilding{
                type=df.building_type.Civzone,
                subtype=df.civzone_type.Bedroom,
                abstract=true,
                pos=pos, width=1, height=1,
                fields={
                    assigned_unit_id=-1,
                    room={x=pos.x, y=pos.y, width=1, height=1,
                          extents=extents},
                },
            }
            expect.ne(nil, zone, 'could not place zone over window: ' ..
                                 tostring(err))
            if not zone then return end
            dfhack.with_finalize(
                function() dfhack.buildings.deconstruct(zone) end,
                function()
                    expect.true_(contains(bld.relations, zone))
                    expect.true_(contains(zone.contained_buildings, bld))
                end)
        end)
end

function test.zoned_gem_window_is_linked_to_zone()
    expect_zoned_window_linked(df.building_type.WindowGem)
end

function test.zoned_glass_window_is_linked_to_zone()
    expect_zoned_window_linked(df.building_type.WindowGlass)
end
