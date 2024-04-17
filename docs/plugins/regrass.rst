regrass
=======

.. dfhack-tool::
    :summary: Regrow surface grass and cavern moss.
    :tags: adventure fort armok animals map

This command can refresh the grass (and subterranean moss) growing on your map.
Operates on floors, stairs, and ramps. Also works underneath shrubs, saplings,
and tree trunks. Ignores furrowed soil, beaches, and tiles under buildings.

Usage
-----

::

    regrass [options]

Regrasses the entire map by default.

Options
-------

``h``, ``help``
    Display this help screen.
``m``, ``max``
    Maxes out every grass type in the tile, giving extra grazing time.
    Not normal DF behavior. Tile will appear to be the first type of grass
    present in the map block until that is depleted, moving on to the next type.
    When this option isn't used, a grass type will be selected randomly instead.
``n``, ``new``
    Adds biome-compatible grass types that were not originally present in the
    map block. Allows regrass to work in blocks that never had any grass to
    begin with. Will still fail in incompatible biomes.
``a``, ``ashes``
    Regrass tiles that've been burnt to ash.
``u``, ``mud``
    Converts non-smoothed, mud-spattered stone into grass. Valid for layer stone,
    obsidian, and ore.
``p``, ``here``
    Only regrass the tile selected by the keyboard cursor.
``b``, ``block``
    Only regrass the map block containing the keyboard cursor.

Examples
--------

``regrass``
    Regrass the entire map, refilling existing and depleted grass.
``regrass here``
    Regrass the selected tile, refilling or restoring grass.
``regrass block ashes max``
    Regrass the block, maxing out grass and converting ashes to grass floors.
``regrass b u n``
    Regrass the block, converting muddy stone and adding new grass types.
