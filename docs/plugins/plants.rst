.. _plant:

plants
======

.. dfhack-tool::
    :summary: Provides commands that interact with plants.
    :tags: untested adventure fort armok map plants
    :no-command:

.. dfhack-command:: plant
   :summary: Create a plant or make an existing plant grow up.

Usage
-----

``plant create <ID>``
    Creates a new plant of the specified type at the active cursor position.
    The cursor must be on a dirt or grass floor tile.
``plant grow``
    Grows saplings into trees. If the cursor is active, it only affects the
    sapling under the cursor. If no cursor is active, it affect all saplings
    on the map.

To see the full list of plant ids, run the following command::

    devel/query --table df.global.world.raws.plants.all --search ^id --maxdepth 1

Example
-------

``plant create TOWER_CAP``
    Create a Tower Cap sapling at the cursor position.
