seedwatch
=========

.. dfhack-tool::
    :summary: Manages seed and plant cooking based on seed stock levels.
    :tags: fort auto plants

Unlike brewing and other kinds of processing, cooking plants does not produce
a usable seed. By default, all plants are allowed to be cooked. This often leads
to the situation where dwarves have no seeds left to plant crops with because
they cooked all the relevant plants. Seedwatch protects you from this problem.

Each seed type can be assigned a target stock amount. If the number of seeds of
that type falls below that target, then the plants and seeds of that type will
be protected from cookery. If the number rises above the target + 20, then
cooking will be allowed again.

Usage
-----

``enable seedwatch``
    Start managing seed and plant cooking. By default, all types are watched
    with a target of ``30``, but you can adjust the list or even
    ``seedwatch clear`` it and start your own if you like.
``seedwatch [status]``
    Display whether seedwatch is enabled and prints out the watch list, along
    with the current seed counts.
``seedwatch <type> <target>``
    Adds the specified type to the watchlist (if it's not already there) and
    sets the target number of seeds to the specified number. You can pass the
    keyword ``all`` instead of a specific type to set the target for all types.
    If you set the target to ``0``, it removes the specified type from the
    watch list.
``seedwatch clear``
    Clears all types from the watch list. Same as ``seedwatch all 0``.

To see a list of all plant types that you might want to set targets for, you can
run this command::

    devel/query --table df.global.world.raws.plants.all --search ^id --maxdepth 1

Examples
--------

``enable seedwatch``
    Adds all seeds to the watch list, sets the targets to 30, and starts
    monitoring your seed stock levels.
``seedwatch MUSHROOM_HELMET_PLUMP 50``
    Add Plump Helmets to the watch list and sets the target to 50.
``seedwatch MUSHROOM_HELMET_PLUMP 0``
    removes Plump Helmets from the watch list.
