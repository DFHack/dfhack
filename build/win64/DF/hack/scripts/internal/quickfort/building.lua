-- building-related logic shared between the quickfort build, place, and zone
-- modules
--@ module = true

if not dfhack_flags.module then
    qerror('this script cannot be called directly')
end

local quickfort_common = reqscript('internal/quickfort/common')
local quickfort_map = reqscript('internal/quickfort/map')
local quickfort_parse = reqscript('internal/quickfort/parse')
local quickfort_preview = reqscript('internal/quickfort/preview')

local log = quickfort_common.log
local logfn = quickfort_common.logfn

-- assumes num is a non-negative integer
local function get_digit_count(num)
    local num_digits = 1
    while num >= 10 do
        num = num / 10
        num_digits = num_digits + 1
    end
    return num_digits
end

-- total string length will be width+1
local function left_pad(num, width)
    local num_digit_count = get_digit_count(num)
    local ret = ''
    for i=num_digit_count,width do
        ret = ret .. ' '
    end
    return ret .. tostring(num)
end

-- pretty-prints the populated range of the seen_grid
local function dump_seen_grid(args)
    local label, seen_grid, max_id = args[1], args[2], args[3]
    local x_min, x_max, y_min, y_max = 30000, -30000, 30000, -30000
    for x, row in pairs(seen_grid) do
        if x < x_min then x_min = x end
        if x > x_max then x_max = x end
        for y, _ in pairs(row) do
            if y < y_min then y_min = y end
            if y > y_max then y_max = y end
        end
    end
    print(string.format('boundary map (%s):', label))
    local field_width = get_digit_count(max_id)
    local blank = string.rep(' ', field_width+1)
    for y=y_min,y_max do
        local line = ''
        for x=x_min,x_max do
            if seen_grid[x] and tonumber(seen_grid[x][y]) then
                line = line .. left_pad(seen_grid[x][y], field_width)
            else
                line = line .. blank
            end
        end
        print(line)
    end
end

-- maps building boundaries, returns number of invalid keys seen
-- populates seen_grid coordinates with the building id so we can build an
-- extent_grid later. spreadsheet cells that define extents (e.g. a(5x5)) create
-- buildings separate from adjacent cells, even if they have the same type.
local function flood_fill(ctx, grid, seen_cells, x, y, data, db, aliases)
    local seen_grid = data.seen_grid
    if safe_index(seen_grid, x, y) then return 0 end
    if not safe_index(grid, y, x) then return 0 end
    local cell, text = grid[y][x].cell, grid[y][x].text
    if seen_cells[cell] then return 0 end
    local keys, extent = quickfort_parse.parse_cell(ctx, text)
    if aliases[string.lower(keys)] then keys = aliases[string.lower(keys)] end
    local db_entry = db[keys]
    if not db_entry then
        ensure_key(seen_grid, x)[y] = true -- seen, but not part of any building
        dfhack.printerr(('invalid key sequence in cell %s: "%s"'):format(cell, text))
        return 1
    end
    if db_entry.transform then
        keys = db_entry:transform(ctx)
        db_entry = keys and db[keys] or nil
        if not db_entry then
            ensure_key(seen_grid, x)[y] = true -- seen, but not part of any building
            dfhack.printerr(('invalid transformed key sequence in cell %s: "%s"->"%s"'):format(cell, text, keys))
            return 1
        end
    end
    if data.db_entry and (data.db_entry.label ~= db_entry.label or extent.specified) then
        return 0
    end
    log('mapping spreadsheet cell %s with text "%s"', cell, text)
    if data.db_entry then
        if data.db_entry.merge_fn then data.db_entry:merge_fn(db_entry) end
    else
        data.db_entry = db_entry
    end
    table.insert(data.cells, cell)
    seen_cells[cell] = true
    -- note that extent width and height can be negative: they can
    -- describe a box from any corner
    for tgt_x=math.min(x,x+extent.width+1),math.max(x+extent.width-1,x) do
        for tgt_y=math.min(y,y+extent.height+1),math.max(y+extent.height-1,y) do
            -- this may overlap with another building, but that's handled later
            ensure_key(seen_grid, tgt_x)[tgt_y] = data.id
            if tgt_x < data.x_min then data.x_min = tgt_x end
            if tgt_x > data.x_max then data.x_max = tgt_x end
            if tgt_y < data.y_min then data.y_min = tgt_y end
            if tgt_y > data.y_max then data.y_max = tgt_y end
        end
    end
    if extent.specified then return 0 end
    return flood_fill(ctx, grid, seen_cells, x-1, y-1, data, db, aliases) +
            flood_fill(ctx, grid, seen_cells, x-1, y, data, db, aliases) +
            flood_fill(ctx, grid, seen_cells, x-1, y+1, data, db, aliases) +
            flood_fill(ctx, grid, seen_cells, x, y-1, data, db, aliases) +
            flood_fill(ctx, grid, seen_cells, x, y+1, data, db, aliases) +
            flood_fill(ctx, grid, seen_cells, x+1, y-1, data, db, aliases) +
            flood_fill(ctx, grid, seen_cells, x+1, y, data, db, aliases) +
            flood_fill(ctx, grid, seen_cells, x+1, y+1, data, db, aliases)
