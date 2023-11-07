--config.target = 'core'

local h = require('helpdb')

local mock_plugin_db = {
    hascommands={
        boxbinders={description="Box your binders.", help=[[Box your binders.
        This command will help you box your binders.]]},
        bindboxers={description="Bind your boxers.", help=[[Bind your boxers.
        This command will help you bind your boxers.]]}
    },
    samename={
        samename={description="Samename.", help=[[Samename.
        This command has the same name as its host plugin.]]}
    },
    nocommand={},
    nodocs_hascommands={
        nodoc_command={description="cpp description.", help=[[cpp description.
        Rest of help.]]}
    },
    nodocs_samename={
        nodocs_samename={description="Nodocs samename.", help=[[Nodocs samename.
        This command has the same name as its host plugin but no rst docs.]]}
    },
    nodocs_nocommand={},
}

local mock_command_db = {}
for k,v in pairs(mock_plugin_db) do
    for c,d in pairs(v) do
        mock_command_db[c] = d
    end
end

local mock_script_db = {
    basic=true,
    ['subdir/scriptname']=true,
    inscript_docs=true,
    inscript_short_only=true,
    nodocs_script=true,
    dev_script=true,
}

local files = {
    ['hack/docs/docs/Tags.txt']=[[
* fort: Tools that are useful while in fort mode.

* armok: Tools that give you complete control over an aspect of the game or provide access to information that the game intentionally keeps hidden.

* map: Tools that interact with the game map.

* units: Tools that interact with units.

* dev: Dev tools.

* nomembers: Nothing is tagged with this.
    ]],
    ['hack/docs/docs/tools/hascommands.txt']=[[
hascommands
===========

Tags: fort | armok | units

  Documented a plugin that has commands.

Command: "boxbinders"

  Documented boxbinders.

Command: "bindboxers"

  Documented bindboxers.

Documented full help.
    ]],
    ['hack/docs/docs/tools/samename.txt']=[[
samename
========

Tags: fort | armok | units

Command: "samename"

  Documented samename.

Documented full help.
    ]],
    ['hack/docs/docs/tools/nocommand.txt']=[[
nocommand
=========

Tags: fort | armok | units

  Documented nocommand.

Documented full help.
    ]],
    ['hack/docs/docs/tools/basic.txt']=[[
basic
=====

Tags: map

Command: "basic"

  Documented basic.

Documented full help.
    ]],
    ['hack/docs/docs/tools/subdir/scriptname.txt']=[[
subdir/scriptname
=================

Tags: map

Command: "subdir/scriptname"

  Documented subdir/scriptname.

Documented full help.
    ]],
    ['hack/docs/docs/tools/dev_script.txt']=[[
dev_script
==========

Tags: dev

Command: "dev_script"

  Short desc.

Full help.
]====]
script contents
    ]],
    ['scripts/scriptpath/basic.lua']=[[
-- in-file short description for basic
-- [====[
basic
=====

Tags: map

Command: "basic"

  in-file basic.

Documented full help.
]====]
script contents
    ]],
    ['scripts/scriptpath/subdir/scriptname.lua']=[[
-- in-file short description for scriptname
-- [====[
subdir/scriptname
=================

Tags: map

Command: "subdir/scriptname"

  in-file scriptname.

Documented full help.
]====]
script contents
    ]],
    ['scripts/scriptpath/inscript_docs.lua']=[[
-- in-file short description for inscript_docs
-- [====[
inscript_docs
=============

Tags: map | badtag

Command: "inscript_docs"

  in-file inscript_docs.

Documented full help.
]====]
script contents
    ]],
    ['scripts/scriptpath/nodocs_script.lua']=[[
    script contents
    ]],
    ['scripts/scriptpath/inscript_short_only.lua']=[[
    -- inscript short desc.

    script contents
    ]],
    ['other/scriptpath/basic.lua']=[[
-- in-file short description for basic (other)
-- [====[
basic
=====

Tags: map

Command: "basic"

  in-file basic (other).

Documented full help.
]====]
script contents
    ]],
    ['other/scriptpath/subdir/scriptname.lua']=[[
-- in-file short description for scriptname (other)
-- [====[
subdir/scriptname
=================

Tags: map

Command: "subdir/scriptname"

  in-file scriptname (other).

Documented full help.
]====]
script contents
    ]],
    ['other/scriptpath/inscript_docs.lua']=[[
-- in-file short description for inscript_docs (other)
-- [====[
inscript_docs
=============

Tags: map

Command: "inscript_docs"

  in-file inscript_docs (other).

Documented full help.
]====]
script contents
    ]],
    ['other/scriptpath/dev_script.lua']=[[
script contents
    ]],
}

