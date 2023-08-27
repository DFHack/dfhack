-- command sequencing and routing logic for the quickfort script
--@ module = true

if not dfhack_flags.module then
    qerror('this script cannot be called directly')
end

local argparse = require('argparse')
local guidm = require('gui.dwarfmode')
local quickfort_common = reqscript('internal/quickfort/common')
local quickfort_list = reqscript('internal/quickfort/list')
local quickfort_orders = reqscript('internal/quickfort/orders')
local quickfort_parse = reqscript('internal/quickfort/parse')
local utils = require('utils')

local mode_modules = {}
for mode, _ in pairs(quickfort_parse.valid_modes) do
    if mode ~= 'ignore' and mode ~= 'aliases' then
        mode_modules[mode] = reqscript('internal/quickfort/'..mode)
    end
end

local command_switch = {
    run='do_run',
    orders='do_orders',
    undo='do_undo',
}

local default_transform_fn = function(pos) return pos end

-- returns map of values that start the same for all contexts
local function make_ctx_base(prev_ctx)
    prev_ctx = prev_ctx or {
        order_specs={},
        stats={out_of_bounds={label='Tiles outside map boundary', value=0},
               invalid_keys={label='Invalid key sequences', value=0}},
               messages={},
               messages_set={},
    }
    return {
        zmin=30000,
        zmax=0,
        transform_fn=default_transform_fn,
        order_specs=prev_ctx.order_specs,
        stats=prev_ctx.stats,
        messages=prev_ctx.messages,
        messages_set=prev_ctx.messages_set,
    }
end

local function make_ctx(prev_ctx, command, blueprint_name, cursor, aliases, quiet,
                        dry_run, preview, preserve_engravings)
    local ctx = make_ctx_base(prev_ctx)
    local params = {
        command=command,
        blueprint_name=blueprint_name,
        cursor=cursor,
        aliases=aliases,
        quiet=quiet,
        dry_run=dry_run,
        preview=preview,
        preserve_engravings=preserve_engravings,
    }

    return utils.assign(ctx, params)
end

-- see make_ctx() above for which params can be specified
function init_ctx(params, prev_ctx)
    if not params.command or not command_switch[params.command] then
        error(('invalid command: "%s"'):format(params.command))
    end
    if not params.blueprint_name or params.blueprint_name == '' then
        error('must specify blueprint_name')
    end
    if not params.cursor then
        error('must specify cursor')
    end

    return make_ctx(
        prev_ctx,
        params.command,
        params.blueprint_name,
        copyall(params.cursor),  -- copy since we modify this during processing
        params.aliases or {},
        params.quiet,
        params.dry_run,
        params.preview and
                {tiles={}, bounds={}, invalid_tiles=0, total_tiles=0} or nil,
        params.preserve_engravings or df.item_quality.Masterful)
end

function do_command_raw(mode, zlevel, grid, ctx)
    -- this error checking is done here again because this function can be
    -- called directly by the quickfort API
    if not mode or not mode_modules[mode] then
        error(string.format('invalid mode: "%s"', mode))
    end

    ctx.cursor.z = zlevel
    ctx.zmin, ctx.zmax = math.min(ctx.zmin, zlevel), math.max(ctx.zmax, zlevel)
    mode_modules[mode][command_switch[ctx.command]](zlevel, grid, ctx)
end

local function make_transform_fn(prev_transform_fn, modifiers, cursor)
    if #modifiers.transform_fn_stack == 0 and
            #modifiers.shift_fn_stack == 0 then
        return prev_transform_fn
    end
    -- when no_shift is true, we transform around the origin instead of the
    -- cursor. this is a convenient way to transform expansion syntax elements
    -- so they are still valid after the cell itself is transformed around the
    -- cursor. e.g. T(5x2) becomes T(-2,5) after a clockwise rotation.
    -- no_shift is also useful for transforming unit vectors around the origin
    -- so we can figure out where the cardinal directions got transformed to.
    local origin = xy2pos(0, 0)
    return function(pos, no_shift)
        for _,tfn in ipairs(modifiers.transform_fn_stack) do
            pos = tfn(pos, no_shift and origin or cursor)
        end
        if not no_shift then
            for _,sfn in ipairs(modifiers.shift_fn_stack) do
                pos = sfn(pos)
            end
        end
        return prev_transform_fn(pos, no_shift)
    end
