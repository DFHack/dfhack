seedwatch
=========

.. dfhack-tool::
    :summary: Manages seed and plant cooking based on seed stock levels.
    :tags: fort auto

Each seed type can be assigned a target. If the number of seeds of that type
falls below that target, then the plants and seeds of that type will be excluded
from cookery. If the number rises above the target + 20, then cooking will be
allowed.

The plugin needs a fortress to be loaded and will deactivate automatically
otherwise. You have to reactivate with ``enable seedwatch`` after you load a
fort.

Usage:

``enable seedwatch``
    Start managing seed and plant cooking. By default, no types are watched.
    You have to add them with further ``seedwatch`` commands.
``seedwatch <type> <target>``
    Adds the specifiied type to the watchlist (if it's not already there) and
    sets the target number of seeds to the specified number. You can pass the
    keyword ``all`` instead of a specific type to set the target for all types.
``seedwatch <type>``
    Removes the specified type from the watch list.
``seedwatch clear``
    Clears all types from the watch list.
``seedwatch info``
    Display whether seedwatch is enabled and prints out the watch list.

To print out a list of all plant types, you can run this command::

    devel/query --table df.global.world.raws.plants.all --search ^id --maxdepth 1

Examples
--------

``seedwatch all 30``
    Adds all seeds to the watch list and sets the targets to 30.
``seedwatch MUSHROOM_HELMET_PLUMP 50``
    Add Plump Helmets to the watch list and sets the target to 50.
``seedwatch MUSHROOM_HELMET_PLUMP``
    removes Plump Helmets from the watch list.
