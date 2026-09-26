config.mode = 'fortress'
config.target = 'core'

local utils = require('utils')

---TODO when we get LuaLS integrated, declare coord in just one place.
---@alias coord { x:integer, y:integer, z:integer } | df.coord

---@param pos coord
---@return df.tile_occupancy
local function tile_occupancy(pos)
    return select(2, dfhack.maps.getTileFlags(pos))
end

---@return df.unit, df.unit
local function two_citizens()
    local a, b
    for _, unit in ipairs(df.global.world.units.active) do
        if dfhack.units.isCitizen(unit) then
            if not a then
                a = unit
            else
                b = unit
                break
            end
        end
    end
    return a, b
end

-- find an allocated, walkable tile with no unit occupancy near pos
---@param pos coord
---@return coord?
---@return string?
local function free_tile_near(pos)
    for dx = -4, 4 do for dy = -4, 4 do
        if dx ~= 0 or dy ~= 0 then
            local other = xyz2pos(pos.x + dx, pos.y + dy, pos.z)
            local occ = tile_occupancy(other)
            if occ and not occ.unit and not occ.unit_grounded
                    and occ.building == df.tile_building_occ.None
                    and df.tiletype_shape.attrs[
                        df.tiletype.attrs[
                            dfhack.maps.getTileType(other)].shape].walkable then
                return other
            end
        end
    end end
    return nil, 'no free tile near the shared tile'
end

-- recompute the unit occupancy flags of the given tiles from the given
-- units, so fabricated flag states do not leak into later tests
---@param tiles coord[]
---@param units df.unit[]
local function resync_occupancy(tiles, units)
    for _, pos in ipairs(tiles) do
        local occ = tile_occupancy(pos)
        if occ then
            occ.unit = false
            occ.unit_grounded = false
        end
    end
    for _, unit in ipairs(units) do
        local occ = tile_occupancy(unit.pos)
        if occ then
            if unit.flags1.on_ground then
                occ.unit_grounded = true
            else
                occ.unit = true
            end
        end
    end
end

-- teleporting one of two grounded units off a shared tile must keep the
-- unit_grounded flag, since the other unit is still there (#5938)
function test.teleport_keeps_grounded_flag_with_other_grounded_unit()
    local a, b = two_citizens()
    expect.ne(nil, b, 'need at least two citizens')
    if not a or not b then return end

    local shared = copyall(b.pos)
    local dest, err = free_tile_near(b.pos)
    expect.ne(nil, dest, err)
    if not dest then return end

    local orig_a_pos, orig_a_ground = copyall(a.pos), a.flags1.on_ground
    local orig_b_ground = b.flags1.on_ground

    return dfhack.with_finalize(function()
        dfhack.units.teleport(a, orig_a_pos)
        dfhack.units.teleport(b, shared)
        a.flags1.on_ground = orig_a_ground
        b.flags1.on_ground = orig_b_ground
        resync_occupancy({orig_a_pos, shared, dest}, {a, b})
    end, function()
        -- teleporting onto a standing unit forces the mover to lie down
        expect.true_(dfhack.units.teleport(a, shared))
        expect.true_(a.flags1.on_ground)
        expect.true_(tile_occupancy(shared).unit_grounded)

        -- fabricate a second grounded unit on the shared tile
        b.flags1.on_ground = true

        expect.true_(dfhack.units.teleport(a, dest))
        -- b is still grounded on the shared tile, so the flag must remain
        expect.true_(tile_occupancy(shared).unit_grounded)
        expect.true_(tile_occupancy(dest).unit_grounded)

        -- removing the last grounded unit still clears the flag
        expect.true_(dfhack.units.teleport(b, orig_a_pos))
        expect.false_(tile_occupancy(shared).unit_grounded)
    end)
end

-- the same invariant applies to the standing 'unit' flag
function test.teleport_keeps_unit_flag_with_other_standing_unit()
    local a, b = two_citizens()
    expect.ne(nil, b, 'need at least two citizens')
    if not a or not b then return end

    local shared = copyall(b.pos)
    local dest, err = free_tile_near(b.pos)
    expect.ne(nil, dest, err)
    if not dest then return end

    local orig_a_pos, orig_a_ground = copyall(a.pos), a.flags1.on_ground
    local orig_b_ground = b.flags1.on_ground

    return dfhack.with_finalize(function()
        dfhack.units.teleport(a, orig_a_pos)
        dfhack.units.teleport(b, shared)
        a.flags1.on_ground = orig_a_ground
        b.flags1.on_ground = orig_b_ground
        resync_occupancy({orig_a_pos, shared, dest}, {a, b})
    end, function()
        expect.true_(dfhack.units.teleport(a, shared))
        expect.true_(a.flags1.on_ground)

        -- make a stand again so both units are standing on the shared tile
        a.flags1.on_ground = false
        tile_occupancy(shared).unit_grounded = false

        expect.true_(dfhack.units.teleport(a, dest))
        expect.true_(tile_occupancy(shared).unit)
        expect.true_(tile_occupancy(dest).unit)

        -- removing the last standing unit still clears the flag
        expect.true_(dfhack.units.teleport(b, orig_a_pos))
        expect.false_(tile_occupancy(shared).unit)
    end)
end

function test.setPathGoal()
    -- catch regression of issue #5978, setPathGoal() is missing CHECK_NULL_POINTER(unit)
    expect.error(function()
            ---@diagnostic disable-next-line: param-type-mismatch
            dfhack.units.setPathGoal(nil, xyz2pos(1, 2, 3), df.unit_path_goal.None)
        end,
        "dfhack.units.setPathGoal should have thrown an error.")

    local unit
    for _,u in ipairs(dfhack.units.getCitizens()) do
        if #u.path.path.x > 0 then
            unit = u
            break
        end
    end
    expect.ne(unit, nil)

    local oldpath = utils.clone(unit.path, true)

    local retval = dfhack.units.setPathGoal(unit, xyz2pos(1, 2, 3), df.unit_path_goal.None)
    expect.nil_(retval)
    expect.eq(df.unit_path_goal.None, unit.path.goal)
    expect.table_eq(xyz2pos(1, 2, 3), utils.clone(unit.path.dest, true))
    expect.table_eq({ x = {}, y = {}, z = {} }, utils.clone(unit.path.path, true))

    unit.path:assign(oldpath)
    expect.eq(oldpath.goal, unit.path.goal)
    expect.table_eq(oldpath.dest, utils.clone(unit.path.dest, true))
    expect.table_eq(oldpath.path, utils.clone(unit.path.path, true))
end
