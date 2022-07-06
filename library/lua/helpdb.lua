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
    local begin_marker_found, header_found = not opts.begin_marker, opts.no_header
    local tags_found, short_help_found, in_short_help = false, false, false
    for line in iterator do
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
        elseif not tags_found and not short_help_found and
                line:find('^Tags: [%w, ]+$') then
            -- tags must appear before the help text begins
            local _,_,tags = line:trim():find('Tags: (.*)')
            entry.tags = tags:split('[ ,]+')
            table.sort(entry.tags)
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
        -- always refresh or never refresh. let's go with never for now.
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
                 {begin_marker=SCRIPT_DOC_BEGIN, end_marker=SCRIPT_DOC_END})
    entry.source_timestamp = source_timestamp
    return entry
end

local function update_db(old_db, db, source, command, script_source_path)
    if db[command] then
        -- already in db (e.g. from a higher-priority script dir); skip update
        return
    end
    local entry, old_entry = nil, old_db[command]
    if source == SOURCES.RENDERED then
        entry = make_rendered_entry(old_entry, command)
    elseif source == SOURCES.PLUGIN then
        entry = make_plugin_entry(old_entry, command)
    elseif source == SOURCES.SCRIPT then
        entry = make_script_entry(old_entry, command, script_source_path)
    elseif source == SOURCES.STUB then
        entry = make_default_entry(command, SOURCES.STUB)
    else
        error('unhandled help source: ' .. source)
    end

    db[command] = entry
    for _,tag in ipairs(entry.tags) do
        table.insert(ensure_key(tag_index, tag), command)
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
                      plugin)
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
                      f.path:sub(1, #f.path - 4), script_source)
            ::continue::
        end
        ::skip_path::
    end
end

-- ensures the db is up to date by scanning all help sources. does not do
-- anything if it has already been run within the last second.
last_refresh_ms = last_refresh_ms or 0
local function ensure_db()
    local now_ms = dfhack.getTickCount()
    if now_ms - last_refresh_ms < 1000 then return end
    last_refresh_ms = now_ms

    local old_db = db
    db, tag_index = {}, {}

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

-- returns the ~54 char summary blurb associated with the command
function get_short_help(command)
    return get_db_property(command, 'short_help')
end

-- returns the full help documentation associated with the command
function get_long_help(command)
    return get_db_property(command, 'long_help')
end

-- returns the list of tags associated with the command
function get_tags(command)
    return get_db_property(command, 'tags')
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

local function add_if_matched(commands, command, strs)
    if strs then
        local matched = false
        for _,str in ipairs(strs) do
            if command:find(str, 1, true) then
                matched = true
                break
            end
        end
        if not matched then
            return
        end
    end
    table.insert(commands, command)
end

-- returns a list of commands, alphabetized by their last path component (e.g.
-- gui/autobutcher will immediately follow autobutcher).
-- the optional filter element is a map with the following elements:
--   str - if a string, filters by the given substring. if a table of strings,
--         includes commands that match any of the given substrings.
--   tag - if a string, filters by the given tag name. if a table of strings,
--         includes commands that match any of the given tags.
function get_commands(filter)
    ensure_db()
    filter = filter or {}
    local commands = {}
    local strs = filter.str
    if filter.str then
        if type(strs) == 'string' then strs = {strs} end
    end
    if not filter.tag then
        for command in pairs(db) do
            add_if_matched(commands, command, strs)
        end
    else
        local command_set = {}
        local tags = filter.tag
        if type(tags) == 'string' then tags = {tags} end
        for _,tag in ipairs(tags) do
            if not tag_index[tag] then
                error('invalid tag: ' .. tag)
            end
            for _,command in ipairs(tag_index[tag]) do
                command_set[command] = true
            end
        end
        for command in pairs(command_set) do
            add_if_matched(commands, command, strs)
        end
    end
    table.sort(commands, sort_by_basename)
    return commands
end

return _ENV