local function mock_getCommandHelp(command)
    if mock_command_db[command] then
        return mock_command_db[command].help
    end
end

local function mock_listPlugins()
    local list = {}
    for k in pairs(mock_plugin_db) do
        table.insert(list, k)
    end
    return list
end

local function mock_listCommands(plugin)
    local list = {}
    for k,v in pairs(mock_plugin_db) do
        if k == plugin then
            for c in pairs(v) do
                table.insert(list, c)
            end
            break
        end
    end
    return list
end

local function mock_getCommandDescription(command)
    if mock_command_db[command] then
        return mock_command_db[command].description
    end
end

local function mock_getScriptPaths()
    return {'scripts/scriptpath', 'other/scriptpath'}
end

local function mock_mtime(path)
    if files[path] then return 1 end
    return -1
end

local function mock_listdir_recursive(script_path)
    local list = {}
    for s in pairs(mock_script_db) do
        table.insert(list, {isdir=false, path=s..'.lua'})
    end
    return list
end

local function mock_getTickCount()
    return 100000
end

local function mock_pcall(fn, fname)
    if fn ~= io.lines then error('unexpected fn for pcall') end
    if not files[fname] then
        return false
    end
    return true, files[fname]:gmatch('([^\n]*)\n?')
end

config.wrapper = function(test_fn)
    mock.patch({
        {h.dfhack.internal, 'getCommandHelp', mock_getCommandHelp},
        {h.dfhack.internal, 'listPlugins', mock_listPlugins},
        {h.dfhack.internal, 'listCommands', mock_listCommands},
        {h.dfhack.internal, 'getCommandDescription', mock_getCommandDescription},
        {h.dfhack.internal, 'getScriptPaths', mock_getScriptPaths},
        {h.dfhack.filesystem, 'mtime', mock_mtime},
        {h.dfhack.filesystem, 'listdir_recursive', mock_listdir_recursive},
        {h.dfhack, 'getTickCount', mock_getTickCount},
        {h, 'pcall', mock_pcall},
    }, test_fn)
end

function test.is_entry()
    expect.true_(h.is_entry('ls'),
        'builtin commands get an entry')
    expect.true_(h.is_entry('hascommands'),
        'plugins whose names do not match their commands get an entry')
    expect.true_(h.is_entry('boxbinders'),
        'commands whose name does not match the host plugin get an entry')
    expect.true_(h.is_entry('samename'),
        'plugins that have a command with the same name get one entry')
    expect.true_(h.is_entry('nocommand'),
        'plugins that do not have commands get an entry')
    expect.true_(h.is_entry('basic'),
        'scripts in the script path get an entry')
    expect.true_(h.is_entry('subdir/scriptname'),
        'scripts in subdirs of a script path get an entry')

    expect.true_(h.is_entry('nodocs_hascommands'),
        'plugins whose names do not match their commands get an entry')
    expect.true_(h.is_entry('nodoc_command'),
        'commands whose name does not match the host plugin get an entry')
    expect.true_(h.is_entry('nodocs_samename'),
        'plugins that have a command with the same name get one entry')
    expect.true_(h.is_entry('nodocs_nocommand'),
        'plugins that do not have commands get an entry')
    expect.true_(h.is_entry('inscript_docs'),
        'scripts in the script path get an entry')
    expect.true_(h.is_entry('nodocs_script'),
        'scripts in the script path get an entry')
    expect.true_(h.is_entry('inscript_short_only'),
        'scripts in the script path get an entry')

    expect.false_(h.is_entry(nil),
        'nil is not an entry')
    expect.false_(h.is_entry(''),
        'blank is not an entry')
    expect.false_(h.is_entry('notanentryname'),
        'strings that are neither plugin names nor command names do not get an entry')

    expect.true_(h.is_entry({'hascommands', 'boxbinders', 'nocommand'}),
        'list of valid entries')
    expect.false_(h.is_entry({'hascommands', 'notanentryname'}),
        'list contains an element that is not an entry')
