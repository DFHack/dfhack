config.target = 'getplants'
config.mode = 'fortress'

local function find_raw(id)
    for _, raw in ipairs(df.global.world.raws.plants.all) do
        if raw.id == id then return raw end
    end
end

local function run(...)
    return dfhack.run_command_silent('getplants', ...)
end

-- count shrubs carrying a gather designation (designation tile == plant tile
-- for shrubs, unlike trees)
local function count_marked(mat_idx)
    local n = 0
    for _, p in ipairs(df.global.world.plants.all) do
        if (mat_idx == nil or p.material == mat_idx) and
            df.plant_type.attrs[p.type].is_shrub
        then
            local blk = dfhack.maps.getTileBlock(p.pos)
            if blk and blk.designation[p.pos.x % 16][p.pos.y % 16].dig ==
                df.tile_dig_designation.Default
            then
                n = n + 1
            end
        end
    end
    return n
end

local function mat_has_product(mat, product)
    if not mat then return false end
    for _, id in ipairs(mat.reaction_product.id) do
        if id.value == product then return true end
    end
    return false
end

-- independent reimplementation of the brewable check used by --brewable
local function raw_is_brewable(raw)
    local mi = dfhack.matinfo.decode(
        raw.material_defs.type[df.plant_material_def.basic_mat],
        raw.material_defs.idx[df.plant_material_def.basic_mat])
    if mat_has_product(mi.material, 'DRINK_MAT') then return true end
    for _, g in ipairs(raw.growths) do
        local gm = dfhack.matinfo.decode(g.mat_type, g.mat_index)
        if mat_has_product(gm.material, 'DRINK_MAT') then return true end
    end
    return false
end

function test.dry_run()
    local before = count_marked()
    local out = run('-s', '-a', '-n', '2', '-d')
    local n_dry = tonumber(out:match('Would update (%d+) plant designations'))
    expect.ne(nil, n_dry)
    -- nothing may have been designated
    expect.eq(before, count_marked())

    -- a real run must update exactly the number the dry run reported
    local out_real = run('-s', '-a', '-n', '2')
    local n_real = tonumber(out_real:match('Updated (%d+) plant designations'))
    expect.eq(n_dry, n_real)
    expect.eq(before + n_real, count_marked())

    -- dry-run clear reports the currently marked count without clearing
    local out_cdry = run('-s', '-a', '-c', '-d')
    local n_cdry = tonumber(out_cdry:match('Would update (%d+) plant designations'))
    expect.eq(count_marked(), n_cdry)
    expect.eq(before + n_real, count_marked())

    local out_clear = run('-s', '-a', '-c')
    local n_clear = tonumber(out_clear:match('Updated (%d+) plant designations'))
    expect.eq(n_cdry, n_clear)
    expect.eq(0, count_marked())
end

function test.trait_filters()
    -- find a brewable and a non-brewable non-tree non-grass species
    local brewable_id, other_id
    for _, raw in ipairs(df.global.world.raws.plants.all) do
        if not raw.flags.TREE and not raw.flags.GRASS then
            if raw_is_brewable(raw) then
                brewable_id = brewable_id or raw.id
            else
                other_id = other_id or raw.id
            end
        end
    end
    expect.ne(nil, brewable_id)
    expect.ne(nil, other_id)

    -- a matching species is accepted by the filter
    local out = run(brewable_id, '--brewable', '-d')
    expect.str_find('Would update', out)

    -- a non-matching species is rejected with a specific message
    local out2 = run(other_id, '--brewable', '-d')
    expect.str_find('does not match the specified trait filters', out2)

    -- every species in the filtered ID listing must be brewable
    local listed = run('--brewable')
    local listed_count = 0
    for id in listed:gmatch('%* %(%a+%) ([%w_%-]+)') do
        listed_count = listed_count + 1
        local raw = find_raw(id)
        expect.true_(raw and raw_is_brewable(raw), id)
    end
    expect.true_(listed_count > 0)
end

function test.cloth_listing()
    local listed = run('--cloth')
    local listed_count = 0
    for id in listed:gmatch('%* %(%a+%) ([%w_%-]+)') do
        listed_count = listed_count + 1
        local raw = find_raw(id)
        expect.true_(raw and raw.flags.THREAD, id)
    end
    expect.true_(listed_count > 0)
end
