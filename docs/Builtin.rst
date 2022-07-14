.. _built-in-commands:

Built-in Commands
=================
The following commands are provided by the 'core' components of DFHack, rather
than plugins or scripts.

.. contents::
   :local:

.. _alias:

alias
-----
The ``alias`` command allows configuring aliases to other DFHack commands.
Aliases are resolved immediately after built-in commands, which means that an
alias cannot override a built-in command, but can override a command implemented
by a plugin or script.

Usage:

:``alias list``: lists all configured aliases
:``alias add <name> <command> [arguments...]``: adds an alias
:``alias replace <name> <command> [arguments...]``: replaces an existing
    alias with a new command, or adds the alias if it does not already exist
:``alias delete <name>``: removes the specified alias

Aliases can be given additional arguments when created and invoked, which will
be passed to the underlying command in order. An example with
`devel/print-args`::

    [DFHack]# alias add pargs devel/print-args example
    [DFHack]# pargs text
    example
    text


.. _cls:

cls
---
Clear the terminal. Does not delete command history.


.. _die:

die
---
Instantly kills DF without saving.


.. _disable:
.. _enable:

enable
------
Many plugins and scripts can be in a distinct enabled or disabled state. Some of
them activate and deactivate automatically depending on the contents of the
world raws. Others store their state in world data. However a number of them
have to be enabled globally, and the init file is the right place to do it.

Most such plugins or scripts support the built-in ``enable`` and ``disable``
commands. Calling them at any time without arguments prints a list of enabled
and disabled plugins, and shows whether that can be changed through the same
commands. Passing plugin names to these commands will enable or disable the
specified plugins. For example, to enable the `manipulator` plugin::

  enable manipulator

It is also possible to enable or disable multiple plugins at once::

  enable manipulator search


.. _fpause:

fpause
------
Forces DF to pause. This is useful when your FPS drops below 1 and you lose
control of the game.


.. _help:

help
----
Most commands support using the ``help <command>`` built-in command to retrieve
further help without having to look online. ``? <cmd>`` and ``man <cmd>`` are
aliases.

Some commands (including many scripts) instead take ``help`` or ``?`` as an
option on their command line - ie ``<cmd> help``.


.. _hide:

hide
----
Hides the DFHack terminal window. Only available on Windows.


.. _keybinding:

keybinding
----------
To set keybindings, use the built-in ``keybinding`` command. Like any other
command it can be used at any time from the console, but bindings are not
remembered between runs of the game unless re-created in `dfhack.init`.

Currently, any combinations of Ctrl/Alt/Shift with A-Z, 0-9, F1-F12, or ``\```
are supported.

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
``add`` command have priority. Commands that are not specifically intended for
use as a hotkey are always considered applicable.

The ``context`` part in the key specifier above can be used to explicitly
restrict the UI state where the binding would be applicable. If called without
parameters, the ``keybinding`` command among other things prints the current
context string.

Only bindings with a ``context`` tag that either matches the current context
fully, or is a prefix ending at a ``/`` boundary would be considered for
execution, i.e. when in context ``foo/bar/baz``, keybindings restricted to any
of ``@foo/bar/baz``, ``@foo/bar``, ``@foo`` or none will be active.

Multiple contexts can be specified by separating them with a pipe (``|``) - for
example, ``@foo|bar|baz/foo`` would match anything under ``@foo``, ``@bar``, or
``@baz/foo``.

Interactive commands like `liquids` cannot be used as hotkeys.


.. _kill-lua:

kill-lua
--------
Stops any currently-running Lua scripts. By default, scripts can only be
interrupted every 256 instructions. Use ``kill-lua force`` to interrupt the next
instruction.


.. _load:
.. _unload:
.. _reload:

load
----
``load``, ``unload``, and ``reload`` control whether a plugin is loaded into
memory - note that plugins are loaded but disabled unless you explicitly enable
them. Usage::

    load|unload|reload PLUGIN|(-a|--all)

Allows dealing with plugins individually by name, or all at once.

Note that plugins do not maintain their enabled state if they are reloaded, so
you may need to use `enable` to re-enable a plugin after reloading it.


.. _ls:
.. _dir:

ls
--
``ls`` (or ``dir``) does not list files like the Unix command, but rather
available commands. In order to group related commands, each command is
associated with a list of tags. You can filter the listed commands by a
tag or a substring of the command name. Usage:

:``ls``: Lists all available commands and the tags associated with them
    (if any).
:``ls TAG``: Shows only commands that have the given tag. Use the `tags` command
    to see the list of available tags.
:``ls STRING``: Shows commands that include the given string. E.g. ``ls auto``
    will show all the commands with "auto" in their names. If the string is also
    the name of a tag, then it will be interpreted as a tag name.

You can also pass some optional parameters to change how ``ls`` behaves:

:``--notags``: Don't print out the tags associated with each command.
:``--dev``: Include commands intended for developers and modders.


.. _plug:

plug
----
Lists available plugins and whether they are enabled.

``plug``
        Lists available plugins (*not* commands implemented by plugins)
``plug [PLUGIN] [PLUGIN] ...``
        List state and detailed description of the given plugins,
        including commands implemented by the plugin.


.. _sc-script:

sc-script
---------
Allows additional scripts to be run when certain events occur (similar to
onLoad\*.init scripts)


.. _script:

script
------
Reads a text file, and runs each line as a DFHack command as if it had been
typed in by the user - treating the input like `an init file <init-files>`.

Some other tools, such as `autobutcher` and `workflow`, export their settings as
the commands to create them - which can later be reloaded with ``script``.


.. _show:

show
----
Shows the terminal window after it has been `hidden <hide>`. Only available on
Windows. You'll need to use it from a `keybinding` set beforehand, or the
in-game `command-prompt`.


.. _tags:

tags
----

List the strings that the DFHack tools can be tagged with. You can find groups
of related tools by passing the tag name to `ls`.

.. _type:

type
----
``type command`` shows where ``command`` is implemented.

.. _devel/dump-rpc:

devel/dump-rpc
--------------

Writes RPC endpoint information to the specified file.

Usage::

    devel/dump-rpc FILENAME

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
