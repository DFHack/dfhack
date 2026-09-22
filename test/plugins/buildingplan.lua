config.mode = 'fortress'
config.target = 'buildingplan'

local buildingplan = require('plugins.buildingplan')

local function find_floor_pos()
    for _, unit in ipairs(df.global.world.units.active) do
        for dx = -4, 4 do
            for dy = -4, 4 do
                local pos = xyz2pos(unit.pos.x + dx, unit.pos.y + dy, unit.pos.z)
                local tt = dfhack.maps.getTileType(pos)
                local block = dfhack.maps.getTileBlock(pos)
                if tt and block
                        and df.tiletype.attrs[tt].shape == df.tiletype_shape.FLOOR
                        and block.occupancy[pos.x % 16][pos.y % 16].building == 0 then
                    return pos
                end
            end
        end
    end
end

local function place_box(pos)
    return dfhack.buildings.constructBuilding{
        pos=pos,
        type=df.building_type.Box, subtype=-1, custom=-1,
        width=1, height=1,
        filters=dfhack.buildings.getFiltersByType({}, df.building_type.Box, -1, -1),
    }
end

function test.set_get_do_now()
    local pos = find_floor_pos()
    expect.ne(nil, pos, 'test needs a free floor tile')
    local bld = place_box(pos)
    expect.ne(nil, bld, 'failed to place test building')
    dfhack.with_finalize(
        function() dfhack.buildings.deconstruct(bld) end,
        function()
            expect.false_(buildingplan.getDoNow(bld))
            expect.true_(buildingplan.setDoNow(bld, true))
            expect.true_(buildingplan.getDoNow(bld))
            expect.true_(bld.jobs[0].flags.do_now)
            expect.true_(buildingplan.setDoNow(bld, false))
            expect.false_(buildingplan.getDoNow(bld))
        end)
end

function test.do_now_invalid_input()
    expect.false_(buildingplan.setDoNow(nil, true))
    expect.false_(buildingplan.getDoNow(nil))
end

function test.do_now_flag_survives_registration()
    local pos = find_floor_pos()
    expect.ne(nil, pos, 'test needs a free floor tile')
    local bld = place_box(pos)
    expect.ne(nil, bld, 'failed to place test building')
    dfhack.with_finalize(
        function() dfhack.buildings.deconstruct(bld) end,
        function()
            buildingplan.addPlannedBuilding(bld)
            expect.true_(buildingplan.isPlannedBuilding(bld))
            expect.true_(buildingplan.setDoNow(bld, true))
            -- the flag is set on the job directly, so buildingplan's own
            -- suspend/cycle handling must not clear it
            buildingplan.doCycle()
            expect.true_(buildingplan.getDoNow(bld))
        end)
end
