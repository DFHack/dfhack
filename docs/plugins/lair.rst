lair
====
This command allows you to mark the map as a monster lair, preventing item
scatter on abandon. When invoked as ``lair reset``, it does the opposite.

Unlike `reveal`, this command doesn't save the information about tiles - you
won't be able to restore state of real monster lairs using ``lair reset``.

Options:

:lair:          Mark the map as monster lair
:lair reset:    Mark the map as ordinary (not lair)
