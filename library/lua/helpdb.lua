-- The help text database and query interface.
--
-- Help text is read from the rendered text in hack/docs/docs/. If no rendered
-- text exists, it is read from the script sources (for scripts) or the string
-- passed to the PluginCommand initializer (for plugins).
--
-- For plugins that don't register a command with the same name as the plugin,
-- the plugin name is registered as a separate entry so documentation on what
-- happens when you enable the plugin can be found.
--
-- The database is lazy-loaded when an API method is called. It rechecks its
-- help sources for updates if an API method has not been called in the last
-- 60 seconds.

local _ENV = mkmodule('helpdb')

-- paths
local RENDERED_PATH = 'hack/docs/docs/tools/'
local TAG_DEFINITIONS = 'hack/docs/docs/Tags.txt'

-- used when reading help text embedded in script sources
local SCRIPT_DOC_BEGIN = '[====['
local SCRIPT_DOC_END = ']====]'
local SCRIPT_DOC_BEGIN_RUBY = '=begin'
local SCRIPT_DOC_END_RUBY = '=end'

-- enums
local ENTRY_TYPES = {
    BUILTIN='builtin',
    PLUGIN='plugin',
    COMMAND='command'
}

local HELP_SOURCES = {
    RENDERED='rendered', -- from the installed, rendered help text
    PLUGIN='plugin',     -- from the plugin source code
    SCRIPT='script',     -- from the script source code
    STUB='stub',         -- from a generated stub
}

-- builtin command names, with aliases mapped to their canonical form
local BUILTINS = {
    ['?']='help',
    alias=true,
    clear='cls',
    cls=true,
    ['devel/dump-rpc']=true,
    die=true,
    dir='ls',
    disable=true,
    enable=true,
    fpause=true,
    help=true,
    hide=true,
    keybinding=true,
    ['kill-lua']=true,
    ['load']=true,
    ls=true,
    man='help',
    plug=true,
    reload=true,
    script=true,
    ['sc-script']=true,
    show=true,
    tags=true,
    ['type']=true,
    unload=true,
}

---------------------------------------------------------------------------
-- data structures
---------------------------------------------------------------------------

-- help text database, keys are a subset of the entry database
-- entry name -> {
--   help_source (element of HELP_SOURCES),
--   short_help (string),
--   long_help (string),
--   tags (set),
--   source_timestamp (mtime, 0 for non-files),
--   source_path (string, nil for non-files)
-- }
textdb = textdb or {}

-- entry database, points to text in textdb
-- entry name -> {
--   entry_types (set of ENTRY_TYPES),
--   short_help (string, if not nil then overrides short_help in text_entry),
--   text_entry (string)
-- }
--
-- entry_types is a set because plugin commands can also be the plugin names.
entrydb = entrydb or {}


-- tag name -> list of entry names
-- Tags defined in the TAG_DEFINITIONS file that have no associated db entries
-- will have an empty list.
tag_index = tag_index or {}

---------------------------------------------------------------------------
-- data ingestion
---------------------------------------------------------------------------

local function get_rendered_path(entry_name)
    return RENDERED_PATH .. entry_name .. '.txt'
end

local function has_rendered_help(entry_name)
    return dfhack.filesystem.mtime(get_rendered_path(entry_name)) ~= -1
end

