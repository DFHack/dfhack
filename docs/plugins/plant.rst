plant
=====

.. dfhack-tool::
    :summary: Grow and remove shrubs or trees.
    :tags: adventure fort armok map plants

Grow and remove shrubs or trees. Modes are ``list``, ``create``, ``grow``,
and ``remove``. ``list`` prints a list of all valid shrub and sapling raw IDs.
``create`` allows the creation of new shrubs and saplings. ``grow`` adjusts
the age of saplings and trees, allowing them to grow instantly. ``remove`` can
remove existing shrubs and saplings.

Usage
-----

Provide a mode (including a ``plant_id`` for ``create``) followed by optional
``pos`` arguments and options. The ``pos`` arguments can limit operation of
``grow`` or ``remove`` to a single tile or a cuboid. ``pos`` should normally be
in the form ``0,0,0``, without spaces. The string ``here`` can be used in place
of numeric coordinates to use the position of the keyboard cursor, if active.

::

    plant list

Prints a list of all shrub and sapling raw IDs for use with the other modes.
Both numerical and string IDs are provided.

::

    plant create <plant_id> [<pos>] [<options>]

Creates a new plant of the specified type at ``pos`` or the cursor position.
The target must be a floor tile, consisting of soil, grass, ashes, or
non-smooth muddy (layer, obsidian, or ore) stone. ``plant_id`` is not
case-sensitive, but must be enclosed in quotes if spaces exist. (No unmodded
shrub or sapling raw IDs have spaces.) A numerical ID can be used in place of a
string. Use ``plant list`` for a list of valid IDs.

::

    plant grow [<pos> [<pos>]] [<options>]

Grows saplings (including dead ones) into trees. Will default to all saplings
on the map if no ``pos`` arguments are used. Saplings will die and fail to grow
if they are blocked by another tree.

::

    plant remove [<pos> [<pos>]] [<options>]

Remove plants from the map (or area defined by ``pos`` arguments). By default,
it only removes invalid plants that exist on non-plant tiles (due to
:bug:`12868`). The ``--shrubs`` and ``--saplings`` options allow normal plants
to be targeted instead. Removal of fully-grown trees isn't currently supported.

Examples
--------

``plant list``
    List all valid shrub and sapling raw IDs.
``plant create tower_cap``
    Create a Tower Cap sapling at the cursor.
``plant create 203 -c -a tree``
    Create a Willow sapling at the cursor, even away from water features,
    ready to mature into a tree.
``plant create single-grain_wheat 70,70,140``
    Create a Single-grain Wheat shrub at (70, 70, 140).
``plant grow``
    Attempt to grow all saplings on the map into trees.
``plant grow -z -f maple,200,sand_pear``
    Attempt to grow all Maple, Acacia, and Sand Pear saplings on the current
    z-level into trees.
``plant grow 0,0,100 19,19,119 -a 10``
    Set the age of all saplings and trees (with their original sapling tile)
    in the defined 20 x 20 x 20 cube to at least 10 years.
``plant remove``
    Remove all invalid plants from the map.
``plant remove here -sp``
    Remove the shrub or sapling at the cursor.
``plant remove -spd``
    Remove all dead shrubs and saplings from the map.
``plant remove 0,0,49 0,0,51 -pz -e nether_cap``
    Remove all saplings on z-levels 49 to 51, excluding Nether Cap.

Create Options
--------------

``-c``, ``--force``
    Create plant even on tiles flagged ``no_grow`` and unset the flag. This
    flag is set on tiles that were originally boulders or pebbles, as well
    as on some tiles found in deserts, etc. Also allow non-``[DRY]`` plants
    (e.g., willow) to grow away (3+ tiles) from water features (i.e., pools,
    brooks, and rivers), and non-``[WET]`` plants (e.g., prickle berry) to
    grow near them.
``-a``, ``--age <value>``
    Set the created plant to a specific age (in years). ``value`` can be a
    non-negative integer, or one of the strings ``tree``/``1x1`` (3 years),
    ``2x2`` (201 years), or ``3x3`` (401 years). ``value`` will be capped at
    1250. Defaults to 0 if option is unused. Only a few tree types grow wider
    than 1x1, but many may grow taller. (Going directly to higher years will
    stunt height. It may be more desirable to instead use ``plant grow`` in
    stages, or just spawn full trees using `gui/sandbox`.)

Grow Options
------------

``-a``, ``--age <value>``
    Define the age (in years) to set saplings to. ``value`` can be a
    non-negative integer, or one of the strings ``tree``/``1x1`` (3 years),
    ``2x2`` (201 years), or ``3x3`` (401 years). ``value`` will be capped at
    1250. Defaults to 3 if option is unused. If a ``value`` larger than 3 is
    used, it will make sure even fully-grown trees have an age of at least the
    given value, allowing them to grow larger. (Going directly to higher years
    will stunt tree height. It may be more desirable to grow in stages rather
    than all at once. Trees grow taller every 10 years.)
``-f``, ``--filter <plant_id>[,<plant_id>...]``
    Define a filter list of plant raw IDs to target, ignoring all other tree
    types. ``plant_id`` is not case-sensitive, but must be enclosed in quotes
    if spaces exist. (No unmodded tree raw IDs have spaces.) A numerical ID
    can be used in place of a string. Use ``plant list`` and check under
    ``Saplings`` for a list of valid IDs.
``-e``, ``--exclude <plant_id>[,<plant_id>...]``
    Same as ``--filter``, but target everything except these. Cannot be used
    with ``--filter``.
``-z``, ``--zlevel``
    Operate on a range of z-levels instead of default targeting. Will do all
    z-levels between ``pos`` arguments if both are given (instead of cuboid),
    z-level of first ``pos`` if one is given (instead of single tile), else
    z-level of current view if no ``pos`` is given (instead of entire map).
``-n``, ``--dry-run``
    Don't actually grow plants. Just print the total number of plants that
    would be grown.

Remove Options
--------------

``-s``, ``--shrubs``
    Target shrubs for removal.
``-p``, ``--saplings``
    Target saplings for removal.
``-d``, ``--dead``
    Only target dead plants for removal. Can't be used without ``--shrubs``
    or ``--saplings``.
``-f``, ``--filter <plant_id>[,<plant_id>...]``
    Define a filter list of plant raw IDs to target, ignoring all other plant
    types. This applies after ``--shrubs`` and ``--saplings`` are targeted,
    and can't be used without one of those options. ``plant_id`` is not
    case-sensitive, but must be enclosed in quotes if spaces exist. (No
    unmodded shrub or sapling raw IDs have spaces.) A numerical ID can be
    used in place of a string. Use ``plant list`` for a list of valid IDs.
``-e``, ``--exclude <plant_id>[,<plant_id>...]``
    Same as ``--filter``, but target everything except these. Cannot be used
    with ``--filter``.
``-z``, ``--zlevel``
    Operate on a range of z-levels instead of default targeting. Will do all
    z-levels between ``pos`` arguments if both are given (instead of cuboid),
    z-level of first ``pos`` if one is given (instead of single tile), else
    z-level of current view if no ``pos`` is given (instead of entire map).
``-n``, ``--dryrun``
    Don't actually remove plants. Just print the total number of plants that
    would be removed.
