config.mode = 'fortress'
config.target = 'core'

local function first_block()
    return df.global.world.map.map_blocks[0]
end

local function block_bounds(block)
    local p = block.map_pos
    return {p.x, p.y, p.z, p.x + 15, p.y + 15, p.z}
end

function test.forEachTile_counts_tiles()
    local block = first_block()
    local res = dfhack.maps.forEachTile(block_bounds(block))
    expect.eq(256, res.scanned)
    expect.eq(256, res.matched)
    expect.eq(0, res.changed)
    expect.eq(0, res.constructed)
    expect.false_(res.aborted)
end

function test.forEachTile_filters_by_attribute()
    local block = first_block()
    local want = 0
    for x = 0, 15 do for y = 0, 15 do
        if df.tiletype.attrs[block.tiletype[x][y]].material ==
                df.tiletype_material.STONE then
            want = want + 1
        end
    end end

    local res = dfhack.maps.forEachTile(block_bounds(block), {
        material = {df.tiletype_material.STONE},
    })
    expect.eq(want, res.matched)
end

function test.forEachTile_filters_by_tiletype()
    local block = first_block()
    local tt = block.tiletype[0][0]
    local want = 0
    for x = 0, 15 do for y = 0, 15 do
        if block.tiletype[x][y] == tt then want = want + 1 end
    end end

    local res = dfhack.maps.forEachTile(block_bounds(block), {tiletype = tt})
    expect.eq(want, res.matched)
end

function test.forEachTile_set_tiletype()
    local block = first_block()
    local p = block.map_pos
    local orig = block.tiletype[0][0]
    return dfhack.with_finalize(function()
        block.tiletype[0][0] = orig
    end, function()
        local res = dfhack.maps.forEachTile(
            {p.x, p.y, p.z, p.x, p.y, p.z}, nil,
            {set_tiletype = df.tiletype.OpenSpace})
        expect.eq(1, res.matched)
        expect.eq(1, res.changed)
        expect.eq(df.tiletype.OpenSpace, block.tiletype[0][0])
    end)
end

function test.forEachTile_designation_action_and_predicate()
    local block = first_block()
    local p = block.map_pos
    local des = block.designation[0][0]
    local orig = des.hidden
    return dfhack.with_finalize(function()
        des.hidden = orig
    end, function()
        local res = dfhack.maps.forEachTile(
            {p.x, p.y, p.z, p.x, p.y, p.z}, nil,
            {designation = {hidden = not orig}})
        expect.eq(1, res.matched)
        expect.eq(not orig, des.hidden)

        -- the same spec used as a predicate now matches
        local res2 = dfhack.maps.forEachTile(
            {p.x, p.y, p.z, p.x, p.y, p.z},
            {designation = {hidden = not orig}})
        expect.eq(1, res2.matched)
    end)
end

function test.forEachTile_callback()
    local block = first_block()
    local seen = 0
    local res = dfhack.maps.forEachTile(block_bounds(block), nil,
        function(x, y, z, block_arg, tt)
            seen = seen + 1
            expect.eq(block.map_pos.x, block_arg.map_pos.x)
            expect.eq(block.tiletype[x % 16][y % 16], tt)
        end)
    expect.eq(256, seen)
    expect.eq(256, res.matched)
end

function test.forEachTile_callback_abort()
    local block = first_block()
    local calls = 0
    local res = dfhack.maps.forEachTile(block_bounds(block), nil, {
        callback = function()
            calls = calls + 1
            return false
        end,
    })
    expect.eq(1, calls)
    expect.eq(1, res.matched)
    expect.true_(res.aborted)
end

function test.forEachTile_construct()
    local block = first_block()
    local p = block.map_pos
    local orig = block.tiletype[0][0]
    local pos = xyz2pos(p.x, p.y, p.z)
    return dfhack.with_finalize(function()
        block.tiletype[0][0] = orig
        -- findAtTile can't remove; drop the record from the vector directly
        local vec = df.global.world.event.constructions
        for i = 0, #vec - 1 do
            local c = vec[i]
            if c.pos.x == p.x and c.pos.y == p.y and c.pos.z == p.z then
                vec:erase(i)
                break
            end
        end
    end, function()
        local res = dfhack.maps.forEachTile(
            {p.x, p.y, p.z, p.x, p.y, p.z}, nil, {
            construct = {
                item_type = df.item_type.BLOCKS,
                mat_type = 0,
                mat_index = 0,
                tiletype = df.tiletype.ConstructedFloor,
                flags = {no_build_item = true},
            },
        })
        expect.eq(1, res.constructed)
        expect.eq(1, res.changed)
        expect.eq(df.tiletype.ConstructedFloor, block.tiletype[0][0])

        local con = dfhack.constructions.findAtTile(pos)
        expect.ne(nil, con)
        if con then
            expect.eq(df.item_type.BLOCKS, con.item_type)
            expect.eq(orig, con.original_tile)
            expect.true_(con.flags.no_build_item)
        end

        -- a second scan must not duplicate the construction
        local res2 = dfhack.maps.forEachTile(
            {p.x, p.y, p.z, p.x, p.y, p.z}, nil, {
            construct = {item_type = df.item_type.BLOCKS, mat_type = 0, mat_index = 0},
        })
        expect.eq(0, res2.constructed)
    end)
end