end

function test.get_entry_types()
    expect.table_eq({builtin=true, command=true}, h.get_entry_types('ls'))

    expect.table_eq({plugin=true}, h.get_entry_types('hascommands'))
    expect.table_eq({command=true}, h.get_entry_types('boxbinders'))
    expect.table_eq({plugin=true, command=true}, h.get_entry_types('samename'))
    expect.table_eq({plugin=true}, h.get_entry_types('nocommand'))
    expect.table_eq({command=true}, h.get_entry_types('basic'))
    expect.table_eq({command=true}, h.get_entry_types('subdir/scriptname'))

    expect.table_eq({plugin=true}, h.get_entry_types('nodocs_hascommands'))
    expect.table_eq({command=true}, h.get_entry_types('nodoc_command'))
    expect.table_eq({plugin=true, command=true}, h.get_entry_types('nodocs_samename'))
    expect.table_eq({plugin=true}, h.get_entry_types('nodocs_nocommand'))
    expect.table_eq({command=true}, h.get_entry_types('nodocs_script'))
    expect.table_eq({command=true}, h.get_entry_types('inscript_docs'))
    expect.table_eq({command=true}, h.get_entry_types('inscript_short_only'))

    expect.error_match('entry not found', function()
        h.get_entry_types('notanentry') end)
end

function test.get_entry_short_help()
    expect.eq('No help available.', h.get_entry_short_help('ls'),
        'no docs for builtin fn result in default short description')

    expect.eq('Documented a plugin that has commands.', h.get_entry_short_help('hascommands'))
    expect.eq('Box your binders.', h.get_entry_short_help('boxbinders'),
        'should get short help from command description')
    expect.eq('Samename.', h.get_entry_short_help('samename'),
        'should get short help from command description')
    expect.eq('Documented nocommand.', h.get_entry_short_help('nocommand'))
    expect.eq('Documented basic.', h.get_entry_short_help('basic'))
    expect.eq('Documented subdir/scriptname.', h.get_entry_short_help('subdir/scriptname'))

    expect.eq('No help available.', h.get_entry_short_help('nodocs_hascommands'))
    expect.eq('cpp description.', h.get_entry_short_help('nodoc_command'),
        'should get short help from command description')
    expect.eq('Nodocs samename.', h.get_entry_short_help('nodocs_samename'),
        'should get short help from command description')
    expect.eq('No help available.', h.get_entry_short_help('nodocs_nocommand'))
    expect.eq('in-file short description for inscript_docs.',
        h.get_entry_short_help('inscript_docs'),
        'should get short help from header comment')
    expect.eq('No help available.',
        h.get_entry_short_help('nodocs_script'))
    expect.eq('inscript short desc.',
        h.get_entry_short_help('inscript_short_only'),
        'should get short help from header comment')
end

