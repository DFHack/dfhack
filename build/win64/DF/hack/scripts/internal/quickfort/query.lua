-- query mode-related logic for the quickfort script
--@ module = true

if not dfhack_flags.module then
    qerror('this script cannot be called directly')
end

local guidm = require('gui.dwarfmode')
local utils = require('utils')
local quickfort_common = reqscript('internal/quickfort/common')
local quickfort_config = reqscript('internal/quickfort/config')
local quickfort_map = reqscript('internal/quickfort/map')
local quickfort_set = reqscript('internal/quickfort/set')

local log = quickfort_common.log

local function is_queryable_tile(pos)
    local flags, occupancy = dfhack.maps.getTileFlags(pos)
    if not flags then return false end
    return not flags.hidden and
        (occupancy.building ~= 0 or
         dfhack.buildings.findCivzonesAt(pos))
end

local function query_pre_tile_fn(ctx, tile_ctx)
    local pos = tile_ctx.pos
    if not quickfort_set.get_setting('query_unsafe') and
            not is_queryable_tile(pos) then
        if not ctx.quiet then
            dfhack.printerr(string.format(
                    'no building at coordinates (%d, %d, %d); skipping ' ..
                    'text in spreadsheet cell %s: "%s"',
                    pos.x, pos.y, pos.z, tile_ctx.cell, tile_ctx.text))
        end
        ctx.stats.query_skipped_tiles.value =
                ctx.stats.query_skipped_tiles.value + 1
        return false
    end
    if not ctx.dry_run then
        quickfort_map.move_cursor(pos)
        tile_ctx.focus_string = dfhack.gui.getCurFocus(true)
    end
    log('applying spreadsheet cell %s with text "%s" to map ' ..
        'coordinates (%d, %d, %d)',
        tile_ctx.cell, tile_ctx.text, pos.x, pos.y, pos.z)
    return true
end

-- If a tile starts or ends with one of these focus strings, the start and end
-- focus strings can differ without us flagging it as an error.
local exempt_focus_strings = utils.invert({
    'dwarfmode/QueryBuilding/Destroying',
    })

local function query_post_tile_fn(ctx, tile_ctx)
    ctx.stats.query_tiles.value = ctx.stats.query_tiles.value + 1
    if ctx.dry_run or quickfort_set.get_setting('query_unsafe') then
        return
    end
    local pos, focus_string = tile_ctx.pos, tile_ctx.focus_string
    local cursor = guidm.getCursorPos()
    if not cursor then
        qerror(string.format(
            'expected to be at cursor position (%d, %d, %d) on ' ..
            'screen "%s" but there is no active cursor; there ' ..
            'is likely a problem with the blueprint text in ' ..
            'cell %s: "%s" (do you need a "q" at the end to get ' ..
            'back into query mode?)',
            pos.x, pos.y, pos.z, focus_string, tile_ctx.cell, tile_ctx.text))
    elseif not same_xyz(pos, cursor) then
        qerror(string.format(
            'expected to be at cursor position (%d, %d, %d) on ' ..
            'screen "%s" but cursor is at (%d, %d, %d); there ' ..
            'is likely a problem with the blueprint text in ' ..
            'cell %s: "%s"', pos.x, pos.y, pos.z, focus_string,
            cursor.x, cursor.y, cursor.z, tile_ctx.cell, tile_ctx.text))
    end
    local new_focus_string = dfhack.gui.getCurFocus(true)
    local is_exempt = exempt_focus_strings[focus_string] or
            exempt_focus_strings[new_focus_string]
    if not is_exempt and focus_string ~= new_focus_string then
        qerror(string.format(
            'expected to be at cursor position (%d, %d, %d) on ' ..
            'screen "%s" but screen is "%s"; there is likely a ' ..
            'problem with the blueprint text in cell %s: "%s" ' ..
            '(do you need a "^" at the end to escape back to ' ..
            'the map screen?)', pos.x, pos.y, pos.z, focus_string,
            new_focus_string, tile_ctx.cell, tile_ctx.text))
    end
end

local function query_post_blueprint_fn(ctx)
    quickfort_map.move_cursor(ctx.cursor)
end

function do_run(zlevel, grid, ctx)
    local stats = ctx.stats
    stats.query_tiles = stats.query_tiles or {label='Tiles configured', value=0}
    stats.query_skipped_tiles = stats.query_skipped_tiles
            or {label='Tiles not configured due to missing buildings', value=0}

    quickfort_config.do_query_config_blueprint(zlevel, grid, ctx,
                                               df.ui_sidebar_mode.QueryBuilding,
                                               query_pre_tile_fn,
                                               query_post_tile_fn,
                                               query_post_blueprint_fn)
end

function do_orders()
    log('nothing to do for blueprints in mode: query')
end

function do_undo()
    log('cannot undo blueprints for mode: query')
end
