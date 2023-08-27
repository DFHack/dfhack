-- Instantly grow crops in farm plots

-- Collections crop name and growdur from the raws.
-- The returned table can be indexed by mat_index
function cacheCropRaws()
    local crops = {}
    -- store ID and grow duration for each crop
    for idx, p in ipairs(df.global.world.raws.plants.all) do
        crops[idx] = { name = p.id, growdur = p.growdur }
    end

    return crops
end

-- Gets a ref by type from the given refs collection or nil if not found
-- e.g. local ref = getRefByType(item.general_refs, df.general_ref_type.BUILDING_HOLDER)
function getRefByType(refs, type)
    for _, ref in ipairs(refs) do
        if ref:getType() == type then
            return ref
        end
    end

    return nil
end

-- Returns true if the given seed item is in a farm plot; false otherwise
function isSeedInFarmPlot(seed)
    local buildingRef = getRefByType(seed.general_refs, df.general_ref_type.BUILDING_HOLDER)
    if not buildingRef then
        return false
    end

    local building = buildingRef:getBuilding()
    return building ~= nil and building:getType() == df.building_type.FarmPlot
end

-- create a table of planted seed counts indexed by mat_index
function collectInventory(crop_raws)
    local inventory = {}
    for _, seed in ipairs(df.global.world.items.other.SEEDS) do
        if not isSeedInFarmPlot(seed) then
            goto skipseed
        end

        if seed.grow_counter < crop_raws[seed.mat_index].growdur then
            inventory[seed.mat_index] = (inventory[seed.mat_index] or 0) + 1
        end

        :: skipseed ::
    end

    return inventory
end

-- Display a list of planted crops
function listPlantedCrops(crop_raws, inventory)
    local crops = {}
    -- create a copy of crops from inventory to use for sorting
    for mat, invCount in pairs(inventory) do
        table.insert(crops, { name = crop_raws[mat].name, count = invCount })
    end

    -- sort the table in descending order by seed count
    table.sort(crops, function(a, b) return a.count > b.count end)

    for _, crop in ipairs(crops) do
        print(("%s %d"):format(crop.name, crop.count))
    end
end

-- Tries to find a crop raw in crop_raws that matches the given name
-- The returned value contains the mat_index and name of the crop or is nil if not found
function findRawsCrop(crop_raws, material)
    if not material then
        return nil
    end

    local lower_mat = string.lower(material)
    local possible_matches = {}
    for mat_index, crop in pairs(crop_raws) do
        local lower_crop_name = string.lower(crop.name)
        if lower_mat == lower_crop_name then
            return {mat_index = mat_index, name = crop.name}
        end

        if string.find(lower_crop_name, lower_mat) then
            table.insert(possible_matches, {mat_index = mat_index, name = crop.name})
        end
    end

    if #possible_matches == 1 then
        return possible_matches[1]
    end

    return nil
end

-- grow specific crop
function growCrop(crop_raws, material)
    -- find the matching crop
    local wanted_mat = findRawsCrop(crop_raws, material)
    if not wanted_mat then
        -- no crop with that name
        qerror("invalid plant material " .. material)
    end

    -- grow each seed for specified crop
    local count = 0
    for _, seed in pairs(df.global.world.items.other.SEEDS) do
        -- Skip seeds that don't match the material
        if seed.mat_index ~= wanted_mat.mat_index then
            goto growskip
        end

        if not isSeedInFarmPlot(seed) then
            goto growskip
        end

        if seed.grow_counter < crop_raws[seed.mat_index].growdur then
            seed.grow_counter = crop_raws[seed.mat_index].growdur
            count = count + 1
        end

        :: growskip ::
    end

    print(("Grew %d %s"):format(count, wanted_mat.name))
end

local args = {...}
local material = args[1]

if material == "help" then
    print(dfhack.script_help())
    return
end

local crop_raws = cacheCropRaws()
if not material or material == "list" then
    local inventory = collectInventory(crop_raws)
    listPlantedCrops(crop_raws, inventory)
elseif material == 'all' then
    local inventory = collectInventory(crop_raws)
    for mat_idx, _ in pairs(inventory) do
        growCrop(crop_raws, crop_raws[mat_idx].name)
    end
else
    growCrop(crop_raws, material)
end