function test.get_entry_long_help()
    local expected = [[
basic
=====

Tags: map

Command:
"basic"

  Documented
  basic.

Documented
full help.
    ]]
    expect.eq(expected, h.get_entry_long_help('basic', 13))

    -- long help for plugins/commands that have doc files should match the
    -- contents of those files exactly (test data is already wrapped)
    expect.eq(files['hack/docs/docs/tools/hascommands.txt'],
        h.get_entry_long_help('hascommands'))
    expect.eq(files['hack/docs/docs/tools/hascommands.txt'],
        h.get_entry_long_help('boxbinders'))
    expect.eq(files['hack/docs/docs/tools/samename.txt'],
        h.get_entry_long_help('samename'))
    expect.eq(files['hack/docs/docs/tools/nocommand.txt'],
        h.get_entry_long_help('nocommand'))
    expect.eq(files['hack/docs/docs/tools/basic.txt'],
        h.get_entry_long_help('basic'))
    expect.eq(files['hack/docs/docs/tools/subdir/scriptname.txt'],
        h.get_entry_long_help('subdir/scriptname'))

    -- plugins/commands that have no doc files get the default template
    expect.eq([[ls
==

No help available.
]], h.get_entry_long_help('ls'))
    expect.eq([[nodocs_hascommands
==================

No help available.
]], h.get_entry_long_help('nodocs_hascommands'))
    expect.eq([[nodocs_hascommands
==================

No help available.
]], h.get_entry_long_help('nodoc_command'))
    expect.eq([[Nodocs samename.
        This command has the same name as its host plugin but no rst docs.]], h.get_entry_long_help('nodocs_samename'))
    expect.eq([[nodocs_nocommand
================

No help available.
]], h.get_entry_long_help('nodocs_nocommand'))
    expect.eq([[nodocs_script
=============

No help available.
]], h.get_entry_long_help('nodocs_script'))
    expect.eq([[inscript_short_only
===================

No help available.
]], h.get_entry_long_help('inscript_short_only'))

    -- scripts that have no doc files get the docs from the script lua source
    expect.eq([[inscript_docs
=============

Tags: map | badtag

Command: "inscript_docs"

  in-file inscript_docs.

Documented full help.]], h.get_entry_long_help('inscript_docs'))
end

function test.get_entry_tags()
    expect.table_eq({fort=true, armok=true, units=true},
        h.get_entry_tags('hascommands'))
    expect.table_eq({fort=true, armok=true, units=true},
        h.get_entry_tags('samename'))
    expect.table_eq({fort=true, armok=true, units=true},
        h.get_entry_tags('nocommand'))
    expect.table_eq({map=true}, h.get_entry_tags('basic'))
    expect.table_eq({map=true}, h.get_entry_tags('inscript_docs'),
        'bad tags should get filtered out')
end

function test.is_tag()
    -- see tags defined in the Tags.txt files entry above
    expect.true_(h.is_tag('map'))
    expect.true_(h.is_tag({'map', 'armok'}))

    expect.false_(h.is_tag(nil))
    expect.false_(h.is_tag(''))
    expect.false_(h.is_tag('not_tag'))
    expect.false_(h.is_tag({'map', 'not_tag', 'armok'}))
end

function test.get_tags()
    expect.table_eq({'armok', 'dev', 'fort', 'map', 'nomembers', 'units'},
        h.get_tags())
end

function test.get_tag_data()
    local tag_data = h.get_tag_data('armok')
    table.sort(tag_data)
    expect.table_eq({description='Tools that give you complete control over an aspect of the game or provide access to information that the game intentionally keeps hidden.',
        'bindboxers', 'boxbinders', 'hascommands', 'nocommand', 'samename'},
        tag_data,
        'multi-line descriptions should get joined into a single line.')

    tag_data = h.get_tag_data('fort')
    table.sort(tag_data)
    expect.table_eq({description='Tools that are useful while in fort mode.',
        'bindboxers', 'boxbinders', 'hascommands', 'nocommand', 'samename'},
        tag_data)

    tag_data = h.get_tag_data('units')
    table.sort(tag_data)
    expect.table_eq({description='Tools that interact with units.',
        'bindboxers', 'boxbinders', 'hascommands', 'nocommand', 'samename'},
        tag_data)

    tag_data = h.get_tag_data('map')
    table.sort(tag_data)
    expect.table_eq({description='Tools that interact with the game map.',
        'basic', 'inscript_docs', 'subdir/scriptname'},
        tag_data)

    expect.table_eq({description='Nothing is tagged with this.'}, h.get_tag_data('nomembers'))

    expect.error_match('tag not found',
        function() h.get_tag_data('notatag') end)
