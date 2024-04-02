keybinding
==========

.. dfhack-tool::
    :summary: Create hotkeys that will run DFHack commands.
    :tags: dfhack

Like any other command, it can be used at any time from the console, but
bindings are not remembered between runs of the game unless re-created in
:file:`dfhack-config/init/dfhack.init`.

Hotkeys can be any combinations of Ctrl/Alt/Shift with A-Z, 0-9, F1-F12, or `
(the key below the :kbd:`Esc` key on most keyboards). You can also represent
mouse buttons beyond the first three with ``MOUSE4`` through ``MOUSE15``.

Usage
-----

``keybinding``
    Show some useful information, including the current game context.
``keybinding list <key>``
    List bindings active for the key combination.
``keybinding clear <key> [<key>...]``
    Remove bindings for the specified keys.
``keybinding add <key> "<cmdline>" ["<cmdline>" ...]``
    Add bindings for the specified key.
``keybinding set <key> "<cmdline>" ["<cmdline>" ...]``
    Clear, and then add bindings for the specified key.

The ``<key>`` parameter above has the following **case-sensitive** syntax::

    [Ctrl-][Alt-][Shift-]KEY[@context[|context...]]

where the ``KEY`` part can be any recognized key and :kbd:`[`:kbd:`]` denote
optional parts.

DFHack commands can advertise the contexts in which they can be usefully run.
For example, a command that acts on a selected unit can tell `keybinding` that
it is not "applicable" in the current context if a unit is not actively
selected.

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

Commands like `liquids` or `tiletypes` cannot be used as hotkeys since they
require the console for interactive input.

Examples
--------

Bind Ctrl-Shift-C to run the `hotkeys` command on any screen at any time::

    keybinding add Ctrl-Shift-C hotkeys

Bind Ctrl-M to run `gui/mass-remove`, but only when on the main map with
nothing else selected::

    keybinding add Ctrl-M@dwarfmode/Default gui/mass-remove

Bind the fourth mouse button to launch `gui/teleport` when a unit is selected
or `gui/autodump` when an item is selected::

    keybinding add MOUSE4@dwarfmode/ViewSheets/UNIT gui/teleport
    keybinding add MOUSE4@dwarfmode/ViewSheets/ITEM gui/autodump

Bind Shift + the fifth mouse button to toggle the keyboard cursor in fort or
adventure mode::

    keybinding add Shift-MOUSE5@dwarfmode|dungeonmode toggle-kbd-cursor