end

local function do_apply_modifiers(filepath, sheet_name, label, ctx, modifiers)
    local first_modeline = nil
    local saved_zmin, saved_zmax = ctx.zmin, ctx.zmax
    if modifiers.repeat_count > 1 then
        -- scope min and max tracking to the closest repeat modifier so we can
        -- figure out which level to jump to when repeating up or down
        ctx.zmin, ctx.zmax = 30000, 0
    end
    local prev_transform_fn = ctx.transform_fn
    ctx.transform_fn = make_transform_fn(prev_transform_fn, modifiers,
                                         ctx.cursor)
    for i=1,modifiers.repeat_count do
        local section_data_list = quickfort_parse.process_section(
                filepath, sheet_name, label, ctx.cursor, ctx.transform_fn)
        for _, section_data in ipairs(section_data_list) do
            if not first_modeline then first_modeline = section_data.modeline end
            do_command_raw(section_data.modeline.mode, section_data.zlevel,
                           section_data.grid, ctx)
        end
        if modifiers.repeat_zoff > 0 then
            ctx.cursor.z = ctx.zmax + modifiers.repeat_zoff
        else
            ctx.cursor.z = ctx.zmin + modifiers.repeat_zoff
        end
    end
    if modifiers.repeat_count > 1 then
        ctx.zmin = math.min(ctx.zmin, saved_zmin)
        ctx.zmax = math.max(ctx.zmax, saved_zmax)
    end
    ctx.transform_fn = prev_transform_fn
    return first_modeline
end

function do_command_section(ctx, section_name, modifiers)
    modifiers = utils.assign(quickfort_parse.get_modifiers_defaults(),
                             modifiers or {})
    local sheet_name, label = quickfort_parse.parse_section_name(section_name)
    ctx.sheet_name = sheet_name
    local filepath = quickfort_list.get_blueprint_filepath(ctx.blueprint_name)
    local first_modeline =
            do_apply_modifiers(filepath, sheet_name, label, ctx, modifiers)
    if first_modeline and first_modeline.message and ctx.command == 'run'
        and not ctx.messages_set[first_modeline.message]
    then
        table.insert(ctx.messages, first_modeline.message)
        ctx.messages_set[first_modeline.message] = true
    end
end

function finish_commands(ctx)
    quickfort_orders.create_orders(ctx)
    for _,message in ipairs(ctx.messages) do
        print('* '..message)
    end
    if not ctx.quiet then
        print('Blueprint statistics:')
        for _,stat in pairs(ctx.stats) do
            if stat.always or stat.value > 0 then
                print(('  %s: %d'):format(stat.label, stat.value))
            end
        end
    end
end

local function do_one_command(prev_ctx, command, cursor, blueprint_name, section_name,
                              mode, quiet, dry_run, preserve_engravings,
                              modifiers)
    if not cursor then
        if command == 'orders' or mode == 'notes' then
            cursor = {x=0, y=0, z=0}
        else
            qerror('please position the keyboard cursor at the blueprint start ' ..
                   'location or use the --cursor option')
        end
    end

    local ctx = init_ctx({
        command=command,
        blueprint_name=blueprint_name,
        cursor=cursor,
        aliases=quickfort_list.get_aliases(blueprint_name),
        quiet=quiet,
        dry_run=dry_run,
        preserve_engravings=preserve_engravings}, prev_ctx)

    do_command_section(ctx, section_name, modifiers)
    if not ctx.quiet then
        print(('%s successfully completed'):format(
        quickfort_parse.format_command(ctx.command, ctx.blueprint_name,
                                       section_name, ctx.dry_run)))
    end
    return ctx