end

function test.search_entries()
    -- all entries, in alphabetical order by last path component
    local expected = {'?', 'alias', 'basic', 'bindboxers', 'boxbinders',
        'clear', 'cls', 'dev_script', 'die', 'dir', 'disable', 'devel/dump-rpc',
        'enable', 'fpause', 'hascommands', 'help', 'hide', 'inscript_docs',
        'inscript_short_only', 'keybinding', 'kill-lua', 'load', 'ls', 'man',
        'nocommand', 'nodoc_command', 'nodocs_hascommands', 'nodocs_nocommand',
        'nodocs_samename', 'nodocs_script', 'plug', 'reload', 'samename',
        'script', 'subdir/scriptname', 'sc-script', 'show', 'tags', 'type',
        'unload'}
    table.sort(expected, h.sort_by_basename)
    expect.table_eq(expected, h.search_entries())
    expect.table_eq(expected, h.search_entries({}))
    expect.table_eq(expected, h.search_entries(nil, {}))
    expect.table_eq(expected, h.search_entries({}, {}))

    expected = {'inscript_docs', 'subdir/scriptname'}
    expect.table_eq(expected, h.search_entries({tag='map', str='script'}))

    expected = {'script', 'sc-script'}
    table.sort(expected, h.sort_by_basename)
    expect.table_eq(expected, h.search_entries({str='script',
                                                entry_type='builtin'}))

    expected = {'dev_script', 'inscript_docs', 'inscript_short_only',
                'nodocs_script', 'subdir/scriptname'}
    expect.table_eq(expected, h.search_entries({str='script'},
                                               {entry_type='builtin'}))

    expected = {'bindboxers', 'boxbinders'}
    expect.table_eq(expected, h.search_entries({str='box'}))

    expected = {'bindboxers', 'boxbinders', 'inscript_docs',
                'inscript_short_only', 'nodocs_script', 'subdir/scriptname'}
    expect.table_eq(expected, h.search_entries({{str='script'}, {str='box'}},
                                               {{entry_type='builtin'},
                                                {tag='dev'}}),
                    'multiple filters for include and exclude')
end

function test.get_commands()
    local expected = {'?', 'alias', 'basic', 'bindboxers', 'boxbinders',
        'clear', 'cls', 'dev_script', 'die', 'dir', 'disable', 'devel/dump-rpc',
        'enable', 'fpause', 'help', 'hide', 'inscript_docs', 'inscript_short_only',
        'keybinding', 'kill-lua', 'load', 'ls', 'man', 'nodoc_command',
        'nodocs_samename', 'nodocs_script', 'plug', 'reload', 'samename',
        'script', 'subdir/scriptname', 'sc-script', 'show', 'tags', 'type',
        'unload'}
    table.sort(expected, h.sort_by_basename)
    expect.table_eq(expected, h.get_commands())
end

function test.is_builtin()
    expect.true_(h.is_builtin('ls'))
    expect.false_(h.is_builtin('basic'))
    expect.false_(h.is_builtin('notanentry'))
end

function test.help()
    expect.printerr_match('No help entry found', function() h.help('blarg') end)

    local mock_print = mock.func()
    mock.patch(h, 'print', mock_print, function()
        h.help('nocommand')
        expect.eq(1, mock_print.call_count)
        expect.eq(files['hack/docs/docs/tools/nocommand.txt'],
            mock_print.call_args[1][1])
    end)
end