end

local function swap_id_and_trim_chunk(chunk, seen_grid, from_id)
    local x_min, x_max = chunk.x_max, chunk.x_min
    local y_min, y_max = chunk.y_max,chunk.y_min
    for x=chunk.x_min,chunk.x_max do
        for y=chunk.y_min,chunk.y_max do
            if safe_index(seen_grid, x, y) == from_id then
                seen_grid[x][y] = chunk.id
                x_min, x_max = math.min(x_min, x), math.max(x_max, x)
                y_min, y_max = math.min(y_min, y), math.max(y_max, y)
            end
        end
    end
    chunk.x_min, chunk.x_max = x_min, x_max
    chunk.y_min, chunk.y_max = y_min, y_max
end

-- if an extent is too large in any dimension, chunk it up intelligently. we
-- can't just split it into a grid since the pieces might not align cleanly
-- (think of a solid block of staggered workshops of the same type). instead,
-- scan the edges and break off pieces as we find them. scan in an order that
-- results in the same chunking regardless of any applied transformations.
local function chunk_extents(data_tables, invert)
    local chunks = {}
    for i, data in ipairs(data_tables) do
        local seen_grid = data.seen_grid
        local db_entry = data.db_entry
        local max_width = db_entry.max_width
        local max_height = db_entry.max_height
        local width = data.x_max - data.x_min + 1
        local height = data.y_max - data.y_min + 1
        if width <= max_width and height <= max_height then
            table.insert(chunks, data)
            goto continue
        end
        local chunk = nil
        local cuts = 0
        local startx, endx, stepx = data.x_min, data.x_max, 1
        local starty, endy, stepy = data.y_min, data.y_max, 1
        if invert.x then
            startx, endx, stepx = data.x_max, data.x_min, -1
        end
        if invert.y then
            starty, endy, stepy = data.y_max, data.y_min, -1
        end
        for x=startx,endx,stepx do
            for y=starty,endy,stepy do
                if safe_index(seen_grid, x, y) ~= data.id then
                    goto inner_continue
                end
                chunk = copyall(data)
                chunk.id = #data_tables - (i - 1) + #chunks + 1
                if invert.x then
                    chunk.x_max = x
                    chunk.x_min = math.max(x - max_width + 1, data.x_min)
                else
                    chunk.x_min = x
                    chunk.x_max = math.min(x + max_width - 1, data.x_max)
                end
                if invert.y then
                    chunk.y_max = y
                    chunk.y_min = math.max(y - max_height + 1, data.y_min)
                else
                    chunk.y_min = y
                    chunk.y_max = math.min(y + max_height - 1, data.y_max)
                end
                swap_id_and_trim_chunk(chunk, seen_grid, data.id)
                table.insert(chunks, chunk)
                cuts = cuts + 1
                ::inner_continue::
            end
        end
        -- use the original data.id for the last chunk so our ids are contiguous
        local old_chunk_id = chunk.id
        chunk.id = data.id
        swap_id_and_trim_chunk(chunk, seen_grid, old_chunk_id)
        log('%s area too big; chunking into %d parts ' ..
            '(defined in spreadsheet cells %s)',
            db_entry.label, cuts, table.concat(data.cells, ', '))
        ::continue::
    end
    return chunks
