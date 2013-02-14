-- Fixes cloth/thread stockpiles by correcting material object data.

local raws = df.global.world.raws

-- Cache references to vectors in lua tables for a speed-up
local organic_types = {}
for i,v in ipairs(raws.mat_table.organic_types) do
    organic_types[i] = v
end
local organic_indexes = {}
for i,v in ipairs(raws.mat_table.organic_indexes) do
    organic_indexes[i] = v
end

local function verify(category,idx,vtype,vidx)
    if idx == -1 then
        -- Purely for reporting reasons
        return true
    end
    local tvec = organic_types[category]
    if idx < 0 or idx >= #tvec or tvec[idx] ~= vtype then
        return false
    end
    return organic_indexes[category][idx] == vidx
end

local patched_cnt = 0
local mat_cnt = 0

function patch_material(mat,mat_type,mat_index)
    local idxarr = mat.food_mat_index

    -- These refer to fish/eggs, i.e. castes and not materials
    idxarr[1] = -1
    idxarr[2] = -1
    idxarr[3] = -1

    for i = 0,#idxarr-1 do
        if not verify(i,idxarr[i],mat_type,mat_index) then
            idxarr[i] = -1
            patched_cnt = patched_cnt+1
        end
    end

    mat_cnt = mat_cnt + 1
end

function patch_materials()
    patched_cnt = 0
    mat_cnt = 0

    print('Fixing cloth stockpile handling (bug 5739)...')

    for i,v in ipairs(raws.inorganics) do
        patch_material(v.material, 0, i)
    end

    for i,v in ipairs(raws.creatures.all) do
        for j,m in ipairs(v.material) do
            patch_material(m, 19+j, i)
        end
    end

    for i,v in ipairs(raws.plants.all) do
        for j,m in ipairs(v.material) do
            patch_material(m, 419+j, i)
        end
    end

    print('Patched '..patched_cnt..' bad references in '..mat_cnt..' materials.')
end

local args = {...}

if args[1] == 'enable' then
    dfhack.onStateChange[_ENV] = function(sc)
        if sc == SC_WORLD_LOADED then
            patch_materials()
        end
    end
    if dfhack.isWorldLoaded() then
        patch_materials()
    end
elseif args[1] == 'disable' then
    dfhack.onStateChange[_ENV] = nil
else
    patch_materials()
end
