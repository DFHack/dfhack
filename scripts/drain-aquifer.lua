-- Remove all aquifers from the map
--[[=begin

drain-aquifer
=============
Remove all 'aquifer' tag from the map blocks. Irreversible.

=end]]

local function drain()
    local layers = {}
    local layer_count = 0
    local tile_count = 0

    for k, block in ipairs(df.global.world.map.map_blocks) do
        if block.flags.has_aquifer then
            block.flags.has_aquifer = false
            block.flags.check_aquifer = false

            for x, row in ipairs(block.designation) do
                for y, tile in ipairs(row) do
                    if tile.water_table then
                        tile.water_table = false
                        tile_count = tile_count + 1
                    end
                end
            end

            if not layers[block.map_pos.z] then
                layers[block.map_pos.z] = true
                layer_count = layer_count + 1
            end
        end
    end

    print("Cleared "..tile_count.." aquifer tile"..((tile_count ~= 1) and "s" or "")..
        " in "..layer_count.." layer"..((layer_count ~= 1) and "s" or "")..".")
end

drain(...)
