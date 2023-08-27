-- Changes heavy aquifers to light globally pre embark or locally post embark

local args = {...}

if args[1] == 'help' then
    print(dfhack.script_help())
    return
end

if not dfhack.isWorldLoaded() then
    qerror("Error: This script requires a world to be loaded.")
end

if dfhack.isMapLoaded() then
    for _, block in ipairs(df.global.world.map.map_blocks) do
        if block.flags.has_aquifer then
            for k = 0, 15 do
                for l = 0, 15 do
                    block.occupancy[k][l].heavy_aquifer = false
                end
            end
        end
    end
    return
end

-- pre-embark
for i = 0, df.global.world.world_data.world_width - 1 do
    for k = 0, df.global.world.world_data.world_height - 1 do
        local tile = df.global.world.world_data.region_map[i]:_displace(k)
        if tile.drainage % 20 == 7 then
            tile.drainage = tile.drainage + 1
        end
    end
end
