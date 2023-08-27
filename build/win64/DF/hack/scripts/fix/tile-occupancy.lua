-- Fix occupancy flags at a given tile

--[====[

fix/tile-occupancy
==================
Clears bad occupancy flags at the selected tile. Useful for getting rid of
phantom "building present" messages. Currently only supports issues with
building and unit occupancy. Requires that a tile is selected with the in-game
cursor (``k``).

Can be used to fix problematic tiles caused by :issue:`1047`.

]====]

if #{...} > 0 then
    qerror('This script takes no arguments.')
end

function findUnit(x, y, z)
    for _, u in pairs(df.global.world.units.active) do
        if u.pos.x == x and u.pos.y == y and u.pos.z == z then
            return true
        end
    end
    return false
end

local cursor = df.global.cursor
local changed = false
function report(flag)
    print('Cleared occupancy flag: ' .. flag)
    changed = true
end

if cursor.x == -30000 then
    qerror('Cursor not active.')
end

local occ = dfhack.maps.getTileBlock(pos2xyz(cursor)).occupancy[cursor.x % 16][cursor.y % 16]

if occ.building ~= df.tile_building_occ.None and not dfhack.buildings.findAtTile(pos2xyz(cursor)) then
    occ.building = df.tile_building_occ.None
    report('building')
end

for _, flag in pairs{'unit', 'unit_grounded'} do
    if occ[flag] and not findUnit(pos2xyz(cursor)) then
        occ[flag] = false
        report(flag)
    end
end

if not changed then
    print('No changes made at this tile.')
end
