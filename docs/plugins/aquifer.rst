aquifer
=======

.. dfhack-tool::
    :summary: Add, remove, or modify aquifers.
    :tags: fort armok map

This tool can examine or alter aquifer properties of map tiles. Also see
`gui/aquifer` for visually highlighting active aquifer tiles and
interactively modifying them.

Only solid (wall), natural (not constructed), rough (not smooth) layer stone
(not mineral vein or gem cluster) tiles can have their aquifer properties
modified. When adding aquifers, walls that would immediately leak into open
space will not be modified unless the override option is given.

Usage
-----

::

    aquifer [list] [<pos> [<pos>] | <target option>] [<options>]
    aquifer add (light|heavy) (<pos> [<pos>] | <target option>) [<options>]
    aquifer drain [light|heavy] (<pos> [<pos>] | <target option>) [<options>]
    aquifer convert (light|heavy) (<pos> [<pos>] | <target option>) [<options>]

For each action, you can either specify a coordinate range or a target option.
If specifying a coordinate range, each ``<pos>`` parameter can be either an xyz
coordinate triple (e.g. ``14,25,143``) or the keyword ``here``, which indicates
the position of the keyboard cursor. If not specified, the second position of
the coordinate range defaults to ``here`` if you have the keyboard cursor
active or is equal to the first coordinate otherwise.

Examples
--------

``aquifer``
    List z-levels (and their associated displayed elevations) that have aquifer
    tiles and how many aquifer tiles are on each listed level.
``aquifer drain --all --skip-top 2``
    Drain all aquifer tiles on the map except for the top 2 levels of aquifer.
``aquifer drain -z --leaky``
    Drain aquifer tiles on this z-level that are actively leaking into open
    adjacent tiles or into empty spaces underneath them.
``aquifer convert light --zdown --levels 5``
    Convert heavy aquifers from the current z-level to 4 levels below to light
    aquifers.
``aquifer add heavy here --leaky``
    Create a heavy aquifer tile in the exposed natural rough wall under the
    keyboard cursor.
``aquifer add light 87,29,126 111,53,126``
    Create a 25x25 block of light aquifer tiles at the specified coordinates.
``aquifer add light --all --skip-top 2 --levels 20``
    Add a 20-level deep light aquifer starting 2 levels beneath the surface.

Target options
--------------

``-a``, ``--all``
    Apply the action to every possible tile in the entire map.
``-d``, ``--zdown``
    Apply the action to the current z-level and below.
``-u``, ``--zup``
    Apply the action to the current z-level and above.
``-z``, ``--cur-zlevel``
    Apply the action to the current z-level only.

Other options
-------------

``-l``, ``--levels <num>``
    If one of the ``--all``, ``--zdown``, or ``--zup`` target options are
    specified, this option will limit the number of levels that will be
    affected. If used with ``--all``, the levels are counted from the topmost
    level that can hold an aquifer tile.
``--leaky``
    Aquifer tiles will leak into orthogonally adjacent open spaces or into
    empty space directly beneath them. These kinds of tiles are not modified by
    default by the ``add`` action. This option will bypass that safety check
    and allow addition of aquifer tiles that will immediately begin leaking. If
    used with the ``drain`` or ``convert`` actions, the action will only be
    applied to tiles that are actively leaking.
``-q``, ``--quiet``
    Don't print any non-error output.
``-s``, ``--skip-top <num>``
    If the ``--all`` target option is specified, this option will ignore the
    first ``<num>`` levels before the specified action starts taking effect.
    For the ``add`` action, it keeps the first ``<num>`` levels starting from
    the topmost surface elevation unchanged before adding aquifers. For the
    ``drain`` and ``convert`` actions, it keeps the top ``<num>`` levels
    starting from the topmost existing aquifer unchanged before removing or
    converting aquifers.
