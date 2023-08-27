-- list-related logic for the quickfort script
--@ module = true

if not dfhack_flags.module then
    qerror('this script cannot be called directly')
end

local scriptmanager = require('script-manager')
local utils = require('utils')
local xlsxreader = require('plugins.xlsxreader')
local quickfort_parse = reqscript('internal/quickfort/parse')
local quickfort_set = reqscript('internal/quickfort/set')

blueprint_dirs = blueprint_dirs or nil

local function get_blueprint_dirs()
    if blueprint_dirs then return blueprint_dirs end
    blueprint_dirs = {}
    for _,v in ipairs(scriptmanager.get_mod_paths('blueprints')) do
        blueprint_dirs[v.id] = v.path
    end
    return blueprint_dirs
end

function get_blueprint_filepath(blueprint_name)
    local fullpath = ('%s/%s'):format(
        quickfort_set.get_setting('blueprints_user_dir'),
        blueprint_name)
    if dfhack.filesystem.exists(fullpath) then
        return fullpath
    end
    local dirmap = get_blueprint_dirs()
    local _, _, prefix = blueprint_name:find('^([^/]+)/')
    if not prefix then
        return fullpath
    end
    blueprint_name = blueprint_name:sub(#prefix + 2)
    if prefix == 'library' then
        return ('%s/%s'):format(
            quickfort_set.get_setting('blueprints_library_dir'),
            blueprint_name)
    end
    if not dirmap[prefix] then
        return fullpath
    end
    return ('%s/%s'):format(dirmap[prefix], blueprint_name)
end

local blueprint_cache = {}

local function scan_csv_blueprint(path)
    local filepath = get_blueprint_filepath(path)
    local mtime = dfhack.filesystem.mtime(filepath)
    if not blueprint_cache[path] or blueprint_cache[path].mtime ~= mtime then
        local modelines, aliases = quickfort_parse.get_metadata(filepath)
        blueprint_cache[path] =
                {modelines=modelines, aliases=aliases, mtime=mtime}
    end
    if #blueprint_cache[path].modelines == 0 then
        print(string.format('skipping "%s": empty file', path))
    end
    return blueprint_cache[path].modelines, blueprint_cache[path].aliases
end

local function get_xlsx_file_sheet_infos(filepath)
    local sheet_infos = {aliases={}}
    local xlsx_file = xlsxreader.open_xlsx_file(filepath)
    if not xlsx_file then return sheet_infos end
    return dfhack.with_finalize(
        function() xlsxreader.close_xlsx_file(xlsx_file) end,
        function()
            for _, sheet_name in ipairs(xlsxreader.list_sheets(xlsx_file)) do
                local modelines, aliases =
                        quickfort_parse.get_metadata(filepath, sheet_name)
                utils.assign(sheet_infos.aliases, aliases)
                if #modelines > 0 then
                    local metadata = {name=sheet_name, modelines=modelines}
                    table.insert(sheet_infos, metadata)
                end
            end
            return sheet_infos
        end
    )
end

local function scan_xlsx_blueprint(path)
    local filepath = get_blueprint_filepath(path)
    local mtime = dfhack.filesystem.mtime(filepath)
    if blueprint_cache[path] and blueprint_cache[path].mtime == mtime then
        return blueprint_cache[path].sheet_infos
    end
    local sheet_infos = get_xlsx_file_sheet_infos(filepath)
    if #sheet_infos == 0 then
        print(string.format('skipping "%s": no sheet with data detected', path))
    end
    blueprint_cache[path] = {sheet_infos=sheet_infos, mtime=mtime}
    return sheet_infos
end

local function get_section_name(sheet_name, label)
    if not sheet_name and not (label and label ~= "1") then return nil end
    local sheet_name_str, label_str = '', ''
    if sheet_name then sheet_name_str = sheet_name end
    if label and label ~= "1" then label_str = '/' .. label end
    return string.format('%s%s', sheet_name_str, label_str)
end

-- normalize paths on windows
local function normalize_path(path)
    return path:gsub(package.config:sub(1,1), "/")
end

local function make_blueprint_modes_key(path, section_name)
    return normalize_path(path) .. '//' .. (section_name or '')
end

local blueprints, blueprint_modes, file_scope_aliases = {}, {}, {}
local num_library_blueprints = 0

local function scan_blueprint_dir(bp_dir, library_prefix)
    local paths = dfhack.filesystem.listdir_recursive(bp_dir, nil, false)
    if not paths then
        dfhack.printerr(('Cannot find blueprints directory: "%s"'):format(bp_dir))
        return
    end
    local is_library = library_prefix and #library_prefix > 0
    for _, v in ipairs(paths) do
        local file_aliases = {}
        local path = (library_prefix or '') .. v.path
        if not v.isdir and v.path:lower():endswith('.csv') then
            local modelines, aliases = scan_csv_blueprint(path)
            file_aliases = aliases
            local first = true
            for _,modeline in ipairs(modelines) do
                table.insert(blueprints,
                        {path=path, modeline=modeline, is_library=is_library})
                if is_library then num_library_blueprints = num_library_blueprints + 1 end
                local key = make_blueprint_modes_key(
                        path, get_section_name(nil, modeline.label))
                blueprint_modes[key] = modeline.mode
                if first then
                    -- first blueprint is also accessible via blank name
                    key = make_blueprint_modes_key(path)
                    blueprint_modes[key] = modeline.mode
                    first = false
                end
            end
        elseif not v.isdir and v.path:lower():endswith('.xlsx') then
            local sheet_infos = scan_xlsx_blueprint(path)
            file_aliases = sheet_infos.aliases
            local first = true
            if #sheet_infos > 0 then
                for _,sheet_info in ipairs(sheet_infos) do
                    local sheet_first = true
                    for _,modeline in ipairs(sheet_info.modelines) do
                        table.insert(blueprints,
                                     {path=path,
                                      sheet_name=sheet_info.name,
                                      modeline=modeline,
                                      is_library=is_library})
                        if is_library then num_library_blueprints = num_library_blueprints + 1 end
                        local key = make_blueprint_modes_key(
                            path,
                            get_section_name(sheet_info.name, modeline.label))
                        blueprint_modes[key] = modeline.mode
                        if first then
                            -- first blueprint in first sheet is also accessible
                            -- via blank name
                            key = make_blueprint_modes_key(path)
                            blueprint_modes[key] = modeline.mode
                            first = false
                        end
                        if sheet_first then
                            -- first blueprint in each sheet is also accessible
                            -- via blank label
                            key = make_blueprint_modes_key(
                                    path, get_section_name(sheet_info.name))
                            blueprint_modes[key] = modeline.mode
                            sheet_first = false
                        end
                    end
                end
            end
        end
        file_scope_aliases[normalize_path(path)] = file_aliases
    end
end

local function scan_blueprints()
    blueprints, blueprint_modes, file_scope_aliases = {}, {}, {}
    num_library_blueprints = 0
    scan_blueprint_dir(quickfort_set.get_setting('blueprints_user_dir'))
    scan_blueprint_dir(quickfort_set.get_setting('blueprints_library_dir'), 'library/')
    for id,path in pairs(get_blueprint_dirs()) do
        if id ~= 'library' then
            scan_blueprint_dir(path, id..'/')
        end
    end
end

function get_blueprint_by_number(list_num)
    scan_blueprints()
    list_num = tonumber(list_num)
    local blueprint = blueprints[list_num]
    if not blueprint then
        qerror(string.format('invalid list index: "%s"', tostring(list_num)))
    end
    local section_name =
            get_section_name(blueprint.sheet_name, blueprint.modeline.label)
    return blueprint.path, section_name, blueprint.modeline.mode
end

function get_blueprint_mode(path, section_name)
    scan_blueprints()
    return blueprint_modes[make_blueprint_modes_key(path, section_name)]
end

-- returns the aliases that are scoped to the given file
function get_aliases(path)
    scan_blueprints()
    return file_scope_aliases[normalize_path(path)]
end

-- returns a sequence of structured data to display. note that the id may not
-- be equal to the list index due to holes left by hidden blueprints.
function do_list_internal(show_library, show_hidden)
    scan_blueprints()
    local display_list = {}
    for i,v in ipairs(blueprints) do
        if not show_library and v.is_library then goto continue end
        if not show_hidden and v.modeline.hidden then goto continue end
        local display_data = {
            id=i,
            path=v.path,
            mode=v.modeline.mode,
            section_name=get_section_name(v.sheet_name, v.modeline.label),
            start_comment=v.modeline.start_comment,
            comment=v.modeline.comment,
        }
        local search_key = ''
        for _,v in pairs(display_data) do
            if v then
                -- order doesn't matter; we just need all the strings in there
                search_key = ('%s %s'):format(search_key, v)
            end
        end
        display_data.search_key = search_key
        table.insert(display_list, display_data)
        ::continue::
    end
    return display_list
end

function do_list(args)
    local show_library, show_hidden, filter_mode = true, false, nil
    local filter_strings = utils.processArgsGetopt(args, {
            {'u', 'useronly', handler=function() show_library = false end},
            {'h', 'hidden', handler=function() show_hidden = true end},
            {'m', 'mode', hasArg=true,
             handler=function(optarg) filter_mode = optarg end},
        })
    if filter_mode and not quickfort_parse.valid_modes[filter_mode] then
        qerror(string.format('invalid mode: "%s"', filter_mode))
    end
    local list = do_list_internal(show_library, show_hidden)
    local num_filtered = 0
    for _,v in ipairs(list) do
        if filter_mode and v.mode ~= filter_mode then
            num_filtered = num_filtered + 1
            goto continue
        end
        for _,filter_string in ipairs(filter_strings) do
            if not string.find(v.search_key, filter_string) then
                num_filtered = num_filtered + 1
                goto continue
            end
        end
        local sheet_spec = ''
        if v.section_name then
            sheet_spec = string.format(
                    ' -n %s',
                    quickfort_parse.quote_if_has_spaces(v.section_name))
        end
        local comment = ')'
        if v.comment then comment = string.format(': %s)', v.comment) end
        local start_comment = ''
        if v.start_comment then
            start_comment = string.format('; cursor start: %s', v.start_comment)
        end
        print(string.format(
                '%d) %s%s (%s%s%s', v.id,
                quickfort_parse.quote_if_has_spaces(v.path),
                sheet_spec, v.mode, comment, start_comment))
        ::continue::
    end
    if num_filtered > 0 then
        print(string.format('  %d blueprints did not match filter',
                            num_filtered))
    end
    if num_library_blueprints > 0 and not show_library then
        print(('  %d library blueprints not shown')
              :format(num_library_blueprints))
    end
end
