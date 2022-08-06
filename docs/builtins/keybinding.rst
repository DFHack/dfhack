keybinding
==========
**Tags:** `tag/system`
:dfhack-keybind:`keybinding`

:index:`Create hotkeys that will run DFHack commands.
<keybinding; Create hotkeys that will run DFHack commands.>` Like any other
command, it can be used at any time from the console, but bindings are not
remembered between runs of the game unless re-created in `dfhack.init`.

Hotkeys can be any combinations of Ctrl/Alt/Shift with A-Z, 0-9, F1-F12, or
``\``` (the key below the ``Esc`` key.

Usage:

``keybinding``
    Show some useful information, including the current game context.
``keybinding list <key>``
    List bindings active for the key combination.
``keybinding clear <key> [<key>...]``
    Remove bindings for the specified keys.
``keybinding add <key> "cmdline" ["cmdline"...]``
    Add bindings for the specified key.
``keybinding set <key> "cmdline" ["cmdline"...]``
    Clear, and then add bindings for the specified key.

The ``<key>`` parameter above has the following **case-sensitive** syntax::

    [Ctrl-][Alt-][Shift-]KEY[@context[|context...]]

where the ``KEY`` part can be any recognized key and [] denote optional parts.

When multiple commands are bound to the same key combination, DFHack selects
the first applicable one. Later ``add`` commands, and earlier entries within one
``add`` command have priority. Commands that are not specifically intended for
use as a hotkey are always considered applicable.

The ``context`` part in the key specifier above can be used to explicitly
restrict the UI state where the binding would be applicable.

Only bindings with a ``context`` tag that either matches the current context
fully, or is a prefix ending at a ``/`` boundary would be considered for
execution, i.e. when in context ``foo/bar/baz``, keybindings restricted to any
of ``@foo/bar/baz``, ``@foo/bar``, ``@foo``, or none will be active.

Multiple contexts can be specified by separating them with a pipe (``|``) - for
example, ``@foo|bar|baz/foo`` would match anything under ``@foo``, ``@bar``, or
``@baz/foo``.

Interactive commands like `liquids` cannot be used as hotkeys.

Examples
--------

``keybinding add Alt-F1 hotkeys``
    Bind Alt-F1 to run the `hotkeys` command on any screen at any time.
``keybinding add Alt-F@dwarfmode gui/quickfort``
    Bind Alt-F to run `gui/quickfort`, but only when on a screen that shows the
    main map.
