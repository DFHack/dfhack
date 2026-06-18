--[====[
This developer script was created to compare the effects of the old
and new versions of the `dig-now` plugin during development.

It works by recording interesting map blocks from the current map
when first run, then comparing those map blocks with the current
map on subsequent runs.  To clear the recorded blocks, run the
script from the main menu (i.e. with no map loaded).

Example usage:
  * Load a test map that has dig-designations.
  * Run the old version of the plugin: e.g. `dig-now-old`.
  * Run this script to record the map state.
  * Quit without saving back to the main menu.
  * Reload the same test map.
  * Run the new version of the plugin: e.g. `dig-now`.
  * Run this script again to compare the map state.
  * Analyze the results.
  * Quit without saving back to the main menu.
  * Run the script a third time to clear the results.
  

It will need to be modified to work with anything else.

code possibly of interest:
    ipairsN iterator, works with the `for` keyword.
    each_tile_in_block iterator, works with the `for` keyword.
    garbage collection of df types created by Lua:
        df.map_block and df.block_square_event
]====]

---@type df.map_block[]
record = record or setmetatable( {}, { __gc = GC_map_block_table, } )	-- global, persistant

--------------------------------------------------------------------------------
local addressof = require('utils').addressof

--------------------------------------------------------------------------------
---@param obj DFBase
---@return string
local function addressofs(obj)
    assert(df.isvalid(obj))
    return(string.format("%016X", addressof(obj)))
end

