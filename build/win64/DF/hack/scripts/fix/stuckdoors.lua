-- Fix doors that are frozen in 'open' state.
-- This may happen after people mess with the game by (incorrectly) teleporting units or items
-- A door may stick open if the map occupancy flags are wrong.

if #{...} > 0 then
    print(dfhack.script_help())
    return
end

-- Util function: find out if there are any units on the tile with coordinates x,y,z
function unitOnTile(x, y, z)
    local units = dfhack.units.getUnitsInBox(x,y,z,x,y,z,dfhack.units.isActive)
    return #(units) > 0
end

-- Util function: find out if there are any items on the tile with coordinates x,y,z
-- There should be one item on the tile: the door itself.
-- This method is not especially precise as it's doing a simple count check.
-- A better one would be somehow linking the door building with the door item
-- and ignoring that particular item in the check.
-- Also this could be sped up significantly with a similar item method to the unit
-- one above like dfhack.items.getItemsInBox().
function nonDoorItemOnTile(x, y, z)
    local count = 0
    for _,item in ipairs(df.global.world.items.all) do
        if item.pos.x == x and item.pos.y == y and item.pos.z == z then
            count = count + 1
            if count > 1 then return true end
        end
    end
    return count > 1
end

-- MAIN function, unstick any doors frozen in an 'open' state
function unstickDoors()
    -- track how many doors we unstick
    local unstuck_doors_count = 0
    -- check just the doors and only the doors
    for _, door in ipairs(df.global.world.buildings.other.DOOR) do
        local x,y,z = door.x1,door.y1,door.z

        -- closed doors have a close_timer of 0, so ignore those
        local door_is_closed = door.close_timer == 0
        if door_is_closed then goto continue end

        -- check if occupancy is set for the tile but there is not a unit on the tile
        local occ = dfhack.maps.getTileBlock(x, y, z).occupancy[x % 16][y % 16]
        local door_is_marked_as_occupied_by_unit = occ.unit or occ.unit_grounded
        local door_is_marked_as_occupied_by_item = occ.item
        local unit_is_on_tile = unitOnTile(x,y,z)
        local item_is_on_tile = nonDoorItemOnTile(x,y,z)
        -- unmark occupancy for false unit presence
        if door_is_marked_as_occupied_by_unit and not unit_is_on_tile then
            unstuck_doors_count = unstuck_doors_count + 1
            occ.unit = false
            occ.unit_grounded = false
        end
        -- unmark occupancy for false item presence
        if door_is_marked_as_occupied_by_item and not item_is_on_tile then
            unstuck_doors_count = unstuck_doors_count + 1
            occ.item = false
        end
        ::continue::
    end

    print("Unstuck " .. unstuck_doors_count .. " doors")

    return unstuck_doors_count
end

unstickDoors()
