-- config mode-related logic for the quickfort script
--@ module = true

if not dfhack_flags.module then
    qerror('this script cannot be called directly')
end

local gui = require('gui')
local guidm = require('gui.dwarfmode')
local quickfort_common = reqscript('internal/quickfort/common')
local quickfort_aliases = reqscript('internal/quickfort/aliases')
local quickfort_keycodes = reqscript('internal/quickfort/keycodes')
local quickfort_map = reqscript('internal/quickfort/map')
local quickfort_preview = reqscript('internal/quickfort/preview')
local quickfort_transform = reqscript('internal/quickfort/transform')

local log = quickfort_common.log

local dir_map = {
    Up=quickfort_transform.unit_vectors.north,
    Right=quickfort_transform.unit_vectors.east,
    Down=quickfort_transform.unit_vectors.south,
    Left=quickfort_transform.unit_vectors.west
}
local dir_revmap = {
    [quickfort_transform.unit_vectors_revmap.north]='Up',
    [quickfort_transform.unit_vectors_revmap.east]='Right',
    [quickfort_transform.unit_vectors_revmap.south]='Down',
    [quickfort_transform.unit_vectors_revmap.west]='Left'
}

local function handle_modifiers(token, modifiers)
    local token_lower = token:lower()
    if token_lower == 'shift' or
            token_lower == 'ctrl' or
            token_lower == 'alt' then
        modifiers[token_lower] = true
        return true
    end
    if token_lower == 'wait' then
        -- accepted for compatibility with Python Quickfort, but waiting has no
        -- effect in DFHack quickfort.
        return true
    end
    return false
end

local function transform_token(ctx, token, transformable_dirs)
    -- don't transform if the token is not a direction key or if we're not on
    -- a map screen. note this is only a heuristic. there are dwarfmode screens
    -- that can't move a cursor anyway. there are also dwarfmode screens where
    -- the arrow keys change settings instead of move a cursor. however, these
    -- screens (e.g. the hospital zone settings screen) are very unlikely to be
    -- visited by a query or config blueprint. we can make this heuristic more
    -- complicated if the above statement is proven incorrect.
    if not transformable_dirs[token] or
            not dfhack.gui.getCurFocus(true):startswith('dwarfmode') then
        return token
    end
    local translated_dir = quickfort_transform.resolve_transformed_vector(
            ctx, dir_map[token], dir_revmap)
    if token ~= translated_dir then
        log(('transforming cursor movement on map screen: %s -> %s')
            :format(token, translated_dir))
    end
    return translated_dir
end

function do_query_config_blueprint(zlevel, grid, ctx, sidebar_mode,
                                   pre_tile_fn, post_tile_fn, post_blueprint_fn)
    local stats = ctx.stats
    stats.query_config_keystrokes = stats.query_config_keystrokes or
            {label='Keystrokes sent', value=0, always=true}

    quickfort_keycodes.init_keycodes()
    local alias_ctx = quickfort_aliases.init_alias_ctx(ctx)

    local dry_run = ctx.dry_run
    local saved_mode = df.global.plotinfo.main.mode
    if not dry_run and saved_mode ~= sidebar_mode then
        guidm.enterSidebarMode(sidebar_mode)
    end

    -- record which direction keys are potentially transformed so we only
    -- look up whether we need to transform when we absolutely have to
    local transformable_dirs = {}
    for k,v in pairs(dir_map) do
        if k ~= quickfort_transform.resolve_transformed_vector(ctx, v,
                                                               dir_revmap) then
            transformable_dirs[k] = true
        end
    end

    for y, row in pairs(grid) do
        for x, cell_and_text in pairs(row) do
            local tile_ctx = {pos=xyz2pos(x, y, zlevel)}
            tile_ctx.cell,tile_ctx.text = cell_and_text.cell,cell_and_text.text
            local is_valid_tile = not pre_tile_fn or pre_tile_fn(ctx, tile_ctx)
            quickfort_preview.set_preview_tile(ctx, tile_ctx.pos, is_valid_tile)
            if not is_valid_tile then goto continue end
            local modifiers = {} -- tracks ctrl, shift, and alt modifiers
            local tokens = quickfort_aliases.expand_aliases(alias_ctx,
                                                            tile_ctx.text)
            for _,token in ipairs(tokens) do
                if handle_modifiers(token, modifiers) then goto continue2 end
                token = transform_token(ctx, token, transformable_dirs)
                local kcodes = quickfort_keycodes.get_keycodes(token, modifiers)
                if not kcodes then
                    qerror(string.format(
                            'unknown alias or keycode: "%s"', token))
                end
                if not dry_run then
                    gui.simulateInput(dfhack.gui.getCurViewscreen(true), kcodes)
                end
                modifiers = {}
                stats.query_config_keystrokes.value =
                        stats.query_config_keystrokes.value + 1
                ::continue2::
            end
            if post_tile_fn then post_tile_fn(ctx, tile_ctx) end
            ::continue::
        end
    end

    if not dry_run then
        if saved_mode ~= sidebar_mode
                    and guidm.SIDEBAR_MODE_KEYS[saved_mode] then
            guidm.enterSidebarMode(saved_mode)
        end
        if post_blueprint_fn then post_blueprint_fn(ctx) end
    end
end

local function config_pre_tile_fn(ctx, tile_ctx)
    log('applying spreadsheet cell %s with text "%s"',
        tile_ctx.cell, tile_ctx.text)
    return true
end

local function config_post_tile_fn(ctx, tile_ctx)
    if ctx.dry_run then return end
    if df.global.plotinfo.main.mode ~= df.ui_sidebar_mode.Default then
        qerror(string.format(
            'expected to be at map screen, but we seem to be in mode "%s"; ' ..
            'there is likely a problem with the blueprint text in ' ..
            'cell %s: "%s" (do you need a "^" at the end to get back to the ' ..
            'main map?)',
            df.ui_sidebar_mode[df.global.plotinfo.main.mode],
            tile_ctx.cell, tile_ctx.text))
    end
end

function do_run(zlevel, grid, ctx)
    do_query_config_blueprint(zlevel, grid, ctx, df.ui_sidebar_mode.Default,
                              config_pre_tile_fn, config_post_tile_fn)
end

function do_orders()
    log('nothing to do for blueprints in mode: config')
end

function do_undo()
    log('cannot undo blueprints for mode: config')
end
