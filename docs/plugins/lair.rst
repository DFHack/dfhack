lair
====

.. dfhack-tool::
    :summary: Mark the map as a monster lair.
    :tags: fort armok map

This avoids item scatter when the fortress is abandoned.

Usage
-----

``lair``
    Mark the map as a monster lair.
``lair reset``
    Mark the map as ordinary (not lair).

This command doesn't save the information about tiles - you won't be able to
restore the state of a real monster lairs using ``lair reset``.