function test.tags()
    local mock_print = mock.func()
    mock.patch(h, 'print', mock_print, function()
        h.tags()
        expect.eq(8, mock_print.call_count)
        expect.eq('armok                Tools that give you complete control over an aspect of the',
            mock_print.call_args[1][1])
        expect.eq('                      game or provide access to information that the game',
            mock_print.call_args[2][1])
        expect.eq('                      intentionally keeps hidden.',
            mock_print.call_args[3][1])
        expect.eq('dev                  Dev tools.',
            mock_print.call_args[4][1])
        expect.eq('fort                 Tools that are useful while in fort mode.',
            mock_print.call_args[5][1])
        expect.eq('map                  Tools that interact with the game map.',
            mock_print.call_args[6][1])
        expect.eq('nomembers            Nothing is tagged with this.',
            mock_print.call_args[7][1])
        expect.eq('units                Tools that interact with units.',
            mock_print.call_args[8][1])
    end)
end

function test.tags_tag()
    local mock_print = mock.func()
    mock.patch(h, 'print', mock_print, function()
        h.tags('armok')
        expect.eq(3, mock_print.call_count)
        expect.eq('bindboxers           Bind your boxers.',
            mock_print.call_args[1][1])
        expect.eq('boxbinders           Box your binders.',
            mock_print.call_args[2][1])
        expect.eq('samename             Samename.',
            mock_print.call_args[3][1])
    end)
end

function test.ls()
    local mock_print = mock.func()
    mock.patch(h, 'print', mock_print, function()
        h.ls('doc') -- interpreted as a string
        expect.eq(5, mock_print.call_count)
        expect.eq('inscript_docs        in-file short description for inscript_docs.',
            mock_print.call_args[1][1])
        expect.eq('                      tags: map', mock_print.call_args[2][1])
        expect.eq('nodoc_command        cpp description.',
            mock_print.call_args[3][1])
        expect.eq('nodocs_samename      Nodocs samename.',
            mock_print.call_args[4][1])
        expect.eq('nodocs_script        No help available.',
            mock_print.call_args[5][1])
    end)

    mock_print = mock.func()
    mock.patch(h, 'print', mock_print, function()
        h.ls('armok') -- interpreted as a tag
        expect.eq(6, mock_print.call_count)
        expect.eq('bindboxers           Bind your boxers.',
            mock_print.call_args[1][1])
        expect.eq('                      tags: armok, fort, units',
            mock_print.call_args[2][1])
        expect.eq('boxbinders           Box your binders.',
            mock_print.call_args[3][1])
        expect.eq('                      tags: armok, fort, units',
            mock_print.call_args[4][1])
        expect.eq('samename             Samename.',
            mock_print.call_args[5][1])
        expect.eq('                      tags: armok, fort, units',
            mock_print.call_args[6][1])
    end)

    mock_print = mock.func()
    mock.patch(h, 'print', mock_print, function()
        h.ls('not a match')
        expect.eq(1, mock_print.call_count)
        expect.eq('No matches.', mock_print.call_args[1][1])
    end)

    -- test skipping tags and excluding strings
    mock_print = mock.func()
    mock.patch(h, 'print', mock_print, function()
        h.ls('armok', true, false, 'boxer,binder')
        expect.eq(1, mock_print.call_count)
        expect.eq('samename             Samename.', mock_print.call_args[1][1])
    end)

    -- test excluding dev scripts
    mock_print = mock.func()
    mock.patch(h, 'print', mock_print, function()
        h.ls('_script', true, false, 'inscript,nodocs')
        expect.eq(1, mock_print.call_count)
        expect.eq('No matches.', mock_print.call_args[1][1])
    end)

    -- test including dev scripts
    mock_print = mock.func()
    mock.patch(h, 'print', mock_print, function()
        h.ls('_script', true, true, 'inscript,nodocs')
        expect.eq(1, mock_print.call_count)
        expect.eq('dev_script           Short desc.',
            mock_print.call_args[1][1])
    end)
end