--------------------------------------------------------------------------------
---@param s string
---@param i integer
---@return string
local function pad(s,i)
    return s .. string.rep(' ', i - #s)
end

--------------------------------------------------------------------------------
---@param fmt string
---@param ... any
local function printf(fmt, ...)
    print(string.format(fmt, ...))
end

--------------------------------------------------------------------------------
--- `ipairsN()` is `ipairs()` that accepts one or more Lua list
---     or DF vector or array and iterates over them in lockstep.
---     * the lists must be of equal size.
---     * the lists must not have `nil` as the last element.
---     * really, the lists should not contain `nil` at all,
---         because such tables are not lists.
---         `Lua 5.3 Reference Manual §3.4.7`
--
--  usage example:
--	```
--      t1 = { 1, 2, 3, 4, }
--      t2 = { 'a', 'b', 'c', 'd', }
--      t3 = { df.job_type[1], df.job_type[2], df.job_type[3], df.job_type[4], }
--      t4 = { df.item_barst, df.item_blocksst, df.item_barrelst, df.item_binst, }
--
--      for i, a, b, c, d in ipairsN(t1, t2, t3, t4) do
--          print(i, a, b, c, d)
--      end
--      ```
-- warning! currently this returns a 1-based index even for Df vectors and arrays.
--      you must subtract 1 to index the proper element of a DF type.
--
---@generic T
---@param ... T[]  DF vectors or arrays, or Lua lists.
---@return fun(nil, integer?):integer?, T?...  iterator
---@return nil  state
---@return integer start  # always 0
local function ipairsN(...)
--  implementation difference: this returns iterator, nil, nil
--      unlike ipairs() which returns iterator, table, 0.
--      (this is because 0 is a valid index for DF vectors.)
--      the second nil is the state.
--      this implementation captures state as upvalues, so the
--      state passed between `for` and the iterator is not used.
    local is_container = require("utils").is_container
    local count = select('#', ...)
    assert(count >= 1)
    local arrays = {...}
    local length = #arrays[1]
    for i = 1, count do
        local array = arrays[i]
        assert(type(array) == "table" or is_container(array), "parameter " .. i .. " is not an array")
        assert(#arrays[i] == length, "array " .. i .. " is not the same size as array 1")
    end

    ---@generic T
    ---@param state nil
    ---@param index integer? 
    ---@return integer?  next-index
    ---@return T?...
    --  note: uses upvalues is_container, count, arrays, length
    --  note: debugging uses upvalues array and debugging_verify_arraysize for asserts.
    --  because of these upvalue captures, this returns a new closure every time.
    --  this could be avoided by nesting these and Array inside a state table.
    --  DONE: given that I'm capturing upvalues anyway, why pass state in and out?
    --      just pass in and out a nil state, and use the captures.
    local function iterator(state, index)
        assert(state == nil)
        assert((math.type(index) == "integer" and index >= 0 and index <= length), "index was bad or out-of-range: " .. tostring(index))
        for i = 1, count do
            local array = arrays[i]
            assert(#arrays[i] == length, "iterator: array " .. i .. " has changed length")
        end

        index = index + 1
        if index > length then
            return nil, table.unpack({}, 1, count)  -- return all nils
        end

        local list = {}
        for i = 1, count do
            local array = arrays[i]
            local idx = is_container(array) and index-1 or index
            list[i] = array[idx]
        end

        return index, table.unpack(list, 1, count)  -- this construct preserves trailing nils.
    end

    return iterator, nil, 0
end

--------------------------------------------------------------------------------
---@class (exact) block_key
---@field value unknown     -- intended to be opaque.

--------------------------------------------------------------------------------
---@param block df.map_block
---@return block_key
local function block_key(block)
    assert(df.isvalid(block))
    assert(df.map_block:is_instance(block))
    ---@diagnostic disable-next-line: return-type-mismatch
    return string.format( "(%d,%d,%d)", pos2xyz(block.map_pos) )
end

--------------------------------------------------------------------------------
---@param block df.map_block
function GC_map_block(block)
    assert(df.isvalid(block))
    assert(df.map_block:is_instance(block))
    block.pool_id = -1
    assert(block.block_burrows.item == nil)
    assert(block.block_burrows.prev == nil)
    assert(block.block_burrows.next == nil)
    -- ignore block.items; it's a int32_t vector so it's safe to destruct.
    assert(#block.flows == 0)
    block.flow_pool.reuse_idx = -1
    for i = #block.block_events-1,0,-1 do
        assert(df.block_square_event:is_instance(block.block_events[i]))
        block.block_events[i]:delete()
        block.block_events:erase(i)
    end
    assert(#block.block_events == 0)
    block:delete()
end

--------------------------------------------------------------------------------
---@param t table<block_key, df.map_block> | df.map_block[]
function GC_map_block_table(t)
    for k, block in pairs(t) do
        if df.map_block:is_instance(block) then
            GC_map_block(block)
        end
        t[k] = nil
    end
end

--------------------------------------------------------------------------------
---@param block df.map_block
---@return df.map_block
local function copy_map_block(block)
    assert(df.isvalid(block))
    assert(df.map_block:is_instance(block))
    local inb = block
    local outb = inb:new()
    outb.block_burrows.item = nil
    outb.block_burrows.prev = nil
    outb.block_burrows.next = nil
    outb.block_events:resize(0)
    outb.items:resize(0)
    outb.flows:resize(0)
    outb.flow_pool.reuse_idx = -1
    outb.flow_pool.flags.active = false
    outb.pool_id = -1
    for i, inbse in ipairs(inb.block_events) do
        assert(df.isvalid(inbse))
        assert(df.block_square_event:is_instance(inbse))
        assert(inbse._type ~= df.block_square_event)
        local outbse = inbse:new()
        assert(df.isvalid(outbse))
        assert(df.block_square_event:is_instance(outbse))
        assert(outbse._type ~= df.block_square_event)
        outb.block_events:insert('#', outbse)
    end
    return outb
end

if false then   -- test code
    collectgarbage("collect")
    ---@type df.map_block?
    local bc = copy_map_block(dfhack.maps.getTileBlock(16,16,135))
    printall(bc)
    GC_map_block(bc)
    bc = nil
    collectgarbage("collect")
    return
end
if false then   -- test code
    collectgarbage("collect")
    for z = 0, xyz2pos(dfhack.maps.getSize()).z - 1 do
        table.insert(record, copy_map_block(dfhack.maps.getTileBlock(16,16,z)))
        local block = copy_map_block(dfhack.maps.getTileBlock(32,32,z))
        record[block_key(block)] = block
    end
    record = nil
    collectgarbage("collect")
    return
end

--------------------------------------------------------------------------------
---@alias coord (df.coord | { x:integer, y:integer, z:integer })    -- a true df.coord or a 'pos' table.

--------------------------------------------------------------------------------
---@type     fun(x:integer, y:integer, z:integer):block:df.map_block?, lx:integer, ly:integer
---@overload fun(x:integer, y:integer, z:integer):block:df.map_block?, lx:integer, ly:integer   -- this seems to be necessary, but I don't know why.
---@overload fun(_nil:nil):_nil:nil, lx:integer, ly:integer
---@overload fun(pos:table):block:df.map_block?, lx:integer, ly:integer
---@overload fun(pos:df.coord):block:df.map_block?, lx:integer, ly:integer
---@overload fun(c:df.construction):block:df.map_block, lx:integer, ly:integer
---@overload fun(b:df.map_block):block:df.map_block?, lx:integer, ly:integer
local function get_block_and_local(x, y, z)
    if df.isvalid(x) then
        ---@cast x -nil
        if x._type == df.coord then
            return dfhack.maps.getTileBlock(x), x.x & 15, x.y & 15
        elseif x._type == df.construction then
            return dfhack.maps.getTileBlock(x.pos), x.pos.x & 15, x.pos.y & 15
        elseif x._type == df.map_block then
            return x, 0, 0
        end
        qerror("wasn't able to interpret parameter: " .. x._type)
    elseif type(x) == "nil" then
        return nil, 0, 0
    elseif math.type(x) == "integer" then
        return dfhack.maps.getTileBlock(x, y, z), x & 15, y & 15
    elseif type(x) == "table" then
        return dfhack.maps.getTileBlock(x), x.x & 15, x.y & 15
    end
    qerror("wasn't able to interpret parameter: " .. type(x))
end

--------------------------------------------------------------------------------
--- this is an iterator function.  it helps the `for` keyword process a map block.
--- for each of the 256 tiles in the block, the for loop gets these variables:
---     the map block, local_x, local_y, the tile's tiletype, the tile's designation,
---         and the tile's occupancy.
--- example code:
---     ```
---     for block, lx, ly, tt, des, occ in each_tile_in_block(12, 34, 56) do
---         print(lx, ly)
---     end
---     ```
--- note: if the block is nil then this loops 0 times, so there is no need to check.
---@param block_or_pos_or_x df.map_block|coord|integer|nil
---@param y integer?
---@param z integer?
---@return (fun(block:df.map_block):block:df.map_block, local_x:integer, local_y:integer, tiletype:df.tiletype, designation:df.tile_designation, occupancy:df.tile_occupancy), df.map_block?
local function each_tile_in_block(block_or_pos_or_x, y, z)

    --- this nested function is the workhorse for `each_tile_in_block`.
    --- it `yield`s with these parameters: `df.map_block block`, `integer local_x`,
    ---     `integer local_y`, `df.tiletype tiletype`,
    ---     `df.tile_designation designation`, `df.tile_occupancy occupancy`.
    --- when it is done, it returns nil, which ends the for loop.
    ---@param Block df.map_block
    ---@return nil
    local function block_iterator(Block)
    --  note: because this function doesn't share any of `each_tile_in_block`s
    --  parameters or locals, this function is not a closure.
        assert(df.map_block:is_instance(Block))
        for local_x = 0,15 do
            local tiletype_sheaf, designation_sheaf, occupancy_sheaf =
                Block.tiletype[local_x], Block.designation[local_x], Block.occupancy[local_x]
            for local_y = 0,15 do
                coroutine.yield(Block, local_x, local_y, tiletype_sheaf[local_y],
                    designation_sheaf[local_y], occupancy_sheaf[local_y])
            end
        end
        return nil
    end

    ---@type df.map_block?
    local block = get_block_and_local(block_or_pos_or_x, y, z)
    if block == nil then
        --  if there is no block, use this null iterator instead.
        --  this tells `for` to loop 0 times.
        return(function() return nil; end), nil
    end

    --  `coroutine.wrap` creates a wrapper of the iterator function.
    --  the `for` keyword will invoke the wrapper with parameter `block`.
    --  the wrapper will call `block_iterator` with parameter `block`.
    --  `block_iterator` will call `yield` once for each tile, with relevant data.
    --  the wrapper will pass this relevant data to the `for` keyword.
    --  when all 256 tiles have been processed via `yield`, `block_iterator`
    --      will return nil.
    --  the wrapper will return this nil to the `for` keyword.
    --  this terminates the for loop.
    --
    --  special note!  these `yield`s are not the same as the `yield`s that
    --  return control to Dwarf Fortress.
    return coroutine.wrap(block_iterator), block
end

--------------------------------------------------------------------------------
--- no longer used
---@param block df.map_block
---@param lx integer
---@param ly integer
---@param tt df.tiletype
---@param des df.tile_designation
---@param occ df.tile_occupancy
---@return boolean
local function interesting_map_block_tile(block, lx, ly, tt, des, occ)
    local attrs = df.tiletype.attrs[tt]
    -- ignore tiles flagged as caverns, magma, hell, adamantine pipes.
    if des.feature_global or des.feature_local then return false; end
    -- tiles that are normal wall aren't interesting.
    if (    (attrs.shape == df.tiletype_shape.WALL and attrs.special == df.tiletype_special.NORMAL)
        or  (attrs.shape == df.tiletype_shape.WALL and attrs.special == df.tiletype_special.NONE)
    ) then return false; end
    -- tiles with oddball materials aren't interesting.
    if (    attrs.material >= df.tiletype_material.FROZEN_LIQUID and attrs.material ~= df.tiletype_material.CONSTRUCTION
    ) then return false; end
    -- lit tiles that are normal floor, pebbles, boulders, ramps, or air aren't interesting.
    if des.light and (
            (attrs.shape == df.tiletype_shape.FLOOR and attrs.special == df.tiletype_special.NORMAL)
            or  attrs.shape == df.tiletype_shape.PEBBLES
            or  attrs.shape == df.tiletype_shape.BOULDER
        or  attrs.shape == df.tiletype_shape.RAMP
        or  attrs.material == df.tiletype_material.AIR
    ) then return false; end
    return true
end

--------------------------------------------------------------------------------
--- no longer used.
---@param block df.map_block
---@return boolean
local function interesting_map_block(block)
    for block, lx, ly, tt, des, occ in each_tile_in_block(block) do
        local attrs = df.tiletype.attrs[tt]
        if interesting_map_block_tile(block, lx, ly, tt, des, occ) then
            -- give a reason why this map block is interesting.
            print(block_key(block), pad(df.tiletype[tt],23), pad(df.tiletype_shape[attrs.shape],15),
                pad(df.tiletype_material[attrs.material],15), df.tiletype_special[attrs.special])
            return true
        end
    end
    return false
end

--------------------------------------------------------------------------------
--- for speed, this tests if a specific field in the first object is byte-for-byte
---     identical to the same field in the second object.
---@param obj1 DFStruct
---@param obj2 DFStruct
---@param field string
---@param size integer
local function identical_field(obj1, obj2, field, size)
    --assert(type(field) == "string")
    --assert(math.type(size) == "integer" and size > 0)
    --assert(df.isvalid(obj1) and df.isvalid(obj2) and obj1._type == obj2._type)
    -----@diagnostic disable-next-line: undefined-field
    --assert(obj1._kind == "struct" and obj1._type._fields[field] ~= nil)
    ---@diagnostic disable-next-line: undefined-field
    local offset = obj1._type._fields[field].offset
    local a1, a2 = addressof(obj1) + offset, addressof(obj2) + offset
    return dfhack.internal.memcmp(a1, a2, size) == 0
end

--------------------------------------------------------------------------------
---@param b1 df.map_block
---@param b2 df.map_block
local function co_compare_map_block(b1, b2)
    local yield = coroutine.yield
    if block_key(b1) ~= block_key(b2) then
        yield("ERROR", "", "different blocks", block_key(b1), block_key(b2))
        return nil
    end

    -- structured types with named fields, e.g. bitfields.
    if b1.flags.whole ~= b2.flags.whole then
        for k in pairs(b1.flags) do
            if k == "update_temperature" then
                -- do nothing
            elseif b1.flags[k] ~= b2.flags[k] then
                yield("flags", "", "." .. k, b1.flags[k], b2.flags[k])
            end
        end
    end

    -- df.tiletype[16][16]
    if not identical_field(b1, b2, "tiletype", df.tiletype:sizeof() * 16 * 16) then
        for x, ttx1, ttx2 in ipairsN(b1.tiletype, b2.tiletype) do
            for y, tt1, tt2 in ipairsN(ttx1, ttx2) do
                local a1, a2 = df.tiletype.attrs[tt1], df.tiletype.attrs[tt2]
                -- special case, digging operations create random variant floor tiles.
                if a1.variant ~= df.tiletype_variant.NONE and a2.variant ~= df.tiletype_variant.NONE then
                    -- findTileType() and findSimilarTileType are not exported to Lua.
                    tt1, tt2 = df.tiletype[tt1], df.tiletype[tt2]	-- string name
                    --tt1, tt2 = tt1:gsub('^(.*)[1234]$', "%11"), tt2:gsub('^(.*)[1234]$', "%11")
                    tt1, tt2 = tt1:sub(-#tt1, -2) .. '1', tt2:sub(-#tt2, -2) .. '1'
                    tt1, tt2 = df.tiletype[tt1], df.tiletype[tt2]	-- back to integer
                end
                if tt1 ~= tt2 then
                    -- TODO need to deal with tiletypes that have variants.
                    yield(x-1, y-1, "tiletype", df.tiletype[tt1], df.tiletype[tt2])
                end
            end
        end
    end

    -- df.tile_designation[16][16]
    if not identical_field(b1, b2, "designation", df.tile_designation:sizeof() * 16 * 16) then
        for x, v1y, v2y in ipairsN(b1.designation, b2.designation) do
            for y, v1, v2 in ipairsN(v1y, v2y) do
                if v1.whole ~= v2.whole then
                    for field in pairs(v1) do
                        if v1[field] ~= v2[field] then
                            yield(x-1, y-1, "designation" .. '.' .. field, v1[field], v2[field])
    end end end end end end

    -- df.tile_occupancy[16][16]
    if not identical_field(b1, b2, "occupancy", df.tile_occupancy:sizeof() * 16 * 16) then
        for x, v1y, v2y in ipairsN(b1.occupancy, b2.occupancy) do
            for y, v1, v2 in ipairsN(v1y, v2y) do
                if v1.whole ~= v2.whole then
                    for field in pairs(v1) do
                        if v1[field] ~= v2[field]
                            and field ~= "item" and field ~= "unit" and field ~= "unit_grounded"
                        then
                            yield(x-1, y-1, "occupancy" .. '.' .. field, v1[field], v2[field])
    end end end end end end

    -- ignore fog_of_war, path_cost, path_tag, walkable, map_edge_distance, temperature_1, temperature_2, lighting.

    -- don't ignore liquid_flow[16][16] ?
    if not identical_field(b1, b2, "liquid_flow", df.tile_liquid_flow:sizeof() * 16 * 16) then
        for x, v1y, v2y in ipairsN(b1.liquid_flow, b2.liquid_flow) do
            for y, v1, v2 in ipairsN(v1y, v2y) do
                if v1.whole ~= v2.whole then
                    for field in pairs(v1) do
                        if v1[field] ~= v2[field] then
                            yield(x-1, y-1, "liquid_flow" .. '.' .. field, v1[field], v2[field])
    end end end end end end

    -- int8_t[9]
    for i, k1, k2 in ipairsN(b1.region_offset, b2.region_offset) do
        if k1 ~= k2 then
            yield("index "..i-1, '', "region_offset", k1, k2)
        end
    end

    -- walk both block_events vectors.  lockstep?  but need to watch for missing events.
    local bev1, bev2 = b1.block_events, b2.block_events
    local i1, i2 = 0, 0
    while (i1 <= #bev1-1 and i2 <= #bev2-1) do
        local be1, be2 = bev1[i1], bev2[i2]
        local be1s, be2s = "event " .. i1, "event " .. i2
        assert(df.block_square_event:is_instance(be1))
        assert(df.block_square_event:is_instance(be2))
        if be1._type == be2._type then
            if be1._type == df.block_square_event_mineralst then
                if be1.inorganic_mat ~= be2.inorganic_mat then
                    yield(be1s, be2s, tostring(be1._type) .. " different inorganic_mat",
                        be1.inorganic_mat, be2.inorganic_mat)
                end
                for x = 0, 15 do
                    for y = 0, 15 do
                        local xy = x..','..y..':'
                        local bm1, bm2 = dfhack.maps.getTileAssignment(be1.tile_bitmask, x, y),
                                         dfhack.maps.getTileAssignment(be2.tile_bitmask, x, y)
                        if bm1 ~= bm2 then
                            yield(be1s, be2s, tostring(be1._type) .. " bitmap different",
                                    xy..tostring(bm1), xy..tostring(bm2))
                        end
                    end
                end
                if be1.flags.whole ~= be2.flags.whole then
                    yield(be1s, be2s, tostring(be1._type) .. " different flags",
                        be1.flags.whole, be2.flags.whole)
                end
            elseif false and be1._type == df.block_square_event_frozen_liquidst then
                -- TODO this is potentially important, check on a frozen map.
            elseif be1._type == df.block_square_event_grassst then

            elseif false and be1._type == df.block_square_event_designation_priorityst then
                for x = 0, 15 do
                    for y = 0, 15 do
                        local xy = x..','..y
                        if be1.priority[x][y] ~= be2.priority[x][y] then
                            yield(be1s, be2s, tostring(be1._type) .. ' ' .. xy,
                                be1.priority[x][y], be2.priority[x][y])
                        end
                    end
                end
            elseif false then
                yield(be1s, "", "ignoring block_square_event", tostring(be1._type), nil)
            end

            i1, i2 = i1+1, i2+1
        -- the following is naive; it assumes the simple case instead of handling every case.
        -- it could be improved somewhat by probing past the mismatched block_square_events
        -- to see if a matching type is next in one of the vectors.
        -- that still wouldn't handle the most complex general case.
        elseif #bev1-i1 < #bev2-i2 then
            yield(be1s, be2s, "missing block_square_event (1), skipping", nil, nil)
            i1 = i1+1
        elseif #bev1-i1 > #bev2-i2 then
            yield(be1s, be2s, "missing block_square_event (2), skipping", nil, nil)
            i2 = i2+1
        else
            yield(be1s, be2s, "mismatched block_square_events; aborting event comparison", be1._type, be2._type)
            i1, i2 = #bev1, #bev2
            break
        end
        ::continue::
    end
    return nil
end

--------------------------------------------------------------------------------
---@param b1 df.map_block
---@param b2 df.map_block
local function compare_map_block(b1, b2)
    local max = 30      -- number of differences to display for this map block.
    local count = 0
    local co = coroutine.create(co_compare_map_block)

    ---@alias errormessage string  -- note: error messages have extra functionality in a metatable.
    ---@type boolean, integer|string|errormessage, integer|string, string, string, string
    local ok, x, y, desc, val1, val2 = dfhack.saferesume(co, b1, b2)

    while(ok and x and count < max) do
        local xy = string.format("%s%s%s", tostring(x), (y == '' and '' or ' '),
                (y ~= nil and tostring(y) or ''))
        if math.type(x) == "integer" and math.type(y) == "integer" then
            xy = string.format("tile %d,%d", x, y)
        end
        print(string.format("map block %s %s %s: %s%s", tostring(block_key(b1)),
                xy, tostring(desc), tostring(val1), val2 ~= nil and (" ~= " .. tostring(val2)) or ''))
        count = count + 1
        ok, x, y, desc, val1, val2 = dfhack.saferesume(co)
    end

    if not ok then
        local err = x
        qerror(err)
    end
    if count >= max then
        print("too many differences in block, skipping.")
    end
    return count
end

--------------------------------------------------------------------------------
-- test code for compare_map_block.
-- this was written for a map_block with six mineral veins, one type of grass,
--      and no other events.
-- it massages the first mineral vein and the grass.
if false then
    compare_map_block(dfhack.maps.getTileBlock(16,16,118),
        dfhack.maps.getTileBlock(16,16,119))  -- test wrong block
    local total = 0
    -- this block on the test map is an underground block with two mineral veins
    -- and no other map events.
    local b = copy_map_block(dfhack.maps.getTileBlock(16,16,142))
    b.flags.has_magma_close = true
    b.tiletype[15][15] = 15
    b.tiletype[14][14] = 14
    b.designation[13][13].light = true
    b.occupancy[12][12].unit_grounded = true    -- ignored
    b.fog_of_war[11][11] = 11                   -- ignored
    b.path_cost[10][10] = 10                    -- ignored
    b.path_tag[9][9] = 9                        -- ignored
    b.walkable[8][8] = 8                        -- ignored
    b.map_edge_distance[7][7] = 7               -- ignored
    b.temperature_1[6][6] = 6                   -- ignored
    b.temperature_2[5][5] = 5                   -- ignored
    b.lighting[4][4] = 4
    b.region_offset[3] = 9                      -- out of range
    if #b.block_events > 0 and b.block_events[0]._type == df.block_square_event_mineralst then
        local bem = b.block_events[0]
        ---@cast bem df.block_square_event_mineralst
        bem.inorganic_mat = 2
        bem.tile_bitmask.bits[1] = bem.tile_bitmask.bits[1] ~ (1<<1)
        bem.flags.whole = 0
    end
    if #b.block_events > 0 and b.block_events[#b.block_events-1]._type == df.block_square_event_grassst then
        local beg = b.block_events[#b.block_events-1]
        ---@cast beg df.block_square_event_grassst
        beg.plant_index = beg.plant_index - 1
        beg.amount[0][0] = 255
    end
    local count = compare_map_block(b, dfhack.maps.getTileBlock(b.map_pos))
    total = total + count
    print("total differences", total)
    -- leak the map_block, we're not testing GC here.
    return
end

--------------------------------------------------------------------------------
local function record_map_blocks()
    printf("recording map blocks.")
    local count = 0
    for _, block in ipairs(df.global.world.map.map_blocks) do
        if true or interesting_map_block(block) then
            count = count + 1
            table.insert(record, copy_map_block(block))
        end
    end
    printf("map recorded.  found %d blocks.", count)
end

--------------------------------------------------------------------------------
local function compare_recorded_map_blocks()
    printf("comparing recorded map with current map.")
    printf("note: it seems normal to have a few differences in the block flags")
    printf(".update_liquid and .update_liquid_twice for a couple of random blocks.")
    local total = 0
    local max = 333     -- the number of differences to list before aborting.
    local aborted = false
    for i, block_copy in ipairs(record) do
        local count = compare_map_block(block_copy, dfhack.maps.getTileBlock(block_copy.map_pos))
        total = total + count
        if total > max then aborted = true; break; end
    end
    if aborted then printf("aborted! too many differences found."); end
    printf("total differences found: %d", total)
end

--------------------------------------------------------------------------------
if not dfhack.isMapLoaded() then
    if type(record) == "table" and next(record, nil) ~= nil then
        print("discarding recorded map blocks.")
    end
    record = nil    -- this uses the garbage collection mechanism to :delete()
                    --  the map blocks.
    collectgarbage("collect")
elseif next(record, nil) == nil then
    record_map_blocks()
else
    compare_recorded_map_blocks()
end

--[[
[DFHack]# lua -f ./../../../compare_map.lua
comparing recorded map with current map.
map block (64,64,12) tile 7,6 tiletype: StoneStairUD ~= MineralStairUD
map block (48,32,125) tile 2,13 tiletype: LavaFloor1 ~= StoneFloor1
map block (48,32,125) tile 2,14 tiletype: LavaFloor1 ~= StoneFloor1
map block (48,32,125) tile 3,13 tiletype: LavaFloor1 ~= StoneFloor1
map block (48,32,125) tile 3,14 tiletype: LavaFloor1 ~= StoneFloor1
map block (48,32,125) tile 4,12 tiletype: LavaFloor1 ~= StoneFloor1
map block (48,32,125) tile 4,13 tiletype: LavaFloor1 ~= StoneFloor1
map block (48,32,125) tile 4,14 tiletype: LavaFloor1 ~= StoneFloor1
map block (48,32,125) tile 5,12 tiletype: LavaFloor1 ~= StoneFloor1
map block (48,32,125) tile 5,13 tiletype: LavaFloor1 ~= StoneFloor1
map block (48,32,125) tile 5,14 tiletype: LavaFloor1 ~= StoneFloor1
map block (32,32,136) flags .designated: true ~= false
map block (64,64,10) tile 7,6 tiletype: StoneStairUD ~= MineralStairUD
map block (64,64,10) tile 8,5 tiletype: StoneFloor1 ~= MineralFloor1
map block (64,64,10) tile 8,6 tiletype: StoneFloor1 ~= MineralFloor1
map block (64,64,10) tile 8,7 tiletype: StoneFloor1 ~= MineralFloor1
map block (64,64,10) tile 9,5 tiletype: StoneFloor1 ~= MineralFloor1
map block (64,64,10) tile 9,7 tiletype: StoneFloor1 ~= MineralFloor1
map block (64,64,10) tile 10,5 tiletype: StoneFloor1 ~= MineralFloor1
map block (64,64,10) tile 10,6 tiletype: StoneFloor1 ~= MineralFloor1
map block (64,64,10) tile 10,7 tiletype: StoneFloor1 ~= LavaFloor1
map block (32,32,123) flags .update_liquid: false ~= true
map block (32,32,123) flags .update_liquid_twice: false ~= true
map block (64,64,9) tile 6,5 tiletype: StoneRamp ~= FeatureRamp
map block (64,64,9) tile 7,5 tiletype: StoneFloor1 ~= FeatureFloor1
map block (64,64,9) tile 7,6 tiletype: StoneStairUD ~= MineralStairUD
map block (32,32,125) flags .update_liquid: true ~= false
map block (32,32,125) flags .update_liquid_twice: true ~= false
map block (32,48,136) flags .designated: true ~= false
map block (64,64,13) tile 7,6 tiletype: StoneStairUD ~= MineralStairUD
map block (64,64,8) tile 7,6 tiletype: StoneStairUD ~= FeatureStairUD
map block (64,64,7) tile 7,6 tiletype: StoneStairUD ~= MineralStairUD
map block (64,64,6) tile 7,6 tiletype: StoneStairU ~= FeatureStairU
map block (64,64,14) tile 7,6 tiletype: StoneStairUD ~= MineralStairUD
map block (64,64,11) tile 7,6 tiletype: StoneStairUD ~= MineralStairUD
map block (48,32,124) tile 2,13 tiletype: LavaFloor1 ~= StoneFloor1
map block (48,32,124) tile 2,14 tiletype: LavaFloor1 ~= StoneFloor1
map block (48,32,124) tile 3,13 tiletype: LavaFloor1 ~= StoneFloor1
map block (48,32,124) tile 3,14 tiletype: LavaFloor1 ~= StoneFloor1
map block (48,32,124) tile 4,12 tiletype: LavaFloor1 ~= StoneFloor1
map block (48,32,124) tile 4,13 tiletype: LavaFloor1 ~= StoneFloor1
map block (48,32,124) tile 4,14 tiletype: LavaFloor1 ~= StoneFloor1
map block (48,32,124) tile 5,12 tiletype: LavaFloor1 ~= StoneFloor1
map block (48,32,124) tile 5,13 tiletype: LavaFloor1 ~= StoneFloor1
map block (48,32,124) tile 5,14 tiletype: LavaFloor1 ~= StoneFloor1
map block (32,48,137) flags .designated: true ~= false
total differences found: 46
[DFHack]#
--]]
