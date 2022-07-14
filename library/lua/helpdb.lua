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
local BUILTIN_HELP = 'hack/docs/docs/Builtin.txt'
local TAG_DEFINITIONS = 'hack/docs/docs/Tags.txt'

-- used when reading help text embedded in script sources
local SCRIPT_DOC_BEGIN = '[====['
local SCRIPT_DOC_END = ']====]'
local SCRIPT_DOC_BEGIN_RUBY = '=begin'
local SCRIPT_DOC_END_RUBY = '=end'

local ENTRY_TYPES = {
    BUILTIN='builtin',
    PLUGIN='plugin',
    COMMAND='command'
}

local HELP_SOURCES = {
    STUB='stub',
    RENDERED='rendered',
    PLUGIN='plugin',
    SCRIPT='script',
}

-- entry name -> {
--   entry_types (set of ENTRY_TYPES),
--   short_help (string),
--   long_help (string),
--   tags (set),
--   help_source (element of HELP_SOURCES),
--   source_timestamp (mtime, 0 for non-files),
--   source_path (string, nil for non-files)
-- }
--
-- entry_types is a set because plugin commands can also be the plugin names.
db = db or {}

-- tag name -> list of entry names
-- Tags defined in the TAG_DEFINITIONS file that have no associated db entries
-- will have an empty list.
tag_index = tag_index or {}

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
local function make_default_entry(entry_name, entry_types, source,
                                  source_timestamp, source_path)
    local default_long_help = DEFAULT_HELP_TEMPLATE:format(
                                            entry_name, ('*'):rep(#entry_name))
    return {
        entry_types=entry_types,
        short_help='No help available.',
        long_help=default_long_help,
        tags={},
        help_source=source,
        source_timestamp=source_timestamp or 0,
        source_path=source_path}
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
    local tags_found, short_help_found, in_short_help = false, false, false
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
    entry.long_help = table.concat(lines, '\n')
end

-- create db entry based on parsing sphinx-rendered help text
local function make_rendered_entry(old_entry, entry_name, entry_types)
    local rendered_path = get_rendered_path(entry_name)
    local source_timestamp = dfhack.filesystem.mtime(rendered_path)
    if old_entry and old_entry.source == HELP_SOURCES.RENDERED and
            old_entry.source_timestamp >= source_timestamp then
        -- we already have the latest info
        return old_entry
    end
    local entry = make_default_entry(entry_name, entry_types,
                HELP_SOURCES.RENDERED, source_timestamp, rendered_path)
    update_entry(entry, io.lines(rendered_path))
    return entry
end

-- create db entry based on the help text in the plugin source (used by
-- out-of-tree plugins)
local function make_plugin_entry(old_entry, entry_name, entry_types)
    if old_entry and old_entry.source == HELP_SOURCES.PLUGIN then
        -- we can't tell when a plugin is reloaded, so we can either choose to
        -- always refresh or never refresh. let's go with never for now for
        -- performance.
        return old_entry
    end
    local entry = make_default_entry(entry_name, entry_types,
                                     HELP_SOURCES.PLUGIN)
    local long_help = dfhack.internal.getCommandHelp(entry_name)
    if long_help and #long_help:trim() > 0 then
        update_entry(entry, long_help:trim():gmatch('[^\n]*'), {no_header=true})
    end
    return entry
end

-- create db entry based on the help text in the script source (used by
-- out-of-tree scripts)
local function make_script_entry(old_entry, entry_name, script_source_path)
    local source_timestamp = dfhack.filesystem.mtime(script_source_path)
    if old_entry and old_entry.source == HELP_SOURCES.SCRIPT and
            old_entry.script_source_path == script_source_path and
            old_entry.source_timestamp >= source_timestamp then
        -- we already have the latest info
        return old_entry
    end
    local entry = make_default_entry(entry_name, {[ENTRY_TYPES.COMMAND]=true},
                    HELP_SOURCES.SCRIPT, source_timestamp, script_source_path)
    local ok, lines = pcall(io.lines, script_source_path)
    if not ok then return entry end
    local is_rb = script_source_path:endswith('.rb')
    update_entry(entry, lines,
            {begin_marker=(is_rb and SCRIPT_DOC_BEGIN_RUBY or SCRIPT_DOC_BEGIN),
             end_marker=(is_rb and SCRIPT_DOC_BEGIN_RUBY or SCRIPT_DOC_END),
             first_line_is_short_help=(is_rb and '#' or '%-%-')})
    return entry
end

-- updates the db (and associated tag index) with a new entry if the entry_name
-- doesn't already exist in the db.
local function update_db(old_db, db, source, entry_name, kwargs)
    if db[entry_name] then
        -- already in db (e.g. from a higher-priority script dir); skip
        return
    end
    local entry, old_entry = nil, old_db[entry_name]
    if source == HELP_SOURCES.RENDERED then
        entry = make_rendered_entry(old_entry, entry_name, kwargs.entry_types)
    elseif source == HELP_SOURCES.PLUGIN then
        entry = make_plugin_entry(old_entry, entry_name, kwargs.entry_types)
    elseif source == HELP_SOURCES.SCRIPT then
        entry = make_script_entry(old_entry, entry_name, kwargs.script_source)
    elseif source == HELP_SOURCES.STUB then
        entry = make_default_entry(entry_name, kwargs.entry_types,
                                   HELP_SOURCES.STUB)
    else
        error('unhandled help source: ' .. source)
    end
    db[entry_name] = entry
    for _,tag in ipairs(entry.tags) do
        -- ignore unknown tags
        if tag_index[tag] then
            table.insert(tag_index[tag], entry_name)
        end
    end
end

local BUILTINS = {
    alias='Configure helper aliases for other DFHack commands.',
    cls='Clear the console screen.',
    clear='Clear the console screen.',
    ['devel/dump-rpc']='Write RPC endpoint information to a file.',
    die='Force DF to close immediately, without saving.',
    enable='Enable a plugin or persistent script.',
    disable='Disable a plugin or persistent script.',
    fpause='Force DF to pause.',
    help='Usage help for the given plugin, command, or script.',
    hide='Hide the terminal window (Windows only).',
    keybinding='Modify bindings of commands to in-game key shortcuts.',
    ['kill-lua']='Stop a misbehaving Lua script.',
    ['load']='Load and register a plugin library.',
    unload='Unregister and unload a plugin.',
    reload='Unload and reload a plugin library.',
    ls='List commands, optionally filtered by a tag or substring.',
    dir='List commands, optionally filtered by a tag or substring.',
    plug='List plugins and whether they are enabled.',
    ['sc-script']='Automatically run specified scripts on state change events.',
    script='Run commands specified in a file.',
    show='Show a hidden terminal window (Windows only).',
    tags='List the tags that the DFHack tools are grouped by.',
    ['type']='Discover how a command is implemented.',
}

-- add the builtin commands to the db
local function scan_builtins(old_db, db)
    local entry = make_default_entry('builtin',
                {[ENTRY_TYPES.BUILTIN]=true, [ENTRY_TYPES.COMMAND]=true},
                HELP_SOURCES.RENDERED, 0, BUILTIN_HELP)
    -- read in builtin help
    local f = io.open(BUILTIN_HELP)
    if f then
        entry.long_help = f:read('*all')
    end
    for b,short_help in pairs(BUILTINS) do
        local builtin_entry = copyall(entry)
        builtin_entry.short_help = short_help
        db[b] = builtin_entry
    end
end

-- scan for plugins and plugin-provided commands and add their help to the db
local function scan_plugins(old_db, db)
    local plugin_names = dfhack.internal.listPlugins()
    for _,plugin in ipairs(plugin_names) do
        local commands = dfhack.internal.listCommands(plugin)
        if #commands == 0 then
            -- use plugin name as the command so we have something to anchor the
            -- documentation to
            update_db(old_db, db,
                      has_rendered_help(plugin) and
                            HELP_SOURCES.RENDERED or HELP_SOURCES.STUB,
                      plugin, {entry_types={[ENTRY_TYPES.PLUGIN]=true}})
            goto continue
        end
        for _,command in ipairs(commands) do
            local entry_types = {[ENTRY_TYPES.COMMAND]=true}
            if command == plugin then
                entry_types[ENTRY_TYPES.PLUGIN]=true
            end
            update_db(old_db, db,
                      has_rendered_help(command) and
                            HELP_SOURCES.RENDERED or HELP_SOURCES.PLUGIN,
                      command, {entry_types=entry_types})
        end
        ::continue::
    end
end

-- scan for scripts and add their help to the db
local function scan_scripts(old_db, db)
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
            local script_source = script_path .. '/' .. f.path
            update_db(old_db, db,
                      has_rendered_help(entry_name) and
                            HELP_SOURCES.RENDERED or HELP_SOURCES.SCRIPT,
                      entry_name,
                      {entry_types=entry_types, script_source=script_source})
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

-- ensures the db is up to date by scanning all help sources. does not do
-- anything if it has already been run within the last 60 seconds.
last_refresh_ms = last_refresh_ms or 0
local function ensure_db()
    local now_ms = dfhack.getTickCount()
    if now_ms - last_refresh_ms < 60000 then return end
    last_refresh_ms = now_ms

    local old_db = db
    db, tag_index = {}, {}

    initialize_tags()
    scan_builtins(old_db, db)
    scan_plugins(old_db, db)
    scan_scripts(old_db, db)
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
    return has_keys(str, db)
end

local function get_db_property(entry_name, property)
    ensure_db()
    if not db[entry_name] then
        error(('helpdb entry not found: "%s"'):format(entry_name))
    end
    return db[entry_name][property]
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

-- returns the description associated with the given tag
function get_tag_description(tag)
    ensure_db()
    if not tag_index[tag] then
        error('invalid tag: ' .. tag)
    end
    return tag_index[tag].description
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
    local db_entry = db[entry_name]
    if filter.tag then
        local matched = false
        for _,tag in ipairs(filter.tag) do
            if db_entry.tags[tag] then
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
        for _,etype in ipairs(filter.types) do
            if db_entry.entry_types[etype] then
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
    for entry in pairs(db) do
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
    return db[command] and db[command].entry_types[ENTRY_TYPES.BUILTIN]
end

---------------------------------------------------------------------------
-- print API (outputs to console)
---------------------------------------------------------------------------

-- implements the 'help' builtin command
function help(entry)
    ensure_db()
    if not db[entry] then
        dfhack.printerr(('No help entry found for "%s"'):format(entry))
        return
    end
    print(get_entry_long_help(entry))
end

local function get_max_width(list, min_width)
    local width = min_width or 0
    for _,item in ipairs(list) do
        width = math.max(width, #item)
    end
    return width
end

-- implements the 'tags' builtin command
function tags()
    local tags = get_tags()
    local width = get_max_width(tags, 10)
    for _,tag in ipairs(tags) do
        print(('  %-'..width..'s %s'):format(tag, get_tag_description(tag)))
    end
end

-- prints the requested entries to the console. include and exclude filters are
-- defined as in search_entries() above.
function list_entries(skip_tags, include, exclude)
    local entries = search_entries(include, exclude)
    local width = get_max_width(entries, 10)
    for _,entry in ipairs(entries) do
        print(('  %-'..width..'s  %s'):format(
                entry, get_entry_short_help(entry)))
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
