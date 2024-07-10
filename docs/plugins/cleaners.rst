.. _clean:
.. _spotclean:

cleaners
========

.. dfhack-tool::
    :summary: Provides commands for cleaning spatter from the map.
    :tags: adventure fort armok fps items map units
    :no-command:

.. dfhack-command:: clean
    :summary: Removes contaminants.

.. dfhack-command:: spotclean
    :summary: Remove all contaminants from the tile under the cursor.

This plugin provides commands that clean the splatter that get scattered all
over the map and that clings to your items and units. In an old fortress,
cleaning with this tool can significantly reduce FPS lag! It can also spoil your
!!FUN!!, so think before you use it.

Usage
-----

::

    clean all|map|items|units|plants [<options>]
    spotclean

By default, cleaning the map leaves mud and snow alone. Note that cleaning units
includes hostiles, and that cleaning items removes poisons from weapons.

``spotclean`` works like ``clean map snow mud``, removing all contaminants from
the tile under the keyboard cursor. This is ideal if you just want to clean a
specific tile but don't want the `clean <cleaners>` command to remove all the
glorious blood from your entranceway.

Mud will not be cleaned out from under farm plots, since that would render the
plot inoperable.

Examples
--------

``clean all``
    Clean everything that can be cleaned (except mud and snow).
``clean map mud item snow``
    Removes all spatter, including mud, leaves, and snow from map tiles. Farm
    plots will retain their mud.

Options
-------

When cleaning the map, you can specify extra options for extra cleaning:

``mud``
    Also remove mud.
``item``
    Also remove item spatter, like fallen leaves and flowers.
``snow``
    Also remove snow coverings.
