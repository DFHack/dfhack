-- Scan the map for ore veins

local argparse = require('argparse')

local tile_attrs = df.tiletype.attrs

local function extractKeys(target_table)
    local keyset = {}

    for k, _ in pairs(target_table) do
        table.insert(keyset, k)
    end

    return keyset
end

local function getRandomTableKey(target_table)
    if not target_table then
        return nil
    end

    local keyset = extractKeys(target_table)
    if #keyset == 0 then
        return nil
    end

    return keyset[math.random(#keyset)]
end

local function getRandomFromTable(target_table)
    if not target_table then
        return nil
    end

    local key = getRandomTableKey(target_table)
    if not key then
        return nil
    end

    return target_table[key]
end

local function randomSort(target_table)
    local rnd = {}
    table.sort( target_table,
        function ( a, b)
            rnd[a] = rnd[a] or math.random()
            rnd[b] = rnd[b] or math.random()
            return rnd[a] > rnd[b]
        end )
end

local function sequence(min, max)
    local tbl = {}
    for i=min,max do
        table.insert(tbl, i)
    end
    return tbl
end

local function sortTableBy(tbl, sort_func)
    local sorted = {}
    for _, value in pairs(tbl) do
        table.insert(sorted, value)
    end

    table.sort(sorted, sort_func)

    return sorted
end

local function matchesMetalOreById(mat_indices, target_ore)
    for _, mat_index in ipairs(mat_indices) do
        local metal_raw = df.global.world.raws.inorganics[mat_index]
        if metal_raw ~= nil and string.lower(metal_raw.id) == target_ore then
            return true
        end
    end

    return false
end

local function findOreVeins(target_ore, show_undiscovered)
    if target_ore then
        target_ore = string.lower(target_ore)
    end

    local ore_veins = {}
    for _, block in pairs(df.global.world.map.map_blocks) do
        for _, bevent in pairs(block.block_events) do
            if bevent:getType() ~= df.block_square_event_type.mineral then
                goto skipevent
            end

            local ino_raw = df.global.world.raws.inorganics[bevent.inorganic_mat]
            if not ino_raw.flags.METAL_ORE then
                goto skipevent
            end

            if not show_undiscovered and not bevent.flags.discovered then
                goto skipevent
            end

            local lower_raw = string.lower(ino_raw.id)
            if not target_ore or lower_raw == target_ore or matchesMetalOreById(ino_raw.metal_ore.mat_index, target_ore) then
                if not ore_veins[bevent.inorganic_mat] then
                    local vein_info = {
                        inorganic_id = ino_raw.id,
                        inorganic_mat = bevent.inorganic_mat,
                        metal_ore = ino_raw.metal_ore,
                        positions = {}
                    }
                    ore_veins[bevent.inorganic_mat] = vein_info
                end

                table.insert(ore_veins[bevent.inorganic_mat].positions, block.map_pos)
            end

            :: skipevent ::
        end
    end

    return ore_veins
end

local function designateDig(pos)
    local designation = dfhack.maps.getTileFlags(pos)
    designation.dig = df.tile_dig_designation.Default
end

local function getOreDescription(ore)
    local str = ("%s ("):format(string.lower(tostring(ore.inorganic_id)))
    for _, mat_index in ipairs(ore.metal_ore.mat_index) do
        local metal_raw = df.global.world.raws.inorganics[mat_index]
        str = ("%s%s, "):format(str, string.lower(metal_raw.id))
    end

    str = str:gsub(", %s*$", "") .. ')'
    return str
end

local options, args = {
    help = false,
    show_undiscovered = false
}, {...}

local positionals = argparse.processArgsGetopt(args, {
    {'h', 'help', handler=function() options.help = true end},
    {'a', 'all', handler=function() options.show_undiscovered = true end},
})

if positionals[1] == "help" or options.help then
    print(dfhack.script_help())
    return
end

if positionals[1] == nil or positionals[1] == "list" then
    print(dfhack.script_help())
    local veins = findOreVeins(nil, options.show_undiscovered)
    local sorted = sortTableBy(veins, function(a, b) return #a.positions < #b.positions end)

    for _, vein in ipairs(sorted) do
        print("  " .. getOreDescription(vein))
    end
    return
else
    local veins = findOreVeins(positionals[1], options.show_undiscovered)
    local vein_keys = extractKeys(veins)

    if #vein_keys == 0 then
        qerror("Cannot find unmined " .. positionals[1])
    end

    local target_vein = getRandomFromTable(veins)
    if target_vein == nil then
        -- really shouldn't happen at this point
        qerror("Failed to choose vein from available choices")
    end

    local pos_keyset = extractKeys(target_vein.positions)
    local dxs = sequence(0, 15)
    local dys = sequence(0, 15)

    randomSort(pos_keyset)
    randomSort(dxs)
    randomSort(dys)

    local target_pos = nil
    for _, k in pairs(pos_keyset) do
        local block_pos = target_vein.positions[k]
        for _, dx in pairs(dxs) do
            for _, dy in pairs(dys) do
                local pos = { x = block_pos.x + dx, y = block_pos.y + dy, z = block_pos.z }
                -- Enforce world boundaries
                if pos.x <= 0 or pos.x >= df.global.world.map.x_count or pos.y <= 0 or pos.y >= df.global.world.map.y_count then
                    goto skip_pos
                end

                if not options.show_undiscovered and not dfhack.maps.isTileVisible(pos) then
                    goto skip_pos
                end

                local tile_type = dfhack.maps.getTileType(pos)
                local tile_mat = tile_attrs[tile_type].material
                local shape = tile_attrs[tile_type].shape
                local designation = dfhack.maps.getTileFlags(pos)
                if tile_mat == df.tiletype_material.MINERAL and designation.dig == df.tile_dig_designation.No and shape == df.tiletype_shape.WALL then
                    target_pos = pos
                    goto complete
                end

                :: skip_pos ::
            end
        end
    end

    :: complete ::

    if target_pos ~= nil then
        dfhack.gui.pauseRecenter(target_pos)
        designateDig(target_pos)
        print(("Here is some %s at (%d, %d, %d)"):format(target_vein.inorganic_id, target_pos.x, target_pos.y, target_pos.z))
    else
        qerror("Cannot find unmined " .. positionals[1])
    end
end
