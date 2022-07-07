-- The help text database.
--
-- Command help is read from the the following sources:
-- 1. rendered text in hack/docs/docs/
-- 2. (for scripts) the script sources if no pre-rendered text exists or if the
--    script file has a modification time that is more recent than the
--    pre-rendered text
-- 3. (for plugins) the string passed to the PluginCommand initializer if no
--    pre-rendered text exists
--
-- For plugins that don't register any commands, the plugin name serves as the
-- command so documentation on what happens when you enable the plugin can be
-- found.

local _ENV = mkmodule('helpdb')

local RENDERED_PATH = 'hack/docs/docs/tools/'
local TAG_DEFINITIONS = 'hack/docs/docs/Tags.txt'

local SCRIPT_DOC_BEGIN = '[====['
local SCRIPT_DOC_END = ']====]'

local SOURCES = {
    STUB='stub',
    RENDERED='rendered',
    PLUGIN='plugin',
    SCRIPT='script',
}

-- command name -> {short_help, long_help, tags, source, source_timestamp}
-- also includes a script_source_path element if the source is a script
-- and a unrunnable boolean if the source is a plugin that does not provide any
-- commands to invoke directly.
db = db or {}

-- tag name -> list of command names
tag_index = tag_index or {}

local function get_rendered_path(command)
    return RENDERED_PATH .. command .. '.txt'
end

local function has_rendered_help(command)
    return dfhack.filesystem.mtime(get_rendered_path(command)) ~= -1
end

local DEFAULT_HELP_TEMPLATE = [[
%s
%s

Tags: None

No help available.
]]

