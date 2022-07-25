.. _liquids-here:

liquids
=======
Allows adding magma, water and obsidian to the game. It replaces the normal
dfhack command line and can't be used from a hotkey. Settings will be remembered
as long as dfhack runs. Intended for use in combination with the command
``liquids-here`` (which can be bound to a hotkey).  See also :issue:`80`.

.. warning::

    Spawning and deleting liquids can mess up pathing data and
    temperatures (creating heat traps). You've been warned.

.. note::

    `gui/liquids` is an in-game UI for this script.

Settings will be remembered until you quit DF. You can call `liquids-here` to execute
the last configured action, which is useful in combination with keybindings.

Usage: point the DF cursor at a tile you want to modify and use the commands.

If you only want to add or remove water or magma from one tile,
`source` may be easier to use.

Commands
--------
Misc commands:

:q:                 quit
:help, ?:           print this list of commands
:<empty line>:      put liquid

Modes:

:m:         switch to magma
:w:         switch to water
:o:         make obsidian wall instead
:of:        make obsidian floors
:rs:        make a river source
:f:         flow bits only
:wclean:    remove salt and stagnant flags from tiles

Set-Modes and flow properties (only for magma/water):

:s+:    only add mode
:s.:    set mode
:s-:    only remove mode
:f+:    make the spawned liquid flow
:f.:    don't change flow state (read state in flow mode)
:f-:    make the spawned liquid static

Permaflow (only for water):

:pf.:           don't change permaflow state
:pf-:           make the spawned liquid static
:pf[NS][EW]:    make the spawned liquid permanently flow
:0-7:           set liquid amount

Brush size and shape:

:p, point:      Single tile
:r, range:      Block with cursor at bottom north-west (any place, any size)
:block:         DF map block with cursor in it (regular spaced 16x16x1 blocks)
:column:        Column from cursor, up through free space
:flood:         Flood-fill water tiles from cursor (only makes sense with wclean)

liquids-here
------------
Run the liquid spawner with the current/last settings made in liquids (if no
settings in liquids were made it paints a point of 7/7 magma by default).

Intended to be used as keybinding. Requires an active in-game cursor.
