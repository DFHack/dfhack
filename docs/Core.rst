.. _dfhack-core:

###########
DFHack Core
###########

DFHack commands can be implemented in three ways, all of which
are used in the same way:

:builtin:   commands are implemented by the core of DFHack. They manage
            other DFhack tools, interpret commands, and control basic
            aspects of DF (force pause or quit).

:plugins:   are stored in ``hack/plugins/`` and must be compiled with the
            same version of DFHack.  They are less flexible than scripts,
            but used for complex or ongoing tasks becasue they run faster.

:scripts:   are Ruby or Lua scripts stored in ``hack/scripts/``.
            Because they don't need to be compiled, scripts are
            more flexible about versions, and easier to distribute.
            Most third-party DFHack addons are scripts.


.. contents::


Built-in Commands
=================
The following commands are provided by the 'core' components
of DFhack, rather than plugins or scripts.


.. _cls:

cls
---
Clear the terminal.  Does not delete command history.


.. _die:

die
---
Instantly kills DF without saving.


.. _disable:

.. _enable:

enable
------
Many plugins can be in a distinct enabled or disabled state. Some of
them activate and deactivate automatically depending on the contents
of the world raws. Others store their state in world data. However a
number of them have to be enabled globally, and the init file is the
right place to do it.

Most such plugins or scripts support the built-in ``enable`` and ``disable``
commands. Calling them at any time without arguments prints a list
of enabled and disabled plugins, and shows whether that can be changed
through the same commands.

To enable or disable plugins that support this, use their names as
arguments for the command::

  enable manipulator search


.. _fpause:

fpause
------
Forces DF to pause. This is useful when your FPS drops below 1 and you lose
control of the game.


.. _help:

help
----
Most commands support using the ``help <command>`` built-in command
to retrieve further help without having to look at this document.
``? <cmd>`` and ``man <cmd>`` are aliases.

Some commands (including many scripts) instead take ``help`` or ``?``
as an option on their command line - ie ``<cmd> help``.


.. _hide:

hide
----
Hides the DFHack terminal window.  Only available on Windows.


.. _keybinding:

keybinding
----------
To set keybindings, use the built-in ``keybinding`` command. Like any other
command it can be used at any time from the console, but bindings are not
remembered between runs of the game unless re-created in `dfhack.init`.

Currently, any combinations of Ctrl/Alt/Shift with A-Z, 0-9, or F1-F12 are supported.

Possible ways to call the command:

``keybinding list <key>``
  List bindings active for the key combination.
``keybinding clear <key> <key>...``
  Remove bindings for the specified keys.
``keybinding add <key> "cmdline" "cmdline"...``
  Add bindings for the specified key.
``keybinding set <key> "cmdline" "cmdline"...``
  Clear, and then add bindings for the specified key.

The ``<key>`` parameter above has the following *case-sensitive* syntax::

    [Ctrl-][Alt-][Shift-]KEY[@context[|context...]]

where the *KEY* part can be any recognized key and [] denote optional parts.

When multiple commands are bound to the same key combination, DFHack selects
the first applicable one. Later ``add`` commands, and earlier entries within one
``add`` command have priority. Commands that are not specifically intended for use
as a hotkey are always considered applicable.

The ``context`` part in the key specifier above can be used to explicitly restrict
the UI state where the binding would be applicable. If called without parameters,
the ``keybinding`` command among other things prints the current context string.

Only bindings with a ``context`` tag that either matches the current context fully,
or is a prefix ending at a ``/`` boundary would be considered for execution, i.e.
when in context ``foo/bar/baz``, keybindings restricted to any of ``@foo/bar/baz``,
``@foo/bar``, ``@foo`` or none will be active.

Multiple contexts can be specified by separating them with a
pipe (``|``) - for example, ``@foo|bar|baz/foo`` would match
anything under ``@foo``, ``@bar``, or ``@baz/foo``.

Interactive commands like `liquids` cannot be used as hotkeys.


.. _kill-lua:

kill-lua
--------
Stops any currently-running Lua scripts. By default, scripts can
only be interrupted every 256 instructions. Use ``kill-lua force``
to interrupt the next instruction.


.. _load:
.. _unload:
.. _reload:

load
----
``load``, ``unload``, and ``reload`` control whether a plugin is loaded
into memory - note that plugins are loaded but disabled unless you do
something.  Usage::

    load|unload|reload PLUGIN|(-a|--all)

Allows dealing with plugins individually by name, or all at once.


.. _ls:

ls
--
``ls`` does not list files like the Unix command, but rather
available commands - first built in commands, then plugins,
and scripts at the end.  Usage:

:ls -a:         Also list scripts in subdirectories of ``hack/scripts/``,
                which are generally not intended for direct use.
:ls <plugin>:   List subcommands for the given plugin.


.. _plug:

plug
----
Lists available plugins, including their state and detailed description.

``plug``
        Lists available plugins (*not* commands implemented by plugins)