end

-- expand multi-tile buildings that are less than their min dimensions around
-- their current center. if the blueprint has been transformed, ensures the
-- expansion respects the original orientation.
local function expand_buildings(data_tables, invert)
    for _, data in ipairs(data_tables) do
        local seen_grid = data.seen_grid
        local db_entry = data.db_entry
        if db_entry.has_extents then goto continue end
        local width = data.x_max - data.x_min + 1
        local height = data.y_max - data.y_min + 1
        local min_width = db_entry.min_width
        local min_height = db_entry.min_height
        if width < min_width then
            local center_x = math.floor((data.x_min + data.x_max) / 2)
            if invert.x then
                data.x_max = math.floor(math.ceil(center_x) + min_width / 2)
                data.x_min = data.x_max - min_width + 1
            else
                data.x_min = math.ceil(math.floor(center_x) - min_width / 2)
                data.x_max = data.x_min + min_width - 1
            end
        end
        if height < min_height then
            local center_y = (data.y_min + data.y_max) / 2
            if invert.y then
                data.y_max = math.floor(math.ceil(center_y) + min_height / 2)
                data.y_min = data.y_max - min_height + 1
            else
                data.y_min = math.ceil(math.floor(center_y) - min_height / 2)
                data.y_max = data.y_min + min_height - 1
            end
        end
        for x=data.x_min,data.x_max do
            if not seen_grid[x] then seen_grid[x] = {} end
            for y=data.y_min,data.y_max do
                -- only expand into unclaimed tiles
                if not seen_grid[x][y] then seen_grid[x][y] = data.id end
            end
        end
        ::continue::
    end
end

local function build_extent_grid(data)
    local seen_grid = data.seen_grid
    local extent_grid, num_tiles = {}, 0
    for x=data.x_min,data.x_max do
        local extent_x = x - data.x_min + 1
        extent_grid[extent_x] = {}
        for y=data.y_min,data.y_max do
            local extent_y = y - data.y_min + 1
            if seen_grid[x] and seen_grid[x][y] == data.id then
                extent_grid[extent_x][extent_y] = true
                num_tiles = num_tiles + 1
            end
        end
    end
    local width = data.x_max - data.x_min + 1
    local height = data.y_max - data.y_min + 1
    return num_tiles > 0 and extent_grid or nil,
            num_tiles == width * height
end

local function merge_building_data(to_data, from_data)
    to_data.db_entry:merge_fn(from_data.db_entry)
    for _,cell in ipairs(from_data.cells) do
        table.insert(to_data.cells, cell)
    end
    to_data.x_min = math.min(to_data.x_min, from_data.x_min)
    to_data.x_max = math.max(to_data.x_max, from_data.x_max)
    to_data.y_min = math.min(to_data.y_min, from_data.y_min)
    to_data.y_max = math.max(to_data.y_max, from_data.y_max)
    for x=from_data.x_min,from_data.x_max do
        for y=from_data.y_min,from_data.y_max do
            if safe_index(from_data.seen_grid, x, y) == from_data.id then
                ensure_key(to_data.seen_grid, x)[y] = to_data.id
            end
        end
    end
end

