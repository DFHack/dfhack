regrass
=======

.. dfhack-tool::
    :summary: Regrow surface grass and cavern moss.
    :tags: adventure fort armok animals map plants

This command can refresh the grass (and subterranean moss) growing on your map.
Operates on floors, stairs, and ramps. Also works underneath shrubs, saplings,
and tree trunks. Ignores furrowed soil and wet sand (beaches).

The `cleaners` tool can help remove grass if you messed up and suddenly there
are staring eyeballs growing all over your fort. `gui/tiletypes` can then be used
to change the soil back to stone.

Usage
-----

::

    regrass [<pos> [<pos>]] [<options>]

Regrasses the entire map by default, restricted to compatible tiles in map
blocks that had grass at some point. Supplying a ``pos`` argument can limit
operation to a single tile. Supplying both can operate on a cuboid region.
``pos`` should normally be in the form ``0,0,0``, without spaces. The string
``here`` can be used in place of numeric coordinates to use the position of the
keyboard cursor, if active. The ``--block`` and ``--zlevel`` options use the
``pos`` values differently.

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
    Regrass tiles in the 20 x 20 x 20 cube defined by the coords, refilling
    existing and depleted grass, and converting ashes and muddy stone. Fails
    in any block that never had grass.
``regrass 10,10,100 -baudnm``
    Regrass the block that contains the given coord. Converts ashes, muddy
    stone, and tiles under buildings. Adds all compatible grass types and
    fills each grass type to max.
``regrass -f``
    Regrass the entire map, refilling existing and depleted grass, else filling
    with a single randomly selected grass type if non-existent.
``regrass -l``
    Print all valid grass raw IDs for use with ``--plant``. Both numerical and
    string IDs are provided. This ignores all other options and skips regrass.
``regrass -zf -p 128``
    Regrass the current z-level, refilling existing and depleted grass, else
    filling with ``underlichen`` (subject to world's raws) if non-existent.
``regrass here -bnf -p "dog's tooth grass"``
    Regrass the selected block, adding all compatible grass types to block data,
    and ``dog's tooth grass`` if no compatible types exist for a tile. Refill
    existing grass on each tile, else select a compatible (or forced) type if
    depleted or previously non-existent.

Options
-------

``-l``, ``--list``
    List all available grass raw IDs that you can later pass to the ``--plant``
    option. The map will not be affected when running with this option.
``-m``, ``--max``
    Maxes out every grass type in the tile, giving extra grazing time.
    Not normal DF behavior. The tile will appear to be the first type of grass
    present in the map block until that is depleted for the tile, moving on to
    the next type. Ignores biome compatibility per tile, using every type
    present in the block data. When this option *isn't* used, non-depleted
    grass tiles will have their existing type refilled, while grass-depleted
    soils will have a type selected randomly.
``-n``, ``--new``
    Adds all biome-compatible grass types that were not originally present in
    the map block. Allows regrass to work in blocks that never had any grass to
    begin with. Will still fail in incompatible biomes. This can add an excessive
    number of grass events to your map, so it may be desirable to run
    ``clean --map --grass --only`` (see: `cleaners`) afterwards to clean up any
    unused events.
``-f``, ``--force``
    Force a grass type on tiles with no compatible grass types. Unsets the
    ``no_grow`` flag on all tiles. The ``--new`` option takes precedence for
    compatible biomes, otherwise such tiles will be forced instead. By default,
    a single random grass type is selected from the world's raws to be the
    forced type for the duration of the command. The ``--plant`` option allows
    a specific grass type to be specified.
``-p``, ``--plant <grass_id>``
    Specify a grass type for the ``--force`` option. ``grass_id`` is not
    case-sensitive, but must be enclosed in quotes if spaces exist. A numerical
    ID can also be used.
``-a``, ``--ashes``
    Regrass tiles that've been burnt to ash. Usually ash must revert to soil
    first before grass can regrow.
``-d``, ``--buildings``
    Regrass tiles under certain passable building tiles including stockpiles,
    planned buildings, workshops, and farms. (Farms will convert grass tiles to
    furrowed soil after a short while, but is useful with the ``--mud`` option.)
    Doesn't work on "dynamic" buildings such as doors and floor grates.
    (Dynamic buildings also include levers and cage traps, for some reason.)
    Existing grass tiles will always be refilled, regardless of building tile.
``-u``, ``--mud``
    Converts non-smoothed, mud-spattered stone into grass.
``-b``, ``--block``
    Only regrass the map block that contains the first ``pos`` argument.
    `devel/block-borders` can be used to visualize map blocks.
``-z``, ``--zlevel``
    Regrass entire z-levels. Will do all z-levels between ``pos`` arguments if
    both are given, z-level of first ``pos`` if one is given, else z-level of
    current view if no ``pos`` is given.

Troubleshooting
---------------

Use ``debugfilter set Debug regrass log`` (or
``debugfilter set Trace regrass log`` for more detail) to figure out why
regrass is failing on a tile. (Avoid regrassing large parts of the map with
this enabled, as it will make the game unresponsive and flood the console for
several minutes!)

Disable with ``debugfilter set Info regrass log``.