``plug [PLUGIN] [PLUGIN] ...``
        List state and detailed description of the given plugins,
        including commands implemented by the plugin.


.. _sc-script:

sc-script
---------
Allows additional scripts to be run when certain events occur
(similar to onLoad*.init scripts)


.. _script:

script
------
Reads a text file, and runs each line as a DFHack command
as if it had been typed in by the user - treating the
input like `an init file <init-files>`.

Some other tools, such as `autobutcher` and `workflow`, export
their settings as the commands to create them - which are later
loaded with ``script``


.. _show:

show
----
Shows the terminal window after it has been `hidden <hide>`.
Only available on Windows.  You'll need to use it from a
`keybinding` set beforehand, or the in-game `command-prompt`.

.. _type:

type
----
``type command`` shows where ``command`` is implemented.

Other Commands
--------------
The following commands are *not* built-in, but offer similarly useful functions.

* `command-prompt`
* `hotkeys`
* `lua`
* `multicmd`
* `nopause`
* `quicksave`
* `rb`
* `repeat`


.. _init-files:

Init Files
==========
DFHack allows users to automatically run commonly-used DFHack commands
when DF is first loaded, when a game is loaded, and when a game is unloaded.

Init scripts function the same way they would if the user manually typed
in their contents, but are much more convenient.  In order to facilitate
savegave portability, mod merging, and general organization of init files,
DFHack supports multiple init files both in the main DF directory and
save-specific init files in the save folders.

DFHack looks for init files in three places each time they could be run:

#. The main DF directory
#. :file:`data/save/{world}/raw`, where ``world`` is the current save, and
#. :file:`data/save/{world}/raw/objects`

When reading commands from dfhack.init or with the `script` command, if the final
character on a line is a backslash then the next uncommented line is considered a
continuation of that line, with the backslash deleted.  Commented lines are skipped,
so it is possible to comment out parts of a command with the ``#`` character.


.. _dfhack.init:

dfhack*.init
------------
If your DF folder contains at least one file named ``dfhack*.init``
(where ``*`` is a placeholder for any string), then all such files
are executed in alphabetical order when DF is first started.

DFHack is distributed with :download:`/dfhack.init-example` as an example
with an up-to-date collection of basic commands; mostly setting standard
keybindings and `enabling <enable>` plugins.  You are encouraged to look
through this file to learn which features it makes available under which
key combinations.  You may also customise it and rename it to ``dfhack.init``.

If your DF folder does not contain any ``dfhack*.init`` files, the example
will be run as a fallback.

These files are best used for keybindings and enabling persistent plugins
which do not require a world to be loaded.


.. _onLoad.init:

onLoad*.init
------------
When a world is loaded, DFHack looks for files of the form ``onLoad*.init``,
where ``*`` can be any string, including the empty string.

All matching init files will be executed in alphebetical order.
A world being loaded can mean a fortress, an adventurer, or legends mode.

These files are best used for non-persistent commands, such as setting
a `fix <fix>` script to run on `repeat`.


.. _onUnload.init:

onUnload*.init
--------------
When a world is unloaded, DFHack looks for files of the form ``onUnload*.init``.
Again, these files may be in any of the above three places.
All matching init files will be executed in alphebetical order.

Modders often use such scripts to disable tools which should not affect
an unmodded save.


Other init files
----------------

* ``onMapLoad*.init`` and ``onMapUnload*.init`` are run when a map,
  distinct from a world, is loaded.  This is good for map-affecting
  commands (eg `clean`), or avoiding issues in Legends mode.

* Any lua script named ``raw/init.d/*.lua``, in the save or main DF
  directory, will be run when any world or that save is loaded.


Miscellaneous Notes
===================
This section is for odd but important notes that don't fit anywhere else.

* The ``dfhack-run`` executable is there for calling DFHack commands in
  an already running DF+DFHack instance from external OS scripts and programs,
  and is *not* the way how you use DFHack normally.

* If a DF :kbd:`H` hotkey is named with a DFHack command, pressing
  the corresponding :kbd:`Fx` button will run that command, instead of
  zooming to the set location.

* The command line has some nice line editing capabilities, including history
  that's preserved between different runs of DF (use up/down keys to go through
  the history).

* The binaries for 0.40.15-r1 to 0.34.11-r4 are on DFFD_.
  Older versions are available here_.

  .. _DFFD: http://dffd.bay12games.com/search.php?string=DFHack&id=15&limit=1000
  .. _here: http://dethware.org/dfhack/download

* To include whitespace in the argument/s to some command, quote it in
  double quotes.  To include a double quote character, use ``\"``.

* If the first non-whitespace character is ``:``, the command is parsed in
  an alternative mode which is very useful for the `lua` and `rb` commands.
  The following two command lines are exactly equivalent::

    :foo a b "c d" e f
    foo "a b \"c d\" e f"

  * non-whitespace characters following the ``:`` are the command name
  * the remaining part of the line is used verbatim as the first argument