local function dump_data_tables(desc, data_tables, common_seen_grid, non_occluding)
    if non_occluding then
        for _,data in ipairs(data_tables) do
            logfn(dump_seen_grid, desc, data.seen_grid, #data_tables)
        end
    else
        logfn(dump_seen_grid, desc, common_seen_grid, #data_tables)
    end
end

-- build boundaries and extent maps from blueprint grid input
function init_buildings(ctx, zlevel, grid, buildings, db, aliases, non_occluding)
    local invalid_keys = 0
    local data_tables = {}
    local global_labels = {} -- label -> id
    local common_seen_grid = {} -- [x][y] -> id
    local seen_cells = {} -- str -> true
    aliases = aliases or {}
    for y, row in pairs(grid) do
        for x in pairs(row) do
            if safe_index(common_seen_grid, x, y) then goto continue end
            local data = {
                id=#data_tables+1, cells={}, db_entry=nil,
                seen_grid=non_occluding and {} or common_seen_grid,
                x_min=30000, x_max=-30000, y_min=30000, y_max=-30000
            }
            invalid_keys = invalid_keys +
                    flood_fill(ctx, grid, seen_cells, x, y, data, db, aliases)
            if data.db_entry then
                if data.db_entry.global_label then
                    local prev_id = global_labels[data.db_entry.global_label]
                    if prev_id then
                        merge_building_data(data_tables[prev_id], data)
                        goto continue
                    end
                    global_labels[data.db_entry.global_label] = data.id
                end
                table.insert(data_tables, data)
            end
            ::continue::
        end
    end
    dump_data_tables('after edge detection', data_tables, common_seen_grid, non_occluding)
    local tvec = ctx.transform_fn({x=1, y=-2}, true)
    local invert = {
        x=tvec.x == -1 or tvec.x == 2,
        y=tvec.y == -1 or tvec.y == 2
    }
    data_tables = chunk_extents(data_tables, invert)
    dump_data_tables('after chunking', data_tables, common_seen_grid, non_occluding)
    expand_buildings(data_tables, invert)
    dump_data_tables('after expansion', data_tables, common_seen_grid, non_occluding)
    for _, data in ipairs(data_tables) do
        local extent_grid, is_solid = build_extent_grid(data)
        if not data.db_entry.has_extents and not is_solid then
            dfhack.printerr(
                ('space needed for "%s" is taken by adjacent structures ' ..
                 '(defined in spreadsheet cells: %s)'):format(
                        data.db_entry.label, table.concat(data.cells, ', ')))
        else
            table.insert(buildings,
                         {cells=data.cells,
                          db_entry=data.db_entry,
                          pos=xyz2pos(data.x_min, data.y_min, zlevel),
                          width=data.x_max-data.x_min+1,
                          height=data.y_max-data.y_min+1,
                          extent_grid=extent_grid})
        end
    end
    return invalid_keys
end

-- y dimension may be sparse, but x must be contiguous, even if empty
local function count_extent_tiles(extent_grid, width, height, startx, starty)
    local num_tiles = 0
    for extent_x=startx or 1,width do
        for extent_y=starty or 1,height do
            if extent_grid[extent_x][extent_y] then
                num_tiles = num_tiles + 1
            end
        end
    end
    return num_tiles
end

local function trim_empty_cols(b)
    local height = b.height
    if height <= 0 then return end
    -- trim from right
    while b.width > 0 and
            count_extent_tiles(b.extent_grid, b.width, height, b.width) == 0 do
        b.extent_grid[b.width] = nil
        b.width = b.width - 1
    end
    -- trim from left
    while b.width > 0 and count_extent_tiles(b.extent_grid, 1, height) == 0 do
        -- x dimension is contiguous so we can use table.remove()
        table.remove(b.extent_grid, 1)
        b.width = b.width - 1
        b.pos.x = b.pos.x + 1
    end
end

local function trim_empty_rows(b)
    local width = b.width
    if width <= 0 then return end
    -- trim from bottom
    while b.height > 0 and
            count_extent_tiles(b.extent_grid, width, b.height,
                               1, b.height) == 0 do
        for extent_x=1,width do
            b.extent_grid[extent_x][b.height] = nil
        end
        b.height = b.height - 1
    end
    -- trim from top
    while b.height > 0 and count_extent_tiles(b.extent_grid, width, 1) == 0 do
        for extent_x=1,width do
            for extent_y=1,b.height do
                b.extent_grid[extent_x][extent_y] =
                        b.extent_grid[extent_x][extent_y+1]
            end
            b.extent_grid[extent_x][b.height] = nil
        end
        b.height = b.height - 1
        b.pos.y = b.pos.y + 1
    end
end

local function has_area(b)
    return b.width > 0 and b.height > 0
end

local function clear_building(b)
    b.width, b.height, b.extent_grid, b.pos = 0, 0, {}, nil
end

-- check bounds against size limits and map edges, adjust pos, width, height,
-- and extent_grid accordingly. nulls or zeroes out config for buildings that
-- are cropped below their minimum dimensions
function crop_to_bounds(ctx, buildings)
    local out_of_bounds_tiles = 0
    local bounds = ctx.bounds or quickfort_map.MapBoundsChecker{}
    for _, b in ipairs(buildings) do
        if not b.pos then goto continue end
        local prev_oob, db_entry = out_of_bounds_tiles, b.db_entry
        -- if zlevel is out of bounds, the whole extent is out of bounds
        if not bounds:is_on_map_z(b.pos.z) then
            out_of_bounds_tiles = out_of_bounds_tiles +
                    count_extent_tiles(b.extent_grid, b.width, b.height)
            clear_building(b)
        end
        -- if building extends off map to bottom or right, crop
        while has_area(b) and not bounds:is_on_map_y(b.pos.y + b.height - 1) do
            for extent_x=1,b.width do
                if b.extent_grid[extent_x][b.height] then
                    out_of_bounds_tiles = out_of_bounds_tiles + 1
                end
                b.extent_grid[extent_x][b.height] = nil
            end
            b.height = b.height - 1
            if b.height < db_entry.min_height then
                out_of_bounds_tiles = out_of_bounds_tiles +
                    count_extent_tiles(b.extent_grid, b.width, b.height)
                clear_building(b)
            end
        end
        while has_area(b) and not bounds:is_on_map_x(b.pos.x+b.width-1) do
            out_of_bounds_tiles = out_of_bounds_tiles +
                count_extent_tiles(b.extent_grid, b.width, b.height, b.width)
            b.extent_grid[b.width] = nil
            b.width = b.width - 1
            if b.width < db_entry.min_width then
                out_of_bounds_tiles = out_of_bounds_tiles +
                    count_extent_tiles(b.extent_grid, b.width, b.height)
                clear_building(b)
            end
        end
        -- if pos is off the map up or to the left, crop and move pos until
        -- we're ok (or empty)
        while has_area(b) and not bounds:is_on_map_y(b.pos.y) do
            for extent_x=1,b.width do
                if b.extent_grid[extent_x][1] then
                    out_of_bounds_tiles = out_of_bounds_tiles + 1
                end
                -- grid is sparse in y so we can't just use table.remove()
                for extent_y=1,b.height do
                    b.extent_grid[extent_x][extent_y] =
                            b.extent_grid[extent_x][extent_y+1]
                end
                b.extent_grid[extent_x][b.height] = nil
            end
            b.height = b.height - 1
            b.pos.y = b.pos.y + 1
            if b.height < db_entry.min_height then
                out_of_bounds_tiles = out_of_bounds_tiles +
                    count_extent_tiles(b.extent_grid, b.width, b.height)
                clear_building(b)
            end
        end
        while has_area(b) and not bounds:is_on_map_x(b.pos.x) do
            out_of_bounds_tiles = out_of_bounds_tiles +
                    count_extent_tiles(b.extent_grid, 1, b.height)
            table.remove(b.extent_grid, 1)
            b.width = b.width - 1
            b.pos.x = b.pos.x + 1
            if b.width < db_entry.min_width then
                out_of_bounds_tiles = out_of_bounds_tiles +
                    count_extent_tiles(b.extent_grid, b.width, b.height)
                clear_building(b)
            end
        end
        trim_empty_cols(b)
        trim_empty_rows(b)
        if not has_area(b) then clear_building(b) end
        if prev_oob ~= out_of_bounds_tiles then
            log('cropping building/stockpile to map boundary, defined in ' ..
                'spreadsheet cells: %s', table.concat(b.cells, ', '))
        end
        ::continue::
    end
    return out_of_bounds_tiles
end

-- check tiles for validity, adjust the extent_grid, and checks the validity of
-- the adjusted extent_grid. marks building as invalid if the extent_grid is
-- invalid.
function check_tiles_and_extents(ctx, buildings)
    local occupied_tiles = 0
    for _, b in ipairs(buildings) do
        if not b.pos then goto continue end
        local db_entry = b.db_entry
        local owns_preview = false
        for extent_x=1,b.width do
            local col = b.extent_grid[extent_x]
            for extent_y=1,b.height do
                local in_extent = col[extent_y]
                if not in_extent then goto continue_inner end
                local pos =
                        xyz2pos(b.pos.x+extent_x-1, b.pos.y+extent_y-1, b.pos.z)
                local is_valid_tile = db_entry.is_valid_tile_fn(pos,db_entry,b)
                owns_preview =
                        quickfort_preview.set_preview_tile(ctx, pos,
                                                           is_valid_tile)
                if not is_valid_tile then
                    log('tile not usable: (%d, %d, %d)', pos.x, pos.y, pos.z)
                    col[extent_y] = false
                    occupied_tiles = occupied_tiles + 1
                end
                ::continue_inner::
            end
        end
        if not db_entry.is_valid_extent_fn(b) then
            log('no room for %s at (%d, %d, %d)',
                db_entry.label, b.pos.x, b.pos.y, b.pos.z)
            if owns_preview then
                for x=b.pos.x,b.pos.x+b.width-1 do
                    for y=b.pos.y,b.pos.y+b.height-1 do
                        local p = xyz2pos(x, y, b.pos.z)
                        quickfort_preview.set_preview_tile(ctx, p, false, true)
                    end
                end
            end
            clear_building(b)
        end
        ::continue::
    end
    return occupied_tiles
end

-- allocate and initialize extents structure from the extents_grid
-- returns extents, num_tiles
-- we assume by this point that the extent is valid and non-empty
function make_extents(b, dry_run)
    local area = b.width * b.height
    local extents = nil
    if not dry_run then
        extents = df.reinterpret_cast(df.building_extents_type,
                                      df.new('uint8_t', area))
    end
    local num_tiles = 0
    for i=1,area do
        local extent_x = (i-1) % b.width + 1
        local extent_y = math.floor((i-1) / b.width) + 1
        local is_in_extent = b.extent_grid[extent_x][extent_y]
        if not dry_run then extents[i-1] = is_in_extent and 1 or 0 end
        if is_in_extent then num_tiles = num_tiles + 1 end
    end
    return extents, num_tiles
end

if dfhack.internal.IN_TEST then
    unit_test_hooks = {
        get_digit_count=get_digit_count,
        left_pad=left_pad,
        dump_seen_grid=dump_seen_grid,
        flood_fill=flood_fill,
        swap_id_and_trim_chunk=swap_id_and_trim_chunk,
        chunk_extents=chunk_extents,
        expand_buildings=expand_buildings,
        build_extent_grid=build_extent_grid,
        init_buildings=init_buildings,
        count_extent_tiles=count_extent_tiles,
        trim_empty_cols=trim_empty_cols,
        trim_empty_rows=trim_empty_rows,
        has_area=has_area,
        clear_building=clear_building,
        crop_to_bounds=crop_to_bounds,
        check_tiles_and_extents=check_tiles_and_extents,
        make_extents=make_extents,
    }
end
