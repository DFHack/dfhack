diggingInvaders
===============

.. dfhack-tool::
    :summary: Invaders dig and destroy to get to your dwarves.
    :tags: unavailable

Usage
-----

``enable diggingInvaders``
    Enable the plugin.
``diggingInvaders add <race>``
    Register the specified race as a digging invader.
``diggingInvaders remove <race>``
    Unregisters the specified race as a digging invader.
``diggingInvaders now``
    Makes invaders try to dig now (if the plugin is enabled).
``diggingInvaders clear``
    Clears the registry of digging invader races.
``diggingInvaders edgesPerTick <n>``
    Makes the pathfinding algorithm work on at most n edges per tick. Set to 0
    or lower to make it unlimited.
``diggingInvaders setCost <race> <action> <n>``
    Set the pathing cost per tile for a particular action. This determines what
    invaders consider to be the shortest path to their target.
``diggingInvaders setDelay <race> <action> <n>``
    Set the time cost (in ticks) for performing a particular action. This
    determines how long it takes for invaders to get to their target.

Note that the race is case-sensitive. You can get a list of races for your world
with this command::

    devel/query --table df.global.world.raws.creatures.all --search creature_id --maxdepth 1 --maxlength 5000

but in general, the race is what you'd expect, just capitalized (e.g. ``GOBLIN``
or ``ELF``).

Actions:

``walk``
    Default cost: 1, default delay: 0. This is the base cost for the pathing
    algorithm.
``destroyBuilding``
    Default cost: 2, default delay: 1,000,
``dig``
    Default cost: 10,000, default delay: 1,000. This is for digging soil or
    natural stone.
``destroyRoughConstruction``
    Default cost: 1,000, default delay: 1,000. This is for destroying
    constructions made from boulders.
``destroySmoothConstruction``
    Default cost: 100, default delay: 100. This is for destroying constructions
    made from blocks or bars.

Example
-------

``diggingInvaders add GOBLIN``
    Registers members of the GOBLIN race as a digging invader.
