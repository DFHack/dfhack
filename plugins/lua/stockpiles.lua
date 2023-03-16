local _ENV = mkmodule('plugins.stockpiles')

local argparse = require('argparse')

local STOCKPILES_DIR = "dfhack-config/stockpiles";
local STOCKPILES_LIBRARY_DIR = "hack/data/stockpiles";

local function get_sp_name(name, num)
    if #name > 0 then return name end
    return ('Stockpile %d'):format(num)
end

local STATUS_FMT = '%6s %s'
local function print_status()
    local sps = df.global.world.buildings.other.STOCKPILE
    print(('Current stockpiles: %d'):format(#sps))
    if #sps > 0 then
        print()
        print(STATUS_FMT:format('ID', 'Name'))
        print(STATUS_FMT:format('------', '----------'))
    end
    for _,sp in ipairs(sps) do
        print(STATUS_FMT:format(sp.id, get_sp_name(sp.name, sp.stockpile_number)))
    end
end

local function list_dir(path, prefix, filters)
    local paths = dfhack.filesystem.listdir_recursive(path, 0, false)
    if not paths then
        dfhack.printerr(('Cannot find stockpile settings directory: "%s"'):format(path))
        return
    end
    local normalized_filters = {}
    for _,filter in ipairs(filters or {}) do
        table.insert(normalized_filters, filter:lower())
    end
    for _,v in ipairs(paths) do
        local normalized_path = prefix .. v.path:lower()
        if v.isdir or not normalized_path:endswith('.dfstock') then goto continue end
        normalized_path = normalized_path:sub(1, -9)
        if #normalized_filters > 0 then
            local matched = false
            for _,filter in ipairs(normalized_filters) do
                if normalized_path:find(filter, 1, true) then
                    matched = true
                    break
                end
            end
            if not matched then goto continue end
        end
        print(('%s%s'):format(prefix, v.path:sub(1, -9)))
        ::continue::
    end
end

local function list_settings_files(filters)
    list_dir(STOCKPILES_DIR, '', filters)
    list_dir(STOCKPILES_LIBRARY_DIR, 'library/', filters)
end

local function assert_safe_name(name)
    if not name or #name == 0 then
        qerror('name missing or empty')
    end
    if name:find('[^%a ._-]') then
        qerror('name can only contain numbers, letters, periods, underscores, dashes, and spaces')
    end
end

local function get_sp_id(opts)
    if opts.id then return opts.id end
    local sp = dfhack.gui.getSelectedStockpile()
    if sp then return sp.id end
    return nil
end

local function export_stockpile(name, opts)
    assert_safe_name(name)
    name = STOCKPILES_DIR .. '/' .. name
    stockpiles_export(name, get_sp_id(opts))
end

local function import_stockpile(name, opts)
    local is_library = false
    if name:startswith('library/') then
        name = name:sub(9)
        is_library = true
    end
    assert_safe_name(name)
    if not is_library and dfhack.filesystem.exists(STOCKPILES_DIR .. '/' .. name .. '.dfstock') then
        name = STOCKPILES_DIR .. '/' .. name
    else
        name = STOCKPILES_LIBRARY_DIR .. '/' .. name
    end
    stockpiles_import(name, get_sp_id(opts))
end

local function process_args(opts, args)
    if args[1] == 'help' then
        opts.help = true
        return
    end

    return argparse.processArgsGetopt(args, {
            {'h', 'help', handler=function() opts.help = true end},
            {'s', 'stockpile', has_arg=true,
             handler=function(arg) opts.id = argparse.nonnegativeInt(art, 'stockpile') end},
        })
end

function parse_commandline(args)
    local opts = {}
    local positionals = process_args(opts, args)

    if opts.help or not positionals then
        return false
    end

    local command = table.remove(positionals, 1)
    if not command or command == 'status' then
        print_status()
    elseif command == 'list' then
        list_settings_files(positionals)
    elseif command == 'export' then
        export_stockpile(positionals[1], opts)
    elseif command == 'import' then
        import_stockpile(positionals[1], opts)
    else
        return false
    end

    return true
end

return _ENV
