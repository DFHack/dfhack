dig-now
=======

.. dfhack-tool::
    :summary: Instantly complete dig designations.
    :tags: fort armok map

This tool will magically complete non-marker dig designations, modifying tile
shapes and creating boulders, ores, and gems as if a miner were doing the mining
or engraving. By default, the entire map is processed and boulder generation
follows standard game rules, but the behavior is configurable.

Note that no units will get mining or engraving experience for the dug/engraved
tiles.

Trees and roots are not currently handled by this plugin and will be skipped.
Requests for engravings are also skipped since they would depend on the skill
and creative choices of individual engravers. Other types of engraving (i.e.
smoothing and track carving) are handled.

Usage::

    dig-now [<pos> [<pos>]] [<options>]

Where the optional ``<pos>`` pair can be used to specify the coordinate bounds
within which ``dig-now`` will operate. If they are not specified, ``dig-now``
will scan the entire map. If only one ``<pos>`` is specified, only the tile at
that coordinate is processed.

Any ``<pos>`` parameters can either be an ``<x>,<y>,<z>`` triple (e.g.
``35,12,150``) or the string ``here``, which means the position of the active
game cursor should be used. You can use the `position` command to get the
current cursor position if you need it.

Examples
--------

``dig-now``
    Dig designated tiles according to standard game rules.
``dig-now --clean``
    Dig all designated tiles, but don't generate any boulders, ores, or gems.
``dig-now --dump here``
    Dig tiles and teleport all generated boulders, ores, and gems to the tile
    under the game cursor.

Options
-------

``-c``, ``--clean``
    Don't generate any boulders, ores, or gems. Equivalent to
    ``--percentages 0,0,0,0``.
``-d``, ``--dump <pos>``
    Dump any generated items at the specified coordinates. If the tile at those
    coordinates is open space or is a wall, items will be generated on the
    closest walkable tile below.
``-e``, ``--everywhere``
    Generate a boulder, ore, or gem for every tile that can produce one.
    Equivalent to ``--percentages 100,100,100,100``.
``-p``, ``--percentages <layer>,<vein>,<small cluster>,<deep>``
    Set item generation percentages for each of the tile categories. The
    ``vein`` category includes both the large oval clusters and the long stringy
    mineral veins. Default is ``25,33,100,100``.
``-z``, ``--cur-zlevel``
    Restricts the bounds to the currently visible z-level.
