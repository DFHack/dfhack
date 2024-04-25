regrass
=======

.. dfhack-tool::
    :summary: Regrow surface grass and cavern moss.
    :tags: adventure fort armok animals map

This command can refresh the grass (and subterranean moss) growing on your map.
Operates on floors, stairs, and ramps. Also works underneath shrubs, saplings,
and tree trunks. Ignores furrowed soil and wet sand (beaches).

Usage
-----

::

    regrass [<pos> [<pos>]] [<options>]

Regrasses the entire map by default, on compatible tiles in map blocks that
had grass at some point. Supplying a ``pos`` argument can limit operation to
a single tile. Supplying both can operate on a cuboid. ``pos`` should normally
be in the form ``0,0,0``, without spaces. The string ``here`` can be used in
place of ``pos`` to use the coords of the keyboard cursor, if active. The
``--block`` and ``--zlevel`` options use the ``pos`` values differently
than normal.

Options
-------

``-m``, ``--max``
    Maxes out every grass type in the tile, giving extra grazing time.
    Not normal DF behavior. Tile will appear to be the first type of grass
    present in the map block until that is depleted, moving on to the next
    type. When this option isn't used, non-depleted grass tiles will have their
    existing type refilled, while grass-depleted soils will have a type
    selected randomly.
``-n``, ``--new``
    Adds biome-compatible grass types that were not originally present in the
    map block. Allows regrass to work in blocks that never had any grass to
    begin with. Will still fail in incompatible biomes.
``-f <grass_id>``, ``--force <grass_id>``
    Force a grass type on tiles with no compatible grass types. ``grass_id`` is
    not case-sensitive, but must be enclosed in quotes if spaces exist. If an
    empty string is used via ``""``, then a single random grass type will be
    selected from the world's raws. The ``--new`` option takes precidence for
    compatible biomes, otherwise such tiles will be forced instead.
``-a``, ``--ashes``
    Regrass tiles that've been burnt to ash.
``-d``, ``--buildings``
    Regrass tiles under certain passable building tiles including stockpiles,
    planned buildings, workshops, and farms. (Farms will convert grass tiles to
    furrowed soil after a short while, but is useful with ``--mud`` option.)
    Doesn't work on "dynamic" buildings such as doors and floor grates.
    (Dynamic buildings also include levers and cage traps, for some reason.)
    Existing grass tiles will always be refilled, regardless of building tile.
``-u``, ``--mud``
    Converts non-smoothed, mud-spattered stone into grass. Valid for layer
    stone, obsidian, and ore.
``-b``, ``--block``
    Only regrass the map block that contains the first ``pos`` argument.
    ``devel/block-borders`` can be used to visualize map blocks.
``-z``, ``--zlevel``
    Regrass entire z-levels. Will do all z-levels between ``pos`` arguments if
    both given, z-level of first ``pos`` if one given, else z-level of
    viewscreen if no ``pos`` given.

Examples
--------

``regrass``
    Regrass the entire map, refilling existing and depleted grass.
``regrass here``
    Regrass the selected tile, refilling existing and depleted grass.
``regrass here 0,0,90 --zlevel``
    Regrass all z-levels including the selected tile's z-level through z-level
    90, refilling existing and depleted grass.
``regrass 0,0,100 19,19,119 --ashes --mud``
    Regrass tiles in the 20x20x20 cube defined by the coords, refilling
    existing and depleted grass, and converting ashes and muddy stone (if
    respective blocks ever had grass.)
``regrass 10,10,100 -baudnm``
    Regrass the block that contains the given coord; converting ashes, muddy
    stone, and tiles under buildings; adding all compatible grass types, and
    filling each grass type to max.
``regrass -f ""``
    Regrass the entire map, refilling existing and depleted grass, else filling
    with a randomly selected grass type if non-existent.
``regrass -z -f underlichen``
    Regrass the current z-level, refilling existing and depleted grass, else
    filling with ``underlichen`` if non-existent.
``regrass here -bn -f "dog's tooth grass"``
    Regrass the selected block, adding all compatible grass types to block data,
    ``dog's tooth grass`` if no compatible types exist. Refill existing grass
    on each tile, else select one of the block's types if depleted or
    previously non-existent.

Troubleshooting
---------------

``debugfilter set Debug regrass log`` can be used to figure out why regrass
is failing on a tile. (Avoid regrassing large parts of the map with this
enabled, as it will make the game unresponsive and flood the console for
several minutes!)

Disable with ``debugfilter set Info regrass log``.
