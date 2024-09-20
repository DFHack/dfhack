.. _clean:
.. _spotclean:

cleaners
========

.. dfhack-tool::
    :summary: Provides commands for cleaning spatter (and grass) from the map.
    :tags: adventure fort armok fps items map plants units
    :no-command:

.. dfhack-command:: clean
    :summary: Removes contaminants.

.. dfhack-command:: spotclean
    :summary: Remove contaminants from the tile under the cursor.

This plugin provides commands that clean the spatter that gets scattered all
over the map and that clings to your items and units. In an old fortress,
cleaning with this tool can significantly reduce FPS lag! It can also spoil
your !!FUN!!, so think before you use it.

This plugin can also be used to remove grass in an area you don't want it
(including those hard to reach areas like stairs and under buildings).
Use it if you messed up with the `regrass` tool, or grass grew in your
dining hall after a flooding mishap. Grass may eventually regrow if the tile
remains soil, so `gui/tiletypes` may come in handy to change it to an
appropriate stone type.

Usage
-----

::

    clean [<pos> [<pos>]] [<options>]
    spotclean

By default, cleaning the map leaves mud, snow, and item spatter (e.g., tree
droppings) alone. Note that cleaning units includes hostiles, and that
cleaning items removes poisons from weapons.

Mud will not be cleaned out from under farm plots unless the tile is furrowed
soil, since that would render the plot inoperable.

Operates on the entire map unless otherwise specified. Supplying a ``pos``
argument can limit operation to a single tile. Supplying both can operate on
a cuboid region. ``pos`` should normally be in the form ``0,0,0``, without
spaces. The string ``here`` can be used in place of numeric coordinates to use
the position of the keyboard cursor, if active. The ``--zlevel`` option uses
the ``pos`` values differently.

``spotclean`` works like ``clean here --map --mud --snow``, removing all
contaminants from a specific tile quickly. Intended as a hotkey command.


Examples
--------

``clean --all``
    Clean most spatter from all map tiles (excluding mud, snow, and item
    spatter), as well as all contaminants from units and items.

``clean --map --mud --snow --item``
    Removes all spatter, including mud, snow, and tree droppings from map
    tiles. Farm plots will retain their mud as needed.

``clean --map --item --only --items``
    Remove only tree droppings from the map (leaving blood and other
    contaminants untouched). Clean all items of their contaminants.

``clean here -mdstu``
    Clean any sort of contaminant from the map tile under the keyboard cursor,
    as well as any contaminant found on a unit there, but don't touch contaminants
    on any item (e.g., the unit's poisoned weapon).

``clean 0,0,90 0,0,120 -uiz``
    Clean all contaminants from units and items on z-levels 90 through 120.
    Don't touch map spatter at all.

``clean -mgxoz``
    Reduce all grass on the current z-level to barren soil. Don't touch
    any contaminants.

``clean 0,0,100 19,19,119 -adstg``
    Remove all contaminants of any type from the 20 x 20 x 20 cube defined
    by the coords, including on any units, items, and ground spatter
    (excluding mud for farms). Also remove any unused grass type events from
    the affected map blocks, but don't remove any grass present.

Options
-------

``-a``, ``--all``
    Equivalent to ``--map --units --items --plants``.
``-m``, ``--map``
    Clean selected map tiles. Cleans most spatter by default, but not mud,
    snow, or item spatter.
``-d``, ``--mud``
    Also remove mud from map tiles.
``-s``, ``--snow``
    Also remove snow coverings from map tiles.
``-t``, ``--item``
    Also remove item spatter (e.g., fallen leaves, flowers, and gatherable
    tree fruit) from map tiles. Not to be confused with ``--items``, which
    cleans contaminants *off* of items.
``-g``, ``--grass``
    Remove unused (entirely depleted) grass events from map blocks. DF will
    create a grass events for each type of grass that grows in a block, but
    doesn't remove them if you were to pave over everything, or they got
    depleted and entirely replaced with a different type. Could possibly
    improve FPS if you had a ton of unused grass events everywhere (a likely
    outcome of using ``regrass --new``). Requires ``--map`` option to be
    specified.

``-x``, ``--desolate``
    Use with caution! Remove grass from the selected area. You probably don't
    want to use this on the entire map, so make sure to use ``pos`` arguments.
    Requires ``--map`` and ``--grass`` options to be specified.
``-o``, ``--only``
    Ignore most spatter (e.g., blood, vomit, and ooze) and focus only on the
    other specified options. Requires ``--map`` and at least one of: ``--mud``,
    ``--snow``, ``--item``, or ``--grass``.
``-u``, ``--units``
    Clean all contaminants off of units in the selected area. Not affected by
    map options that specify spatter types (e.g., snow). They will always be
    completely cleaned.
``-i``, ``--items``
    Clean all contaminants off of items in the selected area (including those
    held by units). Not affected by map options that specify spatter types.
    Not to be confused with ``--item``, which removes tree droppings found on
    the ground.
``-z``, ``--zlevel``
    Select entire z-levels. Will do all z-levels between ``pos`` arguments if
    both are given, z-level of first ``pos`` if one is given, else z-level of
    current view if no ``pos`` is given.

Troubleshooting
---------------

Use ``debugfilter set Debug cleaners log`` (or
``debugfilter set Trace cleaners log`` for more detail) to help diagnose
issues. (Avoid cleaning large parts of the map using many options with
Trace enabled, as it could make the game unresponsive and flood the console
for a good minute.)

Disable with ``debugfilter set Info cleaners log``.