local function make_default_entry(command, source)
    local default_long_help = DEFAULT_HELP_TEMPLATE:format(
                                                command, ('*'):rep(#command))
    return {short_help='No help available.', long_help=default_long_help,
            tags={}, source=source, source_timestamp=0}
end

-- updates the short_text, the long_text, and the tags in the given entry
local function update_entry(entry, iterator, opts)
    opts = opts or {}
    local lines = {}
    local begin_marker_found,header_found = not opts.begin_marker,opts.no_header
    local tags_found, short_help_found, in_short_help = false, false, false
    for line in iterator do
        if not short_help_found and opts.first_line_is_short_help then
            local _,_,text = line:trim():find('^%-%-%s*(.*)')
            if not text:endswith('.') then
                text = text .. '.'
            end
            entry.short_help = text
            short_help_found = true
            goto continue
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

local function make_rendered_entry(old_entry, command)
    local rendered_path = get_rendered_path(command)
    local source_timestamp = dfhack.filesystem.mtime(rendered_path)
    if old_entry and old_entry.source == SOURCES.RENDERED and
            old_entry.source_timestamp >= source_timestamp then
        -- we already have the latest info
        return old_entry
    end
    local entry = make_default_entry(command, SOURCES.RENDERED)
    update_entry(entry, io.lines(rendered_path))
    entry.source_timestamp = source_timestamp
    return entry
end

local function make_plugin_entry(old_entry, command)
    if old_entry and old_entry.source == SOURCES.PLUGIN then
        -- we can't tell when a plugin is reloaded, so we can either choose to
        -- always refresh or never refresh. let's go with never for now for
        -- performance.
        return old_entry
    end
    local entry = make_default_entry(command, SOURCES.PLUGIN)
    local long_help = dfhack.internal.getCommandHelp(command)
    if long_help and #long_help:trim() > 0 then
        update_entry(entry, long_help:trim():gmatch('[^\n]*'), {no_header=true})
    end
    return entry
end

local function make_script_entry(old_entry, command, script_source_path)
    local source_timestamp = dfhack.filesystem.mtime(script_source_path)
    if old_entry and old_entry.source == SOURCES.SCRIPT and
            old_entry.script_source_path == script_source_path and
            old_entry.source_timestamp >= source_timestamp then
        -- we already have the latest info
        return old_entry
    end
    local entry = make_default_entry(command, SOURCES.SCRIPT)
    update_entry(entry, io.lines(script_source_path),
                 {begin_marker=SCRIPT_DOC_BEGIN, end_marker=SCRIPT_DOC_END,
                  first_line_is_short_help=true})
    entry.source_timestamp = source_timestamp
    return entry
end

local function update_db(old_db, db, source, command, flags)
    if db[command] then
        -- already in db (e.g. from a higher-priority script dir); skip
        return
    end
    local entry, old_entry = nil, old_db[command]
    if source == SOURCES.RENDERED then
        entry = make_rendered_entry(old_entry, command)
    elseif source == SOURCES.PLUGIN then
        entry = make_plugin_entry(old_entry, command)
    elseif source == SOURCES.SCRIPT then
        entry = make_script_entry(old_entry, command, flags.script_source)
    elseif source == SOURCES.STUB then
        entry = make_default_entry(command, SOURCES.STUB)
    else
        error('unhandled help source: ' .. source)
    end

    entry.unrunnable = (flags or {}).unrunnable

    db[command] = entry
    for _,tag in ipairs(entry.tags) do
        -- unknown tags are ignored
        if tag_index[tag] then
            table.insert(tag_index[tag], command)
        end
    end
end

local function scan_plugins(old_db, db)
    local plugin_names = dfhack.internal.listPlugins()
    for _,plugin in ipairs(plugin_names) do
        local commands = dfhack.internal.listCommands(plugin)
        if #commands == 0 then
            -- use plugin name as the command so we have something to anchor the
            -- documentation to
            update_db(old_db, db,
                      has_rendered_help(plugin) and
                            SOURCES.RENDERED or SOURCES.STUB,
                      plugin, {unrunnable=true})
            goto continue
        end
        for _,command in ipairs(commands) do
            update_db(old_db, db,
                      has_rendered_help(command) and
                            SOURCES.RENDERED or SOURCES.PLUGIN,
                      command)
        end
        ::continue::
    end
end

local function scan_scripts(old_db, db)
    for _,script_path in ipairs(dfhack.internal.getScriptPaths()) do
        local files = dfhack.filesystem.listdir_recursive(
                                            script_path, nil, false)
        if not files then goto skip_path end
        for _,f in ipairs(files) do
            if f.isdir or not f.path:endswith('.lua') or
                    f.path:startswith('test/') or
                    f.path:startswith('internal/') then
                goto continue
            end
            local script_source = script_path .. '/' .. f.path
            local script_is_newer = dfhack.filesystem.mtime(script_source) >
                    dfhack.filesystem.mtime(get_rendered_path(f.path))
            update_db(old_db, db,
                      script_is_newer and SOURCES.SCRIPT or SOURCES.RENDERED,
                      f.path:sub(1, #f.path - 4), {script_source=script_source})
            ::continue::
        end
        ::skip_path::
    end
end

local function initialize_tags()
    local tag, desc, in_desc = nil, nil, false
    for line in io.lines(TAG_DEFINITIONS) do
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
-- anything if it has already been run within the last 10 seconds.
last_refresh_ms = last_refresh_ms or 0
local function ensure_db()
    local now_ms = dfhack.getTickCount()
    if now_ms - last_refresh_ms < 60000 then return end
    last_refresh_ms = now_ms

    local old_db = db
    db, tag_index = {}, {}

    initialize_tags()
    scan_plugins(old_db, db)
    scan_scripts(old_db, db)
end

local function get_db_property(command, property)
    ensure_db()
    if not db[command] then
        error(('command not found: "%s"'):format(command))
    end
    return db[command][property]
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

local function matches(command, filter)
    local db_entry = db[command]
    if filter.runnable and db_entry.unrunnable then
        return false
    end
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
    if filter.str then
        local matched = false
        for _,str in ipairs(filter.str) do
            if command:find(str, 1, true) then
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

local function normalize_string_list(l)
    if not l then return nil end
    if type(l) == 'string' then
        return {l}
    end
    return l
end

local function normalize_filter(f)
    if not f then return nil end
    local filter = {}
    filter.str = normalize_string_list(f.str)
    filter.tag = normalize_string_list(f.tag)
    filter.runnable = f.runnable
    if not filter.str and not filter.tag and not filter.runnable then
        return nil
    end
    return filter
end

-- returns a list of identifiers, alphabetized by their last path component
-- (e.g. gui/autobutcher will immediately follow autobutcher).
-- the optional include and exclude filter params are maps with the following
-- elements:
--   str - if a string, filters by the given substring. if a table of strings,
--         includes commands that match any of the given substrings.
--   tag - if a string, filters by the given tag name. if a table of strings,
--         includes commands that match any of the given tags.
--   runnable - if true, matches only runnable commands, not plugin names.
function get_entries(include, exclude)
    ensure_db()
    include = normalize_filter(include)
    exclude = normalize_filter(exclude)
    local commands = {}
    for command in pairs(db) do
        if (not include or matches(command, include)) and
                (not exclude or not matches(command, exclude)) then
            table.insert(commands, command)
        end
    end
    table.sort(commands, sort_by_basename)
    return commands
end

local function get_max_width(list, min_width)
    local width = min_width or 0
    for _,item in ipairs(list) do
        width = math.max(width, #item)
    end
    return width
end

-- prints the requested entries to the console. include and exclude filters are
-- as in get_entries above.
function list_entries(include_tags, include, exclude)
    local entries = get_entries(include, exclude)
    local width = get_max_width(entries, 10)
    for _,entry in ipairs(entries) do
        print(('  %-'..width..'s %s'):format(
                entry, get_entry_short_help(entry)))
        if include_tags then
            print(('  '..(' '):rep(width)..' tags(%s)'):format(
                    table.concat(get_entry_tags(entry), ',')))
        end
    end
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

-- prints the defined tags and their descriptions to the console
function list_tags()
    local tags = get_tags()
    local width = get_max_width(tags, 10)
    for _,tag in ipairs(tags) do
        print(('  %-'..width..'s %s'):format(tag, get_tag_description(tag)))
    end
end

return _ENV
