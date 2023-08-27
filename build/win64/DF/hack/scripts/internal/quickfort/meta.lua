-- meta-blueprint logic for the quickfort script
--@ module = true

if not dfhack_flags.module then
    qerror('this script cannot be called directly')
end

local quickfort_command = reqscript('internal/quickfort/command')
local quickfort_common = reqscript('internal/quickfort/common')
local quickfort_parse = reqscript('internal/quickfort/parse')

local log = quickfort_common.log

-- blueprints referenced by meta blueprints must have a label, even if it's just
-- a default numeric label. this is so we can provide intelligent error messages
-- instead of an infinite loop when the blueprint author forgets a slash and
-- a string is interpreted as a sheet name instead of a label (which is ignored
-- for .csv files), the label defaults to "1", and the meta blueprint is the
-- first blueprint in the file.
-- the sheet name is only populated if cur_sheet_name is populated. This ensures
-- the lables are in ".csv format" even if they were fully specified for source
-- .xlsx files, but those .xlsx files have since been serialized to .csv.
local function get_section_name(cell, text, cur_sheet_name)
    local sheet_name, label, extra = quickfort_parse.parse_section_name(text)
    if not label then
        local cell_msg = string.format(
            'malformed blueprint section name in cell %s', cell)
        local suggestion = string.format('/%s', text:gsub('/*$', ''))
        if cur_sheet_name then
            suggestion = string.format('%s or %s/somelabel',
                                       suggestion, text:gsub('/*$', ''))
        end
        local label_msg = string.format(
                '(labels are required; did you mean "%s"?)', suggestion)
        qerror(string.format('%s %s: %s', cell_msg, label_msg, text))
    end
    if not sheet_name then sheet_name = cur_sheet_name or '' end
    if not cur_sheet_name then sheet_name = '' end
    return string.format('%s/%s', sheet_name, label), extra
end

local function do_meta(zlevel, grid, ctx)
    local stats = ctx.stats
    stats.meta_blueprints = stats.meta_blueprints or
            {label='Blueprints applied', value=0, always=true}

    ensure_keys(ctx, 'meta', 'parents') -- context history for infinite loop protection

    -- use get_ordered_grid_cells() to ensure we process blueprints in exactly
    -- the declared order (pairs() over the grid makes no such guarantee)
    for _, cell in ipairs(quickfort_parse.get_ordered_grid_cells(grid)) do
        local section_name, extra =
                get_section_name(cell.cell, cell.text, ctx.sheet_name)
        if ctx.meta.parents[section_name] then
            qerror(('infinite loop detected in blueprint: "%s"'):format(section_name))
        end
        local modifiers =
                quickfort_parse.get_meta_modifiers(extra, ctx.blueprint_name)
        local repeat_str = ''
        if modifiers.repeat_count > 1 then
            repeat_str = (' (%d times %sward)')
                    :format(modifiers.repeat_count,
                            (modifiers.repeat_zoff > 0) and 'up' or 'down')
        end
        log('applying blueprint%s: "%s"', repeat_str, section_name)
        ctx.meta.parents[section_name] = true
        local ok, err = pcall(quickfort_command.do_command_section,
                ctx, section_name, modifiers)
        ctx.meta.parents[section_name] = nil
        if ok then
            stats.meta_blueprints.value =
                    stats.meta_blueprints.value + modifiers.repeat_count
        else
            dfhack.printerr(err)
        end
        -- don't let called blueprints permanently alter the z-level
        ctx.cursor.z = zlevel
    end
end

do_run = do_meta
do_orders = do_meta
do_undo = do_meta