end

local function do_bp_name(commands, cursor, bp_name, sec_names, quiet, dry_run,
                          preserve_engravings, modifiers)
    local ctx
    for _,sec_name in ipairs(sec_names) do
        local mode = quickfort_list.get_blueprint_mode(bp_name, sec_name)
        for _,command in ipairs(commands) do
            ctx = do_one_command(ctx, command, cursor, bp_name, sec_name, mode, quiet,
                           dry_run, preserve_engravings, modifiers)
        end
    end
    return ctx
end

local function do_list_num(commands, cursor, list_nums, quiet, dry_run,
                           preserve_engravings, modifiers)
    local ctx
    for _,list_num in ipairs(list_nums) do
        local bp_name, sec_name, mode =
                quickfort_list.get_blueprint_by_number(list_num)
        for _,command in ipairs(commands) do
            ctx = do_one_command(ctx,  command, cursor, bp_name, sec_name, mode, quiet,
                           dry_run, preserve_engravings, modifiers)
        end
    end
    return ctx
end

function do_command(args)
    for _,command in ipairs(args.commands) do
        if not command or not command_switch[command] then
            qerror(string.format('invalid command: "%s"', command))
        end
    end
    local cursor = guidm.getCursorPos()
    local quiet, verbose, dry_run, section_names = false, false, false, {''}
    local preserve_engravings = df.item_quality.Masterful
    local modifiers = quickfort_parse.get_modifiers_defaults()
    local other_args = argparse.processArgsGetopt(args, {
            {'c', 'cursor', hasArg=true,
             handler=function(optarg) cursor = argparse.coords(optarg) end},
            {nil, 'preserve-engravings', hasArg=true,
             handler=function(optarg)
                preserve_engravings = quickfort_parse.parse_preserve_engravings(
                                                                optarg) end},
            {'d', 'dry-run', handler=function() dry_run = true end},
            {'n', 'name', hasArg=true,
             handler=function(optarg)
                section_names = argparse.stringList(optarg) end},
            {'q', 'quiet', handler=function() quiet = true end},
            {'r', 'repeat', hasArg=true,
             handler=function(optarg)
                quickfort_parse.parse_repeat_params(optarg, modifiers) end},
            {'s', 'shift', hasArg=true,
             handler=function(optarg)
                quickfort_parse.parse_shift_params(optarg, modifiers) end},
            {'t', 'transform', hasArg=true,
             handler=function(optarg)
                quickfort_parse.parse_transform_params(optarg, modifiers) end},
            {'v', 'verbose', handler=function() verbose = true end},
        })
    local blueprint_name = other_args[1]
    if not blueprint_name or blueprint_name == '' then
        qerror('expected <list_num>[,<list_num>...] or <blueprint_name>')
    end
    if #other_args > 1 then
        local extra = other_args[2]
        qerror(('unexpected argument: "%s"; did you mean "-n %s"?')
               :format(extra, extra))
    end

    quickfort_common.verbose = verbose
    dfhack.with_finalize(
        function() quickfort_common.verbose = false end,
        function()
            local ok, list_nums = pcall(argparse.numberList, blueprint_name)
            local ctx
            if not ok then
                ctx = do_bp_name(args.commands, cursor, blueprint_name, section_names,
                        quiet, dry_run, preserve_engravings, modifiers)
            else
                ctx = do_list_num(args.commands, cursor, list_nums, quiet, dry_run,
                        preserve_engravings, modifiers)
            end
            finish_commands(ctx)
        end)
end

if dfhack.internal.IN_TEST then
    unit_test_hooks = {
        make_ctx_base=make_ctx_base,
        init_ctx=init_ctx,
        do_command_raw=do_command_raw,
        do_command=do_command,
    }
end
