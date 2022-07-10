seedwatch
=========
Watches the numbers of seeds available and enables/disables seed and plant cooking.

Each plant type can be assigned a limit. If their number falls below that limit,
the plants and seeds of that type will be excluded from cookery.
If the number rises above the limit + 20, then cooking will be allowed.

The plugin needs a fortress to be loaded and will deactivate automatically otherwise.
You have to reactivate with 'seedwatch start' after you load the game.

Options:

:all:       Adds all plants from the abbreviation list to the watch list.
:start:     Start watching.
:stop:      Stop watching.
:info:      Display whether seedwatch is watching, and the watch list.
:clear:     Clears the watch list.

Examples:

``seedwatch MUSHROOM_HELMET_PLUMP 30``
    add ``MUSHROOM_HELMET_PLUMP`` to the watch list, limit = 30
``seedwatch MUSHROOM_HELMET_PLUMP``
    removes ``MUSHROOM_HELMET_PLUMP`` from the watch list.
``seedwatch all 30``
    adds all plants from the abbreviation list to the watch list, the limit being 30.
