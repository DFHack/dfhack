burrow
======

.. dfhack-tool::
    :summary: Quickly adjust burrow tiles and units.
    :tags: fort auto design productivity units

This tool has two modes. When enabled, it monitors burrows with names that end
in ``+``. If a wall at the edge of such a burrow is dug out, the burrow will be
automatically extended to include the newly-revealed adjacent walls.

When run as a command, it can quickly adjust which tiles and/or units are
associated with the burrow.

Usage
-----

::

    enable burrow
    burrow tiles|units clear <target burrow> [<target burrow> ...] [<options>]
    burrow tiles|units set|add|remove <target burrow> <burrow> [...] [<options>]
    burrow tiles box-add|box-remove <target burrow> [<pos>] [<pos>] [<options>]
    burrow tiles flood-add|flood-remove <target burrow> [<options>]

The burrows can be referenced by name or by the internal numeric burrow ID. If
referenced by name, the first burrow that matches the name (case sensitive)
will be targeted. If a burrow name ends in ``+`` (to indicate that it should be
auto-expanded), the final ``+`` does not need to be specified on the
commandline.

For ``set``, ``add``, or ``remove`` commands, instead of a burrow, you can
specify one of the following all-caps keywords:

- ``ABOVE_GROUND``
- ``SUBTERRANEAN``
- ``INSIDE``
- ``OUTSIDE``
- ``LIGHT``
- ``DARK``
- ``HIDDEN``
- ``REVEALED``

to add or remove tiles with the corresponding properties.

Flood fill selects tiles spreading out from a starting tile if they:

- match the inside/outside and hidden/revealed properties of the starting tile
- match the walkability group of the starting tile OR (if the starting tile is
  walkable) is adjacent to a tile with the same walkability group as the
  starting tile

When flood adding, the flood fill will also stop at any tiles that have already
been added to the burrow. Similarly for flood removing, the flood will also
stop at tiles that are not in the burrow.

Examples
--------

``enable burrow``
    Start monitoring burrows that have names ending in '+' and automatically
    expand them when walls that border the burrows are dug out.
``burrow tiles clear Safety``
    Remove all tiles from the burrow named ``Safety`` (in preparation for
    adding new tiles elsewhere, presumably).
``burrow units clear Farmhouse Workshops``
    Remove all units from the burrows named ``Farmhouse`` and ``Workshops``.
``multicmd burrow tiles set Inside INSIDE; burrow tiles remove Inside HIDDEN``
    Reset the burrow named ``Inside`` to include all the currently revealed,
    interior tiles.
``burrow units set "Core Fort" Peasants Skilled``
    Clear all units from the burrow named ``Core Fort``, then add units
    currently assigned to the ``Peasants`` and ``Skilled`` burrows.
``burrow tiles box-add Safety 0,0,0``
    Add all tiles to the burrow named ``Safety`` that are within the volume of
    the box starting at coordinate 0, 0, 0 (the upper left corner of the bottom
    level) and ending at the current location of the keyboard cursor.
``burrow tiles flood-add Safety --cur-zlevel``
    Flood-add the tiles on the current z-level with the same properties as the
    tile under the keyboard cursor to the burrow named ``Safety``.

Options
-------

``-c``, ``--cursor <pos>``
    Indicate the starting position of the box or flood fill. If not specified,
    the position of the keyboard cursor is used.
``-d``, ``--dry-run``
    Report what would be done, but don't actually change anything.
``-z``, ``--cur-zlevel``
    Restricts the operation to the currently visible z-level.

Note
----

If you are auto-expanding a burrow (whose name ends in a ``+``) and the miner
who is digging to expand the burrow is assigned to that burrow, then 1-wide
corridors that expand the burrow will have very slow progress. This is because
the burrow is expanded to include the next dig job only after the miner has
chosen a next tile to dig, which may be far away. 2-wide cooridors are much
more efficient when expanding a burrow since the "next" tile to dig will still
be nearby.

Overlay
-------

When painting burrows in the vanilla UI, a few extra mouse operations are
supported. If you box select across multiple z-levels, you will be able to
select the entire volume instead of just the selected area on the z-level that
you are currently looking at.

In addition, double-clicking will start a flood fill from the target tile.
