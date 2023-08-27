-- preview data structure management for the quickfort script
--@ module = true

if not dfhack_flags.module then
    qerror('this script cannot be called directly')
end

-- sets the given preview position to the given value if it is not already set
-- if override is true, ignores whether the tile was previously set
-- returns whether the tile was set
function set_preview_tile(ctx, pos, is_valid_tile, override)
    local preview = ctx.preview
    if not preview then return false end
    local preview_row = ensure_key(ensure_key(ctx.preview.tiles, pos.z), pos.y)
    if preview_row[pos.x] == nil then
        preview.total_tiles = preview.total_tiles + 1
    end
    if not override and preview_row[pos.x] ~= nil then
        return false
    end
    if not is_valid_tile and preview_row[pos.x] ~= is_valid_tile then
        preview.invalid_tiles = preview.invalid_tiles + 1
    end
    preview_row[pos.x] = is_valid_tile
    local bounds = ensure_key(preview.bounds, pos.z,
                              {x1=pos.x, x2=pos.x, y1=pos.y, y2=pos.y})
    bounds.x1 = math.min(bounds.x1, pos.x)
    bounds.x2 = math.max(bounds.x2, pos.x)
    bounds.y1 = math.min(bounds.y1, pos.y)
    bounds.y2 = math.max(bounds.y2, pos.y)
    return true
end

-- returns true if the tile is on the blueprint and will be correctly applied
-- returns false if the tile is on the blueprint and is an invalid tile
-- returns nil if the tile is not on the blueprint
function get_preview_tile(tiles, pos)
    return safe_index(tiles, pos.z, pos.y, pos.x)
end
