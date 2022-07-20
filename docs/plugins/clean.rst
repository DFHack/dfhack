clean
=====

Tags:
:dfhack-keybind:

Removes contaminants from tiles, items, and units. More specifically, it
cleans all the splatter that get scattered all over the map and that clings to
your items and units. In an old fortress, this can significantly reduce FPS lag.
It can also spoil your !!FUN!!, so think before you use it.

Usage::

    clean all|map|items|units|plants [<options>]

By default, cleaning the map leaves mud and snow alone. Note that cleaning units
includes hostiles, and that cleaning items removes poisons from weapons.

Options
-------

When cleaning the map, you can specify extra options for extra cleaning:

- ``mud``
    Also remove mud.
- ``item``
    Also remove item spatter, like fallen leaves and flowers.
- ``snow``
    Also remove snow coverings.

Examples
--------

- ``clean all``
    Clean everything that can be cleaned (except mud and snow).
- ``clean all mud item snow``
    Removes all spatter, including mud, leaves, and snow from map tiles.