local DEFAULT_HELP_TEMPLATE = [[
%s
%s

No help available.
]]
local function make_default_entry(entry_name, help_source, kwargs)
    local default_long_help = DEFAULT_HELP_TEMPLATE:format(
                                            entry_name, ('*'):rep(#entry_name))
    return {
        help_source=help_source,
        short_help='No help available.',
        long_help=default_long_help,
        tags={},
        source_timestamp=kwargs.source_timestamp or 0,
        source_path=kwargs.source_path}
end

-- updates the short_text, the long_text, and the tags in the given entry based
-- on the text returned from the iterator.
-- if defined, opts can have the following fields:
--   begin_marker (string that marks the beginning of the help text; all text
--                 before this marker is ignored)
--   end_marker (string that marks the end of the help text; text will stop
--               being parsed after this marker is seen)
--   no_header (don't try to find the entity name at the top of the help text)
--   first_line_is_short_help (if set, then read the short help text from the
--                             first commented line of the script instead of
--                             using the first sentence of the long help text.
--                             value is the comment character.)
local function update_entry(entry, iterator, opts)
    opts = opts or {}
    local lines = {}
    local first_line_is_short_help = opts.first_line_is_short_help
    local begin_marker_found,header_found = not opts.begin_marker,opts.no_header
    local tags_found, short_help_found = false, opts.skip_short_help
    local in_short_help = false
    for line in iterator do
        if not short_help_found and first_line_is_short_help then
            line = line:trim()
            local _,_,text = line:find('^'..first_line_is_short_help..'%s*(.*)')
            if not text then
                -- if no first-line short help found, fall back to getting the
                -- first sentence of the help text.
                first_line_is_short_help = false
            else
                if not text:endswith('.') then
                    text = text .. '.'
                end
                entry.short_help = text
                short_help_found = true
                goto continue
            end
        end
        if not begin_marker_found then
            local _, endpos = line:find(opts.begin_marker, 1, true)
            if endpos == #line then
                begin_marker_found = true
            end
            goto continue
        end
        if opts.end_marker then
            local _, endpos = line:find(opts.end_marker, 1, true)
            if endpos == #line then
                break
            end
        end
        if not header_found and line:find('%w') then
            header_found = true
        elseif not tags_found and line:find('^Tags: [%w, ]+$') then
            local _,_,tags = line:trim():find('Tags: (.*)')
            entry.tags = {}
            for _,tag in ipairs(tags:split('[ ,]+')) do
                entry.tags[tag] = true
            end
            tags_found = true
        elseif not short_help_found and not line:find('^Keybinding:') and
                line:find('%w') then
            if in_short_help then
                entry.short_help = entry.short_help .. ' ' .. line
            else
                entry.short_help = line
            end
            local sentence_end = entry.short_help:find('.', 1, true)
            if sentence_end then
                entry.short_help = entry.short_help:sub(1, sentence_end)
                short_help_found = true
            else
                in_short_help = true
            end
        end
        table.insert(lines, line)
        ::continue::
    end
    if #lines > 0 then
        entry.long_help = table.concat(lines, '\n')
    end
end

-- create db entry based on parsing sphinx-rendered help text
local function make_rendered_entry(old_entry, entry_name, kwargs)
    local source_path = get_rendered_path(entry_name)
    local source_timestamp = dfhack.filesystem.mtime(source_path)
    if old_entry and old_entry.help_source == HELP_SOURCES.RENDERED and
            old_entry.source_timestamp >= source_timestamp then
        -- we already have the latest info
        return old_entry
    end
    kwargs.source_path, kwargs.source_timestamp = source_path, source_timestamp
    local entry = make_default_entry(entry_name, HELP_SOURCES.RENDERED, kwargs)
    update_entry(entry, io.lines(source_path))
    return entry
end

-- create db entry based on the help text in the plugin source (used by
-- out-of-tree plugins)
local function make_plugin_entry(old_entry, entry_name, kwargs)
    if old_entry and old_entry.source == HELP_SOURCES.PLUGIN then
        -- we can't tell when a plugin is reloaded, so we can either choose to
        -- always refresh or never refresh. let's go with never for now for
        -- performance.
        return old_entry
    end
    local entry = make_default_entry(entry_name, HELP_SOURCES.PLUGIN, kwargs)
    local long_help = dfhack.internal.getCommandHelp(entry_name)
    if long_help and #long_help:trim() > 0 then
        update_entry(entry, long_help:trim():gmatch('[^\n]*'), {no_header=true})
    end
    return entry
end

-- create db entry based on the help text in the script source (used by
-- out-of-tree scripts)
local function make_script_entry(old_entry, entry_name, kwargs)
    local source_path = kwargs.source_path
    local source_timestamp = dfhack.filesystem.mtime(source_path)
    if old_entry and old_entry.source == HELP_SOURCES.SCRIPT and
            old_entry.source_path == source_path and
            old_entry.source_timestamp >= source_timestamp then
        -- we already have the latest info
        return old_entry
    end
    kwargs.source_timestamp, kwargs.entry_type = source_timestamp
    local entry = make_default_entry(entry_name, HELP_SOURCES.SCRIPT, kwargs)
    local ok, lines = pcall(io.lines, source_path)
    if not ok then
        return entry
    end
    local is_rb = source_path:endswith('.rb')
    update_entry(entry, lines,
            {begin_marker=(is_rb and SCRIPT_DOC_BEGIN_RUBY or SCRIPT_DOC_BEGIN),
             end_marker=(is_rb and SCRIPT_DOC_BEGIN_RUBY or SCRIPT_DOC_END),
             first_line_is_short_help=(is_rb and '#' or '%-%-')})
    return entry
end

-- updates the dbs (and associated tag index) with a new entry if the entry_name
-- doesn't already exist in the dbs.
local function update_db(old_db, entry_name, text_entry, help_source, kwargs)
    if entrydb[entry_name] then
        -- already in db (e.g. from a higher-priority script dir); skip
        return
    end
    entrydb[entry_name] = {
        entry_types=kwargs.entry_types,
        short_help=kwargs.short_help,
        text_entry=text_entry
    }
    if entry_name ~= text_entry then
        return
    end

    local text_entry, old_entry = nil, old_db[entry_name]
    if help_source == HELP_SOURCES.RENDERED then
        text_entry = make_rendered_entry(old_entry, entry_name, kwargs)
    elseif help_source == HELP_SOURCES.PLUGIN then
        text_entry = make_plugin_entry(old_entry, entry_name, kwargs)
    elseif help_source == HELP_SOURCES.SCRIPT then
        text_entry = make_script_entry(old_entry, entry_name, kwargs)
    elseif help_source == HELP_SOURCES.STUB then
        text_entry = make_default_entry(entry_name, HELP_SOURCES.STUB, kwargs)
    else
        error('unhandled help source: ' .. help_source)
    end
    textdb[entry_name] = text_entry
end

-- add the builtin commands to the db
local function scan_builtins(old_db)
    local entry_types = {[ENTRY_TYPES.BUILTIN]=true, [ENTRY_TYPES.COMMAND]=true}
    for builtin,canonical in pairs(BUILTINS) do
        if canonical == true then canonical = builtin end
        update_db(old_db, builtin, canonical,
            has_rendered_help(canonical) and
                HELP_SOURCES.RENDERED or HELP_SOURCES.STUB,
            {entry_types=entry_types})
    end
end

-- scan for enableable plugins and plugin-provided commands and add their help
-- to the db
local function scan_plugins(old_db)
    local plugin_names = dfhack.internal.listPlugins()
    for _,plugin in ipairs(plugin_names) do
        local commands = dfhack.internal.listCommands(plugin)
        local includes_plugin = false
        for _,command in ipairs(commands) do
            local kwargs = {entry_types={[ENTRY_TYPES.COMMAND]=true}}
            if command == plugin then
                kwargs.entry_types[ENTRY_TYPES.PLUGIN]=true
                includes_plugin = true
            end
            kwargs.short_help = dfhack.internal.getCommandDescription(command)
            update_db(old_db, command, plugin,
                      has_rendered_help(plugin) and
                            HELP_SOURCES.RENDERED or HELP_SOURCES.PLUGIN,
                      kwargs)
        end
        if not includes_plugin then
            update_db(old_db, plugin, plugin,
                      has_rendered_help(plugin) and
                            HELP_SOURCES.RENDERED or HELP_SOURCES.STUB,
                      {entry_types={[ENTRY_TYPES.PLUGIN]=true}})
        end
    end
end

-- scan for scripts and add their help to the db
local function scan_scripts(old_db)
    local entry_types = {[ENTRY_TYPES.COMMAND]=true}
    for _,script_path in ipairs(dfhack.internal.getScriptPaths()) do
        local files = dfhack.filesystem.listdir_recursive(
                                            script_path, nil, false)
        if not files then goto skip_path end
        for _,f in ipairs(files) do
            if f.isdir or
                (not f.path:endswith('.lua') and not f.path:endswith('.rb')) or
                    f.path:startswith('test/') or
                    f.path:startswith('internal/') then
                goto continue
            end
            local dot_index = f.path:find('%.[^.]*$')
            local entry_name = f.path:sub(1, dot_index - 1)
            local source_path = script_path .. '/' .. f.path
            update_db(old_db, entry_name, entry_name,
                      has_rendered_help(entry_name) and
                            HELP_SOURCES.RENDERED or HELP_SOURCES.SCRIPT,
                      {entry_types=entry_types, source_path=source_path})
            ::continue::
        end
        ::skip_path::
    end
end

-- read tags and descriptions from the TAG_DEFINITIONS file and add them all
-- to tag_index, initizlizing each entry with an empty list.
local function initialize_tags()
    local tag, desc, in_desc = nil, nil, false
    local ok, lines = pcall(io.lines, TAG_DEFINITIONS)
    if not ok then return end
    for line in lines do
        if in_desc then
            line = line:trim()
            if #line == 0 then
                in_desc = false
                goto continue
            end
            desc = desc .. ' ' .. line
            tag_index[tag].description = desc
        else
            _,_,tag,desc = line:find('^%* (%w+): (.+)')
            if not tag then goto continue end
            tag_index[tag] = {description=desc}
            in_desc = true
        end
        ::continue::
    end
end

local function index_tags()
    for entry_name,entry in pairs(entrydb) do
        for tag in pairs(textdb[entry.text_entry].tags) do
            -- ignore unknown tags
            if tag_index[tag] then
                table.insert(tag_index[tag], entry_name)
            end
        end
    end
end

-- ensures the db is up to date by scanning all help sources. does not do
-- anything if it has already been run within the last 60 seconds.
last_refresh_ms = last_refresh_ms or 0
local function ensure_db()
    local now_ms = dfhack.getTickCount()
    if now_ms - last_refresh_ms < 60000 then return end
    last_refresh_ms = now_ms

    local old_db = textdb
    textdb, entrydb, tag_index = {}, {}, {}

    initialize_tags()
    scan_builtins(old_db)
    scan_plugins(old_db)
    scan_scripts(old_db)
    index_tags()
end

---------------------------------------------------------------------------
-- get API
---------------------------------------------------------------------------

-- converts strings into single-element lists containing that string
local function normalize_string_list(l)
    if not l or #l == 0 then return nil end
    if type(l) == 'string' then
        return {l}
    end
    return l
end

local function has_keys(str, dict)
    if not str or #str == 0 then
        return false
    end
    ensure_db()
    for _,s in ipairs(normalize_string_list(str)) do
        if not dict[s] then
            return false
        end
    end
    return true
end

-- returns whether the given string (or list of strings) is an entry in the db
function is_entry(str)
    return has_keys(str, entrydb)
end

local function get_db_property(entry_name, property)
    ensure_db()
    if not entrydb[entry_name] then
        error(('helpdb entry not found: "%s"'):format(entry_name))
    end
    return entrydb[entry_name][property] or
            textdb[entrydb[entry_name].text_entry][property]
end

-- returns the ~54 char summary blurb associated with the entry
function get_entry_short_help(entry)
    return get_db_property(entry, 'short_help')
end

-- returns the full help documentation associated with the entry
function get_entry_long_help(entry)
    return get_db_property(entry, 'long_help')
end

local function set_to_sorted_list(set)
    local list = {}
    for item in pairs(set) do
        table.insert(list, item)
    end
    table.sort(list)
    return list
end

-- returns the list of tags associated with the entry, in alphabetical order
function get_entry_tags(entry)
    return set_to_sorted_list(get_db_property(entry, 'tags'))
end

-- returns whether the given string (or list of strings) matches a tag name
function is_tag(str)
    return has_keys(str, tag_index)
end

-- returns the defined tags in alphabetical order
function get_tags()
    ensure_db()
    return set_to_sorted_list(tag_index)
end

function get_tag_data(tag)
    ensure_db()
    if not tag_index[tag] then
        dfhack.printerr('invalid tag: ' .. tag)
        return {}
    end
    return tag_index[tag]
end

---------------------------------------------------------------------------
-- search API
---------------------------------------------------------------------------

-- returns a list of path elements in reverse order
local function chunk_for_sorting(str)
    local parts = str:split('/')
    local chunks = {}
    for i=1,#parts do
        chunks[#parts - i + 1] = parts[i]
    end
    return chunks
end

-- sorts by last path component, then by parent path components.
-- something comes before nothing.
-- e.g. gui/autofarm comes immediately before autofarm
local function sort_by_basename(a, b)
    local a = chunk_for_sorting(a)
    local b = chunk_for_sorting(b)
    local i = 1
    while a[i] do
        if not b[i] then
            return true
        end
        if a[i] ~= b[i] then
            return a[i] < b[i]
        end
        i = i + 1
    end
    return false
end

local function matches(entry_name, filter)
    if filter.tag then
        local matched = false
        local tags = get_db_property(entry_name, 'tags')
        for _,tag in ipairs(filter.tag) do
            if tags[tag] then
                matched = true
                break
            end
        end
        if not matched then
            return false
        end
    end
    if filter.types then
        local matched = false
        local etypes = get_db_property(entry_name, 'entry_types')
        for _,etype in ipairs(filter.types) do
            if etypes[etype] then
                matched = true
                break
            end
        end
        if not matched then
            return false
        end
    end
    if filter.str then
        local matched = false
        for _,str in ipairs(filter.str) do
            if entry_name:find(str, 1, true) then
                matched = true
                break
            end
        end
        if not matched then
            return false
        end
    end
    return true
end

-- normalizes the lists in the filter and returns nil if no filter elements are
-- populated
local function normalize_filter(f)
    if not f then return nil end
    local filter = {}
    filter.str = normalize_string_list(f.str)
    filter.tag = normalize_string_list(f.tag)
    filter.types = normalize_string_list(f.types)
    if not filter.str and not filter.tag and not filter.types then
        return nil
    end
    return filter
end

-- returns a list of entry names, alphabetized by their last path component,
-- with populated path components coming before null path components (e.g.
-- autobutcher will immediately follow gui/autobutcher).
-- the optional include and exclude filter params are maps with the following
-- elements:
--   str - if a string, filters by the given substring. if a table of strings,
--         includes entry names that match any of the given substrings.
--   tag - if a string, filters by the given tag name. if a table of strings,
--         includes entries that match any of the given tags.
--   types - if a string, matches entries of the given type. if a table of
--           strings, includes entries that match any of the given types. valid
--           types are: "builtin", "plugin", "command". note that many plugin
--           commands have the same name as the plugin, so those entries will
--           match both "plugin" and "command" types.
function search_entries(include, exclude)
    ensure_db()
    include = normalize_filter(include)
    exclude = normalize_filter(exclude)
    local entries = {}
    for entry in pairs(entrydb) do
        if (not include or matches(entry, include)) and
                (not exclude or not matches(entry, exclude)) then
            table.insert(entries, entry)
        end
    end
    table.sort(entries, sort_by_basename)
    return entries
end

-- returns a list of all commands. used by Core's autocomplete functionality.
function get_commands()
    local include = {types={ENTRY_TYPES.COMMAND}}
    return search_entries(include)
end

function is_builtin(command)
    ensure_db()
    return entrydb[command] and
            get_db_property(command, 'entry_types')[ENTRY_TYPES.BUILTIN]
end

---------------------------------------------------------------------------
-- print API (outputs to console)
---------------------------------------------------------------------------

-- implements the 'help' builtin command
function help(entry)
    ensure_db()
    if not entrydb[entry] then
        dfhack.printerr(('No help entry found for "%s"'):format(entry))
        return
    end
    print(get_entry_long_help(entry))
end

-- prints col1 (width 20), a 2 space gap, and col2 (width 58)
-- if col1text is longer than 20 characters, col2text is printed on the next
-- line. if col2text is longer than 58 characters, it is wrapped.
local COL1WIDTH, COL2WIDTH = 20, 58
local function print_columns(col1text, col2text)
    col2text = col2text:wrap(COL2WIDTH)
    local wrapped_col2 = {}
    for line in col2text:gmatch('[^'..NEWLINE..']*') do
        table.insert(wrapped_col2, line)
    end
    if #col1text > COL1WIDTH then
        print(col1text)
    else
        print(('%-'..COL1WIDTH..'s  %s'):format(col1text, wrapped_col2[1]))
    end
    for i=2,#wrapped_col2 do
        print(('%'..COL1WIDTH..'s  %s'):format(' ', wrapped_col2[i]))
    end
end

-- implements the 'tags' builtin command
function tags()
    local tags = get_tags()
    for _,tag in ipairs(tags) do
        print_columns(tag, get_tag_data(tag).description)
    end
end

-- prints the requested entries to the console. include and exclude filters are
-- defined as in search_entries() above.
function list_entries(skip_tags, include, exclude)
    local entries = search_entries(include, exclude)
    for _,entry in ipairs(entries) do
        print_columns(entry, get_entry_short_help(entry))
        if not skip_tags then
            local tags = get_entry_tags(entry)
            if #tags > 0 then
                print(('    tags: %s'):format(table.concat(tags, ', ')))
            end
        end
    end
    if #entries == 0 then
        print('No matches.')
    end
end

-- wraps the list_entries() API to provide a more convenient interface for Core
-- to implement the 'ls' builtin command.
--   filter_str - if a tag name, will filter by that tag. otherwise, will filter
--                as a substring
--   skip_tags - whether to skip printing tag info
--   show_dev_commands - if true, will include scripts in the modtools/ and
--                       devel/ directories. otherwise those scripts will be
--                       excluded
function ls(filter_str, skip_tags, show_dev_commands)
    local include = {types={ENTRY_TYPES.COMMAND}}
    if is_tag(filter_str) then
        include.tag = filter_str
    else
        include.str = filter_str
    end
    list_entries(skip_tags, include,
                 show_dev_commands and {} or {str={'modtools/', 'devel/'}})
end

return _ENV
