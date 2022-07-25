.. _tiletypes-here:
.. _tiletypes-here-point:

tiletypes
=========
Can be used for painting map tiles and is an interactive command, much like
`liquids`. Some properties of existing tiles can be looked up with `probe`. If
something goes wrong, `fixveins` may help.

The tool works with two set of options and a brush. The brush determines which
tiles will be processed. First set of options is the filter, which can exclude
some of the tiles from the brush by looking at the tile properties. The second
set of options is the paint - this determines how the selected tiles are
changed.

Both paint and filter can have many different properties including things like
general shape (WALL, FLOOR, etc.), general material (SOIL, STONE, MINERAL,
etc.), state of 'designated', 'hidden' and 'light' flags.

The properties of filter and paint can be partially defined. This means that
you can for example turn all stone fortifications into floors, preserving the
material::

        filter material STONE
        filter shape FORTIFICATION
        paint shape FLOOR

Or turn mineral vein floors back into walls::

        filter shape FLOOR
        filter material MINERAL
        paint shape WALL

The tool also allows tweaking some tile flags::

        paint hidden 1
        paint hidden 0

This will hide previously revealed tiles (or show hidden with the 0 option).

More recently, the tool supports changing the base material of the tile to
an arbitrary stone from the raws, by creating new veins as required. Note
that this mode paints under ice and constructions, instead of overwriting
them. To enable, use::

        paint stone MICROCLINE

This mode is incompatible with the regular ``material`` setting, so changing
it cancels the specific stone selection::

        paint material ANY

Since different vein types have different drop rates, it is possible to choose
which one to use in painting::

        paint veintype CLUSTER_SMALL

When the chosen type is ``CLUSTER`` (the default), the tool may automatically
choose to use layer stone or lava stone instead of veins if its material matches
the desired one.

Any paint or filter option (or the entire paint or filter) can be disabled entirely by using the ANY keyword::

        paint hidden ANY
        paint shape ANY
        filter material any
        filter shape any
        filter any

You can use several different brushes for painting tiles:

:point:     a single tile
:range:     a rectangular range
:column:    a column ranging from current cursor to the first solid tile above
:block:     a DF map block - 16x16 tiles, in a regular grid

Example::

    range 10 10 1

This will change the brush to a rectangle spanning 10x10 tiles on one z-level.
The range starts at the position of the cursor and goes to the east, south and
up.

For more details, use ``tiletypes help``.

tiletypes-command
-----------------
Runs tiletypes commands, separated by ``;``. This makes it possible to change
tiletypes modes from a hotkey or via dfhack-run.

Example::

    tiletypes-command p any ; p s wall ; p sp normal

This resets the paint filter to unsmoothed walls.

tiletypes-here-point
--------------------
Apply the current tiletypes options at the in-game cursor position to a single
tile. Can be used from a hotkey.

This command supports the same options as `tiletypes-here` above.

tiletypes-here
--------------
Apply the current tiletypes options at the in-game cursor position, including
the brush. Can be used from a hotkey.

Options:

:``-c``, ``--cursor <x>,<y>,<z>``:
    Use the specified map coordinates instead of the current cursor position. If
    this option is specified, then an active game map cursor is not necessary.
:``-h``, ``--help``:
    Show command help text.
:``-q``, ``--quiet``:
    Suppress non-error status output.
