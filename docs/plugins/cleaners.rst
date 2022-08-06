.. _clean:
.. _spotclean:

cleaners
========
**Tags:** `tag/adventure`, `tag/fort`, `tag/fps`, `tag/items`, `tag/map`, `tag/units`
:dfhack-keybind:`clean`
:dfhack-keybind:`spotclean`

:index:`Removes contaminants.  <clean; Removes contaminants.>` More
specifically, it cleans all the splatter that get scattered all over the map and
that clings to your items and units. In an old fortress, cleaning with this tool
can significantly reduce FPS lag. It can also spoil your !!FUN!!, so think
before you use it.

Usage::

    clean all|map|items|units|plants [<options>]
    spotclean

By default, cleaning the map leaves mud and snow alone. Note that cleaning units
includes hostiles, and that cleaning items removes poisons from weapons.

``spotclean`` works like ``clean map snow mud``,
:index:`removing all contaminants from the tile under the cursor.
<clean; Remove all contaminants from the tile under the cursor.>` This is ideal
if you just want to clean a specific tile but don't want the `clean` command to
remove all the glorious blood from your entranceway.

Examples
--------

``clean all``
    Clean everything that can be cleaned (except mud and snow).
``clean all mud item snow``
    Removes all spatter, including mud, leaves, and snow from map tiles.

Options
-------

When cleaning the map, you can specify extra options for extra cleaning:

``mud``
    Also remove mud.
``item``
    Also remove item spatter, like fallen leaves and flowers.
``snow``
    Also remove snow coverings.
